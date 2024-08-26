#include "atcboxes/atcboxes.h"
#include "atcboxes/test.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <threads.h>

namespace atcboxes::test {

// !TODO: color support
int run(CBOX_T *cboxes) {
  // size_t li = get_state_element_count() - 1;
  // cboxes[li] = 28765284;
  // assert(cboxes[li] ==
  //        0b0000000000000000000000000000000000000001101101101110110001100100);

  // printf("%zu\n", cboxes[li]);
  // save_state("state1");
  // reset_state();
  // printf("%zu\n", cboxes[li]);

  // load_state("state1");
  // printf("%zu\n", cboxes[li]);
  // assert(cboxes[li] ==
  //        0b0000000000000000000000000000000000000001101101101110110001100100);
  // reset_state();

  // auto start =
  //     std::chrono::high_resolution_clock::now().time_since_epoch().count();
  // switch_state(999999960);
  // switch_state(999999959);
  // switch_state(999999957);
  // switch_state(999999956);
  // switch_state(999999954);
  // switch_state(999999953);
  // switch_state(999999951);
  // switch_state(999999950);
  // switch_state(999999949);
  // switch_state(999999947);
  // switch_state(999999946);
  // printf("%zu\n", gv);
  // assert(gv == 11);
  // switch_state(999999942);
  // switch_state(999999941);
  // switch_state(999999938);
  // printf("%zu\n", gv);
  // assert(gv == 14);
  // assert(cboxes[li] ==
  //        0b0000000000000000000000000000000000000001101101101110110001100100);
  // switch_state(999999938);
  // printf("%zu\n", gv);
  // assert(gv == 13);
  // assert(cboxes[li] ==
  //        0b0000000000000000000000000000000000000001101101101110110001100000);
  // switch_state(999999938);
  // printf("%zu\n", gv);
  // assert(gv == 14);
  // auto end =
  //     std::chrono::high_resolution_clock::now().time_since_epoch().count();
  // printf("took %lld picosecond\n", end - start);

  // assert(cboxes[li] ==
  //        0b0000000000000000000000000000000000000001101101101110110001100100);

  // struct timespec t = {1, 0};
  // thrd_sleep(&t, NULL);

  return 0;
}

} // namespace atcboxes::test
