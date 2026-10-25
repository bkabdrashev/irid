// Compiler profiling: wall-clock time and arena memory per pipeline stage.
//
// The profiler is enabled by default and writes a table to stderr after
// irid_run_path() finishes. Set IRID_STATS=0 to disable it.
//
// Memory numbers are arena-only. "allocated" is the number of bytes the
// compiler asked the arenas for. "used" is the number of bytes actually
// occupied in all live arenas (including the arena guard/redzone space),
// so it can be larger than "allocated".

#include <stdint.h>
#include <time.h>

#define PROFILE_MAX_STAGES 32

typedef struct {
  const char* name;
  uint64_t    lines;
  uint64_t    time_start_ns;
  uint64_t    time_ns;
  uint64_t    alloc_bytes;
  uint64_t    alloc_count;
  uint64_t    live_start;
  uint64_t    live_end;
  uint64_t    live_peak;
} Profile_Stage;

static Profile_Stage profile_stages[PROFILE_MAX_STAGES];
static int32_t       profile_stage_count;
static int32_t       profile_current = -1;
static int32_t       profile_active;

static uint64_t profile_live_bytes;
static uint64_t profile_peak_live_bytes;
static uint64_t profile_run_start_ns;
static uint64_t profile_run_end_ns;
static uint64_t profile_end_live_bytes;
static uint64_t profile_default_lines;

static const char* profile_source_path;
static const char* profile_builtin_path;
static uint64_t    profile_source_lines;
static uint64_t    profile_builtin_lines;

static int profile_is_enabled(void) {
  static int enabled = -1;
  if (enabled < 0) {
    const char* env = getenv("IRID_STATS");
    enabled = 1;
    if (env && (strcmp(env, "0") == 0 || strcmp(env, "false") == 0 || strcmp(env, "off") == 0)) {
      enabled = 0;
    }
  }
  return enabled != 0;
}

static uint64_t profile_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void profile_run_begin(void) {
  memset(profile_stages, 0, sizeof(profile_stages));
  profile_stage_count     = 0;
  profile_current         = -1;
  profile_active          = profile_is_enabled();
  profile_live_bytes      = 0;
  profile_peak_live_bytes = 0;
  profile_run_start_ns    = profile_now_ns();
  profile_run_end_ns      = 0;
  profile_end_live_bytes  = 0;
  profile_default_lines   = 0;
  profile_source_path     = NULL;
  profile_builtin_path    = NULL;
  profile_source_lines    = 0;
  profile_builtin_lines   = 0;
}

void profile_set_input(const char* source_path, uint64_t source_lines,
                       const char* builtin_path, uint64_t builtin_lines) {
  profile_source_path   = source_path;
  profile_source_lines  = source_lines;
  profile_builtin_path  = builtin_path;
  profile_builtin_lines = builtin_lines;
  profile_default_lines = source_lines + builtin_lines;
}

static void profile_stage_close(int32_t index) {
  Profile_Stage* stage = &profile_stages[index];
  stage->time_ns  = profile_now_ns() - stage->time_start_ns;
  stage->live_end = profile_live_bytes;
  if (profile_live_bytes > stage->live_peak) {
    stage->live_peak = profile_live_bytes;
  }
}

void profile_stage_begin(const char* name, uint64_t lines) {
  if (!profile_active) return;
  if (profile_current >= 0) {
    profile_stage_close(profile_current);
    profile_current = -1;
  }
  if (profile_stage_count >= PROFILE_MAX_STAGES) return;

  Profile_Stage* stage = &profile_stages[profile_stage_count++];
  memset(stage, 0, sizeof(*stage));
  stage->name          = name;
  stage->lines         = lines;
  stage->time_start_ns = profile_now_ns();
  stage->live_start    = profile_live_bytes;
  stage->live_peak     = profile_live_bytes;
  profile_current      = profile_stage_count - 1;
}

void profile_stage_end(void) {
  if (!profile_active || profile_current < 0) return;
  profile_stage_close(profile_current);
  profile_current = -1;
}

void profile_run_end(void) {
  if (!profile_active) return;
  profile_stage_end();
  profile_end_live_bytes = profile_live_bytes;
  profile_run_end_ns     = profile_now_ns();
  profile_active         = 0;
}

