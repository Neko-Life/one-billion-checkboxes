#ifndef UTIL_H
#define UTIL_H

#include "atcboxes/atcboxes.h"
#include <cstdio>

namespace atcboxes::util {

FILE *try_open(const char *filepath, const char *mode);

#ifdef WITH_COLOR
static void parse_cbox_wc(const std::string &msg, uint64_t &i, cbox_t &s) {}

static void cbox_t_to_str(uint64_t &i, cbox_t &s, std::string &str) {}
#endif // WITH_COLOR

} // namespace atcboxes::util

#endif // UTIL_H
