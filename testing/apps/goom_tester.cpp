#include "goom_tester.h"
#include "fmt/format.h"
#include "../../src/AudioData.h"
#include "utils/goom_loggers.hpp"
#include "utils/buffer_savers.hpp"
#include "utils/audio_data_buffer.hpp"
#include "utils/CommandLineOptions.hpp"
#include "utils/statistics.hpp"
#include "utils/version.h"
#include "utils/test_utils.hpp"
#include "utils/test_common.hpp"
extern "C" {
  #include "goom/goom.h"
  #include "goom/goom_config.h"
  #include "goom/goom_testing.h"
}
#include <ostream>
#include <mutex>
#include <string>
#include <condition_variable>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <chrono>


GoomTestOptions::GoomTestOptions(const CommandLineOptions& cmdLineOptions)
: goomSeed { std::stoi(cmdLineOptions.getValue(CommonTestOptions::GOOM_SEED)) },
  stateNum { std::stol(cmdLineOptions.getValue(CommonTestOptions::STATE_NUM)) },
  songTitle("Hello Goom"),
  inputDir(cmdLineOptions.getValue(CommonTestOptions::INPUT_DIR)),
  outputDir(cmdLineOptions.getValue(CommonTestOptions::OUTPUT_DIR)),
  audioDataFilePrefix(cmdLineOptions.getValue(CommonTestOptions::AUDIO_FILE_PREFIX)),
  goomBufferFilePrefix(cmdLineOptions.getValue(CommonTestOptions::GOOM_FILE_PREFIX)),
  startBufferNum { std::stol(cmdLineOptions.getValue(CommonTestOptions::START_BUFFER_NUM)) },
  endBufferNum { std::stol(cmdLineOptions.getValue(CommonTestOptions::END_BUFFER_NUM)) }
{
  if (startBufferNum < 0) {
    throw std::runtime_error(fmt::format("Invalid start buffer number: {}.", startBufferNum));
  }
  if (endBufferNum == -1) {
    endBufferNum = std::numeric_limits<long>::max();
  } else if (endBufferNum < -1) {
    throw std::runtime_error(fmt::format("Invalid end buffer number: {}.", endBufferNum));
  } else if (startBufferNum > endBufferNum) {
    throw std::runtime_error(fmt::format(
      "Invalid start and end buffer numbers: {} {}.", startBufferNum, endBufferNum));
  }
}

std::string GoomTestOptions::getLogFilename() const
{
  return outputDir + date_append("/goom_test.log");
}

std::string GoomTestOptions::getAudioInputDir() const
{
  return inputDir + "/audio";
}

std::string GoomTestOptions::getAudioOutputDir() const
{
  return outputDir + "/audio"; 
}

std::string GoomTestOptions::getGoomBufferOutputDir() const
{
  return outputDir + "/goom"; 
}

std::string GoomTestOptions::getGoomStateInputDir() const
{
  return inputDir + "/state";
}

std::string GoomTestOptions::getGoomStateOutputDir() const
{
  return outputDir + "/state";
}

std::string GoomTestOptions::getAudioInputPrefix() const
{ 
  return getAudioInputDir() + "/" + audioDataFilePrefix;
}

std::string GoomTestOptions::getAudioOutputPrefix() const
{ 
  return getAudioOutputDir() + "/" + audioDataFilePrefix; 
}

std::string GoomTestOptions::getGoomBufferOutputPrefix() const
{ 
  return getGoomBufferOutputDir() + "/" + goomBufferFilePrefix; 
}

std::string GoomTestOptions::getGoomStateInputPrefix() const
{
  return getGoomStateInputDir() + "/" + CommonTestOptions::DEFAULT_STATE_FILE_PREFIX;
}

std::string GoomTestOptions::getGoomStateOutputPrefix() const
{
  return getGoomStateOutputDir() + "/" + CommonTestOptions::DEFAULT_STATE_FILE_PREFIX;
}

std::string GoomTestOptions::getGoomStateInputFilename() const
{
  return getGoomStateInputPrefix() + fmt::format("_{:05}.txt", stateNum);
}

std::string GoomTestOptions::getSummaryFilename() const
{
  return outputDir + date_append("/test_goom_summary.log");
}
 

AudioData::AudioData(const long startBuffNum, const long endBuffNum)
: startBufferNum { startBuffNum },
  endBufferNum { endBuffNum },
  buffNum { 0 }
{
  buffNum = startBufferNum;
}

AudioData::~AudioData()
{
}

bool AudioData::doMoreBuffers() const
{
  return buffNum <= endBufferNum;
}

