#ifndef MAIN_APPEND_TEST_HPP
#define MAIN_APPEND_TEST_HPP

//#include "/home/greg/Prj/github/xbmc/visualization.goom/src/Main.h"
#include "Main.h"
#include "fmt/format.h"
#include "utils/CommandLineOptions.hpp"
#include "utils/test_utils.hpp"
#include "utils/test_common.hpp"
#include "utils/buffer_savers.hpp"
#include "utils/audio_data_buffer.hpp"
#include "utils/goom_loggers.hpp"
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <stdexcept>
#include <exception>
#include <ostream>
#include <fstream>
#include <chrono>


class GoomTesterOptions {
  public:
    explicit GoomTesterOptions(const std::string& optionsFile);
    const std::string getAudioOutputDir() const;
    const std::string getAudioOutputPrefix() const;
    const std::string getGoomBufferOutputDir() const;
    const std::string getGoomBufferOutputPrefix() const;
    const std::string getGoomStateOutputDir() const;
    const std::string getGoomStateOutputPrefix() const;
    const std::string getSummaryFilename() const;
    const std::string getLogFilename() const;
    long getStartLoggingBufferNum() const { return startLoggingBufferNum; }
    long getStopLoggingBufferNum() const { return stopLoggingBufferNum; }
    bool saveBuffers() const { return saveBuffs; }
    long getStartBufferNum() const { return startBufferNum; }
    long getEndBufferNum() const;
  private:
    const std::string getRootOutputDir() const { return rootOutputDir; }
  private:
    std::string rootOutputDir;
    std::string audioBufferFilePrefix;
    std::string goomBufferFilePrefix;
    long startLoggingBufferNum;
    long stopLoggingBufferNum;
    bool saveBuffs;
    long startBufferNum;
    long endBufferNum;
    CommandLineOptions getCommandLineOptions(const std::string &optionsFile) const;
};

GoomTesterOptions::GoomTesterOptions(const std::string& optionsFile) 
{
  const CommandLineOptions options = getCommandLineOptions(optionsFile);
  rootOutputDir = options.getValue(CommonTestOptions::OUTPUT_DIR);
  startLoggingBufferNum = 1;
  stopLoggingBufferNum = -1;
  startBufferNum = 0;
  endBufferNum = -1;
//  goomSeed = std::stoi(options.getValue(CommonTestOptions::GOOM_SEED));
  startBufferNum = std::stol(options.getValue(CommonTestOptions::START_BUFFER_NUM));
  endBufferNum = std::stol(options.getValue(CommonTestOptions::END_BUFFER_NUM));

  saveBuffs = true;
}

CommandLineOptions GoomTesterOptions::getCommandLineOptions(const std::string &optionsFile) const
{
  std::ifstream f(optionsFile);
  if (!f.good()) {
    throw std::runtime_error("Could not open args file \"" + optionsFile);
  }
  std::string commandLine; 
  std::getline(f, commandLine);
  commandLine = "kodi " + trim(commandLine);

std::ofstream debug("/tmp/junk.txt");
debug << "command line = \"" << commandLine << "\"" << std::endl;

  const std::vector<CommandLineOptions::OptionDef> optionDefs = {
    {'o', CommonTestOptions::OUTPUT_DIR, true, ""},
    {'r', CommonTestOptions::GOOM_SEED, true, CommonTestOptions::DEFAULT_GOOM_SEED},
    {'s', CommonTestOptions::START_BUFFER_NUM, true, CommonTestOptions::DEFAULT_START_BUFFER_NUM},
    {'e', CommonTestOptions::END_BUFFER_NUM, true, CommonTestOptions::DEFAULT_END_BUFFER_NUM}
  };
  const CommandLineOptions cmdLineOptions(commandLine, optionDefs);
  if (cmdLineOptions.getAreInvalidOptions()) {
    throw std::runtime_error("There are invalid command line options in file \"" + optionsFile + "\".");
  }
for (const auto name : cmdLineOptions.getNames()) {
  debug << name << ": " << cmdLineOptions.getValue(name);
  if (cmdLineOptions.isSet(name)) {
    debug << " (set value)";
  } else {
    debug << " (default value)";
  }
  debug << std::endl;
}
debug << "output-dir = \"" << cmdLineOptions.getValue(CommonTestOptions::OUTPUT_DIR) << "\"" << std::endl;
  return cmdLineOptions;
}

inline long GoomTesterOptions::getEndBufferNum() const 
{ 
  if (endBufferNum == -1) {
    return std::numeric_limits<long>::max(); 
  }
  return endBufferNum; 
}


inline const std::string GoomTesterOptions::getAudioOutputDir() const 
{ 
  return getRootOutputDir() + "/" + "audio"; 
}

