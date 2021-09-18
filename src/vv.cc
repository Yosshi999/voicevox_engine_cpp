
#include <fstream>
#include <vector>
#include <memory>

#include "vvengine/audio_writer.h"
#include "vvengine/engine.h"

#ifdef USE_CUDA
#undef USE_CUDA
#define USE_CUDA true
#else
#define USE_CUDA false
#endif

int main() {
  const char* inputText = u8"ハローワールド";
  vvengine::Engine engine;

  std::shared_ptr<std::ofstream> log(new std::ofstream("log.txt"));
  engine.SetLogger(log);
  engine.Initialize(false);

  std::vector<float> wave;
  engine.TextToSpeech(inputText, 0, wave);

  vvengine::WriteWAV("out.wav", wave, 24000);
  log->close();
  return 0;
}