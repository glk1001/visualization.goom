/**
* file: goom_core.c
 * author: Jean-Christophe Hoelt (which is not so proud of it)
 *
 * Contains the core of goom's work.
 *
 * (c)2000-2003, by iOS-software.
 */

#include "filters.h"
#include "gfontlib.h"
#include "goom.h"
#include "goom_config.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"
#include "goomutils/logging.h"
#include "ifs.h"
#include "lines.h"
#include "sound_tester.h"
#include "tentacle3d.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

constexpr int32_t stopSpeed = 128;
// TODO: put that as variable in PluginInfo
constexpr int32_t timeBetweenChange = 300;

static void init_buffers(PluginInfo* goomInfo, const uint32_t buffsize)
{
  goomInfo->pixel = (uint32_t*)malloc(buffsize * sizeof(uint32_t) + 128);
  bzero(goomInfo->pixel, buffsize * sizeof(uint32_t) + 128);
  goomInfo->back = (uint32_t*)malloc(buffsize * sizeof(uint32_t) + 128);
  bzero(goomInfo->back, buffsize * sizeof(uint32_t) + 128);
  goomInfo->conv = (Pixel*)malloc(buffsize * sizeof(uint32_t) + 128);
  bzero(goomInfo->conv, buffsize * sizeof(uint32_t) + 128);

  goomInfo->outputBuf = goomInfo->conv;

  goomInfo->p1 = (Pixel*)((1 + ((uintptr_t)(goomInfo->pixel)) / 128) * 128);
  goomInfo->p2 = (Pixel*)((1 + ((uintptr_t)(goomInfo->back)) / 128) * 128);
}

static void swapBuffers(PluginInfo* goomInfo)
{
  Pixel* tmp = goomInfo->p1;
  goomInfo->p1 = goomInfo->p2;
  goomInfo->p2 = tmp;
}

/**************************
*         INIT           *
**************************/
PluginInfo* goom_init(const uint16_t resx, const uint16_t resy, const int seed)
{
  logDebug("Initialize goom: resx = {}, resy = {}, seed = {}.", resx, resy, seed);

  PluginInfo* goomInfo = (PluginInfo*)malloc(sizeof(PluginInfo));

  plugin_info_init(goomInfo, 5);

  goomInfo->star_fx = flying_star_create();
  goomInfo->zoomFilter_fx = zoomFilterVisualFXWrapper_create();
  goomInfo->tentacles_fx = tentacle_fx_create();
  goomInfo->convolve_fx = convolve_create();
  goomInfo->ifs_fx = ifs_visualfx_create();

  goomInfo->screen.width = resx;
  goomInfo->screen.height = resy;
  goomInfo->screen.size = resx * resy;

  init_buffers(goomInfo, goomInfo->screen.size);

  if (seed >= 0)
  {
    const uint64_t seedVal =
        seed == 0 ? reinterpret_cast<uint64_t>(goomInfo->pixel) : static_cast<uint64_t>(seed);
    pcg32_init(seedVal);
  }
  goomInfo->gRandom = goom_random_init();

  goomInfo->star_fx.init(&goomInfo->star_fx, goomInfo);
  goomInfo->zoomFilter_fx.init(&goomInfo->zoomFilter_fx, goomInfo);
  goomInfo->tentacles_fx.init(&goomInfo->tentacles_fx, goomInfo);
  goomInfo->convolve_fx.init(&goomInfo->convolve_fx, goomInfo);
  goomInfo->ifs_fx.init(&goomInfo->ifs_fx, goomInfo);
  plugin_info_add_visual(goomInfo, 0, &goomInfo->zoomFilter_fx);
  plugin_info_add_visual(goomInfo, 1, &goomInfo->tentacles_fx);
  plugin_info_add_visual(goomInfo, 2, &goomInfo->star_fx);
  plugin_info_add_visual(goomInfo, 3, &goomInfo->convolve_fx);
  plugin_info_add_visual(goomInfo, 4, &goomInfo->ifs_fx);

  goomInfo->cycle = 0;

  goomInfo->gmline1 = goom_lines_init(goomInfo, resx, goomInfo->screen.height, LineTypes::GML_HLINE,
                                      goomInfo->screen.height, GML_BLACK, LineTypes::GML_CIRCLE,
                                      0.4f * static_cast<float>(goomInfo->screen.height), GML_VERT);
  goomInfo->gmline2 = goom_lines_init(goomInfo, resx, goomInfo->screen.height, LineTypes::GML_HLINE,
                                      0, GML_BLACK, LineTypes::GML_CIRCLE,
                                      0.2f * static_cast<float>(goomInfo->screen.height), GML_RED);

  gfont_load();

  /* goom_set_main_script(goomInfo, goomInfo->main_script_str); */

  return goomInfo;
}

void goom_set_resolution(PluginInfo* goomInfo, const uint16_t resx, const uint16_t resy)
{
  free(goomInfo->pixel);
  free(goomInfo->back);
  free(goomInfo->conv);

  goomInfo->screen.width = resx;
  goomInfo->screen.height = resy;
  goomInfo->screen.size = resx * resy;

  init_buffers(goomInfo, goomInfo->screen.size);

  /* init_ifs (goomInfo, resx, goomInfo->screen.height); */
  goomInfo->ifs_fx.free(&goomInfo->ifs_fx);
  goomInfo->ifs_fx.init(&goomInfo->ifs_fx, goomInfo);

  goom_lines_set_res(goomInfo->gmline1, resx, goomInfo->screen.height);
  goom_lines_set_res(goomInfo->gmline2, resx, goomInfo->screen.height);
}

int goom_set_screenbuffer(PluginInfo* goomInfo, void* buffer)
{
  goomInfo->outputBuf = static_cast<Pixel*>(buffer);
  return 1;
}

/********************************************
 *                  UPDATE                  *
 ********************************************
*/

static float largeSoundFactor(const SoundInfo* sound);
static void updateDecayRecay(PluginInfo* goomInfo);

// baisser regulierement la vitesse
static void regularlyLowerTheSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void lowerSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd);

// on verifie qu'il ne se pas un truc interressant avec le son.
static void changeModeIfMusicChanges(PluginInfo* goomInfo, const int forceMode);

