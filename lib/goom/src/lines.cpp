#include "lines.h"

#include "drawmethods.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <tuple>

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
    const LineType id, const float param, GMUnitPointer* l, const uint32_t rx, const uint32_t ry)
{
  switch (id)
  {
    case LineType::hline:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].x = (static_cast<float>(i) * rx) / static_cast<float>(AUDIO_SAMPLE_LEN);
        l[i].y = param;
        l[i].angle = M_PI / 2.0f;
      }
      return;
    case LineType::vline:
      for (uint32_t i = 0; i < AUDIO_SAMPLE_LEN; i++)
      {
        l[i].y = (static_cast<float>(i) * ry) / static_cast<float>(AUDIO_SAMPLE_LEN);
        l[i].x = param;
        l[i].angle = 0.0f;
      }
      return;
    case LineType::circle:
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
      throw std::logic_error("Unknown LineType enum.");
  }
}

void goomLinesSetResolution(GMLine* gml, const uint32_t rx, const uint32_t ry)
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
    l->powinc = static_cast<float>(l->goomInfo->getNRand(20) + 10) / 300.0f;
  }
  if (l->power > 17.5f)
  {
    l->power = 17.5f;
    l->powinc = -static_cast<float>(l->goomInfo->getNRand(20) + 10) / 300.0f;
  }

  l->amplitude = (99.0f * l->amplitude + l->amplitudeF) / 100.0f;
}

void switchGoomLines(GMLine* gml,
                     const LineType dest,
                     const float param,
                     const float amplitude,
                     const uint32_t color)
{
  genline(dest, param, gml->points2, gml->screenX, gml->screenY);
  gml->IDdest = dest;
  gml->param = param;
  gml->amplitudeF = amplitude;
  gml->color2 = color;
}

GMLine* goomLinesInit(PluginInfo* goomInfo,
                      const uint32_t rx,
                      const uint32_t ry,
                      const LineType IDsrc,
                      const float paramS,
                      const uint32_t srcColor,
                      const LineType IDdest,
                      const float paramD,
                      const uint32_t destColor)
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

  l->color = srcColor;
  l->color2 = destColor;

  l->screenX = rx;
  l->screenY = ry;

  l->power = 0.0f;
  l->powinc = 0.01f;

  switchGoomLines(l, IDdest, paramD, 1.0f, destColor);

  return l;
}

void goomLinesFree(GMLine** l)
{
  delete (*l)->colorMaps;

  free((*l)->points);
  free((*l)->points2);
  free(*l);
  l = nullptr;
}

// les modes couleur possible (si tu mets un autre c'est noir)
#define GML_BLEUBLANC 0
#define GML_RED 1
#define GML_ORANGE_V 2
#define GML_ORANGE_J 3
#define GML_VERT 4
#define GML_BLEU 5
#define GML_BLACK 6

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

uint32_t getBlackLineColor()
{
  return getcouleur(GML_BLACK);
}

uint32_t getGreenLineColor()
{
  return getcouleur(GML_VERT);
}

uint32_t getRedLineColor()
{
  return getcouleur(GML_RED);
}

uint32_t getRandomLineColor(PluginInfo* goomInfo)
{
  if (goomInfo->getNRand(10) == 0)
  {
    return getcouleur(static_cast<int>(goomInfo->getNRand(6)));
  }
  return ColorMap::getRandomColor(goomInfo->gmline1->colorMaps->getRandomColorMap());
}

void drawGoomLines(PluginInfo* goomInfo,
                   GMLine* line,
                   const int16_t data[AUDIO_SAMPLE_LEN],
                   Pixel* p)
{
  if (!line)
  {
    return;
  }

  const GMUnitPointer* pt = &(line->points[0]);
  uint32_t color = line->color;
  lightencolor(&color, line->power);

  const float audioRange =
      static_cast<float>(goomInfo->sound.allTimesMax - goomInfo->sound.allTimesMin);
  if (audioRange < 0.0001)
  {
    // No range - flatline audio
    goomInfo->methods.draw_line(p, pt->x, pt->y, pt->x + AUDIO_SAMPLE_LEN, pt->y, color,
                                line->screenX, line->screenY);
    goom_lines_move(line);
    return;
  }

  // This factor gives height to the audio samples lines. This value seems pleasing.
  constexpr float maxNormalizedPeak = 70000;
  const auto getNormalizedData = [&](const int16_t data) {
    return maxNormalizedPeak * static_cast<float>(data - goomInfo->sound.allTimesMin) / audioRange;
  };

  const uint32_t randColor = getRandomLineColor(goomInfo);

  const auto getNextPoint = [&](const GMUnitPointer* pt, const int16_t dataVal) {
    const float cosa = cos(pt->angle) / 1000.0f;
    const float sina = sin(pt->angle) / 1000.0f;
    const float fdata = getNormalizedData(dataVal);
    const int x = static_cast<int>(pt->x + cosa * line->amplitude * fdata);
    const int y = static_cast<int>(pt->y + sina * line->amplitude * fdata);
    const float t = std::max(0.05f, fdata / static_cast<float>(maxNormalizedPeak));
    const uint32_t modColor = ColorMap::colorMix(color, randColor, t);
    return std::make_tuple(x, y, modColor);
  };

  auto [x1, y1, modColor] = getNextPoint(pt, data[0]);

  for (size_t i = 1; i < AUDIO_SAMPLE_LEN; i++)
  {
    const GMUnitPointer* const pt = &(line->points[i]);
    const auto [x2, y2, modColor] = getNextPoint(pt, data[i]);

    goomInfo->methods.draw_line(p, x1, y1, x2, y2, modColor, line->screenX, line->screenY);

    x1 = x2;
    y1 = y2;
  }

  goom_lines_move(line);
}
