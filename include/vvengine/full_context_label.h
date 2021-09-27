#ifndef VVENGINE_FULL_CONTEXT_LABEL_H_
#define VVENGINE_FULL_CONTEXT_LABEL_H_

#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace vvengine {
struct Phoneme {
  std::map<std::string, std::string> contexts;

  static Phoneme FromLabel(const std::string& label);
  inline std::string phoneme() const { return contexts.at("p3"); }
  inline bool IsPause() const { return contexts.at("f1") == "xx"; }
};

struct Mora {
  std::optional<Phoneme> consonant;
  Phoneme vowel;

  inline void phonemes(std::vector<Phoneme>& out) const {
    if (consonant) {
      out.push_back(consonant.value());
    }
    out.push_back(vowel);
  }

  inline void SetContext(const std::string& key, const std::string& value) {
    vowel.contexts[key] = value;
    if (consonant) {
      consonant.value().contexts[key] = value;
    }
  }
};

struct AccentPhrase {
  std::vector<Mora> moras;
  int accent;

  static AccentPhrase FromPhonemes(const std::vector<Phoneme>& phonemes);

  inline void phonemes(std::vector<Phoneme>& out) const {
    for (const auto& m : moras) {
      m.phonemes(out);
    }
  }

  inline void SetContext(const std::string& key, const std::string& value) {
    for (auto& mora : moras) {
      mora.SetContext(key, value);
    }
  }
};

struct BreathGroup {
  std::vector<AccentPhrase> accentPhrases;

  static BreathGroup FromPhonemes(const std::vector<Phoneme>& phonemes);

  inline void phonemes(std::vector<Phoneme>& out) const {
    for (const auto& accentPhrase : accentPhrases) {
      accentPhrase.phonemes(out);
    }
  }

  inline void SetContext(const std::string& key, const std::string& value) {
    for (auto& accentPhrase : accentPhrases) {
      accentPhrase.SetContext(key, value);
    }
  }
};

struct Utterance {
  std::vector<BreathGroup> breathGroups;
  std::vector<Phoneme> pauses;

  static Utterance FromPhonemes(const std::vector<Phoneme>& phonemes);
  std::vector<Phoneme> phonemes();

  inline void SetContext(const std::string& key, const std::string& value) {
    for (auto& breathGroup : breathGroups) {
      breathGroup.SetContext(key, value);
    }
  }
};

inline Utterance ExtractFullContextLabel(
    const std::vector<std::string>& labels) {
  std::vector<vvengine::Phoneme> phonemes;
  for (const auto& label : labels) {
    phonemes.push_back(vvengine::Phoneme::FromLabel(label));
  }
  return Utterance::FromPhonemes(phonemes);
}

}  // namespace vvengine
#endif  // VVENGINE_FULL_CONTEXT_LABEL_H_