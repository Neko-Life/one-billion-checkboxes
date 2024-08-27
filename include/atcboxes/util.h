#ifndef UTIL_H
#define UTIL_H

#include <cstdio>

namespace atcboxes::util {

FILE *try_open(const char *filepath, const char *mode);

} // namespace atcboxes::util

#endif // UTIL_H
