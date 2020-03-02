//
// SHARE CODE WITH test_goom.cpp

// Compile with
//
//  g++ -std=c++17 -pthread -o show_goom_output show_goom_output.cpp CommandLineOptions.cpp test_common.cpp -I ../lib/goom/src -I/usr/local/include -I/home/greg/Prj/Python/visualization.goom/lib -I/home/greg/Prj/Python/visualization.goom/build/build/depends/include -L/usr/local/lib  -lX11 -lm -lGL -lGLEW -lglut
//
// Run:
//
//   MESA_GLES_VERSION_OVERRIDE=3.2 MESA_GLSL_VERSION_OVERRIDE=3.3 MESA_GL_VERSION_OVERRIDE=3.2 MESA_GLSL_VERSION_OVERRIDE=150 ./show_goom_output

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "fmt/format.h"
#include "utils/CommandLineOptions.hpp"
#include "utils/goom_loggers.hpp"
#include "utils/buffer_savers.hpp"
#include "utils/test_utils.hpp"
#include "utils/test_common.hpp"
extern "C" {
#  include "goom/goom.h"
#  include "goom/goom_config.h"
}
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <unistd.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <iostream>
#include <ostream>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <limits>
#include <filesystem>
#include <stdexcept>


class GoomShowOptions {
  public:
    explicit GoomShowOptions(const CommandLineOptions &cmdLineOptions);
    const std::string getGoomBufferFilePrefix() const;
    size_t getBufferSize() const { return bufferSize; }
    long getStartBuffNum() const { return startBufferNum; }
    long getEndBuffNum() const;
    int getGoomTextureWidth() const { return goomTextureWidth; }
    int getGoomTextureHeight() const { return goomTextureHeight; }
  private:
    const std::string inputDir;
    std::string goomBufferFilePrefix;
    size_t bufferSize = 500;
    const long startBufferNum;
    long endBufferNum;
    int goomTextureWidth = 1280;
    int goomTextureHeight = 720;
};

GoomShowOptions::GoomShowOptions(const CommandLineOptions &cmdLineOptions)
: inputDir(cmdLineOptions.getValue(CommonTestOptions::INPUT_DIR)),
  goomBufferFilePrefix(cmdLineOptions.getValue(CommonTestOptions::GOOM_FILE_PREFIX)),
  startBufferNum(std::stol(cmdLineOptions.getValue(CommonTestOptions::START_BUFFER_NUM))),
  endBufferNum(std::stol(cmdLineOptions.getValue(CommonTestOptions::END_BUFFER_NUM)))
{
}

inline const std::string GoomShowOptions::getGoomBufferFilePrefix() const
{
  return inputDir + "/" + goomBufferFilePrefix;
}
;

inline long GoomShowOptions::getEndBuffNum() const
{
  if (endBufferNum == -1) {
    return std::numeric_limits<long>::max();
  }
  return endBufferNum;
}

struct BufferData {
    explicit BufferData(std::shared_ptr<uint32_t> buff, const long tg)
    : buffer(buff),
      tag(tg)
    {
    }
    std::shared_ptr<uint32_t> buffer;
    long tag;
};

class CVisualizationTest {
  public:
    explicit CVisualizationTest(const GoomShowOptions &opts);
    virtual ~CVisualizationTest();

    bool Start();
    void Stop();
    bool Render();
    void GoomBufferProducerThread();
    void MainLoop();
    const GoomShowOptions& getOptions() const { return options; }
    ;
  private:
    const GoomShowOptions options;

    const static GLchar* const vert;
    const static GLchar* const frag;

    inline size_t NumGoomBufferElements() const
    {
      return 4 * options.getGoomTextureWidth() * options.getGoomTextureHeight();
    }
    inline size_t GoomBufferSize() const
    {
      return NumGoomBufferElements() * sizeof(unsigned char);
    }
    std::queue<BufferData> m_goomBuffers;
    long lastConsumedBuffNum = -1;
    long GetCurrentTag() const;
    bool noMoreGoomBuffers();
    void ConsumeNextGoomBuffer(unsigned char *ptr);
    bool m_allDone = false;
    std::mutex m_goomBufferMutex;
    std::condition_variable goomBufferProducer_cv;
    std::condition_variable goomBufferConsumer_cv;

    float m_window_width;
    float m_window_height;
    float m_window_xpos;
    float m_window_ypos;
    GLuint m_texid;
    GLuint m_pboIds[2];// IDs of PBO

