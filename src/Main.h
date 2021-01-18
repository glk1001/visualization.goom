#ifndef MAIN_H
#define MAIN_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      Copyright (C) 2016-2020 Team Kodi
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

#include "CircularBuffer.h"
#include "goom/goom_config.h"
#include "goom/goom_control.h"
#include "goom/goom_graphic.h"

#include <condition_variable>
#include <functional>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <kodi/General.h>
#include <kodi/addon-instance/Visualization.h>
#include <kodi/gui/gl/Shader.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#define GOOM_TEXTURE_WIDTH 1280
#define GOOM_TEXTURE_HEIGHT 720

class ATTRIBUTE_HIDDEN CVisualizationGoom : public kodi::addon::CAddonBase,
                                            public kodi::addon::CInstanceVisualization,
                                            private kodi::gui::gl::CShaderProgram
{
public:
  CVisualizationGoom();
  ~CVisualizationGoom() override;
  CVisualizationGoom(const CVisualizationGoom&) noexcept = delete;
  CVisualizationGoom(const CVisualizationGoom&&) noexcept = delete;
  auto operator=(const CVisualizationGoom&) -> CVisualizationGoom = delete;
  auto operator=(const CVisualizationGoom&&) -> CVisualizationGoom = delete;

  auto Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName)
      -> bool override;
  void Stop() override;
  auto IsDirty() -> bool override;
  void Render() override;
  void AudioData(const float* audioData,
                 int audioDataLength,
                 float* freqData,
                 int freqDataLength) override;
  auto UpdateTrack(const VisTrack& track) -> bool override;

  // kodi::gui::gl::CShaderProgram
  void OnCompiledAndLinked() override;
  auto OnEnabled() -> bool override;

protected:
  const static size_t NUM_AUDIO_BUFFERS_IN_CIRCULAR_BUFFER = 16;
  [[nodiscard]] auto GetGoomControl() const -> const GOOM::GoomControl& { return *m_goomControl; };
  [[nodiscard]] auto GetGoomControl() -> GOOM::GoomControl& { return *m_goomControl; };
  [[nodiscard]] auto AudioBufferLen() const -> size_t { return m_audioBufferLen; };
  [[nodiscard]] auto TexWidth() const -> int { return m_texWidth; };
  [[nodiscard]] auto TexHeight() const -> int { return m_texHeight; };
  [[nodiscard]] auto GoomBufferLen() const -> int { return m_goomBufferLen; };
  [[nodiscard]] auto CurrentSongName() const -> const std::string& { return m_currentSongName; };
  [[nodiscard]] auto NumChannels() const -> size_t { return m_channels; };
  virtual void NoActiveBufferAvailable() {}
  virtual void AudioDataQueueTooBig() {}
  virtual void SkippedAudioData() {}
  virtual void AudioDataIncorrectReadLength() {}
  virtual void UpdateGoomBuffer(const char* title,
                                const float floatAudioData[],
                                std::shared_ptr<GOOM::PixelBuffer>& pixels);

private:
  void Process();
  auto InitGlObjects() -> bool;
  void InitQuadData();
  auto GetNextActivePixels() -> std::shared_ptr<GOOM::PixelBuffer>;
  void PushUsedPixels(const std::shared_ptr<GOOM::PixelBuffer>& pixels);

  int m_texWidth = GOOM_TEXTURE_WIDTH;
  int m_texHeight = GOOM_TEXTURE_HEIGHT;
  size_t m_goomBufferSize;
  size_t m_goomBufferLen;
  size_t m_audioBufferLen;

  int m_windowWidth;
  int m_windowHeight;
  int m_windowXPos;
  int m_windowYPos;

  size_t m_channels;
  std::string m_currentSongName;
  std::string m_lastSongName;
  bool m_titleChange = false;
  bool m_showTitleAlways = false;

  GLint m_componentsPerVertex;
  GLint m_componentsPerTexel;
  int m_numVertices;
  int m_numElements;
  GLfloat* m_quadData = nullptr;

#ifdef HAS_GL
  bool m_usePixelBufferObjects =
      false; // 'true' is supposed to give better performance but it's not obvious.
  // And when 'true', there may be issues with screen refreshes when changing windows in Kodi.
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
  std::unique_ptr<GOOM::GoomControl> m_goomControl = nullptr;

  // Audio buffer storage
  static const size_t CIRCULAR_BUFFER_SIZE =
      NUM_AUDIO_BUFFERS_IN_CIRCULAR_BUFFER * NUM_AUDIO_SAMPLES * AUDIO_SAMPLE_LEN;
  circular_buffer<float> m_buffer{CIRCULAR_BUFFER_SIZE};

  // Goom process thread handles
  bool m_threadExit = false;
  std::thread m_workerThread;
  std::mutex m_mutex;
  std::condition_variable m_wait;

  // Screen frames storage, m_activeQueue for next view and m_storedQueue to
  // use on next goom round become active again.
protected:
  static constexpr size_t MAX_ACTIVE_QUEUE_LENGTH = 20;

private:
  std::queue<std::shared_ptr<GOOM::PixelBuffer>> m_activeQueue;
  std::queue<std::shared_ptr<GOOM::PixelBuffer>> m_storedQueue;

  // Start flag to know init was OK
  bool m_started = false;
};

#endif
