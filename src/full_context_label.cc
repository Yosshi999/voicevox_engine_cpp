#include "vvengine/full_context_label.h"

#include <boost/xpressive/regex_actions.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace vvengine {

using namespace boost::xpressive;
Phoneme Phoneme::FromLabel(const std::string& label) {
  auto rx = sregex::compile(
      R"(^(?P<p1>.+?)\^(?P<p2>.+?)\-(?P<p3>.+?)\+(?P<p4>.+?)\=(?P<p5>.+?))"
      R"(/A\:(?P<a1>.+?)\+(?P<a2>.+?)\+(?P<a3>.+?))"
      R"(/B\:(?P<b1>.+?)\-(?P<b2>.+?)\_(?P<b3>.+?))"
      R"(/C\:(?P<c1>.+?)\_(?P<c2>.+?)\+(?P<c3>.+?))"
      R"(/D\:(?P<d1>.+?)\+(?P<d2>.+?)\_(?P<d3>.+?))"
      R"(/E\:(?P<e1>.+?)\_(?P<e2>.+?)\!(?P<e3>.+?)\_(?P<e4>.+?)\-(?P<e5>.+?))"
      R"(/F\:(?P<f1>.+?)\_(?P<f2>.+?)\#(?P<f3>.+?)\_(?P<f4>.+?)\@(?P<f5>.+?)\_(?P<f6>.+?)\|(?P<f7>.+?)\_(?P<f8>.+?))"
      R"(/G\:(?P<g1>.+?)\_(?P<g2>.+?)\%(?P<g3>.+?)\_(?P<g4>.+?)\_(?P<g5>.+?))"
      R"(/H\:(?P<h1>.+?)\_(?P<h2>.+?))"
      R"(/I\:(?P<i1>.+?)\-(?P<i2>.+?)\@(?P<i3>.+?)\+(?P<i4>.+?)\&(?P<i5>.+?)\-(?P<i6>.+?)\|(?P<i7>.+?)\+(?P<i8>.+?))"
      R"(/J\:(?P<j1>.+?)\_(?P<j2>.+?))"
      R"(/K\:(?P<k1>.+?)\+(?P<k2>.+?)\-(?P<k3>.+?)$)");
  std::vector<std::string> keys = {
      "p1", "p2", "p3", "p4", "p5", "a1", "a2", "a3", "b1", "b2",
      "b3", "c1", "c2", "c3", "d1", "d2", "d3", "e1", "e2", "e3",
      "e4", "e5", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
      "g1", "g2", "g3", "g4", "g5", "h1", "h2", "i1", "i2", "i3",
      "i4", "i5", "i6", "i7", "i8", "j1", "j2", "k1", "k2", "k3"};

  Phoneme phoneme;
  smatch what;

  regex_search(label, what, rx);
  for (const auto& k : keys) phoneme.contexts[k] = what[k];

  return phoneme;
}

AccentPhrase AccentPhrase::FromPhonemes(const std::vector<Phoneme>& phonemes) {
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

BreathGroup BreathGroup::FromPhonemes(const std::vector<Phoneme>& phonemes) {
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

Utterance Utterance::FromPhonemes(const std::vector<Phoneme>& phonemes) {
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

std::vector<Phoneme> Utterance::phonemes() {
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
      breathGroups[i].phonemes(phonemes);
    }
  }
  return phonemes;
}

}  // namespace vvengine