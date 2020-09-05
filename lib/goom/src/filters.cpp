// --- CHUI EN TRAIN DE SUPPRIMER LES EXTERN RESOLX ET C_RESOLY ---

/* filter.c version 0.7
 * contient les filtres applicable a un buffer
 * creation : 01/10/2000
 *  -ajout de sinFilter()
 *  -ajout de zoomFilter()
 *  -copie de zoomFilter() en zoomFilterRGB(), gerant les 3 couleurs
 *  -optimisation de sinFilter (utilisant une table de sin)
 *	-asm
 *	-optimisation de la procedure de generation du buffer de transformation
 *		la vitesse est maintenant comprise dans [0..128] au lieu de [0..100]
 */

#include "filters.h"

#include "goom_config.h"
#include "goom_fx.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_testing.h"
#include "goom_tools.h"
#include "goomutils/logging_control.h"
// #undef NO_LOGGING
#include "goomutils/logging.h"
#include "goomutils/SimplexNoise.h"
#include "v3d.h"

#include <algorithm>
#include <cstdint>

static inline void setPixelRGB(Pixel* buffer, const uint32_t x, const Color& c)
{
  buffer[x].channels.r = c.r;
  buffer[x].channels.g = c.g;
  buffer[x].channels.b = c.b;
}

static inline void getPixelRGB(Pixel* buffer, const uint32_t x, Color* c)
{
  Pixel i = *(buffer + x);
  c->b = i.channels.b;
  c->g = i.channels.g;
  c->r = i.channels.r;
}

constexpr float BUFFPOINTNBF = static_cast<float>(BUFFPOINTNB);
constexpr int BUFFPOINTMASK = 0xffff;

constexpr int sqrtperte = 16;
// faire : a % sqrtperte <=> a & pertemask
constexpr uint32_t PERTEMASK = 0xf;
// faire : a / sqrtperte <=> a >> PERTEDEC
constexpr int PERTEDEC = 4;

// pure c version of the zoom filter
static void c_zoom(Pixel* expix1,
                   Pixel* expix2,
                   const uint16_t prevX,
                   const uint16_t prevY,
                   const int* brutS,
                   const int* brutD,
                   const int buffratio,
                   const int precalCoef[BUFFPOINTNB][BUFFPOINTNB]);

// simple wrapper to give it the same proto than the others
void zoom_filter_c(const uint16_t sizeX,
                   const uint16_t sizeY,
                   Pixel* src,
                   Pixel* dest,
                   const int* brutS,
                   const int* brutD,
                   const int buffratio,
                   const int precalCoef[BUFFPOINTNB][BUFFPOINTNB])
{
  c_zoom(src, dest, sizeX, sizeY, brutS, brutD, buffratio, precalCoef);
}

static void generatePrecalCoef(int precalCoef[BUFFPOINTNB][BUFFPOINTNB]);

// TODO repeated fields in here
struct FilterDataWrapper
{
  PluginParam enabled_bp;
  PluginParameters params;

  uint32_t* coeffs;
  uint32_t* freecoeffs;

  int* brutS;
  int* freebrutS; // source
  int* brutD;
  int* freebrutD; // dest
  int* brutT;
  int* freebrutT; // temp (en cours de generation)

  uint32_t zoomWidth;
  uint32_t prevX;
  uint32_t prevY;

  ZoomFilterMode theMode;
  float generalSpeed;
  bool reverse; // reverse the speed
  bool waveEffect;
  bool hypercosEffect;
  int vPlaneEffect;
  int hPlaneEffect;
  bool perlinNoisify;
  bool noisify;
  uint32_t middleX;
  uint32_t middleY;

  bool mustInitBuffers;
  int interlaceStart;

  // modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16)
  int buffratio;
  int* firedec;

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  int precalCoef[BUFFPOINTNB][BUFFPOINTNB];
};

