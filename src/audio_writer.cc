#include "vvengine/audio_writer.h"

#include <algorithm>
#include <fstream>
#include <iostream>

namespace vvengine {
template <typename T>
void WriteWord(std::ostream& f, T value, size_t size) {
  while (size > 0) {
    f.put(static_cast<char>(value & 0xFF));
    size--;
    value >>= 8;
  }
}
bool WriteWAV(const char* fileName, const std::vector<float>& wave, int rate) {
  std::ofstream f(fileName, std::ios::binary);
  if (!f.is_open()) return false;

  f.write("RIFF", 4);
  WriteWord(f, 0, 4);
  f.write("WAVEfmt ", 8);
  WriteWord(f, 16, 4);        // fmt header length
  WriteWord(f, 1, 2);         // linear PCM
  WriteWord(f, 1, 2);         // mono
  WriteWord(f, rate, 4);      // sample rate
  WriteWord(f, rate * 2, 4);  // sample rate * bytes per sample * channels
  WriteWord(f, 2, 2);         // data block size
  WriteWord(f, 16, 2);        // bit per sample
  f.write("data", 4);
  size_t data_p = f.tellp();
  WriteWord(f, 0, 4);
  for (float v : wave) {
    WriteWord(f, (short)(std::min(1.0f, std::max(v, -1.0f)) * (float)0x7FFF), 2);
  }
  size_t last_p = f.tellp();
  f.seekp(4);
  WriteWord(f, last_p - 8, 4);
  f.seekp(data_p);
  WriteWord(f, last_p - data_p - 4, 4);

  f.close();
  return true;
}

}  // namespace vvengine