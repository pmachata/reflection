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