static inline v2g zoomVector(FilterDataWrapper* data, const float X, const float Y)
{
  const float sq_dist = X * X + Y * Y;

  /* sx = (X < 0.0f) ? -1.0f : 1.0f;
     sy = (Y < 0.0f) ? -1.0f : 1.0f;
   */
  float coefVitesse = (1.0f + data->generalSpeed) / 50.0f;

  // Effects

  // Centralized FX
  switch (data->theMode)
  {
    case ZoomFilterMode::crystalBallMode:
      coefVitesse -= (sq_dist - 0.3f) / 15.0f;
      break;
    case ZoomFilterMode::amuletteMode:
      coefVitesse += sq_dist * 3.5f;
      break;
    case ZoomFilterMode::waveMode:
      coefVitesse += sin(sq_dist * 20.0f) / 100.0f;
      break;
    case ZoomFilterMode::scrunchMode:
      coefVitesse += sq_dist / 10.0f;
      break;
      //case ZoomFilterMode::HYPERCOS1_MODE:
      break;
      //case ZoomFilterMode::HYPERCOS2_MODE:
      break;
      //case ZoomFilterMode::YONLY_MODE:
      break;
    case ZoomFilterMode::speedwayMode:
      coefVitesse *= 4.0f * Y;
      break;
    default:
      break;
  }

  coefVitesse = std::clamp(coefVitesse, -2.01f, +2.01f);

  float vx = coefVitesse * X;
  float vy = coefVitesse * Y;

  /* Amulette 2 */
  // vx = X * tan(dist);
  // vy = Y * tan(dist);

  /* Rotate */
  //vx = (X+Y)*0.1;
  //vy = (Y-X)*0.1;

  // Effects adds-on
  /* Noise */
  if (data->perlinNoisify)
  {
    const float noise = (0.5*(1.0 + SimplexNoise::noise(X, Y)) - 0.5)/1.0;
    vx += noise;
    vy -= noise;
    vx *= getRandInRange(0.7f, 1.0f);
    vy *= getRandInRange(0.7f, 1.0f);
    vx += ((static_cast<float>(pcg32_rand())) / static_cast<float>(RAND_MAX) - 0.5f) / 5.0f;
    vy += ((static_cast<float>(pcg32_rand())) / static_cast<float>(RAND_MAX) - 0.5f) / 5.0f;
  }
  if (data->noisify)
  {
    const float noise = (0.5*(1.0 + SimplexNoise::noise(X, Y)) - 0.5)/5.0;
    vx += noise;
    vy -= noise;
    vx *= getRandInRange(0.7f, 1.0f);
    vy *= getRandInRange(0.7f, 1.0f);
//    vx += ((static_cast<float>(pcg32_rand())) / static_cast<float>(RAND_MAX) - 0.5f) / 20.0f;
//    vy += ((static_cast<float>(pcg32_rand())) / static_cast<float>(RAND_MAX) - 0.5f) / 20.0f;
  }

  /* Hypercos */
  if (data->hypercosEffect)
  {
    vx += sin(Y * 10.0f) / 120.0f;
    vy += sin(X * 10.0f) / 120.0f;
  }

  /* H Plane */
  if (data->hPlaneEffect)
  {
    vx += Y * 0.0025f * data->hPlaneEffect;
  }

  /* V Plane */
  if (data->vPlaneEffect)
  {
    vy += X * 0.0025f * data->vPlaneEffect;
  }

  /* TODO : Water Mode */
  //    if (data->waveEffect)

  return v2g{vx, vy};
}

/*
 * Makes a stripe of a transform buffer (brutT)
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->prevX)
 */
