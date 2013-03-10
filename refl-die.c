/*
 * Copyright (C) 2013 Petr Machata <pmachata@redhat.com>
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

int
__refl_die_tree (Dwarf_Die *root, Dwarf_Die *ret, bool recurse,
		 enum refl_cb_status (*callback) (Dwarf_Die *die,
						  void *data),
		 void *data)
{
  switch (callback (root, data))
    {
    case refl_cb_stop:
      if (ret != NULL)
	*ret = *root;
      return 0;
    case refl_cb_fail:
      return -1;
    case refl_cb_next:
      ;
    };

  if (recurse)
    return __refl_die_children (root, ret, recurse, callback, data);

  return 1;
}

int
__refl_die_children (Dwarf_Die *root, Dwarf_Die *ret, bool recurse,
		     enum refl_cb_status (*callback) (Dwarf_Die *die,
						      void *data),
		     void *data)
{
  if (!dwarf_haschildren (root))
    return 1;

  Dwarf_Die die_mem;
  if (dwarf_child (root, &die_mem))
    {
      __refl_seterr (REFL_E_DWARF);
      return -1;
    }

  while (1)
    {
      int result = __refl_die_tree (&die_mem, ret, recurse,
				    callback, data);
      if (result <= 0)
	return result;

      switch (dwarf_siblingof (&die_mem, &die_mem))
	{
	case 0: continue;
	case -1:
	  {
	    __refl_seterr (REFL_E_DWARF);
	    return -1;
	  }
	}

      break;
    }

  return 1;
}

int
__refl_each_die (Dwfl_Module *module, Dwarf_Die *root, Dwarf_Die *ret,
		 enum refl_cb_status (*callback) (Dwarf_Die *die,
						  void *data),
		 void *data)
{
  Dwarf_Addr bias;
  if (root == NULL)
    root = dwfl_module_nextcu (module, NULL, &bias);

  uint8_t address_size;
  uint8_t offset_size;
  Dwarf_Die cudie_mem, *cudie
    = dwarf_diecu (root, &cudie_mem, &address_size, &offset_size);

  if (cudie == NULL)
    {
      __refl_seterr (REFL_E_DWARF);
      return -1;
    }

  while (root != NULL)
    {
      Dwarf_Die found;
      int result = __refl_die_tree (root, &found, true, callback, data);
      if (result == 0 && ret != NULL)
	*ret = found;
      if (result <= 0)
	return result;

      cudie = dwfl_module_nextcu (module, cudie, &bias);
      root = cudie;
    }

  return 1;
}

enum refl_cb_status
__refl_die_attr_pred (Dwarf_Die *die, void *data)
{
  struct __refl_die_attr_pred *pred_data = data;
  Dwarf_Attribute attr_mem, *attr
    = dwarf_attr (die, pred_data->at_name, &attr_mem);
  if (attr == NULL)
    return DWARF_CB_OK;

  /* fprintf (stderr, "%#lx\n", dwarf_dieoffset (die)); */
  return pred_data->callback (attr, pred_data->data);
}

enum refl_cb_status
__refl_attr_match_string (Dwarf_Attribute *attr, void *data)
{
  char const *name = data;
  assert (name != NULL);

  char const *value = dwarf_formstring (attr);
  if (value == NULL)
    {
      __refl_seterr (REFL_E_DWARF);
      return refl_cb_fail;
    }

  return strcmp (name, value) != 0 ? refl_cb_next : refl_cb_stop;
}

Dwarf_Attribute *
__refl_attr_integrate (Dwarf_Die *die, int name, Dwarf_Attribute *mem)
{
  Dwarf_Attribute *ret = dwarf_attr_integrate (die, name, mem);
  if (ret == NULL)
    __refl_seterr (REFL_E_DWARF);
  return ret;
}

char const *
__refl_die_name (Dwarf_Die *die)
{
  Dwarf_Attribute attr_mem, *attr
    = __refl_attr_integrate (die, DW_AT_name, &attr_mem);
  if (attr == NULL)
    return NULL;

  const char *str = dwarf_formstring (attr);
  if (str == NULL)
      __refl_seterr (REFL_E_DWARF);

  return str;
}

Dwarf_Die *
__refl_die_type (Dwarf_Die *die, Dwarf_Die *ret_mem)
{
  Dwarf_Attribute type_att_mem, *type_att
    = __refl_attr_integrate (die, DW_AT_type, &type_att_mem);
  Dwarf_Die *ret = NULL;
  if (type_att == NULL
      || (ret = dwarf_formref_die (type_att, ret_mem)) == NULL)
    {
      __refl_seterr (REFL_E_DWARF);
      return NULL;
    }

  return ret;
}

Dwarf_Die *
__refl_die_strip_cvq (Dwarf_Die *die, Dwarf_Die *ret_mem)
{
  while (dwarf_tag (die) == DW_TAG_const_type
	 || dwarf_tag (die) == DW_TAG_volatile_type)
    if (__refl_die_type (die, ret_mem) == NULL)
      return NULL;
  return ret_mem;
}
