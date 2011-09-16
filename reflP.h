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

#ifndef _REFLP_H_INCLUDED
#define _REFLP_H_INCLUDED

#include <elfutils/libdwfl.h>
#include "refl.h"

enum __refl_major_errcode
  {
    REFL_E_NOERROR,
    REFL_E_SYSTEM,
    REFL_E_DWFL,
    REFL_E_DWARF,
  };
enum
  {
    REFL_E_REFL = REFL_E_DWARF + 1,
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

void __refl_seterr_2 (int major, int minor);
void __refl_seterr (enum __refl_major_errcode major);
void __refl_error (enum __refl_minor_errcode minor);
void *__refl_malloc (size_t size);

struct refl_method *__refl_method_begin (Dwarf_Die *die);
struct refl_type *__refl_type_begin (Dwarf_Die *die);
void __refl_type_free (struct refl_type *type);

int __refl_die_tree (Dwarf_Die *root, Dwarf_Die *ret,
		     enum refl_cb_status (*callback) (Dwarf_Die *die,
						      void *data),
		     void *data);

int __refl_each_die (Dwfl_Module *module, Dwarf_Die *root, Dwarf_Die *ret,
		     enum refl_cb_status (*callback) (Dwarf_Die *die,
						      void *data),
		     void *data);

struct refl_object *__refl_object_begin (struct refl_type *type, void *data);
struct refl_object *__refl_object_begin_inline (struct refl_type *type,
						size_t size);
#endif//_REFLP_H_INCLUDED
