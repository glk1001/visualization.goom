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
#include "goomutils/SimplexNoise.h"
#include "goomutils/logging.h"
#include "v3d.h"

#include <algorithm>
#include <cstdint>

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
  ZoomFilterData filterData;
  float generalSpeed;

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

  bool mustInitBuffers;
  int interlaceStart;

  // modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16)
  int buffratio;
  int* firedec;

  // modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos
  int precalCoef[BUFFPOINTNB][BUFFPOINTNB];
};

static inline v2g zoomVector(PluginInfo* goomInfo, const float X, const float Y)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  const float sq_dist = X * X + Y * Y;

  /* sx = (X < 0.0f) ? -1.0f : 1.0f;
     sy = (Y < 0.0f) ? -1.0f : 1.0f;
   */
  float coefVitesse = (1.0f + data->generalSpeed) / 50.0f;

  // Effects

  // Centralized FX
  switch (data->filterData.mode)
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
  if (data->filterData.perlinNoisify)
  {
    const float noise = (0.5 * (1.0 + SimplexNoise::noise(X, Y)) - 0.5) / 1.0;
    vx += noise;
    vy -= noise;
    vx *= getRandInRange(0.7f, 1.0f);
    vy *= getRandInRange(0.7f, 1.0f);
    vx += ((static_cast<float>(pcg32_rand())) / static_cast<float>(RAND_MAX) - 0.5f) / 5.0f;
    vy += ((static_cast<float>(pcg32_rand())) / static_cast<float>(RAND_MAX) - 0.5f) / 5.0f;
  }
  if (data->filterData.noisify)
  {
    //    const float xAmp = 1.0/goomInfo->getRandInRange(50.0f, 200.0f);
    //    const float yAmp = 1.0/goomInfo->getRandInRange(50.0f, 200.0f);
    const float amp = 1.0 / goomInfo->getRandInRange(50.0f, 200.0f);
    vx += amp * (goomInfo->getRandInRange(0.0f, 1.0f) - 0.5f);
    vy += amp * (goomInfo->getRandInRange(0.0f, 1.0f) - 0.5f);
  }

  // Hypercos
  if (data->filterData.hypercosEffect)
  {
    vx += sin(Y * 10.0f) / 120.0f;
    vy += sin(X * 10.0f) / 120.0f;
  }

  // H Plane
  if (data->filterData.hPlaneEffect)
  {
    vx += Y * 0.0025f * data->filterData.hPlaneEffect;
  }

  // V Plane
  if (data->filterData.vPlaneEffect)
  {
    vy += X * 0.0025f * data->filterData.vPlaneEffect;
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
static void makeZoomBufferStripe(PluginInfo* goomInfo, const uint32_t INTERLACE_INCR)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);

  // Ratio from pixmap to normalized coordinates
  const float ratio = 2.0f / static_cast<float>(data->prevX);

  // Ratio from normalized to virtual pixmap coordinates
  const float inv_ratio = BUFFPOINTNBF / ratio;
  const float min = ratio / BUFFPOINTNBF;

  // Y position of the pixel to compute in normalized coordinates
  float Y =
      ratio * static_cast<float>(data->interlaceStart - static_cast<int>(data->filterData.middleY));

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
    float X = -(static_cast<float>(data->filterData.middleX)) * ratio;
    for (uint32_t x = 0; x < data->prevX; x++)
    {
      v2g vector = zoomVector(goomInfo, X, Y);
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
                                    static_cast<int>(data->filterData.middleX * BUFFPOINTNB);
      data->brutT[premul_y_prevX + 1] = static_cast<int>((Y - vector.y) * inv_ratio) +
                                        static_cast<int>(data->filterData.middleY * BUFFPOINTNB);
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

inline Pixel getMixedColor(const uint32_t coeffs,
                           const Pixel& col1,
                           const Pixel& col2,
                           const Pixel& col3,
                           const Pixel& col4)
{
  const uint32_t c1 = coeffs & 0xff;
  const uint32_t c2 = (coeffs >> 8) & 0xff;
  const uint32_t c3 = (coeffs >> 16) & 0xff;
  const uint32_t c4 = (coeffs >> 24) & 0xff;

  uint32_t r =
      col1.channels.r * c1 + col2.channels.r * c2 + col3.channels.r * c3 + col4.channels.r * c4;
  if (r > 5)
  {
    r -= 5;
  }
  r >>= 8;

  uint32_t g =
      col1.channels.g * c1 + col2.channels.g * c2 + col3.channels.g * c3 + col4.channels.g * c4;
  if (g > 5)
  {
    g -= 5;
  }
  g >>= 8;

  uint32_t b =
      col1.channels.b * c1 + col2.channels.b * c2 + col3.channels.b * c3 + col4.channels.b * c4;
  if (b > 5)
  {
    b -= 5;
  }
  b >>= 8;

  return Pixel{.channels = {.r = static_cast<uint8_t>(r),
                            .g = static_cast<uint8_t>(g),
                            .b = static_cast<uint8_t>(b),
                            .a = 0xff}};
}

inline Pixel getPixelRGB(const Pixel* buffer, const uint32_t x)
{
  return *(buffer + x);
}

inline void setPixelRGB(Pixel* buffer, const uint32_t x, const Pixel& p)
{
  buffer[x] = p;
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

    const Pixel col1 = getPixelRGB(expix1, pos);
    const Pixel col2 = getPixelRGB(expix1, pos + 1);
    const Pixel col3 = getPixelRGB(expix1, pos + bufwidth);
    const Pixel col4 = getPixelRGB(expix1, pos + bufwidth + 1);

    const Pixel newColor = getMixedColor(coeffs, col1, col2, col3, col4);

    setPixelRGB(expix2, static_cast<uint32_t>(myPos >> 1), newColor);
  }
}