    GLint m_componentsPerVertex;
    GLint m_componentsPerTexel;
    int numVertices;
    GLfloat *quad_data;
    int numElements;

    GLuint m_prog = 0;
    GLuint m_vaoObject = 0;
    GLuint m_vertexVBO = 0;
    GLint m_uProjModelMatLoc = -1;
    glm::mat4 m_projModelMatrix;
  private:
    void gl_init();
    GLuint CreateProgram(const std::string &aVertexShader, const std::string &aFragmentShader);
    GLuint CreateShader(const GLenum aType, const std::string &aSource);
    void CheckStatus(const GLenum id);

    static CVisualizationTest *currentInstance;
    static void RenderCallback() { currentInstance->Render(); }

  public:
    void SetupRenderCallback()
    {
//    printf("Start render callback.\n");
      currentInstance = this;
      ::glutDisplayFunc(CVisualizationTest::RenderCallback);
    }
    void FillBuffersBeforeStarting();
};

CVisualizationTest *CVisualizationTest::currentInstance = nullptr;

void CVisualizationTest::MainLoop()
{
  logDebug("Starting glutMainLoop.");
  glutMainLoop();
  logDebug("glutMainLoop finished.");
}

void CVisualizationTest::FillBuffersBeforeStarting()
{
  logInfo("### Waiting for Producer to fill buffers...");
  std::unique_lock<std::mutex> lk { m_goomBufferMutex };
  goomBufferConsumer_cv.wait(lk, [this] {return !m_allDone || (m_goomBuffers.size() >= options.getBufferSize());});
  logInfo("### Consumer is ready.");
}

int main(int argc, const char *argv[])
{
  const std::function<void(std::string)> f_console_log =
      [](const std:: string& s) { std::cout << s << std::endl; };
  addLogHandler(f_console_log);
  setLogFile("/tmp/show_goom.log");
  logStart();

  const std::vector<CommandLineOptions::OptionDef> optionDefs = {
      { 'i', CommonTestOptions::INPUT_DIR, true, "" },
      { 'g', CommonTestOptions::GOOM_FILE_PREFIX, true, CommonTestOptions::DEFAULT_GOOM_FILE_PREFIX },
      { 's', CommonTestOptions::START_BUFFER_NUM, true, CommonTestOptions::DEFAULT_START_BUFFER_NUM },
      { 'e', CommonTestOptions::END_BUFFER_NUM, true, CommonTestOptions::DEFAULT_END_BUFFER_NUM }
  };
  const CommandLineOptions cmdLineOptions(argc, argv, optionDefs);
  if (cmdLineOptions.getAreInvalidOptions()) {
    throw std::runtime_error("Invalid command line options.");
  }

  checkFilePathExists(cmdLineOptions.getValue(CommonTestOptions::INPUT_DIR));

  glutInit(&argc, const_cast<char**>(argv));

  const GoomShowOptions options(cmdLineOptions);
  CVisualizationTest visTest(options);
  logInfo(fmt::format("Reading and displaying goom buffers \"{}*\".", visTest.getOptions().getGoomBufferFilePrefix()));

  glutInitWindowSize(visTest.getOptions().getGoomTextureWidth(), visTest.getOptions().getGoomTextureHeight());
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

  std::thread producer(&CVisualizationTest::GoomBufferProducerThread, &visTest);
  visTest.FillBuffersBeforeStarting();

  glutCreateWindow("Goom Buffers on Display");
  if (!visTest.Start()) {
    logError("ERROR: 'Start' returned false.");
    exit(EXIT_FAILURE);
  }
  visTest.SetupRenderCallback();

  std::thread consumer(&CVisualizationTest::MainLoop, &visTest);
  producer.join();
  consumer.join();

  visTest.Stop();

  logInfo("Finished.");
  exit(EXIT_SUCCESS);
}

