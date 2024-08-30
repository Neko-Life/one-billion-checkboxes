#include "atcboxes/commands.h"
#include "atcboxes/atcboxes.h"
#include "atcboxes/util.h"
#include <cstdint>
#include <regex>
#include <string_view>

namespace atcboxes::commands {

static std::pair<int64_t, int64_t> get_subs_pc(const std::string &s) {
  std::regex word_regex("(\\d+)");
  auto begin = std::sregex_iterator(s.begin(), s.end(), word_regex);

  auto end = std::sregex_iterator();
  if (begin == end)
    return {-1, -1};

  std::string p = (*begin).str();
  begin++;

  if (begin == end)
    return {-1, -1};

  std::string c = (*begin).str();

  return {std::stoll(p), std::stoll(c)};
}

static int subs(const std::string &s) {
  auto pc = get_subs_pc(s);

  if (pc.first == -1 || pc.second == -1)
    return -1;

  // !TODO: subs per page? maybe not

  return 0;
}

#ifdef WITH_COLOR
static int gcv(const std::string &s, cbox_t &cs) {
  size_t idx = 0;
  int r = get_state(std::stoull(s, &idx), cs);
  return idx > 0 ? r : -1;
}

static int gcv(uint64_t s, cbox_t &cs) {
  int r = get_state(s, cs);
  return r;
}
#else
static int gcv(const std::string &s) {
  size_t idx = 0;
  int r = get_state(std::stoull(s, &idx));
  return idx > 0 ? r : -1;
}
#endif // WITH_COLOR

static std::string p_gv() { return "v;" + std::to_string(get_gv()); }

static std::pair<CBOX_T const *, size_t> gp(const std::string &s) {
  size_t idx = 0;
  uint64_t p = std::stoull(s, &idx);
  if (idx == 0) {
    return {NULL, 0};
  }

  return get_state_page(p);
}

std::string p_state_wc(uint64_t n, const cbox_t &s) {
#ifdef WITH_COLOR
  std::string str = "";
  util::cbox_t_to_str(n, s, str);

  return "s;" + str;
#else
  return "";
#endif // WITH_COLOR
}

std::string p_state(const std::string &n, int s) {
  return "s;" + n + ';' + (s ? "1" : "0");
}

int run(std::string_view cmd, command_outs_t &out) {
  if (cmd.find("sc;") == 0) {
    if (cmd.length() < 6 || subs(std::string(cmd.substr(3))) == -1) {
      return -1;
    }

    // TODO

    return 0;
  }

  else if (cmd.find("gcv;") == 0) {
#ifdef WITH_COLOR
    int s = 0;
    cbox_t cs = {};
    size_t idx = 0;
    auto n = std::stoull(std::string(cmd.substr(4)));

    if (idx == 0 || cmd.length() < 5 || (s = gcv(n, cs)) == -1) {
      return -2;
    }

    if (s) {
      out.push_back({p_state_wc(n, cs), 1});
      return 0;
    }
#else
    int s = 0;
    auto n = std::string(cmd.substr(4));

    if (cmd.length() < 5 || (s = gcv(n)) == -1) {
      return -2;
    }

    if (s) {
      out.push_back({p_state(n, s), 1});
      return 0;
    }
#endif // WITH_COLOR
  }

  else if (cmd.find("gp;") == 0) {
    // calling get_state_page in gp should lock cbox mutex
    atcboxes::cbox_lock_guard_t lk;

    std::pair<CBOX_T const *, size_t> s = {NULL, 0};
    std::string page_number(cmd.substr(3));

    if (cmd.length() < 4 || (s = gp(page_number)).first == NULL) {
      return -3;
    }

    if (s.first) {
      constexpr const size_t conversion = sizeof(CBOX_T);
      out.push_back({std::string("ws;") + page_number, 0});
      out.push_back({{(const char *)s.first, s.second * conversion}, 1});
      return 0;
    }
  }

  else if (cmd.find("gv;") == 0) {
    if (cmd.length() != 3) {
      return -4;
    }

    out.push_back({p_gv(), 0});
    return 0;
  }

  return 1;
}

} // namespace atcboxes::commands