// changement eventuel de mode
static void changeMode(PluginInfo* goomInfo);

// Changement d'effet de zoom !
static void changeZoomEffect(PluginInfo* goomInfo, ZoomFilterData* pzfd, const int forceMode);

static void applyIfsIfRequired(PluginInfo* goomInfo);

// Affichage tentacule
static void applyTentaclesAndStars(PluginInfo* goomInfo);

// Affichage de texte
void displayText(PluginInfo* goomInfo, const char* songTitle, const char* message, const float fps);

static void drawPointsIfRequired(PluginInfo* goomInfo,
                                 const uint32_t pointWidth,
                                 const uint32_t pointHeight,
                                 const float largfactor);
static void drawPoints(PluginInfo* goomInfo,
                       const uint32_t pointWidth,
                       const uint32_t pointHeight,
                       const float largfactor);

static void choose_a_goom_line(PluginInfo* goomInfo,
                               float* param1,
                               float* param2,
                               int* couleur,
                               LineTypes* mode,
                               float* amplitude,
                               const int far);

// si on est dans un goom : afficher les lignes
static void displayLinesIfInAGoom(PluginInfo* goomInfo,
                                  const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);
static void displayLines(PluginInfo* goomInfo,
                         const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN]);

// arret demande
static void stopRequest(PluginInfo* goomInfo);
static void stopIfRequested(PluginInfo* goomInfo);

// arret aleatore.. changement de mode de ligne..
static void stopRandomLineChangeMode(PluginInfo* goomInfo);

// Permet de forcer un effet.
static void forceEffectIfSet(PluginInfo* oomInfo, ZoomFilterData** pzfd, const int forceMode);
static void forceEffect(PluginInfo* goomInfo, const int forceMode, ZoomFilterData** pzfd);

