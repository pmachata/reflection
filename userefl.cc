/* Example program to demo the refl library.
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

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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

static int
something (int c)
{
  fprintf (stderr, "simon says: %d\n", c);
  return c + 1;
}

int (*ddd)(int) = something;

int value = 111;
int *valuep = &value;
int **valuepp = &valuep;
int ***valueppp = &valuepp;

static void
d (struct mystruct *my)
{
  fprintf (stderr, "{%d, {%d}, %d}\n", my->i, my->o.j, my->k);
}

int
main (int argc, char *argv[])
{
  struct refl *refl = refl_begin ();
  if (refl == NULL)
    error (1, 0, "refl_begin: %s", refl_errmsg (refl_error ()));

  struct refl_module *mod = refl_module_cur (refl);
  if (mod == NULL)
    error (1, 0, "refl_module_cur: %s", refl_errmsg (refl_error ()));

  struct refl_type *mystruct = refl_type_named (refl, mod, "mystruct");

  {
    char *str;
    if (refl_type_dump (refl, mystruct, &str) < 0)
      error (1, 0, "refl_type_named: %s", refl_errmsg (refl_error ()));
    fprintf (stderr, "mystruct is: '%s'\n", str);
    free (str);
  }

  {
    struct refl_object *obj = refl_object_named (refl, mod, "ddd");
    if (obj != NULL)
      {
	struct refl_type *type = refl_object_type (obj);
	assert (type != NULL);

	char *str;
	if (refl_type_dump (refl, type, &str) >= 0)
	  {
	    fprintf (stderr, "type of ddd: '%s'\n", str);
	    free (str);
	  }

	size_t sz = refl_type_sizeof (refl, type);
	assert (sz == sizeof (void *));
	union
	{
	  void (*ptr)(int);
	  char buf[0];
	} u;

	memcpy (u.buf, refl_object_cdata (obj), sizeof u);
	fprintf (stderr, "ddd value: %p\ndirect call: ", u.ptr);
	u.ptr (17);
      }
  }

  {
    struct refl_object *obj = refl_new (refl, mystruct);
    if (obj == NULL)
      error (1, 0, "refl_new: %s", refl_errmsg (refl_error ()));

    struct refl_object *o = refl_access (refl, obj, "o");
    if (o == NULL)
      error (1, 0, "refl_access: %s", refl_errmsg (refl_error ()));
    struct refl_object *o_j = refl_access (refl, o, "j");
    if (o_j == NULL)
      error (1, 0, "refl_access: %s", refl_errmsg (refl_error ()));

    struct mystruct *my = (struct mystruct *)refl_object_cdata (obj);

    refl_assign_int (o_j, 7);
    d (my);

    refl_assign_int (o_j, 4);
    d (my);

    refl_assign_int (refl_access (refl, obj, "i"), 1);
    refl_assign_int (refl_access (refl, obj, "k"), 6);
    d (my);
  }

  {
    struct refl_method *st = refl_method_named (refl, mod, "something");
    assert (st != NULL);

    struct refl_object *ft = refl_new (refl,
				       refl_type_named (refl, mod, "int"));
    assert (ft != NULL);
    refl_assign_int (ft, 42);

    struct refl_object *rv = NULL;
    if (refl_method_call (refl, st, &ft, 1, &rv) < 0)
      error (1, 0, "refl_method_call: %s", refl_errmsg (refl_error ()));
    assert (rv != NULL);
    fprintf (stderr, "return value: %d\n", *(int *)refl_object_cdata (rv));
  }

  {
    struct refl_object *vppp = refl_object_named (refl, mod, "valueppp");
    while (1)
      {
	bool isptr;
	if (refl_type_is_pointer (refl_object_type (vppp), &isptr) < 0)
	  error (1, 0, "refl_type_is_pointer: %s", refl_errmsg (refl_error ()));
	if (!isptr)
	  break;
	fprintf (stderr, "deref\n");
	vppp = refl_deref (refl, vppp);
	if (vppp == NULL)
	  error (1, 0, "refl_deref: %s", refl_errmsg (refl_error ()));
      }
    fprintf (stderr, "v = %d\n", *(int *)refl_object_cdata (vppp));
  }

  refl_end (refl);
  return 0;
}
