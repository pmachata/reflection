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

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <dwarf.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

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

static int type_dump (Dwarf_Die *die, FILE *stream);

struct field_dump
{
  FILE *stream;
  int tag;
  bool seek_name;
  char delim;
  bool term_delim;

  int seen;
};

static enum refl_cb_status
field_dump (Dwarf_Die *die, void *d)
{
  struct field_dump *data = d;
  if (dwarf_tag (die) == data->tag)
    {
      Dwarf_Die type_die_mem, *type_die = __refl_die_type (die, &type_die_mem);
      if (type_die == NULL)
	return refl_cb_fail;

      const char *name = data->seek_name ? __refl_die_name (die) : NULL;
      char delim_s[] = { data->delim, 0 };
      int o = 0;
      if ((o = fprintf (data->stream, "%s%s",
			&delim_s[data->term_delim || !data->seen],
			&" "[!data->seen])) < 0
	  || type_dump (type_die, data->stream) < 0
	  || (o = fprintf (data->stream, "%s%s%s",
			   name == NULL ? "" : " ",
			   name == NULL ? "" : name,
			   &delim_s[!data->term_delim])) < 0)
	{
	  if (o < 0)
	    __refl_seterr (REFL_E_SYSTEM);
	  return refl_cb_fail;
	}
      data->seen = 1;
    }

  return refl_cb_next;
}

static int
type_dump (Dwarf_Die *die, FILE *stream)
{
  Dwarf_Die type_die_mem, *type_die = NULL;
  if (dwarf_hasattr (die, DW_AT_type))
    {
      type_die = __refl_die_type (die, &type_die_mem);
      if (type_die == NULL)
	return -1;
    }

  const char *name = NULL;
  if (dwarf_hasattr (die, DW_AT_name))
    {
      name = __refl_die_name (die);
      if (name == NULL)
	return -1;
    }

  int o = 0;
  switch (dwarf_tag (die))
    {
    case DW_TAG_base_type:
      if (name == NULL)
	{
	  __refl_seterr_2 (REFL_E_REFL, REFL_ME_DWARF);
	  return -1;
	}
      if (fprintf (stream, "%s", name) < 0)
	{
	  __refl_seterr (REFL_E_SYSTEM);
	  return -1;
	}
      return 0;

    case DW_TAG_structure_type:
      {
	struct field_dump data = { stream, DW_TAG_member, true, ';', true };
	if ((o = fprintf (stream,
			  name == NULL ? "struct {" : "struct %s {", name)) < 0
	    || __refl_die_children (die, NULL, true, field_dump, &data) < 0
	    || (o = fprintf (stream, "}")) < 0)
	  {
	    if (o < 0)
	      __refl_seterr (REFL_E_SYSTEM);
	    return -1;
	  }
	return 0;
      }

    case DW_TAG_pointer_type:
      if (type_die == NULL)
	{
	  __refl_seterr_2 (REFL_E_REFL, REFL_ME_DWARF);
	  return -1;
	}
      if (type_dump (type_die, stream) < 0
	  || (o = fprintf (stream, "*")) < 0)
	{
	  if (o < 0)
	    __refl_seterr (REFL_E_SYSTEM);
	  return -1;
	}
      return 0;

    case DW_TAG_subroutine_type:
      {
	struct field_dump data
	  = { stream, DW_TAG_formal_parameter, false, ',', false };
	if ((type_die == NULL && (o = fprintf (stream, "void ")) < 0)
	    || (type_die != NULL
		&& (type_dump (type_die, stream)
		    || (o = fprintf (stream, " ")) < 0))
	    || (o = fprintf (stream, "(")) < 0
	    || __refl_die_children (die, NULL, true, field_dump, &data) < 0
	    || (o = fprintf (stream, ")")) < 0)
	  {
	    if (o < 0)
	      __refl_seterr (REFL_E_SYSTEM);
	    return -1;
	  }
	return 0;
      }

    case DW_TAG_const_type:
      if (type_die == NULL)
	{
	  __refl_seterr_2 (REFL_E_REFL, REFL_ME_DWARF);
	  return -1;
	}
      if (type_dump (type_die, stream) < 0)
	return -1;
      if (fprintf (stream, " const") < 0)
	{
	  __refl_seterr (REFL_E_SYSTEM);
	  return -1;
	}
      return 0;
    }

  fprintf (stderr, "===%#x %lx===\n", dwarf_tag (die),
	   dwarf_dieoffset (die));
  assert (!"unhandled type tag");
  abort ();
}

int
refl_type_dump (struct refl *refl, struct refl_type *type, char **bufp)
{
  size_t size;
  FILE *stream = open_memstream(bufp, &size);
  if (stream == NULL)
    {
      __refl_seterr (REFL_E_SYSTEM);
      return -1;
    }

  int ret = type_dump (&type->die, stream);
  if (fclose (stream) < 0)
    return -1;
  return ret;
}

int
refl_type_is_pointer (struct refl_type *type, bool *whetherp)
{
  Dwarf_Die die = type->die;
  if (__refl_die_strip_cvq (&die, &die) == NULL)
    return -1;

  *whetherp = dwarf_tag (&type->die) == DW_TAG_pointer_type;
  return 0;
}

static int
find_named_assembly (struct refl *refl, struct refl_module *mod,
		     char const *name, enum refl_assembly_kind kind,
		     struct refl_assembly *assembly)
{
  if (refl_assembly_named (refl, mod, name, assembly) < 0)
    {
      __refl_seterr_2 (REFL_E_REFL, REFL_ME_NOT_FOUND);
      return -1;
    }

  if (assembly->kind != kind)
    {
      __refl_seterr_2 (REFL_E_REFL, REFL_ME_MISMATCH);
      return -1;
    }

  return 0;
}

struct refl_type *
refl_type_named (struct refl *refl, struct refl_module *mod, char const *name)
{
  struct refl_assembly assembly;
  if (find_named_assembly (refl, mod, name, refl_as_type, &assembly) < 0)
    return NULL;
  return refl_assembly_get_type (&assembly);
}

struct refl_method *
refl_method_named (struct refl *refl, struct refl_module *mod,
		   char const *name)
{
  struct refl_assembly assembly;
  if (find_named_assembly (refl, mod, name, refl_as_method, &assembly) < 0)
    return NULL;
  return refl_assembly_get_method (&assembly);
}

struct refl_object *
refl_object_named (struct refl *refl, struct refl_module *mod,
		   char const *name)
{
  struct refl_assembly assembly;
  if (find_named_assembly (refl, mod, name, refl_as_object, &assembly) < 0)
    return NULL;
  return refl_assembly_get_object (&assembly);
}
