#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dwarf.h>
#include <stdlib.h>

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

	  int result = __refl_die_tree (&child_mem, &child_mem, callback, data);
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

struct __refl_die_attr_pred
{
  int at_name;
  enum refl_cb_status (*callback) (Dwarf_Attribute *attr, void *data);
  void *data;
};

enum refl_cb_status
__refl_die_attr_pred (Dwarf_Die *die, void *data)
{
  struct __refl_die_attr_pred *pred_data = data;
  Dwarf_Attribute attr_mem, *attr
    = dwarf_attr (die, pred_data->at_name, &attr_mem);
  if (attr == NULL)
    return DWARF_CB_OK;

  //fprintf (stderr, "%#lx\n", dwarf_dieoffset (die));
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

  return strcmp (name, value) ? refl_cb_next : refl_cb_stop;
}

static int
build_type_assembly (Dwarf_Die *die, int tag,
		     struct refl_assembly *ret_assembly)
{
  struct refl_type *type = __refl_type_begin (die);
  if (type == NULL)
    return -1;

  *ret_assembly = (struct refl_assembly) {
    .kind = refl_as_type,
    .type = type
  };
  return 1;
}

static int
build_method_assembly (Dwarf_Die *die, int tag,
		       struct refl_assembly *ret_assembly)
{
  struct refl_method *method = __refl_method_begin (die);
  if (method == NULL)
    return -1;

  *ret_assembly = (struct refl_assembly) {
    .kind = refl_as_method,
    .method = method
  };
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

  __refl_error (REFL_ME_DWARF);
  return -1;
}
