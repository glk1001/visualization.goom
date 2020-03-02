#ifndef LIBS_UTILS_INCLUDE_UTILS_TEST_COMMON_H_
#define LIBS_UTILS_INCLUDE_UTILS_TEST_COMMON_H_

extern "C" {
  #include "goom/goom.h"
  #include "goom/goom_config.h"
}
#include <string>
#include <ostream>

class CommonTestOptions {
  public:
    static const constexpr char *VERSION = "version";
    static const constexpr char *INPUT_DIR = "input-dir";
    static const constexpr char *OUTPUT_DIR = "output-dir";
    static const constexpr char *STATE_NUM = "state-num";
    static const constexpr char *GOOM_SEED = "goom-seed";
    static const constexpr char *DEFAULT_GOOM_SEED = "1";
    static const constexpr char *AUDIO_FILE_PREFIX = "audio-file-prefix";
    static const constexpr char *GOOM_FILE_PREFIX = "goom-file-prefix";
    static const constexpr char *DEFAULT_AUDIO_FILE_PREFIX = "audio_buffer";
    static const constexpr char *DEFAULT_GOOM_FILE_PREFIX = "goom_buffer";
    static const constexpr char *DEFAULT_STATE_FILE_PREFIX = "goom_state";
    static const constexpr char *START_BUFFER_NUM = "start-buffer-num";
    static const constexpr char *END_BUFFER_NUM = "end-buffer-num";
    static const constexpr char *DEFAULT_START_BUFFER_NUM = "0";
    static const constexpr char *DEFAULT_END_BUFFER_NUM = "-1";
};

#endif
