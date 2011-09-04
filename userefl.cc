#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "refl.h"

struct other
{
  int j;
};

struct mystruct
{
  int i;
  struct other o;
  int k;
};

int
main(int argc, char *argv[])
{
  struct refl *refl = refl_begin ();
  if (refl == NULL)
    error (1, 0, "refl_begin: %s", refl_errmsg (refl_error ()));

  struct refl_module *mod = refl_module_cur (refl);
  if (mod == NULL)
    error (1, 0, "refl_module_cur: %s", refl_errmsg (refl_error ()));

  struct refl_assembly assembly;
  if (refl_assembly_named (refl, mod, "mystruct", &assembly) <= 0)
    error (1, 0, "refl_assembly_named: %s", refl_errmsg (refl_error ()));

  assert (assembly.kind == refl_as_type);
  struct refl_type *mystruct = assembly.type;

  struct refl_object *obj = refl_new (refl, mystruct);
  if (obj == NULL)
    error (1, 0, "refl_new: %s", refl_errmsg (refl_error ()));

  struct refl_object *other = refl_access (refl, obj, "o");
  other = refl_access (refl, other, "j");

  struct mystruct *my = (struct mystruct *)refl_object_cdata (obj);

  refl_assign_int (other, 7);
  fprintf (stderr, "%d\n", my->o.j);

  refl_assign_int (other, 4);
  fprintf (stderr, "%d\n", my->o.j);

  refl_end (refl);
  return 0;
}