CVisualizationTest::CVisualizationTest(const GoomShowOptions &opts)
: options(opts),
  m_window_width(opts.getGoomTextureWidth()),
  m_window_height(opts.getGoomTextureHeight()),
  m_window_xpos(0.0),
  m_window_ypos(0.0),
  m_texid(0),
  m_componentsPerVertex(2),
  m_componentsPerTexel(2),
  numVertices { 2 * 3 } // 2 triangles
{
  logInfo("Start CVisualizationTest constructor.");

  const GLfloat x0 = m_window_xpos;
  const GLfloat y0 = m_window_ypos;
  const GLfloat x1 = m_window_xpos + m_window_width;
  const GLfloat y1 = m_window_ypos + m_window_height;
  const GLfloat temp_quad_data[] = {
      // Vertex positions
      x0, y0,// bottom left
      x0, y1,// top left
      x1, y0,// bottom right
      x1, y0,// bottom right
      x1, y1,// top right
      x0, y1,// top left
      // Texture coordinates
      0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0,
    /**
     0.0, 0.0,
     0.0, 1.0,
     1.0, 0.0,
     1.0, 0.0,
     1.0, 1.0,
     0.0, 1.0,
     **/
  };
  numElements = sizeof(temp_quad_data) / sizeof(GLfloat);
  quad_data = new GLfloat[numElements];
  for (int i = 0; i < numElements; i++) {
    quad_data[i] = temp_quad_data[i];
  };
}

CVisualizationTest::~CVisualizationTest()
{
  delete[] quad_data;
}

void CVisualizationTest::gl_init()
{
  logInfo("Start CVisualizationTest gl_init.");

  const GLenum glewError = glewInit();
  if (GLEW_OK != glewError)
    throw std::runtime_error((char*)(glewGetErrorString(glewError)));

  logInfo(fmt::format("GL_VERSION   : {}", glGetString(GL_VERSION)));
  logInfo(fmt::format("GL_VENDOR    : {}", glGetString(GL_VENDOR)));
  logInfo(fmt::format("GL_RENDERER  : {}", glGetString(GL_RENDERER)));
  logInfo(fmt::format("GLEW_VERSION : {}", glewGetString(GLEW_VERSION)));
  logInfo(fmt::format("GLSL VERSION : {}", glGetString(GL_SHADING_LANGUAGE_VERSION)));

  if (!GLEW_VERSION_2_1)
    throw std::runtime_error("OpenGL 2.1 or better required for GLSL support.");

  logInfo("Start CVisualizationTest loading shaders.");
  m_prog = CreateProgram(vert, frag);

  logInfo("Finish CVisualizationTest gl_init.");
}

void CVisualizationTest::CheckStatus(const GLenum id)
{
  GLint status = GL_FALSE;
  GLint loglen = 10;
  if (glIsShader(id))
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
  if (glIsProgram(id))
    glGetProgramiv(id, GL_LINK_STATUS, &status);
  if (GL_TRUE == status)
    return;
  if (glIsShader(id))
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &loglen);
  if (glIsProgram(id))
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &loglen);
  std::vector<char> log(loglen, 'E');
  if (glIsShader(id))
    glGetShaderInfoLog(id, loglen, NULL, &log[0]);
  if (glIsProgram(id))
    glGetProgramInfoLog(id, loglen, NULL, &log[0]);
  throw std::logic_error(std::string(log.begin(), log.end()));
}

GLuint CVisualizationTest::CreateShader(const GLenum aType, const std::string &aSource)
{
  GLuint shader = glCreateShader(aType);
  const GLchar* const shaderString = aSource.c_str();
  glShaderSource(shader, 1, &shaderString, NULL);
  glCompileShader(shader);
  CheckStatus(shader);
  return shader;
}

GLuint CVisualizationTest::CreateProgram(const std::string &aVertexShader, const std::string &aFragmentShader)
{
  logInfo("Start CreateProgram CreateShader GL_VERTEX_SHADER.");
  const GLuint vert = CreateShader( GL_VERTEX_SHADER, aVertexShader);

  logInfo("Start CreateProgram CreateShader GL_FRAGMENT_SHADER.");
  const GLuint frag = CreateShader( GL_FRAGMENT_SHADER, aFragmentShader);

  logInfo("Start CreateProgram glCreateProgram.");
  const GLuint program = glCreateProgram();

  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);
  glDeleteShader(vert);
  glDeleteShader(frag);
  CheckStatus(program);

  return program;
}

const GLchar *const CVisualizationTest::vert = "#version 140\n"
    "#extension GL_ARB_explicit_attrib_location : require\n"
    "uniform mat4 u_projModelMat;"
    "layout (location = 0) in vec2 in_position;"
    "layout (location = 1) in vec2 in_tex_coord;"
    "smooth out vec2 vs_tex_coord;"
    "void main()"
    "{"
    "   gl_Position = u_projModelMat * vec4( in_position, 0.0, 1.0 );"
    "   vs_tex_coord = in_tex_coord;"
    "}";

