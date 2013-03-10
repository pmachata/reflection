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

#ifndef _REFLP_H_INCLUDED
#define _REFLP_H_INCLUDED

#include <elfutils/libdwfl.h>
#include "refl.h"

enum __refl_major_errcode
  {
    REFL_E_NOERROR,
    REFL_E_REFL,
    REFL_E_SYSTEM,
    REFL_E_DWFL,
    REFL_E_DWARF,
  };

enum __refl_minor_errcode
  {
    REFL_ME_DWARF,
  };

struct refl
{
  Dwfl *dwfl;
};

struct refl_method
{
  Dwarf_Die die;
  char const *name;
};

struct refl_type
{
  Dwarf_Die die;
  size_t size;
};

struct refl_object
{
  struct refl_type *type;
  void *data;
};

void __refl_seterr_2 (enum __refl_major_errcode major, int minor);
void __refl_seterr (enum __refl_major_errcode major);

void *__refl_malloc (size_t size);

struct refl_method *__refl_method_begin (Dwarf_Die *die);
struct refl_type *__refl_type_begin (Dwarf_Die *die);
void __refl_type_free (struct refl_type *type);

struct refl_object *__refl_object_begin (struct refl_type *type, void *data);
struct refl_object *__refl_object_begin_inline (struct refl_type *type,
						size_t size);

/* Iterate die tree rooted at ROOT.  At each die, CALLBACK is called
   with that die and DATA.  If CALLBACK returns refl_cb_stop, the
   iteration stops, current die is copied to RET, and 0 is returned.
   If it returns refl_cb_fail, -1 is returned.  Otherwise the
   iteration continues.  */
int __refl_die_tree (Dwarf_Die *root, Dwarf_Die *ret,
		     enum refl_cb_status (*callback) (Dwarf_Die *die,
						      void *data),
		     void *data);

/* Iterate all CU's in module.  Calls __refl_die_tree under the hood,
   the protocol is the same.  */
int __refl_each_die (Dwfl_Module *module, Dwarf_Die *root, Dwarf_Die *ret,
		     enum refl_cb_status (*callback) (Dwarf_Die *die,
						      void *data),
		     void *data);

/* Wrapper around dwarf_attr_integrate that sets error on failure.  */
Dwarf_Attribute *__refl_attr_integrate (Dwarf_Die *die, int name,
					Dwarf_Attribute *mem);

/* Find DW_AT_name of DIE and return the corresponding string.  Return
   NULL and set an error if it fails or name is unavailable.  */
char const *__refl_die_name (Dwarf_Die *die);

struct refl_object *__refl_object_begin (struct refl_type *type, void *data);
struct refl_object *__refl_object_begin_inline (struct refl_type *type,
						size_t size);

struct __refl_die_attr_pred
{
  int at_name;
  enum refl_cb_status (*callback) (Dwarf_Attribute *attr, void *data);
  void *data;
};

/* Callback for die iteration.  Use as an argument to __refl_die_tree
   and friends.  DATA shall be a pointer to struct __refl_die_tree.
   An attribute named DATA->at_name is looked up in each visited die,
   and if present, DATA->callback is called with that attribute and
   DATA->data as arguments.  */
enum refl_cb_status __refl_die_attr_pred (Dwarf_Die *die, void *data);

/* Callback for __refl_die_attr_pred.  ATTR shall be a string
   attribute, whose value is compared to DATA.  Iteration stops, if
   the two match.  */
enum refl_cb_status __refl_attr_match_string (Dwarf_Attribute *attr,
					      void *data);

/* Wrapper around dwarf_attr_integrate that sets error on failure.  */
Dwarf_Attribute *__refl_attr_integrate (Dwarf_Die *die, int name,
					Dwarf_Attribute *mem);

/* Find DW_AT_name of DIE and return the corresponding string.  Return
   NULL and set an error if it fails or name is unavailable.  */
char const *__refl_die_name (Dwarf_Die *die);

#endif//_REFLP_H_INCLUDED
