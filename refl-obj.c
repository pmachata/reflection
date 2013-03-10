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

#include <string.h>
#include <dwarf.h>
#include <assert.h>
#include "reflP.h"

static void
init_object (struct refl_object *obj, struct refl_type *type, void *data)
{
  obj->data = data;
  obj->type = type;
}

struct refl_object *
__refl_object_begin (struct refl_type *type, void *data)
{
  struct refl_object *result = __refl_malloc (sizeof (*result));
  if (result == NULL)
    return NULL;

  init_object (result, type, data);
  return result;
}

struct refl_object *
__refl_object_begin_inline (struct refl_type *type, size_t size)
{
  struct refl_object *result = __refl_malloc (sizeof (*result) + size);
  if (result == NULL)
    return NULL;

  init_object (result, type, (unsigned char *)(result + 1));
  return result;
}

static void
checked_siblingof (Dwarf_Die **diep, Dwarf_Die *die_memp)
{
  switch (dwarf_siblingof (*diep, die_memp))
    {
    case -1:
      __refl_seterr (REFL_E_DWARF);
    case 1:
      *diep = NULL;
    case 0:
      break;
    }
}

struct refl_object *
refl_access (struct refl *refl, struct refl_object *obj, char const *lookfor)
{
  assert (refl != NULL);
  assert (obj != NULL);
  assert (lookfor != NULL);

  struct refl_type *type = obj->type;
  assert (type != NULL);

  Dwarf_Die die_mem, *die = &die_mem;
  if (dwarf_child (&type->die, &die_mem))
    {
    err:
      __refl_seterr (REFL_E_DWARF);
      return NULL;
    }

  for (; die != NULL; checked_siblingof (&die, &die_mem))
    {
      if (dwarf_tag (die) != DW_TAG_member)
	continue;

      Dwarf_Attribute attr_mem, *attr = dwarf_attr (die, DW_AT_name, &attr_mem);
      if (attr == NULL)
	goto err;

      char const *name = dwarf_formstring (attr);
      if (name == NULL)
	{
	err2:
	  __refl_seterr (REFL_E_DWARF);
	  return NULL;
	}

      if (strcmp (name, lookfor) != 0)
	continue;

      attr = dwarf_attr (die, DW_AT_data_member_location, &attr_mem);
      if (attr == NULL)
	goto err;

      Dwarf_Word location;
      if (dwarf_formudata (attr, &location) != 0)
	goto err2;

      attr = dwarf_attr (die, DW_AT_type, &attr_mem);
      if (attr == NULL)
	goto err;

      Dwarf_Die tdie_mem, *tdie = dwarf_formref_die (attr, &tdie_mem);
      if (tdie == NULL)
	goto err2;

      struct refl_type *field_type = __refl_type_begin (tdie);
      if (field_type == NULL)
	return NULL;

      struct refl_object *result
	= __refl_object_begin (field_type, (char *)obj->data + location);
      if (result == NULL)
	__refl_type_free (field_type);

      return result;
    }

  return NULL;
}

void *
refl_object_cdata (struct refl_object *obj)
{
  return obj->data;
}

struct refl_type *
refl_object_type (struct refl_object *obj)
{
  return obj->type;
}

void
refl_assign_int (struct refl_object *obj, int i)
{
  memcpy (refl_object_cdata (obj), &i, sizeof (i));
}