inline const std::string GoomTesterOptions::getGoomBufferOutputDir() const
{ 
  return getRootOutputDir() + "/" + "goom"; 
}

inline const std::string GoomTesterOptions::getGoomStateOutputDir() const
{
  return getRootOutputDir() + "/" + "state";
}

inline const std::string GoomTesterOptions::getAudioOutputPrefix() const
{
  return getAudioOutputDir() + "/" + CommonTestOptions::DEFAULT_AUDIO_FILE_PREFIX; 
}

inline const std::string GoomTesterOptions::getGoomBufferOutputPrefix() const
{
  return getGoomBufferOutputDir() + "/" + CommonTestOptions::DEFAULT_GOOM_FILE_PREFIX;
}

inline const std::string GoomTesterOptions::getGoomStateOutputPrefix() const
{
  return getGoomStateOutputDir() + "/" + CommonTestOptions::DEFAULT_STATE_FILE_PREFIX;
}

inline const std::string GoomTesterOptions::getSummaryFilename() const
{
  return getRootOutputDir() + date_append("/kodi_goom_summary.log");
}

inline const std::string GoomTesterOptions::getLogFilename() const
{
  return getRootOutputDir() + date_append("/kodi_goom.log");
}

class ATTRIBUTE_HIDDEN CVisualizationGoom_Tester: public CVisualizationGoom
{
  public:
    CVisualizationGoom_Tester();
    ~CVisualizationGoom_Tester() override;
    void Render() override;
    bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
    void Stop() override;
  protected:  
    void NoActiveBufferAvailable() override;
    void AudioDataQueueTooBig() override;
    void SkippedAudioData() override;
    void AudioDataIncorrectReadLength() override;
    void UpdateGoomBuffer(const char* title, const float floatAudioData[], uint32_t* pixels) override;
  private:
    void SaveSummary();
    void SaveCurrentGoomState(const int64_t duration);
  private:
    std::unique_ptr<GoomTesterOptions> options;
    std::unique_ptr<BufferSaver<float>> audioBufferSaver;
    std::unique_ptr<BufferSaver<uint32_t>> goomBufferSaver;
    bool cannotTest = false;
    std::string saveStateError = "";
    unsigned long numTimesNoActiveBuffer = 0;
    unsigned long numTimesAudioDataQueueTooBig = 0;
    unsigned long numTimesSkippedAudioData = 0;  
    unsigned long numTimesAudioDataIncorrectReadLength = 0;  
    unsigned long renderNum = 0;    
    unsigned long buffNum = 0;    
};    

CVisualizationGoom_Tester::CVisualizationGoom_Tester()
: CVisualizationGoom()
{
  options = std::unique_ptr<GoomTesterOptions>(new GoomTesterOptions("/tmp/kodi_args.txt"));

  createFilePath(options->getAudioOutputDir());
  createFilePath(options->getGoomBufferOutputDir());
  createFilePath(options->getGoomStateOutputDir());

  audioBufferSaver = std::unique_ptr<BufferSaver<float>>(
    new BufferSaver<float>(options->getAudioOutputPrefix(), true,
      NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN, options->getStartBufferNum(), options->getEndBufferNum())
  );
  goomBufferSaver = std::unique_ptr<BufferSaver<uint32_t>>(
    new BufferSaver<uint32_t>(options->getGoomBufferOutputPrefix(), true,
      GoomBufferLen(), options->getStartBufferNum(), options->getEndBufferNum())
  );
}

CVisualizationGoom_Tester::~CVisualizationGoom_Tester()
{
}

void CVisualizationGoom_Tester::NoActiveBufferAvailable() 
{
  CVisualizationGoom::NoActiveBufferAvailable();
  numTimesNoActiveBuffer++;
}

void CVisualizationGoom_Tester::AudioDataQueueTooBig()
{
  CVisualizationGoom::AudioDataQueueTooBig();
  numTimesAudioDataQueueTooBig++;
}

void CVisualizationGoom_Tester::SkippedAudioData()
{
  CVisualizationGoom::SkippedAudioData();
  numTimesSkippedAudioData++;
}

void CVisualizationGoom_Tester::AudioDataIncorrectReadLength()
{
  CVisualizationGoom::AudioDataIncorrectReadLength();
  numTimesAudioDataIncorrectReadLength++;
}

bool CVisualizationGoom_Tester::Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName)
{
  const bool started = CVisualizationGoom::Start(channels, samplesPerSec, bitsPerSample, songName);
  if (AudioBufferLen() != NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN) 
  {
    audioBufferSaver->ResetBufferLen(AudioBufferLen());
  }
  setLogFile(options->getLogFilename());
  logStart();
  logInfo(fmt::format("Visualization start. Buffer num = {}.", audioBufferSaver->getCurrentBufferNum()));
  return started;
}

