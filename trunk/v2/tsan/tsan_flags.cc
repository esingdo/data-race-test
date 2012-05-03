//===-- tsan_flags.cc -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//

#include "tsan_flags.h"
#include "tsan_rtl.h"
#include "tsan_mman.h"

namespace __tsan {

static const char *const empty_str = "";

static void Flag(const char *env, bool *flag, const char *name, bool def);
static void Flag(const char *env, int *flag, const char *name, int def);
static void Flag(const char *env, const char **flag, const char *name,
                 const char *def);

Flags *flags() {
  return &CTX()->flags;
}

void InitializeFlags(Flags *f, const char *env) {
  Flag(env, &f->enable_annotations, "enable_annotations", true);
  Flag(env, &f->suppress_equal_stacks, "suppress_equal_stacks", true);
  Flag(env, &f->suppress_equal_addresses, "suppress_equal_addresses", true);
  Flag(env, &f->report_thread_leaks, "report_thread_leaks", true);
  Flag(env, &f->report_signal_unsafe, "report_signal_unsafe", true);
  Flag(env, &f->force_seq_cst_atomics, "force_seq_cst_atomics", false);
  Flag(env, &f->strip_path_prefix, "strip_path_prefix", empty_str);
  Flag(env, &f->suppressions, "suppressions", empty_str);
  Flag(env, &f->exitcode, "exitcode", 66);
  Flag(env, &f->log_fileno, "log_fileno", 2);
  Flag(env, &f->atexit_sleep_ms, "atexit_sleep_ms", 1000);
  Flag(env, &f->verbosity, "verbosity", 0);
}

void FinalizeFlags(Flags *flags) {
  if (flags->strip_path_prefix != empty_str)
    internal_free((void*)flags->strip_path_prefix);
  if (flags->suppressions != empty_str)
    internal_free((void*)flags->suppressions);
}

static const char *GetFlagValue(const char *env, const char *name,
                                const char **end) {
  if (env == 0)
    return *end = 0;
  const char *pos = internal_strstr(env, name);
  if (pos == 0)
    return *end = 0;
  pos += internal_strlen(name);
  if (pos[0] != '=')
    return *end = pos;
  pos += 1;
  if (pos[0] == '"') {
    pos += 1;
    *end = internal_strchr(pos, '"');
  } else if (pos[0] == '\'') {
    pos += 1;
    *end = internal_strchr(pos, '\'');
  } else {
    *end = internal_strchr(pos, ' ');
  }
  if (*end == 0)
    *end = pos + internal_strlen(pos);
  return pos;
}

static void Flag(const char *env, bool *flag, const char *name, bool def) {
  *flag = def;
  const char *end = 0;
  const char *val = GetFlagValue(env, name, &end);
  if (val == 0)
    return;
  int len = end - val;
  if (len == 1 && val[0] == '0')
    *flag = false;
  else if (len == 1 && val[0] == '1')
    *flag = true;
}

static void Flag(const char *env, int *flag, const char *name, int def) {
  *flag = def;
  const char *end = 0;
  const char *val = GetFlagValue(env, name, &end);
  if (val == 0)
    return;
  bool minus = false;
  if (val != end && val[0] == '-') {
    minus = true;
    val += 1;
  }
  int v = 0;
  for (; val != end; val++) {
    if (val[0] < '0' || val[0] > '9')
      break;
    v = v * 10 + val[0] - '0';
  }
  if (minus)
    v = -v;
  *flag = v;
}

static void Flag(const char *env, const char **flag, const char *name,
                 const char *def) {
  *flag = def;
  const char *end = 0;
  const char *val = GetFlagValue(env, name, &end);
  if (val == 0)
    return;
  int len = end - val;
  char *f = (char*)internal_alloc(MBlockFlag, len + 1);
  internal_memcpy(f, val, len);
  f[len] = 0;
  *flag = f;
}

}  // namespace __tsan