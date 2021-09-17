#ifndef VVENGINE_ACOUSTIV_FEATURE_EXTRACTOR_H_
#define VVENGINE_ACOUSTIV_FEATURE_EXTRACTOR_H_

#include <map>
#include <random>
#include <string>
#include <vector>

namespace vvengine {
template <typename T>
inline std::vector<T> Resample(const std::vector<T>& wave, float rate,
                               float newRate, int stride = 1) {
  int length = (int)wave.size() / stride * newRate / rate;
  std::vector<T> newWave(length * stride);
  std::random_device rnd;
  std::default_random_engine rng(rnd());
  std::uniform_real_distribution<float> rand(0, 1);
  float offset = rand(rng);
  for (int i = 0; i < length; i++) {
    int index = (int)((offset + (float)i) * (rate / newRate));
    for (int j = 0; j < stride; j++)
      newWave[i * stride + j] = wave[index * stride + j];
  }
  return newWave;
}

struct JvsPhoneme {
  std::string phoneme;
  float start;
  float end;

  inline static void ConvertInPlace(std::vector<JvsPhoneme>& phonemes) {
    if (phonemes[0].phoneme.find("sil") != std::string::npos) {
      phonemes[0].phoneme = JvsPhoneme::spacePhoneme;
    }
    if (phonemes.back().phoneme.find("sil") != std::string::npos) {
      phonemes.back().phoneme = JvsPhoneme::spacePhoneme;
    }
  }
  inline int phonemeId() const { return phonemeMap.at(phoneme); }
  const std::map<std::string, int> phonemeMap = {
      {"pau", 0}, {"I", 1},   {"N", 2},   {"U", 3},   {"a", 4},   {"b", 5},
      {"by", 6},  {"ch", 7},  {"cl", 8},  {"d", 9},   {"dy", 10}, {"e", 11},
      {"f", 12},  {"g", 13},  {"gy", 14}, {"h", 15},  {"hy", 16}, {"h", 17},
      {"i", 18},  {"j", 19},  {"k", 20},  {"ky", 21}, {"m", 22},  {"my", 23},
      {"n", 24},  {"ny", 25}, {"o", 26},  {"p", 27},  {"py", 28}, {"r", 29},
      {"ry", 30}, {"s", 31},  {"sh", 32}, {"t", 33},  {"ts", 34}, {"u", 35},
      {"v", 36},  {"w", 37},  {"y", 38},  {"z", 39}};
  const int num_phoneme = phonemeMap.size();
  static constexpr char* spacePhoneme = "pau";
};

struct OjtPhoneme {
  std::string phoneme;
  float start;
  float end;

  inline static void ConvertInPlace(std::vector<OjtPhoneme>& phonemes) {
    if (phonemes[0].phoneme.find("sil") != std::string::npos) {
      phonemes[0].phoneme = OjtPhoneme::spacePhoneme;
    }
    if (phonemes.back().phoneme.find("sil") != std::string::npos) {
      phonemes.back().phoneme = OjtPhoneme::spacePhoneme;
    }
  }
  inline int phonemeId() const { return phonemeMap.at(phoneme); }
  const std::map<std::string, int> phonemeMap = {
      {"pau", 0}, {"A", 1},   {"E", 2},   {"I", 3},   {"N", 4},   {"O", 5},
      {"U", 6},   {"a", 7},   {"b", 8},   {"by", 9},  {"ch", 10}, {"cl", 11},
      {"d", 12},  {"dy", 13}, {"e", 14},  {"f", 15},  {"g", 16},  {"gw", 17},
      {"gy", 18}, {"h", 19},  {"hy", 20}, {"i", 21},  {"j", 22},  {"k", 23},
      {"kw", 24}, {"ky", 25}, {"m", 26},  {"my", 27}, {"n", 28},  {"ny", 29},
      {"o", 30},  {"p", 31},  {"py", 32}, {"r", 33},  {"ry", 34}, {"s", 35},
      {"sh", 36}, {"t", 37},  {"ts", 38}, {"ty", 39}, {"u", 40},  {"v", 41},
      {"w", 42},  {"y", 43},  {"z", 44}};
  const int num_phoneme = phonemeMap.size();
  static constexpr char* spacePhoneme = "pau";
};

}  // namespace vvengine
#endif  // VVENGINE_ACOUSTIV_FEATURE_EXTRACTOR_H_