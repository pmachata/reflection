/*
 * Copyright (C) 2011, 2013 Petr Machata <pmachata@redhat.com>
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

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "reflP.h"

static __thread struct refl_error global_error;

void
__refl_seterr_2 (enum __refl_major_errcode major, int minor)
{
  global_error.major = major;
  global_error.minor = minor;
}

void
__refl_seterr (enum __refl_major_errcode major)
{
  switch (major)
    {
    case REFL_E_NOERROR:
      __refl_seterr_2 (major, 0);
      return;

    case REFL_E_SYSTEM:
      __refl_seterr_2 (major, errno);
      return;

    case REFL_E_DWFL:
      __refl_seterr_2 (major, dwfl_errno ());
      return;

    case REFL_E_DWARF:
      __refl_seterr_2 (major, dwarf_errno ());
      return;

    case REFL_E_REFL:
      /* Refl-owned error codes are set manually as well.  */
      assert (major != REFL_E_REFL);
      abort ();
    };

  assert (!"unhandled major error code!");
}

void
__refl_error (enum __refl_minor_errcode minor)
{
  __refl_seterr_2 (REFL_E_REFL, minor);
}

struct refl_error
refl_error (void)
{
  struct refl_error result = global_error;
  memset (&global_error, 0, sizeof (global_error));
  return result;
}

char const *
refl_errmsg (struct refl_error error)
{
  fprintf (stderr, "%d %d\n", error.major, error.minor);
  switch (error.major)
    {
    case REFL_E_NOERROR:
      return "no error";
    case REFL_E_SYSTEM:
      return strerror (error.minor);
    case REFL_E_DWFL:
      return dwfl_errmsg (error.minor);
    case REFL_E_DWARF:
      return dwarf_errmsg (error.minor);
    case REFL_E_REFL:
      switch ((enum __refl_minor_errcode)error.minor)
	{
	case REFL_ME_DWARF:
	  return "invalid dwarf";
	case REFL_ME_MISMATCH:
	  return "mismatch in assembly kind";
	case REFL_ME_NOT_FOUND:
	  return "entity not found";
	}
    }

  abort ();
}