AudioDataBuffer* AudioData::getNext()
{
  if (!doMoreBuffers()) {
    logInfo("Finished last buffer.");
    return nullptr;
  }

  AudioDataBuffer *const audioDataBuffer = new AudioDataBuffer { buffNum };
  for (int i=0; i < NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN; i++) {
    audioDataBuffer->getBuffer()[i] = float(random() % 1000)/10000.0;
  }
  buffNum++;

  return audioDataBuffer;
}


AudioDataFiles::AudioDataFiles(const long startBuffNum, const long endBuffNum,
		const std::string &filenamePrefix)
: AudioData { startBuffNum, endBuffNum },
  bufferReader(filenamePrefix, false, NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN, startBuffNum, endBuffNum)
{
}

AudioDataFiles::~AudioDataFiles()
{
}

AudioDataBuffer* AudioDataFiles::getNext() {
  logInfo(fmt::format("Reading buffer from file \"{}\".", bufferReader.getCurrentFilename()));
  AudioDataBuffer *const audioDataBuffer = new AudioDataBuffer { bufferReader.getCurrentBufferNum() };
  const long currentBufferNum = bufferReader.getCurrentBufferNum();
  if (!bufferReader.Read(audioDataBuffer->getBuffer(), true)) {
    logInfo(fmt::format("No file \"{}\". Finished reading.", bufferReader.getCurrentFilename()));
    return nullptr;
  }
  logDebug(fmt::format("Got buffer {}.", currentBufferNum));
  logDebug("Done, returning audioDataBuffer.");
  return audioDataBuffer;
}


GoomTester::GoomTester(const VersionInfo& ver, const GoomTestOptions& opts)
: goom(),
  versionInfo(ver),
  options(opts),
  songTitle(opts.getSongTitle()),
  stats(),
  audioDataMutex(),
  audioDataProducer_cv(),
  audioDataConsumer_cv(),
  audioDataQueue()
{
  if (opts.getGoomStateInputFilename() != "") {
    setLogFile(options.getLogFilename());
    logStart();
    logInfo(versionInfo.getFullInfo());
    logInfo(getFullGoomVersionInfo());
    logInfo("Before goom_init.");
  }

  goom = std::unique_ptr<PluginInfo>(goom_init(options.getGoomTextureWidth(), options.getGoomTextureHeight(), options.getGoomSeed()));

  if (opts.getGoomStateInputFilename() != "") {
    logInfo("After goom_init.");
    SaveCurrentGoomState(-2);
    PluginInfo* oldGoom = goom.release();
    goom = std::unique_ptr<PluginInfo>(LoadGoomPluginInfo(options.getGoomStateInputFilename(), oldGoom));
    goom_close(oldGoom);
  }
}

GoomTester::~GoomTester()
{
  if (goom) {
    goom_close(goom.release());
  }
}

inline const char* GoomTester::getTitle() const
{
  if (options.getGoomStateInputFilename() == "") {
    return songTitle.c_str();
  }

  return nullptr;
}

void GoomTester::AudioDataProducerThread()
{
  std::shared_ptr<AudioData> audioData = GetAudioData();
  long lastBuffNum = -1;

  logInfo("Starting audio producer thread.");
  while (true) {
    std::unique_lock<std::mutex> lk { audioDataMutex };

    if (audioDataQueue.size() >= options.getAudioBufferLen()) {
      logInfo(fmt::format("### Producer is waiting - last buffer number {}.", lastBuffNum));
      audioDataProducer_cv.wait(lk, [this]{ return audioDataQueue.size() < options.getAudioBufferLen(); });
      lk.unlock();
    } else {
      lk.unlock();
      std::shared_ptr<AudioDataBuffer> audioDataBuff(audioData->getNext());
      if (audioDataBuff == nullptr) {
        logInfo(fmt::format("Producer has finished - last buffer number was {} - exiting.", lastBuffNum));
        break;
      }
      lastBuffNum = audioDataBuff->getTag();
      logInfo(fmt::format("Producer produced: audioDataBuff {}.", lastBuffNum));

      lk.lock();
      audioDataQueue.push(audioDataBuff);
      audioDataConsumer_cv.notify_all();
      lk.unlock();
    }
  }

  allDone = true;
  logInfo("Finished audio consumer buffer thread.");
}

