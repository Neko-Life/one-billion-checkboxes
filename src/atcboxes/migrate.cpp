#include "atcboxes/migrate.h"
#include "atcboxes/atcboxes.h"
#include "atcboxes/util.h"
#include <cstdint>
#include <sys/stat.h>

namespace atcboxes::migrate {

static void no_migration_needed() {
  fprintf(stderr, "State file is valid and compatible, no migration needed.\n");
}

static void print_state_file_type(const char *file, int type) {
  const char *t = "unknown";
  switch (type) {
  case -1:
    t = "CORRUPT";
    break;
  case 0:
    t = "BILLION_NO_COLOR";
    break;
  case 1:
    t = "BILLION_WITH_COLOR";
    break;
  case 2:
    t = "TRILLION_NO_COLOR";
    break;
  case 3:
    t = "TRILLION_WITH_COLOR";
    break;
  default:
    t = "UNKNOWN";
  }

  fprintf(stderr, "State file: %s\n", file);
  fprintf(stderr, "Format: %s\n", t);
}

static void print_migrate_target() {
#ifdef BUILD_BILLION_NO_COLOR
#define MIGRATE_TARGET "BILLION_NO_COLOR"
#endif // BUILD_BILLION_NO_COLOR

#ifdef BUILD_BILLION_WITH_COLOR
#define MIGRATE_TARGET "BILLION_WITH_COLOR"
#endif // BUILD_BILLION_WITH_COLOR

#ifdef BUILD_TRILLION_NO_COLOR
#define MIGRATE_TARGET "TRILLION_NO_COLOR"
#endif // BUILD_TRILLION_NO_COLOR

#ifdef BUILD_TRILLION_WITH_COLOR
#define MIGRATE_TARGET "TRILLION_WITH_COLOR"
#endif // BUILD_TRILLION_WITH_COLOR

  fprintf(stderr, "Migrating to " MIGRATE_TARGET "\n");
#undef MIGRATE_TARGET
}

int run(const std::string &file) {
  if (file.empty()) {
    fprintf(stderr, "No state file specified to migrate!\nExiting...\n");
    return -1;
  }

  // detect whether the state file has the size of 5 possible combination
  // 1. billion no color
  // 2. billion with color
  // 3. trillion no color
  // 4. trillion with color
  // 5. broken state, should reset everything

  // state file type
  int state_file_t = -1;

  size_t fsiz = 0;
  {
    FILE *f = util::try_open(file.c_str(), "rb");
    if (f == NULL)
      return -1;

    struct stat s;
    if (fstat(fileno(f), &s) != 0) {
      perror("fstat");
      fclose(f);
      return -1;
    }

    fsiz = s.st_size;

    fclose(f);
    f = NULL;
  }

  constexpr size_t BILLION_NO_COLOR_ELEMENT_COUNT =
      1'000'000'000 / (sizeof(uint64_t) * CHAR_BIT);
  constexpr size_t SIZE_BYTES_BILLION_NO_COLOR =
      BILLION_NO_COLOR_ELEMENT_COUNT * sizeof(uint64_t);

  switch (fsiz) {
  case SIZE_BYTES_BILLION_NO_COLOR:
    // 1. billion no color
#ifdef BUILD_BILLION_NO_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 0;
    break;
  case 1'000'000'000 * (sizeof(cbox_t)):
    // 2. billion with color
#ifdef BUILD_BILLION_WITH_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 1;
    break;
  case 1'000'000'000'000 / (sizeof(uint64_t) * CHAR_BIT) * sizeof(uint64_t):
    // 3. trillion no color
#ifdef BUILD_TRILLION_NO_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 2;
    break;
  case 1'000'000'000'000 * (sizeof(cbox_t)):
    // 4. trillion with color
#ifdef BUILD_TRILLION_WITH_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 3;
    break;
  default:
    // 5. broken state, should reset everything
    fprintf(stderr, "Unrecognized/corrupted state file\n");
  }

  print_state_file_type(file.c_str(), state_file_t);
  fprintf(stderr, "File size: %ld byte(s)\n\n", fsiz);
  print_migrate_target();

  ////////////////////

  static CBOX_T TEMP_VAL[4096] = {{}};

  switch (state_file_t) {
  case -1: {
    // just reset the state
    fprintf(stderr, "Resetting state file...\n");

    FILE *f = util::try_open(file.c_str(), "wb");
    if (f == NULL)
      return -1;

    size_t wrote = 0;
    size_t current_wrote = 0;
    while (wrote < STATE_ELEMENT_COUNT &&
           (current_wrote = fwrite(TEMP_VAL, STATE_ELEMENT_SIZE,
                                   (wrote + 4096) > STATE_ELEMENT_COUNT
                                       ? STATE_ELEMENT_COUNT - wrote
                                       : 4096,
                                   f)) > 0) {
      wrote += current_wrote;
    }

    fprintf(stderr, "Wrote %zu elements to `%s`\n", wrote, file.c_str());

    fclose(f);
    f = NULL;

    if (wrote != STATE_ELEMENT_COUNT) {
      fprintf(stderr, "Mismatched written element count, check your code!\n");
      return -1;
    }
    break;
  }
  case 0: {
#ifdef BUILD_BILLION_WITH_COLOR
    static uint64_t TEMP_IN[4096] = {0};

    const std::string writepath = file + ".migrated-bc";

    int status = -1;
    size_t readsiz = 0;
    size_t curread = 0;
    size_t wrote = 0;
    uint64_t temp_bit = 1;
    size_t tv_idx = 0;

    fprintf(stderr, "Opening new file for writing: %s\n", writepath.c_str());
    FILE *fout = util::try_open(writepath.c_str(), "wb");
    if (fout == NULL)
      return -1;

    FILE *fin = util::try_open(file.c_str(), "rb");
    if (fin == NULL)
      goto ferr0;

    // currently only handles BILLION_NO_COLOR to BILLION_WITH_COLOR only
    // !TODO: handle other cases
    while ((curread = fread(TEMP_IN, sizeof(uint64_t), 4096, fin)) > 0) {
      readsiz += curread;

      // put TEMP_IN into new format TEMP_VAL
      // all element
      for (size_t j = 0; j < curread; j++) {
        // convert all bit
        // all color zeroed
        do {
          if (tv_idx == 4096) {
            // write when it hit the end
            wrote += fwrite(TEMP_VAL, sizeof(CBOX_T), 4096, fout);

            tv_idx = 0;
          }
          TEMP_VAL[tv_idx++].a = TEMP_IN[j] & temp_bit ? 1 : 0;

          temp_bit <<= 1;
        } while (temp_bit);
        temp_bit = 1;
      }
    }

    // write if theres remaining unwritten element
    if (tv_idx > 0) {
      // write when it hit the end
      wrote += fwrite(TEMP_VAL, sizeof(CBOX_T), tv_idx, fout);

      tv_idx = 0;
    }

    fprintf(stderr, "Read %zu*%ld=%zu\n", readsiz, sizeof(uint64_t),
            readsiz * sizeof(uint64_t));
    fprintf(stderr, "Wrote %zu*%ld=%zu\n", wrote, sizeof(CBOX_T),
            wrote * sizeof(CBOX_T));

    // state file should have BILLION_NO_COLOR_ELEMENT_COUNT element
    if (readsiz != BILLION_NO_COLOR_ELEMENT_COUNT) {
      fprintf(stderr, "Mismatch read size from input file.\n");
      fprintf(stderr, "New state file is corrupted: %s\n", writepath.c_str());
      goto ferr1;
    }

    fprintf(stderr, "State file migrated to: %s\n", writepath.c_str());

    fclose(fin);
    fin = NULL;
    fclose(fout);
    fout = NULL;
    break;

  ferr1:
    if (fin) {
      fclose(fin);
      fin = NULL;
    }
  ferr0:
    if (fout) {
      fclose(fout);
      fout = NULL;
    }
    return status;
#endif // BUILD_BILLION_WITH_COLOR
  }
  case 1: {
    // !TODO
    // break;
  }
  case 2: {
    // !TODO
    // break;
  }
  case 3: {
    // !TODO
    fprintf(stderr,
            "State file recognized but migration is not implemented.\n");
    fprintf(stderr, "Exiting...\n");
    return -1;
    break;
  }
  default:
    fprintf(stderr, "Nothing to do, exiting...\n");
  }

end:
  return 0;
}
} // namespace atcboxes::migrate
