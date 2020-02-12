/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      Copyright (C) 2016-2019 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define __STDC_LIMIT_MACROS

#include "workers.hpp"
#include "buffer_savers.hpp"

extern "C" {
#include "goom.h"
#include "goom_config.h"
}

#include <kodi/addon-instance/Visualization.h>
#include <kodi/gui/gl/Shader.h>
#include <kodi/General.h>

#include <condition_variable>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#define GOOM_TEXTURE_WIDTH 1280
#define GOOM_TEXTURE_HEIGHT 720

class ATTRIBUTE_HIDDEN CVisualizationGoom
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstanceVisualization,
    private kodi::gui::gl::CShaderProgram
{
public:
  CVisualizationGoom();
  ~CVisualizationGoom() override;

  bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  void Stop() override;
  bool IsDirty() override;
  void Render() override;
  void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) override;
  bool UpdateTrack(const VisTrack &track) override;

  // kodi::gui::gl::CShaderProgram
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  bool InitGLObjects();
  void InitQuadData();

  const static int g_tex_width = GOOM_TEXTURE_WIDTH;
  const static int g_tex_height = GOOM_TEXTURE_HEIGHT;
  const static int g_numGoomBufferElements = 4 * g_tex_width * g_tex_height;
  const static int g_goomBufferSize = g_numGoomBufferElements * sizeof(unsigned char);
  
  int m_window_width;
  int m_window_height;
  int m_window_xpos;
  int m_window_ypos;

  int m_channels;
  std::string m_currentSongName = "";
  std::string m_lastSongName = "";
  bool m_titleChange = false;
  bool m_showTitleAlways = false;

  GLint m_componentsPerVertex;
  GLint m_componentsPerTexel;
  int m_numVertices;
  int m_numElements;
  GLfloat* m_quadData = nullptr;

#ifdef HAS_GL
  const static bool g_usePixelBufferObjects = true;
#endif
  GLuint m_textureId = 0;
  const static int g_numPbos = 3;
  GLuint m_pboIds[g_numPbos];
  unsigned char* m_pboGoomBuffer[g_numPbos];
  int m_currentPboIndex;
  glm::mat4 m_projModelMatrix;
  GLuint m_vaoObject = 0;
  GLuint m_vertexVBO = 0;
  GLint m_uProjModelMatLoc = -1;
  GLint m_aPositionLoc = -1;
  GLint m_aCoordLoc = -1;

  // Goom's data itself
  PluginInfo* m_goom = nullptr;

  // Audio buffer storage
  const static int g_audioDataBufferLen = NUM_AUDIO_SAMPLES * AUDIO_SAMPLE_LEN;
  const static int g_audioDataBufferSize = g_audioDataBufferLen * sizeof(short);
  short m_audioData[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN];
  bool m_audioStarted = false;
  unsigned long m_numTimesAudioSampled = 0;

  // Goom process thread handles
  std::thread m_workerThread;
  Worker m_worker;

  // Screen frames storage
  unsigned long m_frameNum = 0;
  const static int g_maxNumSkippedFramesInARow = 0;
  int m_numSkippedFramesInARow = 0;
  unsigned long m_numBufferWaits = 0;
  void UpdateGoomBuffer(const void* audioData, unsigned long audioTag, void* goomBuffer);
  void Logger(int severity, const std::string& msg);

  BufferSaver<short> m_audioBufferSaver;
  const static int g_maxNumSavedAudioBuffers = 10000;
  const static bool g_saveAudioBuffers = false;

  BufferSaver<unsigned char> m_goomBufferSaver;
  const static int g_maxNumSavedGoomBuffers = 10000;
  const static bool g_saveGoomBuffers = false;

  // Start flag to know init was OK
  bool m_started = false;
};