static void makeZoomBufferStripe(FilterDataWrapper* data, const uint32_t INTERLACE_INCR)
{
  // Where (vertically) to stop generating the buffer stripe
  // ??? TODO: Unused
  // uint32_t maxEnd = uint32_t(data->interlaceStart + static_cast<int>(INTERLACE_INCR);

  // Ratio from pixmap to normalized coordinates
  const float ratio = 2.0f / static_cast<float>(data->prevX);

  // Ratio from normalized to virtual pixmap coordinates
  const float inv_ratio = BUFFPOINTNBF / ratio;
  const float min = ratio / BUFFPOINTNBF;

  // Y position of the pixel to compute in normalized coordinates
  float Y = ratio * static_cast<float>(data->interlaceStart - static_cast<int>(data->middleY));

  // Where (vertically) to stop generating the buffer stripe
  uint32_t maxEnd = data->prevY;
  if (maxEnd > static_cast<uint32_t>(data->interlaceStart + static_cast<int>(INTERLACE_INCR)))
  {
    maxEnd = static_cast<uint32_t>(data->interlaceStart + static_cast<int>(INTERLACE_INCR));
  }

  // Position of the pixel to compute in pixmap coordinates
  uint32_t y;
  for (y = static_cast<uint32_t>(data->interlaceStart); (y < data->prevY) && (y < maxEnd); y++)
  {
    uint32_t premul_y_prevX = y * data->prevX * 2;
    float X = -(static_cast<float>(data->middleX)) * ratio;
    for (uint32_t x = 0; x < data->prevX; x++)
    {
      v2g vector = zoomVector(data, X, Y);
      // Finish and avoid null displacement
      if (fabs(vector.x) < min)
      {
        vector.x = (vector.x < 0.0f) ? -min : min;
      }
      if (fabs(vector.y) < min)
      {
        vector.y = (vector.y < 0.0f) ? -min : min;
      }

      data->brutT[premul_y_prevX] = static_cast<int>((X - vector.x) * inv_ratio) +
                                    static_cast<int>(data->middleX * BUFFPOINTNB);
      data->brutT[premul_y_prevX + 1] = static_cast<int>((Y - vector.y) * inv_ratio) +
                                        static_cast<int>(data->middleY * BUFFPOINTNB);
      premul_y_prevX += 2;
      X += ratio;
    }
    Y += ratio;
  }
  data->interlaceStart += static_cast<int>(INTERLACE_INCR);
  if (y >= data->prevY - 1)
  {
    data->interlaceStart = -1;
  }
}

static void c_zoom(Pixel* expix1,
                   Pixel* expix2,
                   const uint16_t prevX,
                   const uint16_t prevY,
                   const int* brutS,
                   const int* brutD,
                   const int buffratio,
                   const int precalCoef[BUFFPOINTNB][BUFFPOINTNB])
{
  const uint32_t ax = static_cast<uint32_t>((prevX - 1) << PERTEDEC);
  const uint32_t ay = static_cast<uint32_t>((prevY - 1) << PERTEDEC);

  const uint32_t bufsize = static_cast<uint32_t>(prevX * prevY * 2);
  const uint32_t bufwidth = prevX;

  expix1[0].val = expix1[prevX - 1].val = expix1[prevX * prevY - 1].val =
      expix1[prevX * prevY - prevX].val = 0;

  Color couleur;
  uint32_t myPos2;
  for (uint32_t myPos = 0; myPos < bufsize; myPos += 2)
  {
    myPos2 = myPos + 1;

    int brutSmypos = brutS[myPos];
    const uint32_t px = static_cast<uint32_t>(
        brutSmypos + (((brutD[myPos] - brutSmypos) * buffratio) >> BUFFPOINTNB));
    brutSmypos = brutS[myPos2];
    const uint32_t py = static_cast<uint32_t>(
        brutSmypos + (((brutD[myPos2] - brutSmypos) * buffratio) >> BUFFPOINTNB));

    uint32_t coeffs;
    uint32_t pos;
    if ((py >= ay) || (px >= ax))
    {
      pos = coeffs = 0;
    }
    else
    {
      pos = ((px >> PERTEDEC) + prevX * (py >> PERTEDEC));
      // coef en modulo 15
      coeffs = static_cast<uint32_t>(precalCoef[px & PERTEMASK][py & PERTEMASK]);
    }
    Color col1, col2, col3, col4;
    getPixelRGB(expix1, pos, &col1);
    getPixelRGB(expix1, pos + 1, &col2);
    getPixelRGB(expix1, pos + bufwidth, &col3);
    getPixelRGB(expix1, pos + bufwidth + 1, &col4);

    uint32_t c1 = coeffs;
    const uint32_t c2 = (c1 >> 8) & 0xFF;
    const uint32_t c3 = (c1 >> 16) & 0xFF;
    const uint32_t c4 = (c1 >> 24) & 0xFF;
    c1 = c1 & 0xff;

    couleur.r = col1.r * c1 + col2.r * c2 + col3.r * c3 + col4.r * c4;
    if (couleur.r > 5)
    {
      couleur.r -= 5;
    }
    couleur.r >>= 8;

    couleur.g = col1.g * c1 + col2.g * c2 + col3.g * c3 + col4.g * c4;
    if (couleur.g > 5)
    {
      couleur.g -= 5;
    }
    couleur.g >>= 8;

    couleur.b = col1.b * c1 + col2.b * c2 + col3.b * c3 + col4.b * c4;
    if (couleur.b > 5)
    {
      couleur.b -= 5;
    }
    couleur.b >>= 8;

    setPixelRGB(expix2, static_cast<uint32_t>(myPos >> 1), couleur);
  }
}

