/* Example program to demo the refl library.
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
  struct refl_type *mystruct = assembly.u.type;

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
