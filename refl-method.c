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
#include <string.h>
#include <dwarf.h>
#include <ffi.h>
#include <assert.h>
#include <elfutils/libdw.h>

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
refl_method_at (struct refl *refl, void *ptr)
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
      __refl_seterr (REFL_E_DWARF);
      return NULL;
    }

  if (dwarf_child (die, &die_mem))
    {
      __refl_seterr (REFL_E_DWARF);
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
  return refl_method_at (refl, __builtin_return_address (0));
}

char const *
refl_method_name (struct refl_method *method)
{
  if (method->name == NULL)
    method->name = __refl_die_name (&method->die);
  return method->name;
}

static int
dw_base_type_to_ffi (Dwarf_Die *die, ffi_type **ret)
{
  assert (dwarf_tag (die) == DW_TAG_base_type);

  Dwarf_Attribute size_att_mem, *size_att
    = __refl_attr_integrate (die, DW_AT_byte_size, &size_att_mem);
  if (size_att == NULL)
    return -1;

  Dwarf_Attribute enc_att_mem, *enc_att
    = __refl_attr_integrate (die, DW_AT_encoding, &enc_att_mem);
  if (enc_att == NULL)
    return -1;

  Dwarf_Word size;
  Dwarf_Word encoding;
  if (dwarf_formudata (size_att, &size) < 0
      || dwarf_formudata (enc_att, &encoding) < 0)
    {
      __refl_seterr (REFL_E_DWARF);
      return -1;
    }

  bool sign;
  switch (encoding)
    {
    case DW_ATE_signed:
      sign = true;
      break;
    case DW_ATE_unsigned:
      sign = false;
      break;
    default:
      assert (!"unhandled encoding");
      abort ();
    }

  switch (size)
    {
    case 1:
      *ret = sign ? &ffi_type_sint8 : &ffi_type_uint8;
      return 0;
    case 2:
      *ret = sign ? &ffi_type_sint16 : &ffi_type_uint16;
      return 0;
    case 4:
      *ret = sign ? &ffi_type_sint32 : &ffi_type_uint32;
      return 0;
    case 8:
      *ret = sign ? &ffi_type_sint64 : &ffi_type_uint64;
      return 0;
    default:
      assert (!"unhandled size");
      abort ();
    };
}

static int
dw_type_to_ffi (Dwarf_Die *die, ffi_type **ret)
{
  switch (dwarf_tag (die))
    {
    case DW_TAG_base_type:
      return dw_base_type_to_ffi (die, ret);

    default:
      assert (!"unhandled type in refl/ffi translation");
      abort ();
    }
}

struct build_ffi_arglist
{
  size_t given;
  struct refl_object **args;
  void **ptrs;
  ffi_type **ffi_types;

  size_t found;
};

static enum refl_cb_status
build_ffi_arglist (Dwarf_Die *die, void *d)
{
  struct build_ffi_arglist *data = d;
  if (dwarf_tag (die) == DW_TAG_formal_parameter)
    {
      size_t i = data->found++;
      if (data->found > data->given)
	{
	  __refl_seterr_2 (REFL_E_REFL, REFL_ME_MISMATCH);
	  return refl_cb_fail;
	}

      Dwarf_Die type_die_mem, *type_die = __refl_die_type (die, &type_die_mem);
      if (type_die == NULL)
	return refl_cb_fail;

      ffi_type *ffit;
      if (dw_type_to_ffi (type_die, &ffit) < 0)
	return -1;

      data->ffi_types[i] = ffit;
      data->ptrs[i] = refl_object_cdata (data->args[i]);
    }

  return refl_cb_next;
}

int
refl_method_call (struct refl *refl, struct refl_method *method,
		  struct refl_object *args[], size_t nargs,
		  struct refl_object **ret)
{
  Dwarf_Attribute lowpc_att_mem, *lowpc_att
    = __refl_attr_integrate (&method->die, DW_AT_low_pc, &lowpc_att_mem);
  if (lowpc_att == NULL)
    return -1;

  Dwarf_Addr low_pc;
  if (dwarf_formaddr (lowpc_att, &low_pc) < 0)
    {
      __refl_seterr (REFL_E_DWARF);
      return -1;
    }

  void *ptrs[nargs];
  ffi_type *ffi_types[nargs];
  struct build_ffi_arglist data = { nargs, args, ptrs, ffi_types };
  if (__refl_die_children (&method->die, NULL, false,
			   build_ffi_arglist, &data) < 0)
    return -1;

  ffi_type *rettype;
  if (dwarf_hasattr (&method->die, DW_AT_type))
    {
      Dwarf_Die type_die_mem, *type_die
	= __refl_die_type (&method->die, &type_die_mem);
      if (type_die == NULL
	  || dw_type_to_ffi (type_die, &rettype) < 0)
	return -1;
    }
  else
    rettype = &ffi_type_void;

  ffi_cif cif;
  ffi_status stat
    = ffi_prep_cif (&cif, FFI_DEFAULT_ABI, nargs, rettype, ffi_types);
  /* We should be able to avoid making any errors that FFI would
     complain about.  */
  assert (stat == FFI_OK);

  int rc;   /* XXX -- need full fledged return object */
  ffi_call(&cif, (void (*)())low_pc, &rc, ptrs);

  return 0;
}
