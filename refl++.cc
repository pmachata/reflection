#include <iostream>

#include "refl++.hh"

extern "C" {
#include "reflP.h"
}

namespace
{
  struct exception_impl
    : public reflpp::exception
  {
    exception_impl (struct refl_error const &err)
      : reflpp::exception (refl_errmsg (err))
    {}
  };

  void
  throw_refl_exc ()
  {
    throw ::exception_impl (refl_error ());
  }

  template <class T>
  T *
  refl_nonnull (T *p)
  {
    if (p == nullptr)
      throw_refl_exc ();
    return p;
  }
}

reflpp::type::type (reflpp::refl &__r, struct ::refl_type *__t)
  : __refl (__r)
  , __type (__t)
{
}

size_t
reflpp::type::size_of ()
{
  return refl_type_sizeof (__refl.__refl, __type);
}

reflpp::object
reflpp::type::create ()
{
  return object (__refl, refl_nonnull (refl_new (__refl.__refl, __type)));
}

bool
reflpp::type::is_pointer ()
{
  bool isptr;
  if (refl_type_is_pointer (__type, &isptr) < 0)
    throw_refl_exc ();
  return isptr;
}

std::ostream &
reflpp::operator<< (std::ostream &o, reflpp::type const &t)
{
  char *str;
  if (refl_type_dump (t.__refl.__refl, t.__type, &str) < 0)
    throw_refl_exc ();
  o << str;
  free (str);
  return o;
}

reflpp::object::object (refl &__r, struct ::refl_object *__o)
  : __refl (__r)
  , __obj (__o)
{
}

void
reflpp::object::check_sizeof (size_t sz)
{
  if (sz != type ().size_of ())
    {
      __refl_seterr_2 (REFL_E_REFL, REFL_ME_MISMATCH);
      throw_refl_exc ();
    }
}

void
reflpp::object::copy_from (void const *src, size_t sz)
{
  memcpy (refl_nonnull (refl_object_cdata (__obj)), src, sz);
}

void *
reflpp::object::checked_cdata ()
{
  return refl_nonnull (refl_object_cdata (__obj));
}

reflpp::object &
reflpp::object::operator= (reflpp::object const &other)
{
  assert (&__refl == &other.__refl);
  if (this != &other)
    __obj = other.__obj;
  return *this;
}

reflpp::type
reflpp::object::type ()
{
  return reflpp::type (__refl, refl_nonnull (refl_object_type (__obj)));
}

reflpp::object
reflpp::object::operator[] (std::string const &name)
{
  return object (__refl, refl_nonnull (refl_access (__refl.__refl, __obj,
						    name.c_str ())));
}

reflpp::object
reflpp::object::operator* ()
{
  return object (__refl, refl_nonnull (refl_deref (__refl.__refl, __obj)));
}

reflpp::method::method (refl &__r, struct ::refl_method *__m)
  : __refl (__r)
  , __method (__m)
{}

reflpp::module::module (reflpp::refl &__r, struct ::refl_module *__m)
  : __refl (__r)
  , __mod (__m)
{}

reflpp::type
reflpp::module::type_named (std::string const &name)
{
  return type (__refl,
	       refl_nonnull (refl_type_named (__refl.__refl, __mod,
					      name.c_str ())));
}

reflpp::object
reflpp::module::object_named (std::string const &name)
{
  return object (__refl,
		 refl_nonnull (refl_object_named (__refl.__refl, __mod,
						  name.c_str ())));
}

reflpp::method
reflpp::module::method_named (std::string const &name)
{
  return method (__refl,
		 refl_nonnull (refl_method_named (__refl.__refl, __mod,
						  name.c_str ())));
}

reflpp::refl::refl ()
  : __refl (refl_nonnull (refl_begin ()))
{
}

reflpp::module
reflpp::refl::module_cur ()
{
  struct ::refl_module *mod
    = refl_module_addr (__refl, __builtin_return_address (0));
  return module (*this, refl_nonnull (mod));
}
