#include "vvengine/engine.h"

#include <voicevox_core/core.h>

#include <iostream>
#include <algorithm>

#include "vvengine/acoustic_feature_extractor.h"
#include "vvengine/full_context_label.h"
#include "vvengine/openjtalk_wrapper.h"

namespace vvengine {

class NullStream : public std::streambuf, public std::ostream {
 public:
  virtual int overflow(int c) { return c; }
  NullStream() : std::ostream(this) {}
};

struct Engine::Impl {
  OpenJtalkWrapper openjtalk;
  bool initialized;
  std::shared_ptr<std::ostream> pLogger;

  Impl()
      : openjtalk(), initialized(false), pLogger(new NullStream) {}
  ~Impl() {}
};

Engine::Engine()
    : impl(new Impl) {
}
Engine::~Engine() {}
void Engine::SetLogger(const std::shared_ptr<std::ostream>& os) {
  impl->pLogger = os;
}
bool Engine::Initialize(bool useCUDA) {
  *impl->pLogger << (useCUDA ? "GPU" : "CPU") << " MODE" << std::endl;

  if (!initialize((char*)kCoreDir, useCUDA)) {
    *impl->pLogger << "[ERROR] Failed to initialize VoiceVox Core."
                   << std::endl;
    return false;
  }

  if (impl->initialized) {
    *impl->pLogger << "Openjtalk is already initialized. Skipping..."
                   << std::endl;
  } else {
    if (!impl->openjtalk.Initialize()) {
      *impl->pLogger << "[ERROR] Failed to initialize Openjtalk." << std::endl;
      return false;
    }
    if (!impl->openjtalk.Load(kMecabDir)) {
      *impl->pLogger << "[ERROR] Failed to load the mecab dictionary."
                     << std::endl;
      return false;
    }
  }
  impl->initialized = true;
  return true;
}
bool Engine::TextToSpeech(const char* textUtf8, long speakerId,
                          std::vector<float>& wave) {
  if (!impl->initialized) {
    *impl->pLogger << "[ERROR] This engine is not initialized." << std::endl;
    return false;
  }

  std::vector<std::string> labels;
  impl->openjtalk.ExtractFullContext(textUtf8, labels);

  *impl->pLogger << "===== extract fullcontext =====" << std::endl;
  for (const auto& s : labels) *impl->pLogger << s << std::endl;

  int rate = 200;
  auto utterance = ExtractFullContextLabel(labels);
  *impl->pLogger << "utterance ok" << std::endl;
  auto labelDataList = utterance.phonemes();
  *impl->pLogger << "phonemes ok" << std::endl;

  bool isType1 = false;
  std::vector<std::string> phonemeStrList;
  std::vector<long> startAccentList(labelDataList.size());
  std::vector<long> endAccentList(labelDataList.size());
  std::vector<long> startAccentPhraseList(labelDataList.size());
  std::vector<long> endAccentPhraseList(labelDataList.size());
  for (size_t i = 0; i < labelDataList.size(); i++) {
    const auto& label = labelDataList[i];
    bool isEndAccent = label.contexts.at("a1") == "0";
    if (label.contexts.at("a2") == "1") {
      isType1 = isEndAccent;
    }
    bool isStartAccent =
        (label.contexts.at("a2") == "1" && isType1)
            ? true
            : (label.contexts.at("a2") == "2" && !isType1) ? true : false;
    phonemeStrList.push_back(label.phoneme());
    startAccentList[i] = isStartAccent ? 1 : 0;
    endAccentList[i] = isEndAccent ? 1 : 0;
    startAccentPhraseList[i] = label.contexts.at("a2") == "1" ? 1 : 0;
    endAccentPhraseList[i] = label.contexts.at("a3") == "1" ? 1 : 0;
  }

  std::vector<OjtPhoneme> phonemeDataList;
  for (size_t i = 0; i < phonemeStrList.size(); i++) {
    phonemeDataList.push_back(
        OjtPhoneme{phonemeStrList[i], (float)i, (float)i + 1});
  }
  OjtPhoneme::ConvertInPlace(phonemeDataList);
  std::vector<long> phonemeListS;
  for (const auto& p : phonemeDataList) {
    phonemeListS.push_back(p.phonemeId());
  }

  std::vector<float> phonemeLength(phonemeListS.size());
  if (yukarin_s_forward(phonemeListS.size(), phonemeListS.data(), &speakerId,
                        phonemeLength.data())) {
    *impl->pLogger << "s forward OK." << std::endl;
  } else {
    return false;
  }

  phonemeLength.front() = phonemeLength.back() = 0.1;
  for (auto& len : phonemeLength) len = std::round(len * rate) / rate;

  for (const auto& p : phonemeDataList) *impl->pLogger << p.phoneme << " ";
  *impl->pLogger << std::endl;

  // split mora
  const std::vector<std::string> unvoiceList = {"A", "I",  "U",  "E",
                                                "O", "cl", "pau"};
  const std::vector<std::string> vowelList = {
      "A", "I", "U", "E", "O", "cl", "pau", "a", "i", "u", "e", "o", "N"};
  std::vector<int> vowelIndices;
  std::vector<OjtPhoneme> vowelPhonemeDataList;
  std::vector<std::optional<OjtPhoneme> > consonantPhonemeDataList;
  for (int i = 0; i < (int)phonemeDataList.size(); i++) {
    if (std::find(vowelList.begin(), vowelList.end(),
                  phonemeDataList[i].phoneme) != vowelList.end()) {
      vowelIndices.push_back(i);
      vowelPhonemeDataList.push_back(phonemeDataList[i]);
    }
  }
  consonantPhonemeDataList.push_back(std::nullopt);
  for (int i = 0; i < (int)vowelIndices.size() - 1; i++) {
    int post = vowelIndices[i + 1];
    int prev = vowelIndices[i];
    if (post - prev == 1)
      consonantPhonemeDataList.push_back(std::nullopt);
    else
      consonantPhonemeDataList.push_back(phonemeDataList[post - 1]);
  }
  *impl->pLogger << "split mora ok" << std::endl;

  std::vector<long> vowelPhonemeList, consonantPhonemeList;
  for (const auto& p : vowelPhonemeDataList)
    vowelPhonemeList.push_back(p.phonemeId());
  for (const auto& p : consonantPhonemeDataList)
    consonantPhonemeList.push_back(p ? p.value().phonemeId() : -1);

  std::vector<float> phonemeLengthSA;
  {
    int i = 0;
    for (size_t vi = 0; vi + 1 < vowelIndices.size(); vi++) {
      int v = vowelIndices[vi];
      float a = 0;
      for (; i < v + 1; i++) a += phonemeLength[i];
      phonemeLengthSA.push_back(a);
    }
    float a = 0;
    for (; i < (int)phonemeLength.size(); i++) a += phonemeLength[i];
    phonemeLengthSA.push_back(a);
  }

  std::vector<float> f0List(vowelPhonemeList.size());
  std::vector<long> startAccentListVowel, endAccentListVowel,
      startAccentPhraseListVowel, endAccentPhraseListVowel;
  for (int i : vowelIndices) {
    startAccentListVowel.push_back(startAccentList[i]);
    endAccentListVowel.push_back(endAccentList[i]);
    startAccentPhraseListVowel.push_back(startAccentPhraseList[i]);
    endAccentPhraseListVowel.push_back(endAccentPhraseList[i]);
  }
  if (yukarin_sa_forward(
          vowelPhonemeList.size(), vowelPhonemeList.data(),
          consonantPhonemeList.data(), startAccentListVowel.data(),
          endAccentListVowel.data(), startAccentPhraseListVowel.data(),
          endAccentPhraseListVowel.data(), &speakerId, f0List.data())) {
    *impl->pLogger << "sa forward OK." << std::endl;
  } else {
    return false;
  }

  for (size_t i = 0; i < vowelPhonemeDataList.size(); i++) {
    if (std::find(unvoiceList.begin(), unvoiceList.end(),
                  vowelPhonemeDataList[i].phoneme) != unvoiceList.end()) {
      f0List[i] = 0;
    }
  }

  std::vector<long> phoneme, f0;
  for (size_t i = 0; i < phonemeListS.size(); i++)
    for (int j = 0; j < std::round(phonemeLength[i] * rate); j++)
      phoneme.push_back(phonemeListS[i]);

  for (size_t i = 0; i < f0List.size(); i++)
    for (int j = 0; j < std::round(phonemeLengthSA[i] * rate); j++)
      f0.push_back(f0List[i]);

  int phonemeSize = OjtPhoneme{}.num_phoneme;
  std::vector<float> onehotPhoneme(phoneme.size() * phonemeSize),
      ff0(phoneme.size());
  for (size_t i = 0; i < phoneme.size(); i++)
    onehotPhoneme[i * phonemeSize + phoneme[i]] = 1;
  for (size_t i = 0; i < phoneme.size(); i++) ff0[i] = (float)f0[i];

  ff0 = Resample(ff0, rate, 24000.0 / 256.0);
  onehotPhoneme = Resample(onehotPhoneme, rate, 24000.0 / 256.0, phonemeSize);

  wave.resize(ff0.size() * 256);
  if (decode_forward(ff0.size(), phonemeSize, ff0.data(), onehotPhoneme.data(),
                     &speakerId, wave.data())) {
    *impl->pLogger << "decode ok" << std::endl;
  } else {
    return false;
  }

  return true;
}

}  // namespace vvengine