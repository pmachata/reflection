#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "reflP.h"

static __thread struct refl_error global_error;

void
__refl_seterr_2 (int major, int minor)
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
      switch (error.minor)
	{
	case REFL_ME_DWARF:
	  return "invalid dwarf";
	}
    }

  abort ();
}
