/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>

#include "pthread_accessor.h"

int pthread_join(pthread_t t, void ** ret_val) {
  if (t == pthread_self()) {
    return EDEADLK;
  }

  pthread_accessor thread;
  pthread_accessor_init(&thread, t);
  if (pthread_accessor_get(&thread) == NULL) {
      pthread_accessor_fini(&thread);
      return ESRCH;
  }

  if (pthread_accessor_get(&thread)->attr.flags & PTHREAD_ATTR_FLAG_DETACHED) {
    pthread_accessor_fini(&thread);
    return EINVAL;
  }

  // Wait for thread death when needed.

  // If the 'join_count' is negative, this is a 'zombie' thread that
  // is already dead and without stack/TLS. Otherwise, we need to increment 'join-count'
  // and wait to be signaled
  int count = pthread_accessor_get(&thread)->join_count;
  if (count >= 0) {
    pthread_accessor_get(&thread)->join_count += 1;
    pthread_cond_wait(&pthread_accessor_get(&thread)->join_cond, &gThreadListLock);
    count = --pthread_accessor_get(&thread)->join_count;
  }
  if (ret_val) {
    *ret_val = pthread_accessor_get(&thread)->return_value;
  }

  // Remove thread from thread list when we're the last joiner or when the
  // thread was already a zombie.
  if (count <= 0) {
    _pthread_internal_remove_locked(pthread_accessor_get(&thread));
  }
  pthread_accessor_fini(&thread);
  return 0;
}
