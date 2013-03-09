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

#include <dwarf.h>
#include "reflP.h"

int
__refl_die_tree (Dwarf_Die *root, Dwarf_Die *ret,
		 enum refl_cb_status (*callback) (Dwarf_Die *die,
						  void *data),
		 void *data)
{
  Dwarf_Die die_mem = *root, *die = &die_mem;
  while (die != NULL)
    {
      switch (callback (die, data))
	{
	case refl_cb_stop:
	  *ret = *die;
	  return 0;
	case refl_cb_fail:
	  return -1;
	case refl_cb_next:
	  ;
	};

      if (dwarf_haschildren (die))
	{
	  Dwarf_Die child_mem;
	  if (dwarf_child (die, &child_mem))
	    {
	    err:
	      __refl_seterr (REFL_E_DWARF);
	      return -1;
	    }

	  int result = __refl_die_tree (&child_mem, &child_mem,
					callback, data);
	  if (result == 0)
	      *ret = child_mem;
	  if (result <= 0)
	    return result;
	}

      switch (dwarf_siblingof (die, &die_mem))
	{
	case 0: continue;
	case -1: goto err;
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
      int result = __refl_die_tree (root, &found, callback, data);
      if (result == 0)
	*ret = found;
      if (result <= 0)
	return result;

      cudie = dwfl_module_nextcu (module, cudie, &bias);
      root = cudie;
    }

  return 1;
}
