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

#include "Main.h"

#include "goom/goom_graphic.h"
#include "goom/sound_info.h"
#include "goomutils/logging_control.h"

#include <memory>
#undef NO_LOGGING
#include "goomutils/logging.h"

using GOOM::Pixel;
using GOOM::PixelBuffer;
using GOOM::UTILS::Logging;

static constexpr int MAX_QUALITY = 2;
static constexpr std::array<int, MAX_QUALITY + 1> WIDTHS_BY_QUALITY{
    512,
    640,
    1280,
};
static constexpr std::array<int, 3> HEIGHTS_BY_QUALITY{
    256,
    360,
    720,
};

CVisualizationGoom::CVisualizationGoom()
  : m_texWidth{WIDTHS_BY_QUALITY[static_cast<size_t>(
        std::max(MAX_QUALITY, kodi::GetSettingInt("quality")))]},
    m_texHeight{HEIGHTS_BY_QUALITY[static_cast<size_t>(
        std::max(MAX_QUALITY, kodi::GetSettingInt("quality")))]},
    m_goomBufferLen{static_cast<size_t>(m_texWidth * m_texHeight)},
    m_goomBufferSize{m_goomBufferLen * sizeof(uint32_t)},
    m_windowWidth{Width()},
    m_windowHeight{Height()},
    m_windowXPos{X()},
    m_windowYPos{Y()}
{

#ifdef HAS_GL
  m_usePixelBufferObjects = kodi::GetSettingBoolean("use_pixel_buffer_objects");
#endif

  InitQuadData();
}

