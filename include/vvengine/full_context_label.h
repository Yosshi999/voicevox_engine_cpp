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

  inline std::vector<Phoneme> phonemes() const {
    std::vector<Phoneme> _phonemes;
    if (consonant) {
      _phonemes.push_back(consonant.value());
    }
    _phonemes.push_back(vowel);
    return _phonemes;
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

  inline static AccentPhrase FromPhonemes(
      const std::vector<Phoneme>& phonemes) {
    std::vector<Mora> moras;
    std::vector<Phoneme> moraPhonemes;
    for (size_t i = 0; i < phonemes.size(); i++) {
      if (phonemes[i].contexts.at("a2") == "49") break;

      moraPhonemes.push_back(phonemes[i]);
      if (i + 1 == phonemes.size() ||
          phonemes[i].contexts.at("a2") != phonemes[i + 1].contexts.at("a2")) {
        if (moraPhonemes.size() == 1) {
          moras.push_back(Mora{std::nullopt, moraPhonemes[0]});
        } else if (moraPhonemes.size() == 2) {
          moras.push_back(Mora{moraPhonemes[0], moraPhonemes[1]});
        } else {
          throw std::runtime_error("too long moraPhonemes.");
        }
        moraPhonemes.clear();
      }
    }
    int accent = std::stoi(moras[0].vowel.contexts.at("f2"));
    accent = std::min(accent, (int)moras.size());
    return AccentPhrase{moras, accent};
  }

  inline std::vector<Phoneme> phonemes() const {
    std::vector<Phoneme> moraPhonemes;
    for (const auto& m : moras) {
      auto mp = m.phonemes();
      std::copy(mp.begin(), mp.end(), std::back_inserter(moraPhonemes));
    }
    return moraPhonemes;
  }

  inline void SetContext(const std::string& key, const std::string& value) {
    for (auto& mora : moras) {
      mora.SetContext(key, value);
    }
  }
};

struct BreathGroup {
  std::vector<AccentPhrase> accentPhrases;

  inline static BreathGroup FromPhonemes(const std::vector<Phoneme>& phonemes) {
    std::vector<AccentPhrase> accentPhrases;
    std::vector<Phoneme> accentPhonemes;
    for (size_t i = 0; i < phonemes.size(); i++) {
      accentPhonemes.push_back(phonemes[i]);
      if (i + 1 == phonemes.size() ||
          phonemes[i].contexts.at("i3") != phonemes[i + 1].contexts.at("i3") ||
          phonemes[i].contexts.at("f5") != phonemes[i + 1].contexts.at("f5")) {
        accentPhrases.push_back(AccentPhrase::FromPhonemes(accentPhonemes));
        accentPhonemes.clear();
      }
    }
    return BreathGroup{accentPhrases};
  }

  inline std::vector<Phoneme> phonemes() {
    std::vector<Phoneme> accentPhonemes;
    for (const auto& accentPhrase : accentPhrases) {
      auto ap = accentPhrase.phonemes();
      std::copy(ap.begin(), ap.end(), std::back_inserter(accentPhonemes));
    }
    return accentPhonemes;
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

  inline static Utterance FromPhonemes(const std::vector<Phoneme>& phonemes) {
    std::vector<Phoneme> pauses;
    std::vector<BreathGroup> breathGroups;
    std::vector<Phoneme> groupPhonemes;
    for (const auto& p : phonemes) {
      if (!p.IsPause()) {
        groupPhonemes.push_back(p);
      } else {
        pauses.push_back(p);
        if (groupPhonemes.size() > 0) {
          breathGroups.push_back(BreathGroup::FromPhonemes(groupPhonemes));
          groupPhonemes.clear();
        }
      }
    }
    return Utterance{breathGroups, pauses};
  }

  inline void SetContext(const std::string& key, const std::string& value) {
    for (auto& breathGroup : breathGroups) {
      breathGroup.SetContext(key, value);
    }
  }

  inline std::vector<Phoneme> phonemes() {
    std::vector<AccentPhrase> accentPhrases;
    std::vector<int> accentPhrasesIndices;
    for (const auto& breathGroup : breathGroups) {
      accentPhrasesIndices.push_back(accentPhrases.size());
      std::copy(breathGroup.accentPhrases.begin(),
                breathGroup.accentPhrases.end(),
                std::back_inserter(accentPhrases));
    }
    for (size_t i = 0; i < accentPhrases.size(); i++) {
      auto& cent = accentPhrases[i];
      int moraNum = cent.moras.size();
      int accent = cent.accent;
      if (i > 0) {
        accentPhrases[i - 1].SetContext("g1", std::to_string(moraNum));
        accentPhrases[i - 1].SetContext("g2", std::to_string(accent));
      }
      if (i + 1 < accentPhrases.size()) {
        accentPhrases[i + 1].SetContext("e1", std::to_string(moraNum));
        accentPhrases[i + 1].SetContext("e2", std::to_string(accent));
      }
      cent.SetContext("f1", std::to_string(moraNum));
      cent.SetContext("f2", std::to_string(accent));
      for (size_t j = 0; j < cent.moras.size(); j++) {
        auto& mora = cent.moras[j];
        mora.SetContext("a1", std::to_string(j - accent + 1));
        mora.SetContext("a2", std::to_string(j + 1));
        mora.SetContext("a3", std::to_string(moraNum - j));
      }
    }
    for (size_t i = 0; i < breathGroups.size(); i++) {
      auto& cent = breathGroups[i];
      int accentPhraseNum = cent.accentPhrases.size();
      if (i > 0) {
        breathGroups[i - 1].SetContext("j1", std::to_string(accentPhraseNum));
      }
      if (i + 1 < breathGroups.size()) {
        breathGroups[i + 1].SetContext("h1", std::to_string(accentPhraseNum));
      }
      cent.SetContext("i1", std::to_string(accentPhraseNum));
      cent.SetContext("i5", std::to_string(accentPhrasesIndices[i] + 1));
      cent.SetContext("i6", std::to_string((int)accentPhrases.size() -
                                           accentPhrasesIndices[i]));
    }
    int k2sum = 0;
    for (const auto& breathGroup : breathGroups)
      k2sum += breathGroup.accentPhrases.size();
    SetContext("k2", std::to_string(k2sum));

    std::vector<Phoneme> phonemes;
    for (size_t i = 0; i < pauses.size(); i++) {
      // if (pauses[i]) {
      if (true) {
        phonemes.push_back(pauses[i]);
      }
      if (i + 1 < pauses.size()) {
        auto bp = breathGroups[i].phonemes();
        std::copy(bp.begin(), bp.end(), std::back_inserter(phonemes));
      }
    }
    return phonemes;
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