// arreter de decrémenter au bout d'un certain temps
static void stopDecrementingAfterAWhile(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void stopDecrementing(PluginInfo* goomInfo, ZoomFilterData** pzfd);

// tout ceci ne sera fait qu'en cas de non-blocage
static void bigUpdateIfNotLocked(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void bigUpdate(PluginInfo* goomInfo, ZoomFilterData** pzfd);

// gros frein si la musique est calme
static void bigBreakIfMusicIsCalm(PluginInfo* goomInfo, ZoomFilterData** pzfd);
static void bigBreak(PluginInfo* goomInfo, ZoomFilterData** pzfd);

static void update_message(PluginInfo* goomInfo, const char* message);

static void pointFilter(PluginInfo* goomInfo,
                        Pixel* pix1,
                        const Color& c,
                        const float t1,
                        const float t2,
                        const float t3,
                        const float t4,
                        const uint32_t cycle);


uint32_t* goom_update(PluginInfo* goomInfo,
                      const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN],
                      const int forceMode,
                      const float fps,
                      const char* songTitle,
                      const char* message)
{
  // elargissement de l'intervalle d'évolution des points
  // ! calcul du deplacement des petits points ...
  const uint16_t pointWidth = (goomInfo->screen.width * 2) / 5;
  const uint16_t pointHeight = (goomInfo->screen.height * 2) / 5;

  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
  logDebug("pcg32_get_last_state = {}", pcg32_get_last_state());
  logDebug("goomInfo->gRandom->pos = {}", goomInfo->gRandom->pos);

  /* ! etude du signal ... */
  evaluate_sound(data, &(goomInfo->sound));
  const float largfactor = largeSoundFactor(&goomInfo->sound);

  updateDecayRecay(goomInfo);

  applyIfsIfRequired(goomInfo);

  drawPointsIfRequired(goomInfo, pointWidth, pointHeight, largfactor);

  /* par défaut pas de changement de zoom */
  ZoomFilterData* pzfd = nullptr;
  if (forceMode != 0)
  {
    logDebug("forcemode = {}\n", forceMode);
  }

  if (--goomInfo->update.lockvar < 0)
  {
    goomInfo->update.lockvar = 0;
  }
  /* note pour ceux qui n'ont pas suivis : le lockvar permet d'empecher un */
  /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
  // -- Note for those who have not followed: the lockvar prevents a change
  // of state of the plugin just after another change of state.

  changeModeIfMusicChanges(goomInfo, forceMode);

  bigUpdateIfNotLocked(goomInfo, &pzfd);

  bigBreakIfMusicIsCalm(goomInfo, &pzfd);

  regularlyLowerTheSpeed(goomInfo, &pzfd);

  stopDecrementingAfterAWhile(goomInfo, &pzfd);

  forceEffectIfSet(goomInfo, &pzfd, forceMode);

  changeZoomEffect(goomInfo, pzfd, forceMode);

  // Zoom here!
  zoomFilterFastRGB(goomInfo, goomInfo->p1, goomInfo->p2, pzfd, goomInfo->screen.width,
                    goomInfo->screen.height, goomInfo->update.switchIncr,
                    goomInfo->update.switchMult);

  applyTentaclesAndStars(goomInfo);

  displayText(goomInfo, songTitle, message, fps);

  // Gestion du Scope - Scope management
  stopIfRequested(goomInfo);
  stopRandomLineChangeMode(goomInfo);
  displayLinesIfInAGoom(goomInfo, data);

  // affichage et swappage des buffers...
  Pixel* return_val = goomInfo->p1;
  swapBuffers(goomInfo);
  goomInfo->cycle++;
  goomInfo->convolve_fx.apply(&goomInfo->convolve_fx, return_val, goomInfo->outputBuf, goomInfo);

  logDebug("About to return.");

  return (uint32_t*)goomInfo->outputBuf;
}

/****************************************
*                CLOSE                 *
****************************************/
void goom_close(PluginInfo* goomInfo)
{
  if (goomInfo->pixel)
  {
    free(goomInfo->pixel);
  }
  if (goomInfo->back)
  {
    free(goomInfo->back);
  }
  if (goomInfo->conv)
  {
    free(goomInfo->conv);
  }

  goomInfo->pixel = goomInfo->back = nullptr;
  goomInfo->conv = nullptr;
  goom_random_free(goomInfo->gRandom);
  goom_lines_free(&goomInfo->gmline1);
  goom_lines_free(&goomInfo->gmline2);

  /* release_ifs (); */
  goomInfo->ifs_fx.free(&goomInfo->ifs_fx);
  goomInfo->convolve_fx.free(&goomInfo->convolve_fx);
  goomInfo->star_fx.free(&goomInfo->star_fx);
  goomInfo->tentacles_fx.free(&goomInfo->tentacles_fx);
  goomInfo->zoomFilter_fx.free(&goomInfo->zoomFilter_fx);

  // Release info visual
  free(goomInfo->params);
  free(goomInfo->sound.params.params);

  // Release PluginInfo
  free(goomInfo->visuals);

  free(goomInfo);
}

static void choose_a_goom_line(PluginInfo* goomInfo,
                               float* param1,
                               float* param2,
                               int* couleur,
                               LineTypes* mode,
                               float* amplitude,
                               const int far)
{
  *amplitude = 1.0f;
  *mode = static_cast<LineTypes>(goom_irand(goomInfo->gRandom, numLineTypes));
  switch (*mode)
  {
    case LineTypes::GML_CIRCLE:
      if (far)
      {
        *param1 = *param2 = 0.47f;
        *amplitude = 0.8f;
        break;
      }
      if (goom_irand(goomInfo->gRandom, 3) == 0)
      {
        *param1 = *param2 = 0;
        *amplitude = 3.0f;
      }
      else if (goom_irand(goomInfo->gRandom, 2))
      {
        *param1 = 0.40f * goomInfo->screen.height;
        *param2 = 0.22f * goomInfo->screen.height;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.height * 0.35;
      }
      break;
    case LineTypes::GML_HLINE:
      if (goom_irand(goomInfo->gRandom, 4) || far)
      {
        *param1 = goomInfo->screen.height / 7;
        *param2 = 6.0f * goomInfo->screen.height / 7.0f;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.height / 2.0f;
        *amplitude = 2.0f;
      }
      break;
    case LineTypes::GML_VLINE:
      if (goom_irand(goomInfo->gRandom, 3) || far)
      {
        *param1 = goomInfo->screen.width / 7.0f;
        *param2 = 6.0f * goomInfo->screen.width / 7.0f;
      }
      else
      {
        *param1 = *param2 = goomInfo->screen.width / 2.0f;
        *amplitude = 1.5f;
      }
      break;
    default:
      throw std::logic_error("Unknown LineTypes enum.");
  }

  *couleur = static_cast<int>(goom_irand(goomInfo->gRandom, 6));
}

constexpr float ECART_VARIATION = 1.5;
constexpr float POS_VARIATION = 3.0;
constexpr int SCROLLING_SPEED = 80;

/*
 * Met a jour l'affichage du message defilant
 */
static void update_message(PluginInfo* goomInfo, const char* message)
{
  int fin = 0;

  if (message)
  {
    strcpy(goomInfo->update_message.message, message);
    uint32_t i = 1;
    for (size_t j = 0; goomInfo->update_message.message[j]; j++)
    {
      if (goomInfo->update_message.message[j] == '\n')
      {
        i++;
      }
    }
    goomInfo->update_message.numberOfLinesInMessage = i;
    goomInfo->update_message.affiche =
        goomInfo->screen.height + goomInfo->update_message.numberOfLinesInMessage * 25 + 105;
    goomInfo->update_message.longueur = strlen(goomInfo->update_message.message);
  }
  if (goomInfo->update_message.affiche)
  {
    uint32_t i = 0;
    char* msg = (char*)malloc((size_t)goomInfo->update_message.longueur + 1);
    char* ptr = msg;
    message = msg;
    strcpy(msg, goomInfo->update_message.message);

    while (!fin)
    {
      while (1)
      {
        if (*ptr == 0)
        {
          fin = 1;
          break;
        }
        if (*ptr == '\n')
        {
          *ptr = 0;
          break;
        }
        ++ptr;
      }
      uint32_t pos = goomInfo->update_message.affiche -
                     (goomInfo->update_message.numberOfLinesInMessage - i) * 25;
      pos += POS_VARIATION * (cos((double)pos / 20.0));
      pos -= SCROLLING_SPEED;
      const float ecart = (ECART_VARIATION * sin((double)pos / 20.0));
      if ((fin) && (2 * pos < goomInfo->screen.height))
      {
        pos = goomInfo->screen.height / 2;
      }
      pos += 7;

      goom_draw_text(goomInfo->p1, goomInfo->screen.width, goomInfo->screen.height,
                     static_cast<int>(goomInfo->screen.width / 2), static_cast<int>(pos), message,
                     ecart, 1);
      message = ++ptr;
      i++;
    }
    goomInfo->update_message.affiche--;
    free(msg);
  }
}

static void drawPointsIfRequired(PluginInfo* goomInfo,
                                 const uint32_t pointWidth,
                                 const uint32_t pointHeight,
                                 const float largfactor)
{
  logDebug("goomInfo->curGState->drawPoints = {}", goomInfo->curGState->drawPoints);
  if (goomInfo->curGState->drawPoints)
  {
    logDebug("goomInfo->curGState->drawPoints = {} is true", goomInfo->curGState->drawPoints);
    drawPoints(goomInfo, pointWidth, pointHeight, largfactor);
  }
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
}

static void drawPoints(PluginInfo* goomInfo,
                       const uint32_t pointWidth,
                       const uint32_t pointHeight,
                       const float largfactor)
{
  const uint32_t speedvarMult80Plus15 = goomInfo->sound.speedvar * 80 + 15;
  const uint32_t speedvarMult50Plus1 = goomInfo->sound.speedvar * 50 + 1;
  logDebug("speedvarMult80Plus15 = {}", speedvarMult80Plus15);
  logDebug("speedvarMult50Plus1 = {}", speedvarMult50Plus1);

  const float pointWidthDiv2 = pointWidth / 2;
  const float pointHeightDiv2 = pointHeight / 2;
  const float pointWidthDiv3 = pointWidth / 3;
  const float pointHeightDiv3 = pointHeight / 3;
  const float yellow_t1 = (pointWidth - 6.0f) * largfactor + 5.0f;
  const float yellow_t2 = (pointHeight - 6.0f) * largfactor + 5.0f;
  const float black_t1 = pointHeightDiv3 * largfactor + 20.0f;
  const float black_t2 = black_t1;
  const float pointWidthDiv2MultLarge = pointWidthDiv2 * largfactor;
  const float pointHeightDiv2MultLarge = pointHeightDiv2 * largfactor;
  const float pointWidthDiv3MultLarge = (pointWidthDiv3 + 5.0f) * largfactor;
  const float pointHeightDiv3MultLarge = (pointHeightDiv3 + 5.0f) * largfactor;
  const float pointWidthMultLarge = pointWidth * largfactor;
  const float pointHeightMultLarge = pointHeight * largfactor;

  logDebug("goomInfo->update.loopvar = {}", goomInfo->update.loopvar);
  for (uint32_t i = 1; i * 15 <= speedvarMult80Plus15; i++)
  {
    goomInfo->update.loopvar += speedvarMult50Plus1;
    logDebug("goomInfo->update.loopvar = {}", goomInfo->update.loopvar);

    const uint32_t loopvar_div_i = goomInfo->update.loopvar / i;
    const float i_mult_10 = 10.0f * i;

    const float yellow_t3 = i * 152.0f;
    const float yellow_t4 = 128.0f;
    const uint32_t yellow_cycle = goomInfo->update.loopvar + i * 2032;
    const float orange_t1 = pointWidthDiv2MultLarge / i + i_mult_10;
    const float orange_t2 = pointHeightDiv2MultLarge / i + i_mult_10;
    const float orange_t3 = 96.0f;
    const float orange_t4 = i * 80.0f;
    const uint32_t orange_cycle = loopvar_div_i;
    const float violet_t1 = pointWidthDiv3MultLarge / i + i_mult_10;
    const float violet_t2 = pointHeightDiv3MultLarge / i + i_mult_10;
    const float violet_t3 = i + 122.0f;
    const float violet_t4 = 134.0f;
    const uint32_t violet_cycle = loopvar_div_i;
    const float black_t3 = 58.0f;
    const float black_t4 = i * 66.0f;
    const uint32_t black_cycle = loopvar_div_i;
    const float white_t1 = (pointWidthMultLarge + i_mult_10) / i;
    const float white_t2 = (pointHeightMultLarge + i_mult_10) / i;
    const float white_t3 = 66.0f;
    const float white_t4 = 74.0f;
    const uint32_t white_cycle = goomInfo->update.loopvar + i * 500;

    pointFilter(goomInfo, goomInfo->p1, YELLOW, yellow_t1, yellow_t2, yellow_t3, yellow_t4,
                yellow_cycle);
    pointFilter(goomInfo, goomInfo->p1, ORANGE, orange_t1, orange_t2, orange_t3, orange_t4,
                orange_cycle);
    pointFilter(goomInfo, goomInfo->p1, VIOLET, violet_t1, violet_t2, violet_t3, violet_t4,
                violet_cycle);
    pointFilter(goomInfo, goomInfo->p1, BLACK, black_t1, black_t2, black_t3, black_t4, black_cycle);
    pointFilter(goomInfo, goomInfo->p1, WHITE, white_t1, white_t2, white_t3, white_t4, white_cycle);
  }
}

static void changeModeIfMusicChanges(PluginInfo* goomInfo, const int forceMode)
{
  logDebug("goomInfo->sound.timeSinceLastGoom = {}, forceMode = {}, "
           "goomInfo->update.cyclesSinceLastChange = {}",
           goomInfo->sound.timeSinceLastGoom, forceMode, goomInfo->update.cyclesSinceLastChange);
  if ((forceMode != -1) && ((goomInfo->sound.timeSinceLastGoom == 0) || (forceMode > 0) ||
                            (goomInfo->update.cyclesSinceLastChange > timeBetweenChange)))
  {
    logDebug("goomInfo->sound.timeSinceLastGoom = {}, forceMode = {}, "
             "goomInfo->update.cyclesSinceLastChange = {}",
             goomInfo->sound.timeSinceLastGoom, forceMode, goomInfo->update.cyclesSinceLastChange);
    changeMode(goomInfo);
  }
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
}

static void changeMode(PluginInfo* goomInfo)
{
  const uint32_t rand16 = goom_irand(goomInfo->gRandom, 16);
  logDebug("goom_irand(goomInfo->gRandom,16) = {}", rand16);
  if (rand16 != 0)
  {
    return;
  }

  logDebug("goom_irand(goomInfo->gRandom, 16) = 0");
  switch (goom_irand(goomInfo->gRandom, 34))
  {
    case 0:
    case 10:
      goomInfo->update.zoomFilterData.hypercosEffect =
          static_cast<int>(goom_irand(goomInfo->gRandom, 2));
      [[fallthrough]]; // TODO: Is this really a fallthrough????
    case 13:
    case 20:
    case 21:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::waveMode;
      goomInfo->update.zoomFilterData.reverse = false;
      goomInfo->update.zoomFilterData.waveEffect = (goom_irand(goomInfo->gRandom, 3) == 0);
      if (goom_irand(goomInfo->gRandom, 2))
      {
        goomInfo->update.zoomFilterData.vitesse =
            (goomInfo->update.zoomFilterData.vitesse + 127) >> 1;
      }
      break;
    case 1:
    case 11:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case 2:
    case 12:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::amuletteMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case 3:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::waterMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case 4:
    case 14:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case 5:
    case 15:
    case 22:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::hyperCos1Mode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = (goom_irand(goomInfo->gRandom, 3) == 0);
      break;
    case 6:
    case 16:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::hyperCos2Mode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
      break;
    case 7:
    case 17:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::crystalBallMode;
      goomInfo->update.zoomFilterData.waveEffect = (goom_irand(goomInfo->gRandom, 4) == 0);
      goomInfo->update.zoomFilterData.hypercosEffect =
          static_cast<int>(goom_irand(goomInfo->gRandom, 2));
      break;
    case 8:
    case 18:
    case 19:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::scrunchMode;
      goomInfo->update.zoomFilterData.waveEffect = true;
      goomInfo->update.zoomFilterData.hypercosEffect = true;
      break;
    case 29:
    case 30:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::yOnlyMode;
      break;
    case 31:
    case 32:
    case 33:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::speedwayMode;
      break;
    default:
      goomInfo->update.zoomFilterData.mode = ZoomFilterMode::normalMode;
      goomInfo->update.zoomFilterData.waveEffect = false;
      goomInfo->update.zoomFilterData.hypercosEffect = false;
  }
}

static void bigUpdate(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  // reperage de goom (acceleration forte de l'acceleration du volume)
  //   -> coup de boost de la vitesse si besoin..
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
  if (goomInfo->sound.timeSinceLastGoom == 0)
  {
    logDebug("goomInfo->sound.timeSinceLastGoom = 0");

    goomInfo->update.goomvar++;

    // Selection of the Goom state
    if ((!goomInfo->update.stateSelectionBlocker) && (goom_irand(goomInfo->gRandom, 3)))
    {
      goomInfo->update.stateSelectionRand = static_cast<int>(
          goom_irand(goomInfo->gRandom, static_cast<uint32_t>(goomInfo->statesRangeMax)));
      goomInfo->update.stateSelectionBlocker = 3;
    }
    else if (goomInfo->update.stateSelectionBlocker)
    {
      goomInfo->update.stateSelectionBlocker--;
    }

    for (int i = 0; i < goomInfo->statesNumber; i++)
    {
      if ((goomInfo->update.stateSelectionRand >= goomInfo->states[i].minSelect) &&
          (goomInfo->update.stateSelectionRand <= goomInfo->states[i].maxSelect))
      {
        goomInfo->curGState = &(goomInfo->states[i]);
        goomInfo->curGStateIndex = i;
        break;
      }
    }

    if ((goomInfo->curGState->drawIFS) && (goomInfo->update.ifs_incr <= 0))
    {
      goomInfo->update.recay_ifs = 5;
      goomInfo->update.ifs_incr = 11;
    }
    if ((!goomInfo->curGState->drawIFS) && (goomInfo->update.ifs_incr > 0) &&
        (goomInfo->update.decay_ifs <= 0))
    {
      goomInfo->update.decay_ifs = 100;
    }

    if (!goomInfo->curGState->drawScope)
    {
      goomInfo->update.stop_lines = 0xf000 & 5;
    }
    if (!goomInfo->curGState->farScope)
    {
      goomInfo->update.stop_lines = 0;
      goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
    }

    // if (goomInfo->update.goomvar % 1 == 0)
    {
      goomInfo->update.lockvar = 50;
      const int32_t newvit =
          stopSpeed + 1 - static_cast<int32_t>(3.5f * log10(goomInfo->sound.speedvar * 60 + 1));
      // retablir le zoom avant..
      if ((goomInfo->update.zoomFilterData.reverse) && (!(goomInfo->cycle % 13)) &&
          (pcg32_rand() % 5 == 0))
      {
        goomInfo->update.zoomFilterData.reverse = false;
        goomInfo->update.zoomFilterData.vitesse = stopSpeed - 2;
        goomInfo->update.lockvar = 75;
      }
      if (goom_irand(goomInfo->gRandom, 10) == 0)
      {
        goomInfo->update.zoomFilterData.reverse = true;
        goomInfo->update.lockvar = 100;
      }

      if (goom_irand(goomInfo->gRandom, 10) == 0)
      {
        goomInfo->update.zoomFilterData.vitesse = stopSpeed - 1;
      }
      if (goom_irand(goomInfo->gRandom, 12) == 0)
      {
        goomInfo->update.zoomFilterData.vitesse = stopSpeed + 1;
      }

      // changement de milieu..
      switch (goom_irand(goomInfo->gRandom, 25))
      {
        case 0:
        case 3:
        case 6:
          goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height - 1;
          goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
          break;
        case 1:
        case 4:
          goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width - 1;
          break;
        case 2:
        case 5:
          goomInfo->update.zoomFilterData.middleX = 1;
          break;
        default:
          goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
          goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
      }

      if ((goomInfo->update.zoomFilterData.mode == ZoomFilterMode::waterMode) ||
          (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::yOnlyMode) ||
          (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode))
      {
        goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
        goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
      }

      switch (const uint32_t vtmp = (goom_irand(goomInfo->gRandom, 15)))
      {
        case 0:
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 3)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 3));
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 3)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 3));
          break;
        case 3:
          goomInfo->update.zoomFilterData.vPlaneEffect = 0;
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 8)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 8));
          break;
        case 4:
        case 5:
        case 6:
        case 7:
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 5)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 5));
          goomInfo->update.zoomFilterData.hPlaneEffect =
              -goomInfo->update.zoomFilterData.vPlaneEffect;
          break;
        case 8:
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(5 + goom_irand(goomInfo->gRandom, 8));
          goomInfo->update.zoomFilterData.vPlaneEffect =
              -goomInfo->update.zoomFilterData.hPlaneEffect;
          break;
        case 9:
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(5 + goom_irand(goomInfo->gRandom, 8));
          goomInfo->update.zoomFilterData.hPlaneEffect =
              -goomInfo->update.zoomFilterData.hPlaneEffect;
          break;
        case 13:
          goomInfo->update.zoomFilterData.hPlaneEffect = 0;
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 10)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 10));
          break;
        case 14:
          goomInfo->update.zoomFilterData.hPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 10)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 10));
          goomInfo->update.zoomFilterData.vPlaneEffect =
              static_cast<int>(goom_irand(goomInfo->gRandom, 10)) -
              static_cast<int>(goom_irand(goomInfo->gRandom, 10));
          break;
        default:
          if (vtmp < 10)
          {
            goomInfo->update.zoomFilterData.vPlaneEffect = 0;
            goomInfo->update.zoomFilterData.hPlaneEffect = 0;
          }
      }

      if (goom_irand(goomInfo->gRandom, 5) != 0)
      {
        goomInfo->update.zoomFilterData.noisify = 0;
      }
      else
      {
        goomInfo->update.zoomFilterData.noisify =
            static_cast<int>(goom_irand(goomInfo->gRandom, 2) + 1);
        goomInfo->update.lockvar *= 2;
      }

      if (goomInfo->update.zoomFilterData.mode == ZoomFilterMode::amuletteMode)
      {
        goomInfo->update.zoomFilterData.vPlaneEffect = 0;
        goomInfo->update.zoomFilterData.hPlaneEffect = 0;
        goomInfo->update.zoomFilterData.noisify = 0;
      }

      if ((goomInfo->update.zoomFilterData.middleX == 1) ||
          (goomInfo->update.zoomFilterData.middleX == goomInfo->screen.width - 1))
      {
        goomInfo->update.zoomFilterData.vPlaneEffect = 0;
        if (goom_irand(goomInfo->gRandom, 2))
        {
          goomInfo->update.zoomFilterData.hPlaneEffect = 0;
        }
      }

      logDebug("newvit = {}, goomInfo->update.zoomFilterData.vitesse = {}", newvit,
               goomInfo->update.zoomFilterData.vitesse);
      if (newvit < goomInfo->update.zoomFilterData.vitesse)
      { // on accelere
        logDebug("newvit = {} < {} = goomInfo->update.zoomFilterData.vitesse", newvit,
                 goomInfo->update.zoomFilterData.vitesse);
        *pzfd = &goomInfo->update.zoomFilterData;
        if (((newvit < (stopSpeed - 7)) &&
             (goomInfo->update.zoomFilterData.vitesse < stopSpeed - 6) &&
             (goomInfo->cycle % 3 == 0)) ||
            (goom_irand(goomInfo->gRandom, 40) == 0))
        {
          goomInfo->update.zoomFilterData.vitesse =
              stopSpeed - static_cast<int32_t>(goom_irand(goomInfo->gRandom, 2)) +
              static_cast<int32_t>(goom_irand(goomInfo->gRandom, 2));
          goomInfo->update.zoomFilterData.reverse = !goomInfo->update.zoomFilterData.reverse;
        }
        else
        {
          goomInfo->update.zoomFilterData.vitesse =
              (newvit + goomInfo->update.zoomFilterData.vitesse * 7) / 8;
        }
        goomInfo->update.lockvar += 50;
      }
    }

    if (goomInfo->update.lockvar > 150)
    {
      goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
      goomInfo->update.switchMult = 1.0f;
    }
  }

  // mode mega-lent
  const uint32_t rand700 = goom_irand(goomInfo->gRandom, 700);
  logDebug("rand700 = {}", rand700);
  if (rand700 == 0)
  {
    logDebug("rand700 = 0");
    *pzfd = &goomInfo->update.zoomFilterData;
    goomInfo->update.zoomFilterData.vitesse = stopSpeed - 1;
    goomInfo->update.zoomFilterData.pertedec = 8;
    goomInfo->update.goomvar = 1;
    goomInfo->update.lockvar += 50;
    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
    goomInfo->update.switchMult = 1.0f;
  }
}

