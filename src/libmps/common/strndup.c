/*
 * This file is part of MPSolve 3.1.5
 *
 * Copyright (C) 2001-2014, Dipartimento di Matematica "L. Tonelli", Pisa.
 * License: http://www.gnu.org/licenses/gpl.html GPL version 3 or higher
 *
 * Authors:
 *   Leonardo Robol <robol@mail.dm.unipi.it>
 */

#include <mps/mps.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_STRNDUP

char *
mps_strndup (const char * source, size_t n)
{
  char *dest;
  size_t length = strlen (source);

  /* Lower n if it's greater than the length of the original
   * string that we should copy. */
  if (length < n)
    n = length;

  dest = mps_newv (char, n + 1);
  memcpy (dest, source, n);

  return dest;
}

#endif