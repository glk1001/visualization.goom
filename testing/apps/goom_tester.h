#ifndef APPS_GOOM_TESTER_H_
#define APPS_GOOM_TESTER_H_

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
#include <queue>
#include <memory>


class GoomTestOptions {
  public:
    explicit GoomTestOptions(const CommandLineOptions& cmdLineOptions);
    size_t getAudioBufferLen() const { return audioBufferLen; }
    bool getUseAudioFiles() const { return useAudioFiles; };
    const std::string& getSongTitle() const { return songTitle; };
    std::string getLogFilename() const;
    const std::string& getInputDir() const { return inputDir; };
    std::string getAudioInputDir() const;
    std::string getAudioOutputDir() const;
    std::string getAudioInputPrefix() const;
    std::string getAudioOutputPrefix() const;
    const std::string& getOutputDir() const { return outputDir; };
    std::string getGoomBufferOutputDir() const;
    std::string getGoomBufferOutputPrefix() const;
    std::string getGoomStateInputDir() const;
    std::string getGoomStateOutputDir() const;
    std::string getGoomStateInputPrefix() const;
    std::string getGoomStateOutputPrefix() const;
    std::string getGoomStateInputFilename() const;
    std::string getSummaryFilename() const;
    int getGoomSeed() const { return goomSeed; }
    int getNumAudioChannels() const { return numAudioChannels; }
    uint32_t getGoomTextureWidth() const { return goomTextureWidth; }
    uint32_t getGoomTextureHeight() const { return goomTextureHeight; }
    long getStartBufferNum() const { return startBufferNum; }
    long getEndBufferNum() const { return endBufferNum; }
  private:
    const int goomSeed;
    const long stateNum;
    const size_t audioBufferLen = 500;
    const bool useAudioFiles = true;
    const std::string songTitle;
    const std::string inputDir;
    const std::string outputDir;
    const std::string audioDataFilePrefix;
    const std::string goomBufferFilePrefix;
    const long startBufferNum;
    long endBufferNum;
    const int numAudioChannels = 2;
    const uint32_t goomTextureWidth = 1280;
    const uint32_t goomTextureHeight = 720;
};
 
class AudioData {
  public:
	AudioData(const long startBuffNum, const long endBuffNum);
    virtual ~AudioData();
    virtual AudioDataBuffer* getNext();
  private:
    const long startBufferNum;
    const long endBufferNum;
    long buffNum;  
    bool doMoreBuffers() const;
};

class AudioDataFiles: public AudioData {
  public:
	AudioDataFiles(const long startBuffNum, const long endBuffNum, const std::string &filenamePrefix);
    virtual ~AudioDataFiles();
    AudioDataBuffer* getNext() override;
  private:
    BufferSaver<float> bufferReader;
};

class GoomTester {
public:    
    explicit GoomTester(const VersionInfo& ver, const GoomTestOptions& opts);
    ~GoomTester();
    void AudioDataConsumerThread();
    void AudioDataProducerThread();
private:
  std::unique_ptr<PluginInfo> goom;
  const VersionInfo& versionInfo;
  GoomTestOptions options;
  const std::string songTitle;
  bool allDone = false;
  Statistics stats;
  std::mutex audioDataMutex;
  std::condition_variable audioDataProducer_cv;
  std::condition_variable audioDataConsumer_cv;
  std::queue<std::shared_ptr<AudioDataBuffer> > audioDataQueue;
  void UpdateGoomBuffer(const long bufferNum, const float floatAudioData[], uint32_t* pixels);
  void SaveCurrentGoomState(const long currentBufferNum, const int64_t duration=-1);
  std::shared_ptr<AudioData> GetAudioData() const;
  const char* getTitle() const;
  void saveSummary();
};

#endif