//-- Destroy -------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
CVisualizationGoom::~CVisualizationGoom()
{
  delete[] m_quadData;

  kodi::Log(ADDON_LOG_DEBUG, "~CVisualizationGoom: Destroyed CVisualizationGoom object.");
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
auto CVisualizationGoom::Start(int iChannels,
                               [[maybe_unused]] int iSamplesPerSec,
                               [[maybe_unused]] int iBitsPerSample,
                               std::string szSongName) -> bool
{
  if (m_started)
  {
    kodi::Log(ADDON_LOG_WARNING, "Start: Already started without a stop - skipping this.");
    return true;
  }

  static const auto s_fKodiLog = [](const Logging::LogLevel lvl, const std::string& s) {
    const auto kodiLvl =
        static_cast<AddonLog>(static_cast<size_t>(lvl) + 1); // map 'info' to 'notice'
    kodi::Log(kodiLvl, s.c_str());
  };
  addLogHandler("kodi-logger", s_fKodiLog);
  setLogFile("/tmp/kodi_goom.log");
  setLogLevelForFiles(Logging::LogLevel::info);
  //  setLogLevelForFiles(Logging::LogLevel::debug);
  logStart();
  logInfo("Visualization start. SongName = \"{}\".", szSongName);

  m_channels = static_cast<size_t>(iChannels);
  m_audioBufferLen = m_channels * AUDIO_SAMPLE_LEN;
  m_currentSongName = szSongName;
  m_titleChange = true;

  // Make one init frame in black
  std::shared_ptr<PixelBuffer> sp{
      new PixelBuffer{static_cast<uint32_t>(m_texWidth), static_cast<uint32_t>(m_texHeight)}};
  sp->Fill(Pixel::BLACK);
  m_activeQueue.push(sp);

  // Init GL parts
  if (!LoadShaderFiles(kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/vert.glsl"),
                       kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/frag.glsl")))
  {
    kodi::Log(ADDON_LOG_FATAL, "Start: Failed to load GL shaders.");
    return false;
  }

  if (!CompileAndLink())
  {
    kodi::Log(ADDON_LOG_FATAL, "Start: Failed to compile GL shaders.");
    return false;
  }

  if (!InitGlObjects())
  {
    kodi::Log(ADDON_LOG_FATAL, "Start: Could not intilize GL objects.");
    return false;
  }

  // Start the goom process thread
  kodi::Log(ADDON_LOG_DEBUG, "Start: Setting up buffer worker thread.");
  m_workerThread = std::thread(&CVisualizationGoom::Process, this);

  m_started = true;
  return true;
}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by Kodi
//-----------------------------------------------------------------------------
void CVisualizationGoom::Stop()
{
  if (!m_started)
  {
    kodi::Log(ADDON_LOG_WARNING, "Stop: Not started - skipping this.");
    return;
  }

  m_started = false;

  kodi::Log(ADDON_LOG_DEBUG, "Stop: Stopping processed buffers thread...");
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_threadExit = true;
    m_wait.notify_one();
  }
  if (m_workerThread.joinable())
  {
    m_workerThread.join();
  }

  kodi::Log(ADDON_LOG_NOTICE, "Stop: Processed buffers thread stopped.");

  glDeleteTextures(1, &m_textureId);
  m_textureId = 0;

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_vertexVBO);
  m_vertexVBO = 0;
}

void CVisualizationGoom::OnCompiledAndLinked()
{
  m_uProjModelMatLoc = glGetUniformLocation(ProgramHandle(), "u_projModelMat");
  m_aPositionLoc = glGetAttribLocation(ProgramHandle(), "in_position");
  m_aCoordLoc = glGetAttribLocation(ProgramHandle(), "in_tex_coord");
}

auto CVisualizationGoom::OnEnabled() -> bool
{
  glUniformMatrix4fv(m_uProjModelMatLoc, 1, GL_FALSE, glm::value_ptr(m_projModelMatrix));
  return true;
}

//-- Audiodata ----------------------------------------------------------------
// Called by Kodi to pass new audio data to the vis
//-----------------------------------------------------------------------------
void CVisualizationGoom::AudioData(const float* pAudioData,
                                   int iAudioDataLength,
                                   [[maybe_unused]] float* pFreqData,
                                   [[maybe_unused]] int iFreqDataLength)
{
  if (!m_started)
  {
    kodi::Log(ADDON_LOG_WARNING, "AudioData: Not started - skipping this.");
    return;
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_buffer.DataAvailable() >= CIRCULAR_BUFFER_SIZE)
  {
    AudioDataQueueTooBig();
    return;
  }

  (void)m_buffer.Write(pAudioData, static_cast<unsigned int>(iAudioDataLength));
  m_wait.notify_one();
}

auto CVisualizationGoom::UpdateTrack(const VisTrack& track) -> bool
{
  if (m_goomControl)
  {
    m_lastSongName = m_currentSongName;
    std::string artist = track.artist;
    if (artist.empty())
    {
      artist = track.albumArtist;
    }
    std::string title;
    if (!artist.empty())
    {
      m_currentSongName = artist + " - " + track.title;
    }
    else
    {
      m_currentSongName = track.title;
    }

    if (m_lastSongName != m_currentSongName)
    {
      m_titleChange = true;
    }

    m_showTitleAlways = kodi::GetSettingBoolean("show_title_always");
  }
  return true;
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------

auto CVisualizationGoom::IsDirty() -> bool
{
  return true; //!m_worker.RequestsQueueEmpty();
}

void CVisualizationGoom::Render()
{
  if (!m_started)
  {
    kodi::Log(ADDON_LOG_WARNING, "Render: Not started - skipping this.");
    return;
  }

  // Setup vertex attributes.
#ifdef HAS_GL
  glBindVertexArray(m_vaoObject);
#else
  glVertexAttribPointer(m_aPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, m_quadData);
  glEnableVertexAttribArray(m_aPositionLoc);
  glVertexAttribPointer(m_aCoordLoc, 2, GL_FLOAT, GL_FALSE, 0,
                        m_quadData + m_numVertices * m_componentsPerVertex);
  glEnableVertexAttribArray(m_aCoordLoc);
#endif

  // Setup texture.
  glDisable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_textureId);

  std::shared_ptr<PixelBuffer> pixels = GetNextActivePixels();
  if (pixels != nullptr)
  {
#ifdef HAS_GL
    if (m_usePixelBufferObjects)
    {
      m_currentPboIndex = (m_currentPboIndex + 1) % g_numPbos;
      const int nextPboIndex = (m_currentPboIndex + 1) % g_numPbos;

      // Bind to current PBO and send pixels to texture object.
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[m_currentPboIndex]);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_texWidth, m_texHeight, GL_BGRA, GL_UNSIGNED_BYTE,
                      0);

      // Bind to next PBO and update data directly on the mapped buffer.
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[nextPboIndex]);
      (void)std::memcpy(m_pboGoomBuffer[nextPboIndex],
                        std::const_pointer_cast<const PixelBuffer>(pixels)->GetIntBuff(),
                        m_goomBufferSize);

      (void)glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    else
#endif
    {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_texWidth, m_texHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                      std::const_pointer_cast<const PixelBuffer>(pixels)->GetIntBuff());
    }

    PushUsedPixels(pixels);
  }

  (void)EnableShader();
  glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
  DisableShader();

  glEnable(GL_BLEND);

