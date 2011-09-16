/*
 * Copyright (C) 2011 Petr Machata <pmachata@redhat.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "reflP.h"

static Dwfl_Callbacks callbacks = {
  .find_elf = dwfl_linux_proc_find_elf,
  .find_debuginfo = dwfl_standard_find_debuginfo,
};

void *
__refl_malloc (size_t size)
{
  void *result = malloc (size);
  if (result == NULL)
    __refl_seterr (REFL_E_SYSTEM);

  return result;
}

struct refl *
refl_begin (void)
{
  struct refl *result = malloc (sizeof (*result));
  if (result == NULL)
    {
      __refl_seterr (REFL_E_SYSTEM);
      return NULL;
    }

  Dwfl *dwfl = dwfl_begin (&callbacks);
  if (dwfl == NULL)
    {
      __refl_seterr (REFL_E_DWFL);
      goto err_out;
    }

  dwfl_report_begin (dwfl);
  int status = dwfl_linux_proc_report (dwfl, getpid ());
  dwfl_report_end (dwfl, NULL, NULL);

  if (status < 0)
    {
      __refl_seterr (REFL_E_DWFL);
      goto err_out;
    }

  result->dwfl = dwfl;

  return result;

 err_out:
  free (result);
  return NULL;
}

void
refl_end (struct refl *refl)
{
  if (refl != NULL)
    {
      dwfl_end (refl->dwfl);
    }
  free (refl);
}