/* Changement d'effet de zoom !
 */
static void changeZoomEffect(PluginInfo* goomInfo, ZoomFilterData* pzfd, const int forceMode)
{
  if (pzfd != NULL)
  {
    logDebug("pzfd != NULL");

    goomInfo->update.cyclesSinceLastChange = 0;
    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;

    int dif = static_cast<int>(goomInfo->update.zoomFilterData.vitesse) -
              static_cast<int>(goomInfo->update.previousZoomSpeed);
    if (dif < 0)
    {
      dif = -dif;
    }

    if (dif > 2)
    {
      goomInfo->update.switchIncr *= (dif + 2) / 2;
    }
    goomInfo->update.previousZoomSpeed = goomInfo->update.zoomFilterData.vitesse;
    goomInfo->update.switchMult = 1.0f;

    if (((goomInfo->sound.timeSinceLastGoom == 0) && (goomInfo->sound.totalgoom < 2)) ||
        (forceMode > 0))
    {
      goomInfo->update.switchIncr = 0;
      goomInfo->update.switchMult = goomInfo->update.switchMultAmount;
    }
  }
  else
  {
    logDebug("pzfd = NULL");
    logDebug("goomInfo->update.cyclesSinceLastChange = {}", goomInfo->update.cyclesSinceLastChange);
    if (goomInfo->update.cyclesSinceLastChange > timeBetweenChange)
    {
      logDebug("goomInfo->update.cyclesSinceLastChange = {} > {} = timeBetweenChange",
               goomInfo->update.cyclesSinceLastChange, timeBetweenChange);
      pzfd = &goomInfo->update.zoomFilterData;
      goomInfo->update.cyclesSinceLastChange = 0;
    }
    else
    {
      goomInfo->update.cyclesSinceLastChange++;
    }
  }

  if (pzfd)
  {
    logDebug("pzfd->mode = {}", pzfd->mode);
  }
}