#ifdef HAS_GL
  glBindVertexArray(0);
#else
  glDisableVertexAttribArray(m_aPositionLoc);
  glDisableVertexAttribArray(m_aCoordLoc);
#endif
}

inline auto CVisualizationGoom::GetNextActivePixels() -> std::shared_ptr<PixelBuffer>
{
  std::shared_ptr<PixelBuffer> pixels{};
  std::lock_guard<std::mutex> lk(m_mutex);
  if (m_activeQueue.empty())
  {
    NoActiveBufferAvailable();
  }
  else
  {
    pixels = m_activeQueue.front();
    m_activeQueue.pop();
  }
  return pixels;
}

inline void CVisualizationGoom::PushUsedPixels(const std::shared_ptr<PixelBuffer>& pixels)
{
  std::lock_guard<std::mutex> lk(m_mutex);
  m_storedQueue.push(pixels);
}

void CVisualizationGoom::Process()
{
  // goom will use same random sequence if following is uncommented
  // goom::GoomControl::setRandSeed(1);

  m_goomControl = std::make_unique<GOOM::GoomControl>(static_cast<uint16_t>(m_texWidth),
                                                      static_cast<uint16_t>(m_texHeight),
                                                      kodi::GetAddonPath("resources"));
  if (!m_goomControl)
  {
    kodi::Log(ADDON_LOG_FATAL, "CVisualizationGoom: Goom could not be initialized!");
    return;
  }
  m_goomControl->Start();

  std::vector<float> floatAudioData(m_audioBufferLen);
  const char* title;
  unsigned long buffNum = 0;

  while (true)
  {
    std::unique_lock<std::mutex> lk(m_mutex);
    if (m_threadExit)
    {
      break;
    }
    if (m_buffer.DataAvailable() < m_audioBufferLen)
    {
      m_wait.wait(lk);
    }
    unsigned read = m_buffer.Read(floatAudioData.data(), m_audioBufferLen);
    if (read != m_audioBufferLen)
    {
      kodi::Log(
          ADDON_LOG_WARNING,
          "Process: Num read audio length %u != %d = expected audio data length - skipping this.",
          read, m_audioBufferLen);
      AudioDataIncorrectReadLength();
      continue;
    }
    lk.unlock();

    if (m_titleChange || m_showTitleAlways)
    {
      title = m_currentSongName.c_str();
      m_titleChange = false;
    }
    else
    {
      title = nullptr;
    }

    if (m_threadExit)
    {
      break;
    }

    lk.lock();
    if (m_activeQueue.size() > MAX_ACTIVE_QUEUE_LENGTH)
    {
      // Too far behind, skip this audio data.
      SkippedAudioData();
      continue;
    }
    lk.unlock();

    std::shared_ptr<GOOM::PixelBuffer> pixels;
    lk.lock();
    if (!m_storedQueue.empty())
    {
      pixels = m_storedQueue.front();
      m_storedQueue.pop();
    }
    else
    {
      std::shared_ptr<PixelBuffer> sp{
          new PixelBuffer{static_cast<uint32_t>(m_texWidth), static_cast<uint32_t>(m_texHeight)}};
      pixels = sp;
    }
    lk.unlock();

    UpdateGoomBuffer(title, floatAudioData, pixels);
    buffNum++;

    lk.lock();
    m_activeQueue.push(pixels);
    lk.unlock();
  }

  m_goomControl->Finish();
}

void CVisualizationGoom::UpdateGoomBuffer(const char* title,
                                          const std::vector<float>& floatAudioData,
                                          std::shared_ptr<PixelBuffer>& pixels)
{
  const GOOM::AudioSamples audioData{m_channels, floatAudioData.data()};
  m_goomControl->SetScreenBuffer(*pixels);
  m_goomControl->Update(audioData, 0.0F, title, "");
}