void profile_arena_push(uint64_t requested_bytes, uint64_t arena_bytes) {
  if (!profile_active) return;
  profile_live_bytes += arena_bytes;
  if (profile_live_bytes > profile_peak_live_bytes) {
    profile_peak_live_bytes = profile_live_bytes;
  }
  if (profile_current < 0) return;
  Profile_Stage* stage = &profile_stages[profile_current];
  stage->alloc_bytes += requested_bytes;
  stage->alloc_count += 1;
  if (profile_live_bytes > stage->live_peak) {
    stage->live_peak = profile_live_bytes;
  }
}

void profile_arena_extend(int64_t arena_delta, uint64_t requested_bytes) {
  if (!profile_active) return;
  if (arena_delta > 0) {
    profile_live_bytes += (uint64_t)arena_delta;
    if (profile_live_bytes > profile_peak_live_bytes) {
      profile_peak_live_bytes = profile_live_bytes;
    }
  }
  else if (arena_delta < 0) {
    uint64_t released = (uint64_t)(-arena_delta);
    profile_live_bytes = profile_live_bytes >= released ? profile_live_bytes - released : 0;
  }
  if (profile_current < 0) return;
  Profile_Stage* stage = &profile_stages[profile_current];
  stage->alloc_bytes += requested_bytes;
  stage->alloc_count += 1;
  if (profile_live_bytes > stage->live_peak) {
    stage->live_peak = profile_live_bytes;
  }
}

void profile_arena_release(uint64_t bytes) {
  if (!profile_active) return;
  profile_live_bytes = profile_live_bytes >= bytes ? profile_live_bytes - bytes : 0;
}

static const char* profile_fmt_buffer(void) {
  static char buffers[16][32];
  static int  index;
  const char* buffer = buffers[index % 16];
  index += 1;
  return buffer;
}

static const char* profile_fmt_time(uint64_t ns) {
  char* buffer = (char*)profile_fmt_buffer();
  if (ns < 1000) {
    snprintf(buffer, 32, "%llu ns", (unsigned long long)ns);
  }
  else if (ns < 1000000) {
    snprintf(buffer, 32, "%.2f us", (double)ns / 1e3);
  }
  else if (ns < 1000000000ULL) {
    snprintf(buffer, 32, "%.2f ms", (double)ns / 1e6);
  }
  else {
    snprintf(buffer, 32, "%.3f s", (double)ns / 1e9);
  }
  return buffer;
}

