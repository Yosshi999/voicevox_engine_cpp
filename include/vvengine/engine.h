#ifndef VVENGINE_ENGINE_H_
#define VVENGINE_ENGINE_H_

#ifndef MECAB_DIR
#define MECAB_DIR "./open_jtalk_dic_utf_8-1.11"
#endif

#include <memory>
#include <string>
#include <vector>

namespace vvengine {
constexpr const char* kMecabDir = MECAB_DIR;
constexpr const char* kCoreDir = "./";

class Engine {
 public:
  Engine();
  ~Engine();

  bool Initialize(bool useCUDA);
  void SetLogger(const std::shared_ptr<std::ostream>& os);
  bool TextToSpeech(const char* textUtf8, long speakerId,
                    std::vector<float>& wave);

 protected:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
}  // namespace vvengine

#endif  // VVENGINE_ENGINE_H_