static void applyTentaclesAndStars(PluginInfo* goomInfo)
{
  logDebug("Before goomInfo->tentacles_fx.apply, goomInfo->star_fx.apply");
  goomInfo->tentacles_fx.apply(&goomInfo->tentacles_fx, goomInfo->p2, goomInfo->p1, goomInfo);
  goomInfo->star_fx.apply(&goomInfo->star_fx, goomInfo->p2, goomInfo->p1, goomInfo);
}

void displayText(PluginInfo* goomInfo, const char* songTitle, const char* message, const float fps)
{
  // Le message
  update_message(goomInfo, message);

  if (fps > 0)
  {
    char text[256];
    sprintf(text, "%2.0f fps", fps);
    goom_draw_text(goomInfo->p1, goomInfo->screen.width, goomInfo->screen.height, 10, 24, text, 1,
                   0);
  }

  // Le titre
  if (songTitle != NULL)
  {
    strncpy(goomInfo->update.titleText, songTitle, 1023);
    goomInfo->update.titleText[1023] = 0;
    goomInfo->update.timeOfTitleDisplay = 200;
  }

  if (goomInfo->update.timeOfTitleDisplay)
  {
    goom_draw_text(goomInfo->p1, goomInfo->screen.width, goomInfo->screen.height,
                   static_cast<int>(goomInfo->screen.width / 2),
                   static_cast<int>(goomInfo->screen.height / 2 + 7), goomInfo->update.titleText,
                   static_cast<float>(190 - goomInfo->update.timeOfTitleDisplay) / 10.0f, 1);
    goomInfo->update.timeOfTitleDisplay--;
    if (goomInfo->update.timeOfTitleDisplay < 4)
    {
      goom_draw_text(goomInfo->p2, goomInfo->screen.width, goomInfo->screen.height,
                     static_cast<int>(goomInfo->screen.width / 2),
                     static_cast<int>(goomInfo->screen.height / 2 + 7), goomInfo->update.titleText,
                     static_cast<float>(190 - goomInfo->update.timeOfTitleDisplay) / 10.0f, 1);
    }
  }
}