// Generate the water fx horizontal direction buffer
static void generateTheWaterFXHorizontalDirectionBuffer(PluginInfo* goomInfo,
                                                        FilterDataWrapper* data)
{
  int decc = static_cast<int>(goomInfo->getNRand(8) - 4);
  int spdc = static_cast<int>(goomInfo->getNRand(8) - 4);
  int accel = static_cast<int>(goomInfo->getNRand(8) - 4);

  for (size_t loopv = data->prevY; loopv != 0;)
  {
    loopv--;
    data->firedec[loopv] = decc;
    decc += spdc / 10;
    spdc += static_cast<int>(goomInfo->getNRand(3) - goomInfo->getNRand(3));

    if (decc > 4)
    {
      spdc -= 1;
    }
    if (decc < -4)
    {
      spdc += 1;
    }

    if (spdc > 30)
    {
      spdc = spdc - static_cast<int>(goomInfo->getNRand(3)) + accel / 10;
    }
    if (spdc < -30)
    {
      spdc = spdc + static_cast<int>(goomInfo->getNRand(3)) + accel / 10;
    }

    if (decc > 8 && spdc > 1)
    {
      spdc -= static_cast<int>(goomInfo->getNRand(3)) - 2;
    }
    if (decc < -8 && spdc < -1)
    {
      spdc += static_cast<int>(goomInfo->getNRand(3)) + 2;
    }
    if (decc > 8 || decc < -8)
    {
      decc = decc * 8 / 9;
    }

    accel += static_cast<int>(goomInfo->getNRand(2) - goomInfo->getNRand(2));
    if (accel > 20)
    {
      accel -= 2;
    }
    if (accel < -20)
    {
      accel += 2;
    }
  }
}

