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

#include <assert.h>
#include <string.h>
#include <dwarf.h>

#include "reflP.h"

struct refl_module *
refl_module_addr (struct refl *refl, void *ptr)
{
  Dwarf_Addr addr = (Dwarf_Addr)ptr;
  Dwfl_Module *module = dwfl_addrmodule (refl->dwfl, addr);
  if (module == NULL)
    __refl_seterr (REFL_E_DWFL);

  return (struct refl_module *)module;
}

struct refl_module *
refl_module_cur (struct refl *refl)
{
  return refl_module_addr (refl, __builtin_return_address (0));
}

static int
build_type_assembly (Dwarf_Die *die, int tag,
		     struct refl_assembly *ret_assembly)
{
  struct refl_type *type = __refl_type_begin (die);
  if (type == NULL)
    return -1;

  ret_assembly->kind = refl_as_type;
  ret_assembly->u.type = type;
  return 1;
}

static int
build_method_assembly (Dwarf_Die *die, int tag,
		       struct refl_assembly *ret_assembly)
{
  struct refl_method *method = __refl_method_begin (die);
  if (method == NULL)
    return -1;

  ret_assembly->kind = refl_as_method;
  ret_assembly->u.method = method;
  return 1;
}

static int
build_object_assembly (Dwarf_Die *die, int tag,
		       struct refl_assembly *ret_assembly)
{
  Dwarf_Die type_die_mem, *type_die = __refl_die_type (die, &type_die_mem);
  if (type_die == NULL)
    return -1;

  Dwarf_Attribute loc_att_mem, *loc_att
    = __refl_attr_integrate (die, DW_AT_location, &loc_att_mem);
  if (loc_att == NULL)
    return -1;

  struct refl_type *type = __refl_type_begin (type_die);
  if (type == NULL)
    return -1;

  Dwarf_Op *ops;
  size_t nops;
  if (dwarf_getlocation (loc_att, &ops, &nops) < 0)
    {
      __refl_type_free (type);
      __refl_seterr (REFL_E_DWARF);
      return -1;
    }

  struct refl_object *ret = NULL;

  for (size_t i = 0; i < nops; ++i)
    switch (ops[i].atom)
      {
      case DW_OP_addr:
	assert (i == nops - 1);
	ret = __refl_object_begin (type, (void *)ops[i].number);
	if (ret == NULL)
	  {
	  fail:
	    __refl_type_free (type);
	    return -1;
	  }

      default:
	//fprintf (stderr, "unhandled location operator %#x\n", ops[i].atom);
	assert (ops[i].atom == DW_OP_addr);
      }

  if (ret == NULL)
    goto fail;

  ret_assembly->kind = refl_as_object;
  ret_assembly->u.object = ret;
  return 0;
}

int
refl_assembly_named (struct refl *refl, struct refl_module *reflmod,
		     char const *name, struct refl_assembly *ret_assembly)
{
  Dwfl_Module *module = (Dwfl_Module *)reflmod;
  Dwarf_Die die_mem, *die = &die_mem;
  int result = __refl_each_die (module, NULL, &die_mem, &__refl_die_attr_pred,
				&(struct __refl_die_attr_pred)
				{ DW_AT_name, __refl_attr_match_string,
				    (void *)name });
  if (result != 0)
    return result;

  int tag = dwarf_tag (die);
  switch (tag)
    {
    case DW_TAG_array_type:
    case DW_TAG_class_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_reference_type:
    case DW_TAG_pointer_type:
    case DW_TAG_string_type:
    case DW_TAG_structure_type:
    case DW_TAG_typedef:
    case DW_TAG_subrange_type:
    case DW_TAG_union_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_set_type:
    case DW_TAG_base_type:
    case DW_TAG_const_type:
    case DW_TAG_file_type:
    case DW_TAG_packed_type:
    case DW_TAG_volatile_type:
    case DW_TAG_restrict_type:
    case DW_TAG_interface_type:
    case DW_TAG_unspecified_type:
    case DW_TAG_mutable_type:
    case DW_TAG_rvalue_reference_type:
      return build_type_assembly (die, tag, ret_assembly);

    case DW_TAG_subprogram:
      return build_method_assembly (die, tag, ret_assembly);

    case DW_TAG_variable:
      return build_object_assembly (die, tag, ret_assembly);
    };

  __refl_seterr (REFL_ME_DWARF);
  return -1;
}

struct refl_type *
refl_assembly_get_type (struct refl_assembly *assembly)
{
  assert (assembly->kind == refl_as_type);
  return assembly->u.type;
}

struct refl_method *
refl_assembly_get_method (struct refl_assembly *assembly)
{
  assert (assembly->kind == refl_as_method);
  return assembly->u.method;
}

struct refl_object *
refl_assembly_get_object (struct refl_assembly *assembly)
{
  assert (assembly->kind == refl_as_object);
  return assembly->u.object;
}