static void stopRequest(PluginInfo* goomInfo)
{
  logDebug("goomInfo->update.stop_lines = {}, goomInfo->curGState->drawScope = {}",
           goomInfo->update.stop_lines, goomInfo->curGState->drawScope);

  float param1 = 0;
  float param2 = 0;
  float amplitude = 0;
  int couleur = 0;
  LineTypes mode;
  choose_a_goom_line(goomInfo, &param1, &param2, &couleur, &mode, &amplitude, 1);
  couleur = GML_BLACK;

  goom_lines_switch_to(goomInfo->gmline1, mode, param1, amplitude, couleur);
  goom_lines_switch_to(goomInfo->gmline2, mode, param2, amplitude, couleur);
  goomInfo->update.stop_lines &= 0x0fff;
}

/* arret aleatore.. changement de mode de ligne..
  */
static void stopRandomLineChangeMode(PluginInfo* goomInfo)
{
  if (goomInfo->update.lineMode != goomInfo->update.drawLinesDuration)
  {
    goomInfo->update.lineMode--;
    if (goomInfo->update.lineMode == -1)
    {
      goomInfo->update.lineMode = 0;
    }
  }
  else if ((goomInfo->cycle % 80 == 0) && (goom_irand(goomInfo->gRandom, 5) == 0) &&
           goomInfo->update.lineMode)
  {
    goomInfo->update.lineMode--;
  }

  if ((goomInfo->cycle % 120 == 0) && (goom_irand(goomInfo->gRandom, 4) == 0) &&
      (goomInfo->curGState->drawScope))
  {
    if (goomInfo->update.lineMode == 0)
    {
      goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
    }
    else if (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration)
    {
      goomInfo->update.lineMode--;

      float param1 = 0;
      float param2 = 0;
      float amplitude = 0;
      int couleur1 = 0;
      LineTypes mode;
      choose_a_goom_line(goomInfo, &param1, &param2, &couleur1, &mode, &amplitude,
                         goomInfo->update.stop_lines);

      int couleur2 = 5 - couleur1;
      if (goomInfo->update.stop_lines)
      {
        goomInfo->update.stop_lines--;
        if (goom_irand(goomInfo->gRandom, 2))
        {
          couleur2 = couleur1 = GML_BLACK;
        }
      }

      logDebug("goomInfo->update.lineMode = {} == {} = goomInfo->update.drawLinesDuration",
               goomInfo->update.lineMode, goomInfo->update.drawLinesDuration);
      goom_lines_switch_to(goomInfo->gmline1, mode, param1, amplitude, couleur1);
      goom_lines_switch_to(goomInfo->gmline2, mode, param2, amplitude, couleur2);
    }
  }
}

