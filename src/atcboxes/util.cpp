#include "atcboxes/util.h"
#include <regex>

namespace atcboxes::util {

FILE *try_open(const char *filepath, const char *mode) {
  FILE *f = fopen(filepath, mode);
  if (!f) {
    perror("[util::try_open ERROR]");
    return NULL;
  }

  return f;
}

#ifdef WITH_COLOR
int parse_cbox_wc(const std::string &msg, uint64_t &i, cbox_t &s) {
  // idx;r;g;b;a
  std::regex word_regex("(\\d+)");
  auto begin = std::sregex_iterator(msg.begin(), msg.end(), word_regex);

  auto end = std::sregex_iterator();

  std::vector<uint64_t> l = {};

  while (begin != end) {
    std::string c = (*begin).str();

    try {
      l.push_back(std::stoull(c));
    } catch (...) {
      return -1;
    }

    begin++;
  }

  if (l.size() != 5)
    return -2;

  i = l.at(0);
  s.r = l.at(1);
  s.g = l.at(2);
  s.b = l.at(3);
  s.a = l.at(4);

  return 0;
}

void cbox_t_to_str(uint64_t i, const cbox_t &s, std::string &str) {
  str = std::to_string(i) + ';' + std::to_string(s.r) + ';' +
        std::to_string(s.g) + ';' + std::to_string(s.b) + ';' +
        std::to_string(s.a);
}
#endif // WITH_COLOR

} // namespace atcboxes::util
