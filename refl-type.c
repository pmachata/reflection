#include <stdlib.h>
#include <dwarf.h>
#include "reflP.h"

struct refl_type *
__refl_type_begin (Dwarf_Die *die)
{
  struct refl_type *result = malloc (sizeof (*result));
  if (result == NULL)
    {
      __refl_seterr (REFL_E_SYSTEM);
      return NULL;
    }
  result->die = *die;
  result->size = (size_t)-1;
  return result;
}

void
__refl_type_free (struct refl_type *type)
{
  free (type);
}

size_t
refl_type_sizeof (struct refl *refl, struct refl_type *type)
{
  if (type->size != (size_t)-1)
    return type->size;

  Dwarf_Attribute attr_mem, *attr
    = dwarf_attr_integrate (&type->die, DW_AT_byte_size, &attr_mem);
  if (attr == NULL)
    {
      __refl_error (REFL_ME_DWARF);
      return (size_t)-1;
    }

  Dwarf_Word size;
  if (dwarf_formudata (attr, &size) != 0)
    {
      __refl_seterr (REFL_E_DWARF);
      return (size_t)-1;
    }

  type->size = size;

  return size;
}

struct refl_object *
refl_new (struct refl *refl, struct refl_type *type)
{
  size_t size = refl_type_sizeof (refl, type);
  if (size == (size_t)-1)
    return NULL;

  return __refl_object_begin_inline (type, size);
}
