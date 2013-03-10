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

int
refl_assembly_named (struct refl *refl, struct refl_module *reflmod,
		     char const *name, struct refl_assembly *ret_assembly)
{
  Dwfl_Module *module = (Dwfl_Module *)reflmod;
  Dwarf_Addr bias;
  Dwarf *dwarf = dwfl_module_getdwarf (module, &bias);
  if (dwarf == NULL)
    {
      __refl_seterr (REFL_E_DWFL);
      return -1;
    }

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
    };

  __refl_seterr (REFL_ME_DWARF);
  return -1;
}