// fragment shader
const GLchar *const CVisualizationTest::frag = "#version 140\n"
    "#extension GL_ARB_explicit_attrib_location : require\n"
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "#extension GL_EXT_gpu_shader4 : enable\n"
    "uniform sampler2D tex;"
    "smooth in vec2 vs_tex_coord;"
    "layout (location = 0) out vec4 color;"
    "void main()"
    "{"
    "   color = texture(tex, vs_tex_coord);"
    "}";

void PrintByteBGRABuffer(size_t num, const unsigned char BGRA_Buffer[])
{
//    kodi::Log(ADDON_LOG_DEBUG, "Unsigned Byte Buffer (size %lu):", num);
  int j = 0;
  for (size_t i = 0; i < num; i++) {
//        kodi::Log(ADDON_LOG_DEBUG, "%f %f %f %f", 
//          float(BGRA_Buffer[j])/255.0, float(BGRA_Buffer[j+1])/255.0, 
//          float(BGRA_Buffer[j+2])/255.0, float(BGRA_Buffer[j+3])/255.0);
    j += 4;
  }
}

bool CVisualizationTest::Start()
{
  gl_init();

  m_projModelMatrix = glm::ortho(0.0f, float(m_window_width), 0.0f, float(m_window_height));
  m_uProjModelMatLoc = glGetUniformLocation(m_prog, "u_projModelMat");

  // Create VAO and VBO.
  glGenVertexArrays(1, &m_vaoObject);
  glBindVertexArray(m_vaoObject);
  glGenBuffers(1, &m_vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, m_componentsPerVertex, GL_FLOAT, GL_FALSE, 0, (GLvoid*) 0);
  glVertexAttribPointer(1, m_componentsPerTexel, GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid*>(numVertices * m_componentsPerVertex * sizeof(GLfloat)));
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(numElements * sizeof(GLfloat)), quad_data, GL_STATIC_DRAW);
  glBindVertexArray(0);

  // Create texture.
  glGenTextures(1, &m_texid);
  if (!m_texid) {
    logError("Could not do glGenTextures.");
    exit(1);
  }
  glClear(GL_COLOR_BUFFER_BIT);
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texid);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, options.getGoomTextureWidth(), options.getGoomTextureHeight(), 0, GL_RGBA,
      GL_UNSIGNED_BYTE, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenBuffers(2, m_pboIds);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[0]);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, static_cast<int>(GoomBufferSize()), 0, GL_STREAM_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[1]);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, static_cast<int>(GoomBufferSize()), 0, GL_STREAM_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  return true;
}

void CVisualizationTest::Stop()
{
  logInfo("Calling Stop.");

  if (m_texid) {
    glDeleteTextures(1, &m_texid);
    m_texid = 0;
  }
}

bool CVisualizationTest::Render()
{
// logInfo("Starting Render.");

  static int index = 0;
  int nextIndex = 0;// pbo index used for next frame

  index = (index + 1) % 2;
  nextIndex = (index + 1) % 2;

  glUseProgram(m_prog);

  if (noMoreGoomBuffers()) {
    logInfo(fmt::format("Finished, 'lastConsumedBuffNum = {}' is the last buffer.", lastConsumedBuffNum - 1));
    return true;
  }

  // Setup vertex attributes
  glBindVertexArray(m_vaoObject);

  // Setup texture
  glBindTexture(GL_TEXTURE_2D, m_texid);
  //  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_tex_width, g_tex_height, GL_RGBA, GL_UNSIGNED_BYTE, m_goom_buffer);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[index]);

  // copy pixels from PBO to texture object
  // Use offset instead of pointer.
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, options.getGoomTextureWidth(), options.getGoomTextureHeight(), GL_RGBA,
      GL_UNSIGNED_BYTE, 0);

  // bind PBO to update pixel values
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pboIds[nextIndex]);

  // map the buffer object into client's memory
  // Note that glMapBuffer() causes sync issue.
  // If GPU is working with this buffer, glMapBuffer() will wait(stall)
  // for GPU to finish its job. To avoid waiting (stall), you can call
  // first glBufferData() with NULL pointer before glMapBuffer().
  // If you do that, the previous data in PBO will be discarded and
  // glMapBuffer() returns a new allocated pointer immediately
  // even if GPU is still working with the previous data.
  glBufferData(GL_PIXEL_UNPACK_BUFFER, static_cast<int>(GoomBufferSize()), 0, GL_STREAM_DRAW);
  unsigned char* const goom_buffer = static_cast<unsigned char*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
  if (!goom_buffer) {
    logError("Could not allocate goom_buffer from glMapBuffer.");
    exit(1);
  }

  ConsumeNextGoomBuffer(goom_buffer);
  //  std::cout << "Consumer ate buffer - lastConsumedBuffNum = " << lastConsumedBuffNum << std::endl;