static void displayLines(PluginInfo* goomInfo,
                         const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  goomInfo->gmline2->power = goomInfo->gmline1->power;

  goom_lines_draw(goomInfo, goomInfo->gmline1, data[0], goomInfo->p2);
  goom_lines_draw(goomInfo, goomInfo->gmline2, data[1], goomInfo->p2);

  if (((goomInfo->cycle % 121) == 9) && (goom_irand(goomInfo->gRandom, 3) == 1) &&
      ((goomInfo->update.lineMode == 0) ||
       (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration)))
  {

    logDebug("goomInfo->cycle % 121 etc.: goomInfo->cycle = {}, rand1_3 = ?", goomInfo->cycle);
    float param1 = 0;
    float param2 = 0;
    float amplitude = 0;
    int couleur1 = 0;
    LineTypes mode;
    choose_a_goom_line(goomInfo, &param1, &param2, &couleur1, &mode, &amplitude,
                       goomInfo->update.stop_lines);
    int couleur2 = 5 - couleur1;

    if (goomInfo->update.stop_lines)
    {
      goomInfo->update.stop_lines--;
      if (goom_irand(goomInfo->gRandom, 2))
      {
        couleur2 = couleur1 = GML_BLACK;
      }
    }
    goom_lines_switch_to(goomInfo->gmline1, mode, param1, amplitude, couleur1);
    goom_lines_switch_to(goomInfo->gmline2, mode, param2, amplitude, couleur2);
  }
}

static void bigBreakIfMusicIsCalm(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->sound.speedvar = {:.2}, goomInfo->update.zoomFilterData.vitesse = {}, "
           "goomInfo->cycle = {}",
           goomInfo->sound.speedvar, goomInfo->update.zoomFilterData.vitesse, goomInfo->cycle);
  if ((goomInfo->sound.speedvar < 0.01f) &&
      (goomInfo->update.zoomFilterData.vitesse < (stopSpeed - 4)) && (goomInfo->cycle % 16 == 0))
  {
    logDebug("goomInfo->sound.speedvar = {:.2}", goomInfo->sound.speedvar);
    bigBreak(goomInfo, pzfd);
  }
}

static void bigBreak(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse += 3;
  goomInfo->update.zoomFilterData.pertedec = 8;
  goomInfo->update.goomvar = 0;
}

static void forceEffect(PluginInfo* goomInfo, const int forceMode, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  (*pzfd)->mode = static_cast<ZoomFilterMode>(forceMode - 1);
}

static void lowerSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.vitesse++;
}

static void stopDecrementing(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  *pzfd = &goomInfo->update.zoomFilterData;
  goomInfo->update.zoomFilterData.pertedec = 8;
}

static float largeSoundFactor(const SoundInfo* sound)
{
  float largfactor = sound->speedvar / 150.0f + sound->volume / 1.5f;
  if (largfactor > 1.5f)
  {
    largfactor = 1.5f;
  }
  return largfactor;
}

