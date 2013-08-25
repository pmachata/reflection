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

#ifndef _REFL_HH_INCLUDED_
#define _REFL_HH_INCLUDED_

#include <stdexcept>
#include <memory>
#include <string>
#include <iosfwd>
#include <cstring>
#include <cassert>

#include "refl.h"

namespace reflpp
{
  struct exception
    : public std::runtime_error
  {
    exception (const char* what_arg)
      : std::runtime_error (what_arg)
    {}
  };

  class type;
  class object;
  class method;
  class module;
  class refl;

  class type
  {
    friend class module;
    friend class object;
    friend std::ostream &operator<< (std::ostream &, type const &);

    refl &__refl;
    struct ::refl_type *__type;

    type (refl &__r, struct ::refl_type *__t);

  public:
    size_t size_of ();
    object create ();
    bool is_pointer ();
  };

  std::ostream &operator<< (std::ostream &, type const &);

  class object
  {
    friend class module;
    friend class type;

    refl &__refl;
    struct ::refl_object *__obj;

    object (refl &__r, struct ::refl_object *__o);
    void check_sizeof (size_t sz);
    void copy_from (void const *src, size_t sz);
    void *checked_cdata ();

  public:
    object &operator= (object const &other);

    reflpp::type type ();
    object operator[] (std::string const &name);
    object operator* ();

    template <class T>
    void
    operator= (T const &other)
    {
      assert ((void *)this != (void *)&other);
      check_sizeof (sizeof other);
      void *buffer = checked_cdata ();
      static_cast <T *> (buffer)->~T ();
      new (buffer) T (other);
    }

    template <class T>
    T &
    as ()
    {
      check_sizeof (sizeof (T));
      return *(T *)checked_cdata ();
    }
  };

  class method
  {
    friend class module;

    refl &__refl;
    struct ::refl_method *__method;

    method (refl &__r, struct ::refl_method *__m);

  public:
  };

  class module
  {
    friend class refl;

    refl &__refl;
    struct ::refl_module *__mod;

    module (refl &__r, struct ::refl_module *__m);

  public:
    type type_named (std::string const &name);
    object object_named (std::string const &name);
    method method_named (std::string const &name);
  };

  class refl
  {
    friend class module;
    friend class type;
    friend class object;
    friend std::ostream &operator<< (std::ostream &, type const &);

    struct ::refl *__refl;

  public:
    refl ();
    module module_cur ();
    module module_addr (void *addr);
  };
}

#endif /* _REFL_HH_INCLUDED_ */