void CVisualizationGoom::InitQuadData()
{
  auto x0 = static_cast<GLfloat>(this->m_windowXPos);
  auto y0 = static_cast<GLfloat>(this->m_windowYPos);
  auto x1 = static_cast<GLfloat>(this->m_windowXPos + this->m_windowWidth);
  auto y1 = static_cast<GLfloat>(this->m_windowYPos + this->m_windowHeight);
  // clang-format off
  const GLfloat tempQuadData[] =
  {
    // Vertex positions
    x0, y0,  // bottom left
    x0, y1,  // top left
    x1, y0,  // bottom right
    x1, y0,  // bottom right
    x1, y1,  // top right
    x0, y1,  // top left
    // Texture coordinates
    0.0, 1.0,
    0.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    1.0, 0.0,
    0.0, 0.0,
  };
  // clang-format on

  m_numElements = sizeof(tempQuadData) / sizeof(GLfloat);
  m_quadData = new GLfloat[m_numElements];
  for (int i = 0; i < m_numElements; i++)
  {
    m_quadData[i] = tempQuadData[i];
  }
  m_componentsPerVertex = 2;
  m_componentsPerTexel = 2;
  m_numVertices = 2 * 3; // 2 triangles
  m_vertexVBO = 0;
}

auto CVisualizationGoom::InitGlObjects() -> bool
{
  m_projModelMatrix =
      glm::ortho(0.0F, static_cast<float>(Width()), 0.0F, static_cast<float>(Height()));

  // Setup vertex attributes
#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_vertexVBO);
  glGenVertexArrays(1, &m_vaoObject);
  glBindVertexArray(m_vaoObject);
  glGenBuffers(1, &m_vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);
  glEnableVertexAttribArray(static_cast<GLuint>(m_aPositionLoc));
  glEnableVertexAttribArray(static_cast<GLuint>(m_aCoordLoc));
  glVertexAttribPointer(static_cast<GLuint>(m_aPositionLoc), m_componentsPerVertex, GL_FLOAT,
                        GL_FALSE, 0, (GLvoid*)0);
  glVertexAttribPointer(
      static_cast<GLuint>(m_aCoordLoc), m_componentsPerTexel, GL_FLOAT, GL_FALSE, 0,
      (GLvoid*)(static_cast<size_t>(m_numVertices * m_componentsPerVertex) * sizeof(GLfloat)));
  glBufferData(GL_ARRAY_BUFFER, static_cast<size_t>(m_numElements) * sizeof(GLfloat), m_quadData,
               GL_STATIC_DRAW);
  glBindVertexArray(0);
#endif

  // Create texture.
  glGenTextures(1, &m_textureId);
  if (!m_textureId)
  {
    kodi::Log(ADDON_LOG_ERROR, "InitGlObjects: Could not do glGenTextures.");
    return false;
  }
  glClear(GL_COLOR_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D, m_textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef HAS_GL
  glGenerateMipmap(GL_TEXTURE_2D);
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_texWidth, m_texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);

#ifdef HAS_GL
  if (!m_usePixelBufferObjects)
  {
    kodi::Log(ADDON_LOG_NOTICE, "InitGlObjects: Not using pixel buffer objects.");
  }
  else
  {
    kodi::Log(ADDON_LOG_NOTICE, "InitGlObjects: Using pixel buffer objects.");
    m_currentPboIndex = 0;

    glGenBuffers(g_numPbos, m_pboIds);
    for (int i = 0; i < g_numPbos; i++)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[i]);
      glBufferData(GL_PIXEL_UNPACK_BUFFER, static_cast<GLsizeiptr>(m_goomBufferSize), 0,
                   GL_STREAM_DRAW);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[i]);
      glBufferData(GL_PIXEL_UNPACK_BUFFER, static_cast<GLsizeiptr>(m_goomBufferSize), 0,
                   GL_STREAM_DRAW);
      m_pboGoomBuffer[i] =
          static_cast<unsigned char*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
      if (!m_pboGoomBuffer[i])
      {
        kodi::Log(ADDON_LOG_ERROR, "InitGlObjects: Could not do glMapBuffer for pbo %d.", i);
        return false;
      }
    }
    (void)glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }
#endif

  return true;
}


#ifndef DO_TESTING
ADDONCREATOR(CVisualizationGoom) // Don't touch this!
#else
#pragma message("Compiling " __FILE__ " with 'DO_TESTING' ON.")
#endif
