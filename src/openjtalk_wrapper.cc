#include "vvengine/openjtalk_wrapper.h"

#include <openjtalk/mecab.h>
#include <openjtalk/njd.h>
#include <openjtalk/jpcommon.h>
#include <openjtalk/mecab2njd.h>
#include <openjtalk/njd2jpcommon.h>
#include <openjtalk/njd_set_accent_phrase.h>
#include <openjtalk/njd_set_accent_type.h>
#include <openjtalk/njd_set_digit.h>
#include <openjtalk/njd_set_long_vowel.h>
#include <openjtalk/njd_set_pronunciation.h>
#include <openjtalk/njd_set_unvoiced_vowel.h>
#include <openjtalk/text2mecab.h>

#include <iostream>

namespace vvengine {
struct OpenJtalkWrapper::Impl {
  Mecab* mecab;
  NJD* njd;
  JPCommon* jpcommon;

  Impl() {
    mecab = new Mecab;
    njd = new NJD;
    jpcommon = new JPCommon;
  }

  ~Impl() {
    Mecab_clear(mecab);
    NJD_clear(njd);
    JPCommon_clear(jpcommon);

    delete mecab;
    delete njd;
    delete jpcommon;
  }
};

OpenJtalkWrapper::OpenJtalkWrapper() : impl(new Impl) {}
OpenJtalkWrapper::~OpenJtalkWrapper() {}
bool OpenJtalkWrapper::Initialize() {
  BOOL mecabState;

  mecabState = Mecab_initialize(impl->mecab);
  NJD_initialize(impl->njd);
  JPCommon_initialize(impl->jpcommon);

  return mecabState == TRUE;
}

bool OpenJtalkWrapper::Load(const char* mecabDir) {
  BOOL mecabState;
  mecabState = Mecab_load(impl->mecab, mecabDir);
  return mecabState == TRUE;
}

void OpenJtalkWrapper::ExtractFullContext(const char* text,
                                          std::vector<std::string>& labels) {
  BOOL mecabState;
  char buff[8192];

  text2mecab(buff, text);
  mecabState = Mecab_analysis(impl->mecab, buff);
  mecab2njd(impl->njd, impl->mecab->feature, impl->mecab->size);
  njd_set_pronunciation(impl->njd);
  njd_set_digit(impl->njd);
  njd_set_accent_phrase(impl->njd);
  njd_set_accent_type(impl->njd);
  njd_set_unvoiced_vowel(impl->njd);
  njd_set_long_vowel(impl->njd);
  njd2jpcommon(impl->jpcommon, impl->njd);
  JPCommon_make_label(impl->jpcommon);

  int labelSize = JPCommon_get_label_size(impl->jpcommon);
  char** labelFeature = JPCommon_get_label_feature(impl->jpcommon);
  labels.clear();
  for (int i = 0; i < labelSize; i++) labels.emplace_back(labelFeature[i]);

  JPCommon_refresh(impl->jpcommon);
  NJD_refresh(impl->njd);
  Mecab_refresh(impl->mecab);
}

}  // namespace vvengine