static void InitBuffers(PluginInfo* goomInfo, const uint32_t resx, const uint32_t resy)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  if ((data->prevX != resx) || (data->prevY != resy))
  {
    data->prevX = resx;
    data->prevY = resy;

    if (data->brutS)
    {
      free(data->freebrutS);
    }
    data->brutS = 0;
    if (data->brutD)
    {
      free(data->freebrutD);
    }
    data->brutD = 0;
    if (data->brutT)
    {
      free(data->freebrutT);
    }
    data->brutT = 0;

    data->middleX = resx / 2;
    data->middleY = resy / 2;
    data->mustInitBuffers = 1;
    if (data->firedec)
    {
      free(data->firedec);
    }
    data->firedec = 0;
  }

  data->mustInitBuffers = 0;
  data->freebrutS = (int*)calloc(resx * resy * 2 + 128, sizeof(int));
  data->brutS = (int32_t*)((1 + (uintptr_t((data->freebrutS))) / 128) * 128);

  data->freebrutD = (int*)calloc(resx * resy * 2 + 128, sizeof(int));
  data->brutD = (int32_t*)((1 + (uintptr_t((data->freebrutD))) / 128) * 128);

  data->freebrutT = (int*)calloc(resx * resy * 2 + 128, sizeof(int));
  data->brutT = (int32_t*)((1 + (uintptr_t((data->freebrutT))) / 128) * 128);

  data->buffratio = 0;

  data->firedec = (int*)malloc(data->prevY * sizeof(int));
  generateTheWaterFXHorizontalDirectionBuffer(goomInfo, data);

  data->interlaceStart = 0;
  makeZoomBufferStripe(data, resy);

  /* Copy the data from temp to dest and source */
  memcpy(data->brutS, data->brutT, resx * resy * 2 * sizeof(int));
  memcpy(data->brutD, data->brutT, resx * resy * 2 * sizeof(int));
}

/**
 * Main work for the dynamic displacement map.
 * 
 * Reads data from pix1, write to pix2.
 *
 * Useful datas for this FX are stored in ZoomFilterData.
 * 
 * If you think that this is a strange function name, let me say that a long time ago,
 *  there has been a slow version and a gray-level only one. Then came these function,
 *  fast and workin in RGB colorspace ! nice but it only was applying a zoom to the image.
 *  So that is why you have this name, for the nostalgy of the first days of goom
 *  when it was just a tiny program writen in Turbo Pascal on my i486...
 */
void zoomFilterFastRGB(PluginInfo* goomInfo,
                       Pixel* pix1,
                       Pixel* pix2,
                       ZoomFilterData* zf,
                       const uint16_t resx,
                       const uint16_t resy,
                       const int switchIncr,
                       const float switchMult)
{
  logDebug("resx = {}, resy = {}, switchIncr = {}, switchMult = {:.2}", resx, resy, switchIncr,
           switchMult);

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  if (!BVAL(data->enabled_bp))
  {
    return;
  }

  // changement de taille
  logDebug("data->prevX = {}, data->prevY = {}", data->prevX, data->prevY);
  if (data->mustInitBuffers)
  {
    logDebug("Calling InitBuffers");
    InitBuffers(goomInfo, resx, resy);
  }

  if (data->interlaceStart != -2)
  {
    zf = NULL;
  }

  // changement de config
  if (zf)
  {
    data->reverse = zf->reverse;
    data->generalSpeed = static_cast<float>(zf->vitesse - 128) / 128.0f;
    if (data->reverse)
    {
      data->generalSpeed = -data->generalSpeed;
    }
    data->middleX = zf->middleX;
    data->middleY = zf->middleY;
    data->theMode = zf->mode;
    data->hPlaneEffect = zf->hPlaneEffect;
    data->vPlaneEffect = zf->vPlaneEffect;
    data->waveEffect = zf->waveEffect;
    data->hypercosEffect = zf->hypercosEffect;
    data->perlinNoisify = zf->perlinNoisify;
    data->noisify = zf->noisify;
    data->interlaceStart = 0;
  }

  // generation du buffer de trans
  logDebug("data->interlaceStart = {}", data->interlaceStart);
  if (data->interlaceStart == -1)
  {
    /* sauvegarde de l'etat actuel dans la nouvelle source
     * TODO: write that in MMX (has been done in previous version, but did not follow
     * some new fonctionnalities) */
    const uint32_t y = data->prevX * data->prevY * 2;
    for (uint32_t x = 0; x < y; x += 2)
    {
      int brutSmypos = data->brutS[x];
      const uint32_t x2 = x + 1;

      data->brutS[x] =
          brutSmypos + (((data->brutD[x] - brutSmypos) * data->buffratio) >> BUFFPOINTNB);
      brutSmypos = data->brutS[x2];
      data->brutS[x2] =
          brutSmypos + (((data->brutD[x2] - brutSmypos) * data->buffratio) >> BUFFPOINTNB);
    }
    data->buffratio = 0;
  }

  logDebug("data->interlaceStart = {}", data->interlaceStart);
  if (data->interlaceStart == -1)
  {
    int* tmp = data->brutD;
    data->brutD = data->brutT;
    data->brutT = tmp;
    tmp = data->freebrutD;
    data->freebrutD = data->freebrutT;
    data->freebrutT = tmp;
    data->interlaceStart = -2;
  }

  logDebug("data->interlaceStart = {}", data->interlaceStart);
  if (data->interlaceStart >= 0)
  {
    // creation de la nouvelle destination
    makeZoomBufferStripe(data, resy / 16);
  }

  if (switchIncr != 0)
  {
    data->buffratio += switchIncr;
    if (data->buffratio > BUFFPOINTMASK)
    {
      data->buffratio = BUFFPOINTMASK;
    }
  }

  if (switchMult != 1.0f)
  {
    data->buffratio = static_cast<int>(static_cast<float>(BUFFPOINTMASK) * (1.0f - switchMult) +
                                       static_cast<float>(data->buffratio) * switchMult);
  }

  data->zoomWidth = data->prevX;

  goomInfo->methods.zoom_filter(data->prevX, data->prevY, pix1, pix2, data->brutS, data->brutD,
                                data->buffratio, data->precalCoef);
}