static const char* profile_fmt_bytes(uint64_t bytes) {
  char* buffer = (char*)profile_fmt_buffer();
  if (bytes < 1024) {
    snprintf(buffer, 32, "%llu B", (unsigned long long)bytes);
  }
  else if (bytes < 1024ULL * 1024) {
    snprintf(buffer, 32, "%.1f KiB", (double)bytes / 1024.0);
  }
  else if (bytes < 1024ULL * 1024 * 1024) {
    snprintf(buffer, 32, "%.2f MiB", (double)bytes / (1024.0 * 1024.0));
  }
  else {
    snprintf(buffer, 32, "%.2f GiB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
  }
  return buffer;
}

static const char* profile_fmt_growth(int64_t bytes) {
  if (bytes >= 0) {
    return profile_fmt_bytes((uint64_t)bytes);
  }
  char* buffer = (char*)profile_fmt_buffer();
  snprintf(buffer, 32, "-%s", profile_fmt_bytes((uint64_t)(-bytes)));
  return buffer;
}

static const char* profile_fmt_per_line(uint64_t value, uint64_t lines) {
  if (lines == 0) return "-";
  return profile_fmt_bytes(value / lines);
}

static const char* profile_fmt_lines_per_time(uint64_t lines, uint64_t ns) {
  char* buffer = (char*)profile_fmt_buffer();
  if (lines == 0 || ns == 0) {
    snprintf(buffer, 32, "-");
    return buffer;
  }
  double lines_per_second = (double)lines * 1e9 / (double)ns;
  if (lines_per_second >= 1e9) {
    snprintf(buffer, 32, "%.2f G/s", lines_per_second / 1e9);
  }
  else if (lines_per_second >= 1e6) {
    snprintf(buffer, 32, "%.2f M/s", lines_per_second / 1e6);
  }
  else if (lines_per_second >= 1e3) {
    snprintf(buffer, 32, "%.1f K/s", lines_per_second / 1e3);
  }
  else {
    snprintf(buffer, 32, "%.0f /s", lines_per_second);
  }
  return buffer;
}

void profile_print(FILE* out) {
  if (!profile_is_enabled() || profile_stage_count == 0) return;

  uint64_t total_time   = profile_run_end_ns - profile_run_start_ns;
  uint64_t total_alloc  = 0;
  uint64_t total_allocs = 0;
  for (int32_t i = 0; i < profile_stage_count; i++) {
    total_alloc  += profile_stages[i].alloc_bytes;
    total_allocs += profile_stages[i].alloc_count;
  }

  fprintf(out, "\nIrid compiler statistics\n");
  if (profile_source_path) {
    fprintf(out, "  source : %s (%llu lines)\n",
            profile_source_path, (unsigned long long)profile_source_lines);
  }
  if (profile_builtin_path) {
    fprintf(out, "  builtin: %s (%llu lines)\n",
            profile_builtin_path, (unsigned long long)profile_builtin_lines);
  }
  fprintf(out, "  total  : %llu lines\n", (unsigned long long)profile_default_lines);
  fprintf(out, "  memory : arena only; allocated = requested bytes,\n");
  fprintf(out, "           used = live arena bytes incl. guard space; LLVM excluded\n\n");

  fprintf(out, "%-20s %9s %6s %10s %7s %10s %10s %10s %10s %11s %10s\n",
          "stage", "time", "share", "allocated", "allocs",
          "peak used", "retained", "growth", "lines/s", "alloc/line", "used/line");
  fprintf(out, "%-20s %9s %6s %10s %7s %10s %10s %10s %10s %11s %10s\n",
          "--------------------", "---------", "------", "----------", "-------",
          "----------", "----------", "----------", "----------", "-----------", "----------");

  for (int32_t i = 0; i < profile_stage_count; i++) {
    Profile_Stage* stage = &profile_stages[i];
    uint64_t lines = stage->lines ? stage->lines : profile_default_lines;
    double share = total_time ? 100.0 * (double)stage->time_ns / (double)total_time : 0.0;
    int64_t growth = (int64_t)stage->live_end - (int64_t)stage->live_start;
    fprintf(out, "%-20s %9s %5.1f%% %10s %7llu %10s %10s %10s %10s %11s %10s\n",
            stage->name,
            profile_fmt_time(stage->time_ns),
            share,
            profile_fmt_bytes(stage->alloc_bytes),
            (unsigned long long)stage->alloc_count,
            profile_fmt_bytes(stage->live_peak),
            profile_fmt_bytes(stage->live_end),
            profile_fmt_growth(growth),
            profile_fmt_lines_per_time(lines, stage->time_ns),
            profile_fmt_per_line(stage->alloc_bytes, lines),
            profile_fmt_per_line(stage->live_peak, lines));
  }

  fprintf(out, "%-20s %9s %6s %10s %7s %10s %10s %10s %10s %11s %10s\n",
          "--------------------", "---------", "------", "----------", "-------",
          "----------", "----------", "----------", "----------", "-----------", "----------");
  {
    uint64_t lines = profile_default_lines;
    int64_t growth = (int64_t)profile_end_live_bytes;
    fprintf(out, "%-20s %9s %5.1f%% %10s %7llu %10s %10s %10s %10s %11s %10s\n",
            "total",
            profile_fmt_time(total_time),
            100.0,
            profile_fmt_bytes(total_alloc),
            (unsigned long long)total_allocs,
            profile_fmt_bytes(profile_peak_live_bytes),
            profile_fmt_bytes(profile_end_live_bytes),
            profile_fmt_growth(growth),
            profile_fmt_lines_per_time(lines, total_time),
            profile_fmt_per_line(total_alloc, lines),
            profile_fmt_per_line(profile_peak_live_bytes, lines));
  }

  if (profile_source_lines) {
    fprintf(out, "\nper input line (%llu):  %s allocated   %s peak used   throughput %s\n",
            (unsigned long long)profile_default_lines,
            profile_fmt_per_line(total_alloc, profile_default_lines),
            profile_fmt_per_line(profile_peak_live_bytes, profile_default_lines),
            profile_fmt_lines_per_time(profile_default_lines, total_time));
    fprintf(out, "per source line (%llu): %s allocated   %s peak used   throughput %s\n",
            (unsigned long long)profile_source_lines,
            profile_fmt_per_line(total_alloc, profile_source_lines),
            profile_fmt_per_line(profile_peak_live_bytes, profile_source_lines),
            profile_fmt_lines_per_time(profile_source_lines, total_time));
  }
}