static void updateDecayRecay(PluginInfo* goomInfo)
{
  goomInfo->update.decay_ifs--;
  if (goomInfo->update.decay_ifs > 0)
  {
    goomInfo->update.ifs_incr += 2;
  }
  if (goomInfo->update.decay_ifs == 0)
  {
    goomInfo->update.ifs_incr = 0;
  }

  if (goomInfo->update.recay_ifs)
  {
    goomInfo->update.ifs_incr -= 2;
    goomInfo->update.recay_ifs--;
    if ((goomInfo->update.recay_ifs == 0) && (goomInfo->update.ifs_incr <= 0))
    {
      goomInfo->update.ifs_incr = 1;
    }
  }
}

static void bigUpdateIfNotLocked(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->update.lockvar = {}", goomInfo->update.lockvar);
  if (goomInfo->update.lockvar == 0)
  {
    logDebug("goomInfo->update.lockvar = 0");
    bigUpdate(goomInfo, pzfd);
  }
  logDebug("goomInfo->sound.timeSinceLastGoom = {}", goomInfo->sound.timeSinceLastGoom);
}

static void forceEffectIfSet(PluginInfo* goomInfo, ZoomFilterData** pzfd, const int forceMode)
{
  constexpr size_t numFilterFx = static_cast<size_t>(ZoomFilterMode::_size);

  logDebug("forceMode = {}", forceMode);
  if ((forceMode > 0) && (size_t(forceMode) <= numFilterFx))
  {
    logDebug("forceMode = {} <= numFilterFx = {}.", forceMode, numFilterFx);
    forceEffect(goomInfo, forceMode, pzfd);
  }
  if (forceMode == -1)
  {
    pzfd = NULL;
  }
}

static void stopIfRequested(PluginInfo* goomInfo)
{
  logDebug("goomInfo->update.stop_lines = {}, goomInfo->curGState->drawScope = {}",
           goomInfo->update.stop_lines, goomInfo->curGState->drawScope);
  if ((goomInfo->update.stop_lines & 0xf000) || !goomInfo->curGState->drawScope)
  {
    stopRequest(goomInfo);
  }
}

static void displayLinesIfInAGoom(PluginInfo* goomInfo,
                                  const int16_t data[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN])
{
  logDebug("goomInfo->update.lineMode = {} != 0 || goomInfo->sound.timeSinceLastGoom = {}",
           goomInfo->update.lineMode, goomInfo->sound.timeSinceLastGoom);
  if ((goomInfo->update.lineMode != 0) || (goomInfo->sound.timeSinceLastGoom < 5))
  {
    logDebug("goomInfo->update.lineMode = {} != 0 || goomInfo->sound.timeSinceLastGoom = {} < 5",
             goomInfo->update.lineMode, goomInfo->sound.timeSinceLastGoom);

    displayLines(goomInfo, data);
  }
}

static void applyIfsIfRequired(PluginInfo* goomInfo)
{
  logDebug("goomInfo->update.ifs_incr = {}", goomInfo->update.ifs_incr);
  if (goomInfo->update.ifs_incr > 0)
  {
    logDebug("goomInfo->update.ifs_incr = {} > 0", goomInfo->update.ifs_incr);
    goomInfo->ifs_fx.apply(&goomInfo->ifs_fx, goomInfo->p2, goomInfo->p1, goomInfo);
  }
}

static void regularlyLowerTheSpeed(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->update.zoomFilterData.vitesse = {}, goomInfo->cycle = {}",
           goomInfo->update.zoomFilterData.vitesse, goomInfo->cycle);
  if ((goomInfo->cycle % 73 == 0) && (goomInfo->update.zoomFilterData.vitesse < (stopSpeed - 5)))
  {
    logDebug("goomInfo->cycle % 73 = 0 && dgoomInfo->update.zoomFilterData.vitesse = {} < {} - 5, ",
             goomInfo->cycle, goomInfo->update.zoomFilterData.vitesse, stopSpeed);
    lowerSpeed(goomInfo, pzfd);
  }
}

static void stopDecrementingAfterAWhile(PluginInfo* goomInfo, ZoomFilterData** pzfd)
{
  logDebug("goomInfo->cycle = {}, goomInfo->update.zoomFilterData.pertedec = {}", goomInfo->cycle,
           goomInfo->update.zoomFilterData.pertedec);
  if ((goomInfo->cycle % 101 == 0) && (goomInfo->update.zoomFilterData.pertedec == 7))
  {
    logDebug("goomInfo->cycle % 101 = 0 && dgoomInfo->update.zoomFilterData.pertedec = 7, ",
             goomInfo->cycle, goomInfo->update.zoomFilterData.vitesse);
    stopDecrementing(goomInfo, pzfd);
  }
}

static inline void setPixelRGB(
    PluginInfo* goomInfo, Pixel* buffer, const uint32_t x, const uint32_t y, const Color& c)
{
  Pixel i;
  i.channels.b = c.b;
  i.channels.g = c.g;
  i.channels.r = c.r;

  *(buffer + (x + (y * goomInfo->screen.width))) = i;
}

static void pointFilter(PluginInfo* goomInfo,
                        Pixel* pix1,
                        const Color& c,
                        const float t1,
                        const float t2,
                        const float t3,
                        const float t4,
                        const uint32_t cycle)
{
  const uint32_t x = goomInfo->screen.width / 2 + uint32_t(t1 * cos(float(cycle) / t3));
  const uint32_t y = goomInfo->screen.height / 2 + uint32_t(t2 * sin(float(cycle) / t4));

  if ((x > 1) && (y > 1) && (x < uint32_t(goomInfo->screen.width) - 2) &&
      (y < uint32_t(goomInfo->screen.height) - 2))
  {
    setPixelRGB(goomInfo, pix1, x + 1, y, c);
    setPixelRGB(goomInfo, pix1, x, y + 1, c);
    setPixelRGB(goomInfo, pix1, x + 1, y + 1, WHITE);
    setPixelRGB(goomInfo, pix1, x + 2, y + 1, c);
    setPixelRGB(goomInfo, pix1, x + 1, y + 2, c);
  }
}