void CVisualizationGoom_Tester::Stop()
{
  logInfo(fmt::format("Visualization stop. Buffer num = {}.", audioBufferSaver->getCurrentBufferNum()));

  CVisualizationGoom::Stop();

  SaveSummary();
  logStop();
}

void CVisualizationGoom_Tester::Render()
{
  renderNum++;
  CVisualizationGoom::Render();
}

void CVisualizationGoom_Tester::UpdateGoomBuffer(
  const char* title, const float floatAudioData[], uint32_t* pixels) 
{
  if (options->saveBuffers() && !cannotTest) {
    const long currentBufferNum = goomBufferSaver->getCurrentBufferNum();
    logInfo(fmt::format("Updating goom buffer {}.", currentBufferNum));
    if (buffNum < options->getStartLoggingBufferNum()) {
      logInfo("Suspending logging.");
      logSuspend();
    }
  }  

  const auto t1 = std::chrono::high_resolution_clock::now();

  CVisualizationGoom::UpdateGoomBuffer(title, floatAudioData, pixels);

  const auto t2 = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

  if (options->saveBuffers() && !cannotTest) {
    if (!isLogging()) {
      logResume();
      logInfo("Resumed logging.");
    }

    const long currentBufferNum = goomBufferSaver->getCurrentBufferNum();
    audioBufferSaver->Write(floatAudioData, true);
    goomBufferSaver->Write(pixels, true);

    logInfo(fmt::format("Produced goom buffer {} in {} mu.", currentBufferNum, duration));

    SaveCurrentGoomState(duration);
  }

  buffNum++;
}

void CVisualizationGoom_Tester::SaveCurrentGoomState(const int64_t duration)
{
  const std::string filename = fmt::format("{}_{:05}.txt", options->getGoomStateOutputPrefix(), buffNum);
  try {
    std::ofstream f(filename, std::ios::out);

    f << "# Update buffer time (mu) = " << duration << std::endl;
    f << std::endl;

    SaveGoomPluginInfo(f, goomPluginInfo());

    logInfo(fmt::format("Saved goom state to file \"{}\".", filename));
  } catch (const std::exception& e) {
    logError(fmt::format("Could not save goom state to file \"{}\".", filename));
    const std::string msg = e.what();
    logError(fmt::format("Exception: \"{}\".", msg));
  }
}

void CVisualizationGoom_Tester::SaveSummary()
{
  const std::string summaryFilename = options->getSummaryFilename();
  logInfo(fmt::format("Saving summary to file \"{}\".", summaryFilename));

  std::ofstream f(summaryFilename, std::ios::out);
  f << "audio title = \"" << CurrentSongName() << "\"" << std::endl;
  f << "options start buffer num = " << options->getStartBufferNum() << "" << std::endl;
  f << "options end buffer num = " << options->getEndBufferNum() << "" << std::endl;
  f << "audio saver start buffer num = " << audioBufferSaver->getStartBufferNum() << "" << std::endl;
  f << "audio saver end buffer num = " << audioBufferSaver->getEndBufferNum() << "" << std::endl;
  f << "goom saver start buffer num = " << goomBufferSaver->getStartBufferNum() << "" << std::endl;
  f << "goom saver end buffer num = " << goomBufferSaver->getEndBufferNum() << "" << std::endl;
  f << "goom width = " << TexWidth() << std::endl;
  f << "goom height = " << TexHeight() << std::endl;
  f << "goom buffer len = " << GoomBufferLen() << std::endl;
  f << "num channels = " << NumChannels() << std::endl;
  f << "audio buffer len = " << AudioBufferLen() << std::endl;
  f << "num audio buffers in circular buffer = " << g_num_audio_buffers_in_circular_buffer << std::endl;
  f << "max active queue len = " << g_maxActiveQueueLength << std::endl;
  f << "current audio buffer num = " << audioBufferSaver->getCurrentBufferNum() << std::endl;
  f << "current goom buffer num = " << goomBufferSaver->getCurrentBufferNum() << std::endl;
  f << "number of submitted audio buffers = " << buffNum << std::endl;
  f << "number of audio data buffers skipped = " << numTimesSkippedAudioData << std::endl;
  f << "number of times audio data queue too big = " << numTimesAudioDataQueueTooBig << std::endl;
  f << "number of render calls = " << renderNum << std::endl;
  f << "number of renders skipped = " << numTimesNoActiveBuffer << std::endl;
  f << "saveStateError = '" << saveStateError << "'" << std::endl;
}

#include "utils/format.cc"
#include "utils/CommandLineOptions.cpp"
#include "utils/test_utils.cpp"
#include "utils/test_common.cpp"

#endif
