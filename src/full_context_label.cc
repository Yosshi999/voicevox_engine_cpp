#include "vvengine/full_context_label.h"

#include <boost/xpressive/regex_actions.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>

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
    "p1", "p2", "p3", "p4", "p5",
    "a1", "a2", "a3",
    "b1", "b2", "b3",
    "c1", "c2", "c3",
    "d1", "d2", "d3",
    "e1", "e2", "e3", "e4", "e5",
    "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
    "g1", "g2", "g3", "g4", "g5",
    "h1", "h2",
    "i1", "i2", "i3", "i4", "i5", "i6", "i7", "i8",
    "j1", "j2",
    "k1", "k2", "k3"
  };

  Phoneme phoneme;
  smatch what;

  regex_search(label, what, rx);
  for (const auto& k : keys) phoneme.contexts[k] = what[k];

  return phoneme;
}
}  // namespace vvengine