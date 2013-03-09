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

#include <stdlib.h>
#include <dwarf.h>
#include "reflP.h"

struct refl_type *
__refl_type_begin (Dwarf_Die *die)
{
  struct refl_type *result = malloc (sizeof (*result));
  if (result == NULL)
    {
      __refl_seterr (REFL_E_SYSTEM);
      return NULL;
    }
  result->die = *die;
  result->size = (size_t)-1;
  return result;
}

void
__refl_type_free (struct refl_type *type)
{
  free (type);
}

size_t
refl_type_sizeof (struct refl *refl, struct refl_type *type)
{
  if (type->size != (size_t)-1)
    return type->size;

  Dwarf_Attribute attr_mem, *attr
    = __refl_attr_integrate (&type->die, DW_AT_byte_size, &attr_mem);
  if (attr == NULL)
    return (size_t)-1;

  Dwarf_Word size;
  if (dwarf_formudata (attr, &size) != 0)
    {
      __refl_seterr (REFL_E_DWARF);
      return (size_t)-1;
    }

  type->size = size;

  return size;
}

struct refl_object *
refl_new (struct refl *refl, struct refl_type *type)
{
  size_t size = refl_type_sizeof (refl, type);
  if (size == (size_t)-1)
    return NULL;

  return __refl_object_begin_inline (type, size);
}
