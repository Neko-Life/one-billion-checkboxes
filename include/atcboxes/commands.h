#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>

namespace atcboxes::commands {

struct command_out_t {
  std::string out;
  uint64_t flags;
};

using command_outs_t = std::vector<command_out_t>;

std::string p_state(const std::string &n, int s);

int run(std::string_view cmd, command_outs_t &out);

} // namespace atcboxes::commands

#endif // COMMANDS_H
