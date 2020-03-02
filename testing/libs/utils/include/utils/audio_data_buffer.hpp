#ifndef LIBS_UTILS_INCLUDE_UTILS_AUDIO_DATA_BUFFER_H_
#define LIBS_UTILS_INCLUDE_UTILS_AUDIO_DATA_BUFFER_H_

extern "C" {
  #include "goom/goom.h"
  #include "goom/goom_config.h"
}


class AudioDataBuffer {
  public:  
    explicit AudioDataBuffer(const long newTag, const int newLen=NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN);
    long getTag() const { return tag; };
    int getBuffLen() const { return buffLen; };
    const float* getBuffer() const { return buffer; }
    float* getBuffer() { return buffer; }
  private:  
    float buffer[NUM_AUDIO_SAMPLES*AUDIO_SAMPLE_LEN];
    const long tag;
    const int buffLen;
};

inline AudioDataBuffer::AudioDataBuffer(const long newTag, const int newLen)
: tag { newTag },
  buffLen { newLen }
{
}
  
#endif
