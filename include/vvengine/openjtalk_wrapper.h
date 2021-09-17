#ifndef VVENGINE_OPENJTALK_WRAPPER_H_
#define VVENGINE_OPENJTALK_WRAPPER_H_

#include <vector>
#include <string>
#include <memory>

namespace vvengine {
  class OpenJtalkWrapper {
    public:
    OpenJtalkWrapper();
    ~OpenJtalkWrapper();
    int Initialize();
    int Load(const char* mecabDir);
    void ExtractFullContext(const char* text, std::vector<std::string>& labels);

    protected:
    struct Impl;
    std::unique_ptr<Impl> impl;
  };
}

#endif // VVENGINE_OPENJTALK_WRAPPER_H_