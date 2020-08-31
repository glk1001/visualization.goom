#include "lines.h"

#include "drawmethods.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

static inline unsigned char lighten(unsigned char value, float power)
{
  int val = value;
  float t = (float)val * log10(power) / 2.0;

  if (t > 0)
  {
    val = (int)t; /* (32.0f * log (t)); */
    if (val > 255)
    {
      val = 255;
    }
    if (val < 0)
    {
      val = 0;
    }
    return val;
  }
  else
  {
    return 0;
  }
}

static void lightencolor(uint32_t* col, float power)
{
  unsigned char* color = (unsigned char*)(col);

  *color = lighten(*color, power);
  color++;
  *color = lighten(*color, power);
  color++;
  *color = lighten(*color, power);
  color++;
  *color = lighten(*color, power);
}

static void genline(
    const LineTypes id, const float param, GMUnitPointer* l, const uint32_t rx, const uint32_t ry)
{
  switch (id)
  {
    case LineTypes::GML_HLINE:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].x = (static_cast<float>(i) * rx) / static_cast<float>(AUDIO_SAMPLE_LEN);
        l[i].y = param;
        l[i].angle = M_PI / 2.0f;
      }
      return;
    case LineTypes::GML_VLINE:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].y = (static_cast<float>(i) * ry) / static_cast<float>(AUDIO_SAMPLE_LEN);
        l[i].x = param;
        l[i].angle = 0.0f;
      }
      return;
    case LineTypes::GML_CIRCLE:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(AUDIO_SAMPLE_LEN);
        const float cosa = param * cos(l[i].angle);
        const float sina = param * sin(l[i].angle);
        l[i].x = static_cast<float>(rx) / 2.0f + cosa;
        l[i].y = static_cast<float>(ry) / 2.0f + sina;
      }
      return;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }
}

static uint32_t getcouleur(const int mode)
{
  switch (mode)
  {
    case GML_RED:
      return (230 << (ROUGE * 8)) | (120 << (VERT * 8)) | (18 << (BLEU * 8));
    case GML_ORANGE_J:
      return (120 << (VERT * 8)) | (252 << (ROUGE * 8)) | (18 << (BLEU * 8));
    case GML_ORANGE_V:
      return (160 << (VERT * 8)) | (236 << (ROUGE * 8)) | (40 << (BLEU * 8));
    case GML_BLEUBLANC:
      return (40 << (BLEU * 8)) | (220 << (ROUGE * 8)) | (140 << (VERT * 8));
    case GML_VERT:
      return (200 << (VERT * 8)) | (80 << (ROUGE * 8)) | (18 << (BLEU * 8));
    case GML_BLEU:
      return (250 << (BLEU * 8)) | (30 << (VERT * 8)) | (80 << (ROUGE * 8));
    case GML_BLACK:
      return (16 << (BLEU * 8)) | (16 << (VERT * 8)) | (16 << (ROUGE * 8));
    default:
      throw std::logic_error("Unknown line color.");
  }
  return 0;
}

void goom_lines_set_res(GMLine* gml, const uint32_t rx, const uint32_t ry)
{
  if (gml)
  {
    gml->screenX = rx;
    gml->screenY = ry;

    genline(gml->IDdest, gml->param, gml->points2, rx, ry);
  }
}

static void goom_lines_move(GMLine* l)
{
  for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
  {
    l->points[i].x = (l->points2[i].x + 39.0f * l->points[i].x) / 40.0f;
    l->points[i].y = (l->points2[i].y + 39.0f * l->points[i].y) / 40.0f;
    l->points[i].angle = (l->points2[i].angle + 39.0f * l->points[i].angle) / 40.0f;
  }

  unsigned char* c1 = reinterpret_cast<unsigned char*>(&l->color);
  unsigned char* c2 = reinterpret_cast<unsigned char*>(&l->color2);
  for (int i = 0; i < 4; i++)
  {
    int cc1 = *c1;
    int cc2 = *c2;
    *c1 = static_cast<unsigned char>((cc1 * 63 + cc2) >> 6);
    ++c1;
    ++c2;
  }

  l->power += l->powinc;
  if (l->power < 1.1f)
  {
    l->power = 1.1f;
    l->powinc = static_cast<float>(goom_irand(l->goomInfo->gRandom, 20) + 10) / 300.0f;
  }
  if (l->power > 17.5f)
  {
    l->power = 17.5f;
    l->powinc = -static_cast<float>(goom_irand(l->goomInfo->gRandom, 20) + 10) / 300.0f;
  }

  l->amplitude = (99.0f * l->amplitude + l->amplitudeF) / 100.0f;
}

