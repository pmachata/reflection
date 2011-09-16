/*
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

#include <stdlib.h>
#include <string.h>
#include <dwarf.h>
#include "reflP.h"

struct refl_method *
__refl_method_begin (Dwarf_Die *die)
{
  struct refl_method *result = malloc (sizeof (*result));
  if (result == NULL)
    {
      __refl_seterr (REFL_E_SYSTEM);
      return NULL;
    }
  result->die = *die;
  return result;
}

struct refl_method *
refl_method_addr (struct refl *refl, void *ptr)
{
  Dwarf_Addr addr = (Dwarf_Addr)ptr;

  Dwarf_Addr bias;
  Dwarf *dwarf = dwfl_addrdwarf (refl->dwfl, addr, &bias);
  if (dwarf == NULL)
    {
      __refl_seterr (REFL_E_DWFL);
      return NULL;
    }

  Dwarf_Die die_mem, *die = dwarf_addrdie (dwarf, addr, &die_mem);
  if (die == NULL)
    {
      __refl_seterr (REFL_E_DWARF);
      return NULL;
    }

  int tag = dwarf_tag (die);
  if (tag != DW_TAG_compile_unit)
    {
      __refl_error (REFL_ME_DWARF);
      return NULL;
    }

  if (dwarf_child (die, &die_mem))
    {
      __refl_error (REFL_ME_DWARF);
      return NULL;
    }

  do
    {
      tag = dwarf_tag (die);
      if (tag == DW_TAG_subprogram)
	{
	  Dwarf_Addr low_pc, high_pc;
	  if (dwarf_lowpc (die, &low_pc) != 0
	      || dwarf_highpc (die, &high_pc) != 0)
	    continue;

	  if (low_pc <= addr && addr < high_pc)
	    return __refl_method_begin (die);
	}
    }
  while (dwarf_siblingof (die, &die_mem) == 0);

  return NULL;
}

struct refl_method *
refl_method_cur (struct refl *refl)
{
  return refl_method_addr (refl, __builtin_return_address (0));
}

char const *
refl_method_name (struct refl_method *method)
{
  if (method->name != NULL)
    return method->name;

  Dwarf_Attribute att_mem, *att
    = dwarf_attr_integrate (&method->die, DW_AT_name, &att_mem);

  char const *result = NULL;
  if (att != NULL)
    result = dwarf_formstring (att);
  if (result == NULL)
    result = "???";

  method->name = result;
  return result;
}
