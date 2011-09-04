#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

struct refl_error
{
  int major;
  int minor;
};

struct refl;
struct refl_module;
struct refl_type;
struct refl_method;
struct refl_object;

enum refl_assembly_kind
  {
    refl_as_type,
    refl_as_method
  };

struct refl_assembly
{
  enum refl_assembly_kind kind;
  union
  {
    struct refl_type *type;
    struct refl_method *method;
  };
};

enum refl_cb_status
  {
    refl_cb_next,
    refl_cb_fail,
    refl_cb_stop,
  };

struct refl_error refl_error (void);
char const *refl_errmsg (struct refl_error error);

struct refl *refl_begin (void);
void refl_end (struct refl *refl);

struct refl_module *refl_module_exe (struct refl *refl);
struct refl_module *refl_module_addr (struct refl *refl, void *ptr);
struct refl_module *refl_module_cur (struct refl *refl);

struct refl_method *refl_method_addr (struct refl *refl, void *ptr);
struct refl_method *refl_method_cur (struct refl *refl);
char const *refl_method_name (struct refl_method *method);

int refl_assembly_named (struct refl *refl,
			 struct refl_module *mod, char const *name,
			 struct refl_assembly *ret_assembly);

struct refl_type *refl_type_named (struct refl *refl,
				   struct refl_module *mod, char const *name);

struct refl_object *refl_new (struct refl *refl, struct refl_type *type);
struct refl_object *refl_access (struct refl *refl, struct refl_object *obj,
				 char const *name);
void refl_assign_int (struct refl_object *obj, int value);

size_t refl_type_sizeof (struct refl *refl, struct refl_type *type);

void *refl_object_cdata (struct refl_object *obj);

#ifdef __cplusplus
}
#endif
