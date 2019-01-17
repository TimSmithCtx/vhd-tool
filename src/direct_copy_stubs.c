/*
 * Copyright (C) 2012-2013 Citrix Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/signals.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/bigarray.h>

#ifdef __linux__
# include <linux/fs.h>
# include <sys/sendfile.h>
# include <malloc.h>
#endif

/* ocaml/ocaml/unixsupport.c */
extern void uerror(char *cmdname, value cmdarg);
#define Nothing ((value) 0)

#define NOT_IMPLEMENTED (-1)
#define TRIED_AND_FAILED (1)
#define OK 0

#define XFER_BUFSIZ (2*1024*1024)

CAMLprim value stub_init(value in_fd, value out_fd)
{
  CAMLparam2(in_fd, out_fd);
  int c_in_fd = Int_val(in_fd);
  int c_out_fd = Int_val(out_fd);
  void *handle = NULL;

  /* initialise handle */

  CAMLreturn((value) handle);
}

CAMLprim value stub_cleanup(value handle)
{
  CAMLparam1(handle);

  /* Cast handle to the thing you need and assign it to a local variable. */
  /* Then do the cleanup. */

  CAMLreturn(Val_unit);
}

CAMLprim value stub_direct_copy(value handle, value len){
  CAMLparam2(handle, len);
  CAMLlocal1(result);
  size_t c_len = Int64_val(len);
  size_t bytes;
  size_t remaining, chunk;

  /* TODO: Instead of this... */
  int c_in_fd = 0;
  int c_out_fd = 0;
  /* ...cast handle to the thing you need and assign it to a local variable. */

  /* We do not want to use a static buffer. This is a hack and is not
     thread-safe */
  static char *buffer = NULL;
  int flags;

  int rc = NOT_IMPLEMENTED;

  enter_blocking_section();

#ifdef __linux__
  rc = TRIED_AND_FAILED;
  bytes = 0;
  if (!buffer) {
    /* Let's have a 2MB chunk for copying */
    buffer = memalign(sysconf(_SC_PAGESIZE), XFER_BUFSIZ);
    if (!buffer) goto fail;
  }
  /* HACK!! Forcing O_DIRECT on for hackish testing */
  flags = fcntl(c_out_fd, F_GETFL, NULL);
  if (flags > 0 && !(flags & O_DIRECT))
    fcntl(c_out_fd, F_SETFL, flags | O_DIRECT);
  
  remaining = c_len;
  while (remaining > 0) {
    ssize_t bread;
    ssize_t bwritten = 0;

    bread = read(c_in_fd, buffer, (remaining < XFER_BUFSIZ)?remaining:XFER_BUFSIZ) ;
    if (bread < 0) goto fail;
    while (bwritten < bread) {
      ssize_t ret;

      ret = write(c_out_fd, buffer + bwritten, bread - bwritten);
      if (ret <= 0) goto fail;
      bytes += ret;
      bwritten += ret;
      remaining -= ret;
    }
  }
  rc = OK;
fail:
#endif

  leave_blocking_section();

  switch (rc) {
    case NOT_IMPLEMENTED:
      caml_failwith("This platform does not support sendfile()");
      break;
    case TRIED_AND_FAILED:
      uerror("sendfile", Nothing);
      break;
    default: break;
  }
  result = caml_copy_int64(bytes);
  CAMLreturn(result);
}