static void generatePrecalCoef(int precalCoef[16][16])
{
  for (int coefh = 0; coefh < 16; coefh++)
  {
    for (int coefv = 0; coefv < 16; coefv++)
    {
      const int diffcoeffh = sqrtperte - coefh;
      const int diffcoeffv = sqrtperte - coefv;

      int i;
      if (!(coefh || coefv))
      {
        i = 255;
      }
      else
      {
        int i1 = diffcoeffh * diffcoeffv;
        int i2 = coefh * diffcoeffv;
        int i3 = diffcoeffh * coefv;
        int i4 = coefh * coefv;

        // TODO: faire mieux...
        if (i1)
        {
          i1--;
        }
        if (i2)
        {
          i2--;
        }
        if (i3)
        {
          i3--;
        }
        if (i4)
        {
          i4--;
        }

        i = (i1) | (i2 << 8) | (i3 << 16) | (i4 << 24);
      }
      precalCoef[coefh][coefv] = i;
    }
  }
}

/* VisualFX Wrapper */

static const char *const vfxname = "ZoomFilter";

static void zoomFilterSave(VisualFX* _this, const PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "w");

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);

  save_int_setting(f, vfxname, "data->zoomWidth", static_cast<int>(data->zoomWidth));
  save_int_setting(f, vfxname, "data->prevX", static_cast<int>(data->prevX));
  save_int_setting(f, vfxname, "data->prevY", static_cast<int>(data->prevY));
  save_float_setting(f, vfxname, "data->generalSpeed", data->generalSpeed);
  save_int_setting(f, vfxname, "data->reverse", data->reverse);
  save_int_setting(f, vfxname, "data->theMode", static_cast<int>(data->theMode));
  save_int_setting(f, vfxname, "data->waveEffect", data->waveEffect);
  save_int_setting(f, vfxname, "data->hypercosEffect", data->hypercosEffect);
  save_int_setting(f, vfxname, "data->vPlaneEffect", data->vPlaneEffect);
  save_int_setting(f, vfxname, "data->hPlaneEffect", data->hPlaneEffect);
  save_int_setting(f, vfxname, "data->perlinNoisify", static_cast<int>(data->perlinNoisify));
  save_int_setting(f, vfxname, "data->noisify", static_cast<int>(data->noisify));
  save_int_setting(f, vfxname, "data->middleX", static_cast<int>(data->middleX));
  save_int_setting(f, vfxname, "data->middleY", static_cast<int>(data->middleY));

  save_int_setting(f, vfxname, "data->mustInitBuffers", data->mustInitBuffers);
  save_int_setting(f, vfxname, "data->interlaceStart", data->interlaceStart);

  save_int_setting(f, vfxname, "data->buffratio", data->buffratio);
  //    int precalCoef[BUFFPOINTNB][BUFFPOINTNB];

  fclose(f);
}