void goom_lines_switch_to(
    GMLine* gml, const LineTypes IDdest, const float param, const float amplitude, const int col)
{
  genline(IDdest, param, gml->points2, gml->screenX, gml->screenY);
  gml->IDdest = IDdest;
  gml->param = param;
  gml->amplitudeF = amplitude;
  gml->color2 = getcouleur(col);
}

GMLine* goom_lines_init(PluginInfo* goomInfo,
                        const uint32_t rx,
                        const uint32_t ry,
                        const LineTypes IDsrc,
                        const float paramS,
                        const int coulS,
                        const LineTypes IDdest,
                        const float paramD,
                        const int coulD)
{
  GMLine* l = (GMLine*)malloc(sizeof(GMLine));

  l->goomInfo = goomInfo;

  l->colorMaps = new ColorMaps{};

  l->points = (GMUnitPointer*)malloc(AUDIO_SAMPLE_LEN * sizeof(GMUnitPointer));
  l->points2 = (GMUnitPointer*)malloc(AUDIO_SAMPLE_LEN * sizeof(GMUnitPointer));
  l->nbPoints = AUDIO_SAMPLE_LEN;

  l->IDdest = IDdest;
  l->param = paramD;

  l->amplitude = l->amplitudeF = 1.0f;

  genline(IDsrc, paramS, l->points, rx, ry);
  genline(IDdest, paramD, l->points2, rx, ry);

  l->color = getcouleur(coulS);
  l->color2 = getcouleur(coulD);

  l->screenX = rx;
  l->screenY = ry;

  l->power = 0.0f;
  l->powinc = 0.01f;

  goom_lines_switch_to(l, IDdest, paramD, 1.0f, coulD);

  return l;
}

void goom_lines_free(GMLine** l)
{
  delete (*l)->colorMaps;

  free((*l)->points);
  free((*l)->points2);
  free(*l);
  l = nullptr;
}

// This factor gives width to the audio samples lines. 30000 seems pleasing.
constexpr int32_t maxNormalizedPeak = 30000;

static inline float getNormalizedData(PluginInfo* goomInfo, const int16_t data)
{
  return static_cast<float>((maxNormalizedPeak * static_cast<int32_t>(data)) /
                            static_cast<int32_t>(goomInfo->sound.allTimesMax));
}

void goom_lines_draw(PluginInfo* goomInfo,
                     GMLine* line,
                     const int16_t data[AUDIO_SAMPLE_LEN],
                     Pixel* p)
{
  if (line != NULL)
  {
    uint32_t color = line->color;
    const GMUnitPointer* pt = &(line->points[0]);

    const float cosa = cos(pt->angle) / 1000.0f;
    const float sina = sin(pt->angle) / 1000.0f;

    lightencolor(&color, line->power);

    const uint32_t randColor = ColorMap::getRandomColor(line->colorMaps->getRandomColorMap());

    const float fdata = getNormalizedData(goomInfo, data[0]);
    int x1 = static_cast<int>(pt->x + cosa * line->amplitude * fdata);
    int y1 = static_cast<int>(pt->y + sina * line->amplitude * fdata);

    for (int i = 1; i < AUDIO_SAMPLE_LEN; i++)
    {
      const GMUnitPointer* pt = &(line->points[i]);

      const float cosa = cos(pt->angle) / 1000.0f;
      const float sina = sin(pt->angle) / 1000.0f;
      const float fdata = getNormalizedData(goomInfo, data[i]);
      const int x2 = static_cast<int>(pt->x + cosa * line->amplitude * fdata);
      const int y2 = static_cast<int>(pt->y + sina * line->amplitude * fdata);

      const float t = fdata / static_cast<float>(maxNormalizedPeak);
      const uint32_t modColor = ColorMap::colorMix(color, randColor, t);

      goomInfo->methods.draw_line(p, x1, y1, x2, y2, modColor, line->screenX, line->screenY);

      x1 = x2;
      y1 = y2;
    }
    goom_lines_move(line);
  }
}