static void generateWaterFXHorizontalBuffer(PluginInfo* goomInfo, FilterDataWrapper* data)
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

static void initBuffers(PluginInfo* goomInfo, const uint32_t resx, const uint32_t resy)
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

    data->filterData.middleX = resx / 2;
    data->filterData.middleY = resy / 2;
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
  generateWaterFXHorizontalBuffer(goomInfo, data);

  data->interlaceStart = 0;
  makeZoomBufferStripe(goomInfo, resy);

  /* Copy the data from temp to dest and source */
  memcpy(data->brutS, data->brutT, resx * resy * 2 * sizeof(int));
  memcpy(data->brutD, data->brutT, resx * resy * 2 * sizeof(int));
}

void zoomFilterInitData(PluginInfo* goomInfo)
{
  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(goomInfo->zoomFilter_fx.fx_data);
  data->filterData = goomInfo->update.zoomFilterData;
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
    logDebug("Calling initBuffers");
    initBuffers(goomInfo, resx, resy);
  }

  if (data->interlaceStart != -2)
  {
    zf = nullptr;
  }

  // changement de config
  if (zf)
  {
    data->filterData = *zf;
    data->generalSpeed = static_cast<float>(zf->vitesse - 128) / 128.0f;
    if (data->filterData.reverse)
    {
      data->generalSpeed = -data->generalSpeed;
    }
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
    makeZoomBufferStripe(goomInfo, resy / 16);
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

static const char* const vfxname = "ZoomFilter";

static void zoomFilterSave(VisualFX* _this, const PluginInfo*, const char* file)
{
  FILE* f = fopen(file, "w");

  FilterDataWrapper* data = static_cast<FilterDataWrapper*>(_this->fx_data);

  save_int_setting(f, vfxname, "data->zoomWidth", static_cast<int>(data->zoomWidth));
  save_int_setting(f, vfxname, "data->prevX", static_cast<int>(data->prevX));
  save_int_setting(f, vfxname, "data->prevY", static_cast<int>(data->prevY));
  save_float_setting(f, vfxname, "data->generalSpeed", data->generalSpeed);
  save_int_setting(f, vfxname, "data->reverse", data->filterData.reverse);
  save_int_setting(f, vfxname, "data->mode", static_cast<int>(data->filterData.mode));
  save_int_setting(f, vfxname, "data->waveEffect", data->filterData.waveEffect);
  save_int_setting(f, vfxname, "data->hypercosEffect", data->filterData.hypercosEffect);
  save_int_setting(f, vfxname, "data->vPlaneEffect", data->filterData.vPlaneEffect);
  save_int_setting(f, vfxname, "data->hPlaneEffect", data->filterData.hPlaneEffect);
  save_int_setting(f, vfxname, "data->perlinNoisify",
                   static_cast<int>(data->filterData.perlinNoisify));
  save_int_setting(f, vfxname, "data->noisify", static_cast<int>(data->filterData.noisify));
  save_int_setting(f, vfxname, "data->middleX", static_cast<int>(data->filterData.middleX));
  save_int_setting(f, vfxname, "data->middleY", static_cast<int>(data->filterData.middleY));

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
  data->filterData.reverse = get_int_setting(f, vfxname, "data->reverse");
  data->filterData.mode = static_cast<ZoomFilterMode>(get_int_setting(f, vfxname, "data->mode"));
  data->filterData.waveEffect = get_int_setting(f, vfxname, "data->waveEffect");
  data->filterData.hypercosEffect = get_int_setting(f, vfxname, "data->hypercosEffect");
  data->filterData.vPlaneEffect = get_int_setting(f, vfxname, "data->vPlaneEffect");
  data->filterData.hPlaneEffect = get_int_setting(f, vfxname, "data->hPlaneEffect");
  data->filterData.perlinNoisify = get_int_setting(f, vfxname, "data->perlinNoisify");
  data->filterData.noisify = get_int_setting(f, vfxname, "data->noisify");
  data->filterData.middleX = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->middleX"));
  data->filterData.middleY = static_cast<uint32_t>(get_int_setting(f, vfxname, "data->middleY"));

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
  data->filterData.reverse = false;
  data->filterData.mode = ZoomFilterMode::crystalBallMode;
  data->filterData.waveEffect = false;
  data->filterData.hypercosEffect = false;
  data->filterData.vPlaneEffect = 0;
  data->filterData.hPlaneEffect = 0;
  data->filterData.perlinNoisify = false;
  data->filterData.noisify = true;

  /** modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16) */
  data->buffratio = 0;
  data->firedec = 0;

  data->enabled_bp = secure_b_param("Enabled", 1);

  data->params = plugin_parameters("ZoomFilter", 1);
  data->params.params[0] = &data->enabled_bp;

  _this->params = &data->params;
  _this->fx_data = data;

  /** modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos */
  generatePrecalCoef(data->precalCoef);

  initBuffers(info, info->screen.width, info->screen.height);
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

VisualFX zoomFilterVisualFXWrapper_create()
{
  VisualFX fx;
  fx.init = zoomFilterVisualFXWrapper_init;
  fx.free = zoomFilterVisualFXWrapper_free;
  fx.apply = zoomFilterVisualFXWrapper_apply;
  fx.save = zoomFilterSave;
  fx.restore = zoomFilterRestore;
  return fx;
}