void GoomTester::AudioDataConsumerThread()
{
  BufferSaver<float> audioBufferWriter(
    options.getAudioOutputPrefix(), true,
    NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN,
    options.getStartBufferNum(), options.getEndBufferNum());
  BufferSaver<uint32_t> goomBufferWriter(
    options.getGoomBufferOutputPrefix(), true,
    options.getGoomTextureWidth() * options.getGoomTextureHeight(),
    options.getStartBufferNum(), options.getEndBufferNum());

  SaveCurrentGoomState(options.getStartBufferNum() - 1);

  logInfo("Starting goom buffer thread.");
  while (true) {
    std::unique_lock<std::mutex> lk(audioDataMutex);

    if (audioDataQueue.empty()) {
      if (allDone) {
        logInfo("Producer has finished - exiting.");
        break;
      }
      logInfo("*** Consumer is waiting...");
      audioDataConsumer_cv.wait(lk, [this]{ return !audioDataQueue.empty(); });
    } else {
      std::shared_ptr<AudioDataBuffer> audioDataBuff = audioDataQueue.front();
      logDebug(fmt::format("Consumer ate audioData - buffNum = {}.", audioDataBuff->getTag()));
      audioDataQueue.pop();
      audioDataProducer_cv.notify_all();
      lk.unlock();

      const long currentBufferNum = goomBufferWriter.getCurrentBufferNum();
      std::shared_ptr<uint32_t> nextBuffer = goomBufferWriter.getNewBuffer();
      UpdateGoomBuffer(currentBufferNum, audioDataBuff->getBuffer(), nextBuffer.get());
      logDebug(fmt::format("Done goom_update for buffer {}.", audioDataBuff->getTag()));

      // audioData.Write("/tmp/audio_buffer_%05ld", *audioDataBuff.get());
      audioBufferWriter.Write(audioDataBuff->getBuffer(), true);
      goomBufferWriter.Write(nextBuffer.get(), true);
    }
  }

  logInfo("Finished goom buffer thread.");
  saveSummary();
  logStop();
}

void GoomTester::UpdateGoomBuffer(const long bufferNum, const float floatAudioData[], uint32_t* pixels)
{
  logInfo(fmt::format("Updating goom buffer {}.", bufferNum));

  const auto t1 = std::chrono::high_resolution_clock::now();

  static int16_t audioData[NUM_AUDIO_SAMPLES][AUDIO_SAMPLE_LEN];
  FillAudioDataBuffer(audioData, floatAudioData, options.getNumAudioChannels());
  goom_set_screenbuffer(goom.get(), pixels);
  goom_update(goom.get(), audioData, 0, 0.0f, getTitle(), "Test");

  const auto t2 = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

  stats.add(duration);
  SaveCurrentGoomState(bufferNum, duration);

  logInfo(fmt::format("Produced goom buffer {} in {} mu.", bufferNum, duration));
}

std::shared_ptr<AudioData> GoomTester::GetAudioData() const
{
  if (options.getUseAudioFiles()) {
    return std::shared_ptr<AudioData>(
      new AudioDataFiles(
        options.getStartBufferNum(), options.getEndBufferNum(),
        options.getAudioInputPrefix()
      )
    );
  }
  return std::shared_ptr<AudioData>(new AudioData(options.getStartBufferNum(), options.getEndBufferNum()));
}

void GoomTester::SaveCurrentGoomState(const long currentBufferNum, const int64_t duration)
{
  const std::string filename = fmt::format("{}_{:05}.txt", options.getGoomStateOutputPrefix().c_str(), currentBufferNum);
  std::ofstream f(filename, std::ios::out);

  f << "# " << versionInfo.getFullInfo() << std::endl;
  f << "# " << getFullGoomVersionInfo() << std::endl;

  if (duration == -1) {
    f << "# Update buffer time = UNKNOWN" << duration << std::endl;
  } else {
    f << "# Update buffer time (mu) = " << duration << std::endl;
  }
  f << std::endl;

  SaveGoomPluginInfo(f, goom.get());
}

void GoomTester::saveSummary()
{
  const std::string summaryFilename = options.getSummaryFilename();
  logInfo(fmt::format("Saving summary to file \"{}\".", summaryFilename));

  const Statistics::Summary summary = stats.getSummary();

  std::ofstream f(summaryFilename, std::ios::out);
  f << "goom test version = " << versionInfo.getFullInfo() << std::endl;
  f << "goom lib version = " << getFullGoomVersionInfo() << std::endl;
  f << "goom width = " << options.getGoomTextureWidth() << std::endl;
  f << "goom height = " << options.getGoomTextureHeight() << std::endl;
  f << "num channels = " << 2 << std::endl;
  f << "options start buffer num = " << options.getStartBufferNum() << "" << std::endl;
  f << "options end buffer num = " << options.getEndBufferNum() << "" << std::endl;
  f << "mean update goom buffer time = " << summary.mean << std::endl;
  f << "min update goom buffer time  = " << summary.min << std::endl;
  f << "max update goom buffer time  = " << summary.max << std::endl;
  f << "num update goom buffers      = " << summary.num << std::endl;
//  f << "audio buffer len = " << AudioBufferLen() << std::endl;
}
