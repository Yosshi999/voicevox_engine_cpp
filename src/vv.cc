#include <voicevox_core/core.h>

#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "vvengine/acoustic_feature_extractor.h"
#include "vvengine/audio_writer.h"
#include "vvengine/full_context_label.h"
#include "vvengine/openjtalk_wrapper.h"

#ifdef USE_CUDA
#undef USE_CUDA
#define USE_CUDA true
#else
#define USE_CUDA false
#endif

#ifndef MECAB_DIR
#define MECAB_DIR "./open_jtalk_dic_utf_8-1.11"
#endif

using json = nlohmann::json;
int main() {
  std::cout << "USE_CUDA: " << (USE_CUDA ? "True" : "False") << std::endl;
  if (!initialize("./", USE_CUDA)) {
    std::cout << "Core Initialize Error" << std::endl;
    return -1;
  }

  std::cout << "Core Initialize OK" << std::endl;

  const char* inputText = u8"ハローワールド";

  vvengine::OpenJtalkWrapper wrapper;
  wrapper.Initialize();
  wrapper.Load(MECAB_DIR);
  std::vector<std::string> labels;
  wrapper.ExtractFullContext(inputText, labels);
  std::cout << "===== extract fullcontext =====" << std::endl;
  for (const auto& s : labels) std::cout << s << std::endl;

  int rate = 200;
  auto utterance = vvengine::ExtractFullContextLabel(labels);
  std::cout << "utterance" << std::endl;
  auto labelDataList = utterance.phonemes();
  std::cout << "phonemes" << std::endl;

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

  std::vector<vvengine::OjtPhoneme> phonemeDataList;
  for (size_t i = 0; i < phonemeStrList.size(); i++) {
    phonemeDataList.push_back(
        vvengine::OjtPhoneme{phonemeStrList[i], (float)i, (float)i + 1});
  }
  vvengine::OjtPhoneme::ConvertInPlace(phonemeDataList);
  std::vector<long> phonemeListS;
  for (const auto& p : phonemeDataList) {
    phonemeListS.push_back(p.phonemeId());
  }

  long speaker_id = 1;
  std::vector<float> phonemeLength(phonemeListS.size());
  if (yukarin_s_forward(phonemeListS.size(), phonemeListS.data(), &speaker_id,
                        phonemeLength.data())) {
    std::cout << "s forward OK." << std::endl;
  } else {
    return -1;
  }

  phonemeLength.front() = phonemeLength.back() = 0.1;
  for (auto& len : phonemeLength) len = std::round(len * rate) / rate;

  for (const auto& p : phonemeDataList) std::cout << p.phoneme << " ";
  std::cout << std::endl;
  // split mora
  const std::vector<std::string> unvoiceList = {"A", "I",  "U",  "E",
                                                "O", "cl", "pau"};
  const std::vector<std::string> vowelList = {
      "A", "I", "U", "E", "O", "cl", "pau", "a", "i", "u", "e", "o", "N"};
  std::vector<int> vowelIndices;
  std::vector<vvengine::OjtPhoneme> vowelPhonemeDataList;
  std::vector<std::optional<vvengine::OjtPhoneme> > consonantPhonemeDataList;
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
  std::cout << "split mora" << std::endl;

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
  // std::cout << vowelPhonemeList.size() << "," << consonantPhonemeList.size()
  // << ","
  //   << startAccentListVowel.size() << "," << endAccentListVowel.size() << ","
  //   << startAccentPhraseListVowel.size() << "," <<
  //   endAccentPhraseListVowel.size() << ","
  //   << f0List.size() << std::endl;
  if (yukarin_sa_forward(
          vowelPhonemeList.size(), vowelPhonemeList.data(),
          consonantPhonemeList.data(), startAccentListVowel.data(),
          endAccentListVowel.data(), startAccentPhraseListVowel.data(),
          endAccentPhraseListVowel.data(), &speaker_id, f0List.data())) {
    std::cout << "sa forward OK." << std::endl;
  } else {
    return -1;
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

  int phonemeSize = vvengine::OjtPhoneme{}.num_phoneme;
  std::vector<float> onehotPhoneme(phoneme.size() * phonemeSize),
      ff0(phoneme.size());
  for (size_t i = 0; i < phoneme.size(); i++)
    onehotPhoneme[i * phonemeSize + phoneme[i]] = 1;
  for (size_t i = 0; i < phoneme.size(); i++) ff0[i] = (float)f0[i];

  ff0 = vvengine::Resample(ff0, rate, 24000.0 / 256.0);
  onehotPhoneme =
      vvengine::Resample(onehotPhoneme, rate, 24000.0 / 256.0, phonemeSize);

  std::vector<float> wave(ff0.size() * 256);
  // std::cout << ff0.size() << "," << phonemeSize << "," << ff0.size() << ","
  //           << onehotPhoneme.size() << "," << wave.size() << std::endl;
  if (decode_forward(ff0.size(), phonemeSize, ff0.data(),
                     onehotPhoneme.data(), &speaker_id, wave.data())) {
    std::cout << "decode ok" << std::endl;
  } else {
    return -1;
  }

  vvengine::WriteWAV("out.wav", wave, 24000);

  return 0;
}