////    PrintByteBGRABuffer(numTexels, goom_buffer);
  // update data directly on the mapped buffer
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);// release pointer to mapping buffer
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  glBindTexture(GL_TEXTURE_2D, m_texid);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);

  glUniformMatrix4fv(m_uProjModelMatLoc, 1, GL_FALSE, glm::value_ptr(m_projModelMatrix));
  glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

  glEnable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);

  glutSwapBuffers();

  glutPostRedisplay();

  return true;
}

long CVisualizationTest::GetCurrentTag() const
{
  return m_goomBuffers.empty() ? 0 : m_goomBuffers.front().tag;
}

bool CVisualizationTest::noMoreGoomBuffers()
{
  std::unique_lock<std::mutex> lk { m_goomBufferMutex };
  //  std::cout << fmt::format("Buffer {} -- tag: {}", g_startBuffNum + lastConsumedBuffNum, GetCurrentTag()) << std::endl;
  if (!m_goomBuffers.empty()) {
    return false;
  }
  if (m_allDone) {
    return true;
  }

  logInfo("*** Consumer is waiting...");
  goomBufferConsumer_cv.wait(lk, [this] {return !m_goomBuffers.empty();});
  return false;
}

void CVisualizationTest::ConsumeNextGoomBuffer(unsigned char *ptr)
{
  const std::lock_guard<std::mutex> lk { m_goomBufferMutex };
  const BufferData &bufferData = m_goomBuffers.front();
  memcpy(ptr, bufferData.buffer.get(), GoomBufferSize());
  lastConsumedBuffNum = bufferData.tag;
  m_goomBuffers.pop();
  goomBufferProducer_cv.notify_all();
}

void CVisualizationTest::GoomBufferProducerThread()
{
  logInfo("### Starting Producer thread.");

  BufferSaver<uint32_t> goomBufferReader(
      options.getGoomBufferFilePrefix(),
      false,
      static_cast<size_t>(options.getGoomTextureWidth() * options.getGoomTextureHeight()),
      options.getStartBuffNum(),
      options.getEndBuffNum()
      );

  long readBuffNum = options.getStartBuffNum();
  logInfo(fmt::format("Start with buffer from \"{}\".", goomBufferReader.getCurrentFilename()));
  while (true) {
    std::unique_lock<std::mutex> lk { m_goomBufferMutex };

    if (m_goomBuffers.size() >= options.getBufferSize()) {
      logInfo(fmt::format("### Producer is waiting - last consumed buffer number {}.", lastConsumedBuffNum));
      goomBufferProducer_cv.wait(lk, [this] {return m_goomBuffers.size() < options.getBufferSize();});
      lk.unlock();
    } else {
      lk.unlock();
      logInfo(fmt::format("Reading buffer from \"{}\".", goomBufferReader.getCurrentFilename()));
      std::shared_ptr<uint32_t> nextBuffer = goomBufferReader.getNewBuffer();
      if (!goomBufferReader.Read(nextBuffer.get(), true)) {
        logInfo(fmt::format("No file \"{}\". Finished reading.", goomBufferReader.getCurrentFilename()));
        break;
      }
      readBuffNum++;
      if (readBuffNum != goomBufferReader.getCurrentBufferNum()) {
        throw std::runtime_error(
            fmt::format("CurrentBufferNum {} does not match readBuffNum {}.", goomBufferReader.getCurrentBufferNum(),
                readBuffNum));
      }
      lk.lock();
      m_goomBuffers.push(BufferData(nextBuffer, goomBufferReader.getCurrentBufferNum()));
      goomBufferConsumer_cv.notify_one();
      lk.unlock();
//      std::cout << "Producer produced: goom buffer " << m_goomBuffers.back().tag << " from file " << filename << "." << std::endl;
//      std::this_thread::sleep_for(std::chrono::milliseconds(random() % 200 + 100));
    }
  }

  m_allDone = true;
  logInfo("### Producer thread finished.");
  logStop();
}