static void zoomFilterRestore(VisualFX* _this, PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "r");
  if (f == NULL)
  {
    exit(EXIT_FAILURE);
  }

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);

  data->zoomWidth = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->zoomWidth"));
  data->prevX = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->prevX"));
  data->prevY = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->prevY"));
  data->generalSpeed = get_float_setting(f, vfxname, "data->generalSpeed");
  data->reverse = get_int_setting(f, vfxname, "data->reverse");
  data->theMode = static_cast<ZoomFilterMode>(get_int_setting(f, vfxname, "data->theMode"));
  data->waveEffect = get_int_setting(f, vfxname, "data->waveEffect");
  data->hypercosEffect = get_int_setting(f, vfxname, "data->hypercosEffect");
  data->vPlaneEffect = get_int_setting(f, vfxname, "data->vPlaneEffect");
  data->hPlaneEffect = get_int_setting(f, vfxname, "data->hPlaneEffect");
  data->perlinNoisify = get_int_setting(f, vfxname, "data->perlinNoisify");
  data->noisify = get_int_setting(f, vfxname, "data->noisify");
  data->middleX = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->middleX"));
  data->middleY = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->middleY"));

  data->mustInitBuffers = get_int_setting(f, vfxname, "data->mustInitBuffers");
  data->interlaceStart = get_int_setting(f, vfxname, "data->interlaceStart");

  data->buffratio = get_int_setting(f, vfxname, "data->buffratio");

  fclose(f);
}

static void zoomFilterVisualFXWrapper_init(VisualFX* _this, PluginInfo* info)
{
  FilterDataWrapper* data = (FilterDataWrapper*)(malloc(sizeof(FilterDataWrapper)));

  data->coeffs = 0;
  data->freecoeffs = 0;
  data->brutS = 0;
  data->freebrutS = 0;
  data->brutD = 0;
  data->freebrutD = 0;
  data->brutT = 0;
  data->freebrutT = 0;
  data->prevX = 0;
  data->prevY = 0;

  data->mustInitBuffers = 1;
  data->interlaceStart = -2;

  data->generalSpeed = 0.0f;
  data->reverse = false;
  data->theMode = ZoomFilterMode::amuletteMode;
  data->waveEffect = false;
  data->hypercosEffect = false;
  data->vPlaneEffect = 0;
  data->hPlaneEffect = 0;
  data->perlinNoisify = false;
  data->noisify = true;

  /** modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16) */
  data->buffratio = 0;
  data->firedec = 0;

  data->enabled_bp = secure_b_param("Enabled", 1);

  data->params = plugin_parameters("ZoomFilter", 1);
  data->params.params[0] = &data->enabled_bp;

  _this->params = &data->params;
  _this->fx_data = (void*)data;

  /** modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos */
  generatePrecalCoef(data->precalCoef);

  InitBuffers(info, info->screen.width, info->screen.height);
}

static void zoomFilterVisualFXWrapper_free(VisualFX* _this)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);
  free(data->freebrutS);
  free(data->freebrutD);
  free(data->freebrutT);
  free(data->firedec);
  free(data->params.params);
  free(_this->fx_data);
}

static void zoomFilterVisualFXWrapper_apply(VisualFX*, Pixel*, Pixel*, PluginInfo*)
{
}

VisualFX zoomFilterVisualFXWrapper_create(void)
{
  VisualFX fx;
  fx.init = zoomFilterVisualFXWrapper_init;
  fx.free = zoomFilterVisualFXWrapper_free;
  fx.apply = zoomFilterVisualFXWrapper_apply;
  fx.save = zoomFilterSave;
  fx.restore = zoomFilterRestore;
  return fx;
}
