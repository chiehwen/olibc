/* $OpenBSD: strerror_r.c,v 1.6 2005/08/08 08:05:37 espie Exp $ */
/* Public Domain <marc@snafu.org> */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "private/ErrnoRestorer.h"

typedef struct Pair Pair;
struct Pair {
  int code;
  const char* msg;
};

static const char* __code_string_lookup(const Pair* strings, int code) {
  size_t i;
  for (i = 0; strings[i].msg != NULL; ++i) {
    if (strings[i].code == code) {
      return strings[i].msg;
    }
  }
  return NULL;
}

static const Pair _sys_error_strings[] = {
#define  __BIONIC_ERRDEF(x,y,z)  { x, z },
#include <sys/_errdefs.h>
  { 0, NULL }
};

__LIBC_HIDDEN__ const char* __strerror_lookup(int error_number) {
  return __code_string_lookup(_sys_error_strings, error_number);
}

static const Pair _sys_signal_strings[] = {
#define  __BIONIC_SIGDEF(x,y,z)  { y, z },
#include <sys/_sigdefs.h>
  { 0, NULL }
};

__LIBC_HIDDEN__ const char* __strsignal_lookup(int signal_number) {
  return __code_string_lookup(_sys_signal_strings, signal_number);
}

int strerror_r(int error_number, char* buf, size_t buf_len) {
  ErrnoRestorer errno_restorer;
  ErrnoRestorer_init(&errno_restorer);
  size_t length;

  const char* error_name = __strerror_lookup(error_number);
  if (error_name != NULL) {
    length = snprintf(buf, buf_len, "%s", error_name);
  } else {
    length = snprintf(buf, buf_len, "Unknown error %d", error_number);
  }
  if (length >= buf_len) {
    ErrnoRestorer_override(&errno_restorer, ERANGE);
    ErrnoRestorer_fini(&errno_restorer);
    return -1;
  }

  ErrnoRestorer_fini(&errno_restorer);
  return 0;
}

__LIBC_HIDDEN__ const char* __strsignal(int signal_number, char* buf, size_t buf_len) {
  const char* signal_name = __strsignal_lookup(signal_number);
  if (signal_name != NULL) {
    return signal_name;
  }

  const char* prefix = "Unknown";
  if (signal_number >= SIGRTMIN && signal_number <= SIGRTMAX) {
    prefix = "Real-time";
    signal_number -= SIGRTMIN;
  }
  size_t length = snprintf(buf, buf_len, "%s signal %d", prefix, signal_number);
  if (length >= buf_len) {
    return NULL;
  }
  return buf;
}
