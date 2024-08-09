#ifndef ATCBOXES_H
#define ATCBOXES_H

#include <cstdint>
namespace atcboxes {

// unused for now, i havent even start the frontend
struct cbox_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

uint64_t get_gv();

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @return 0 off, 1 on, -1 err
 */
int switch_state(uint64_t i);

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @return 0 off, 1 on, -1 err
 */
int get_state(uint64_t i);

int run(const int argc, const char *const argv[]);

} // namespace atcboxes

#endif // ATCBOXES_H
