#ifndef VVENGINE_AUDIO_WRITER_H_
#define VVENGINE_AUDIO_WRITER_H_

#include <vector>

namespace vvengine {
  bool WriteWAV(const char* fileName, const std::vector<float>& wave, int rate);
}

#endif // VVENGINE_AUDIO_WRITER_H_