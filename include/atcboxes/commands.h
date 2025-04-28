#ifndef COMMANDS_H
#define COMMANDS_H

#include "atcboxes/atcboxes.h"
#include <string>
#include <vector>

namespace atcboxes::commands {

struct command_out_t {
  std::string out;
  uint64_t flags;
};

using command_outs_t = std::vector<command_out_t>;

std::string p_state(const std::string &n, int s);

std::string p_state_wc(uint64_t n, const cbox_t &s);

int run(std::string_view cmd, command_outs_t &out);

} // namespace atcboxes::commands

#endif // COMMANDS_H
