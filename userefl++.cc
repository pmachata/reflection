#include <iostream>
#include <cassert>
#include "refl++.hh"

struct other
{
  int j;
};

struct mystruct
{
  int i;
  struct other o;
  int k;
};

static int
something (int c)
{
  std::cerr << "simon says: " << c << std::endl;
  return c + 1;
}

int (*ddd)(int) = something;

static void
d (mystruct const &my)
{
  std::cerr << '{' << my.i << ", {" << my.o.j << "}, "
	    << my.k << "}" << std::endl;
}

int value = 111;
int *valuep = &value;
int **valuepp = &valuep;
int ***valueppp = &valuepp;

int
main (int argc, char *argv[])
{
  reflpp::refl refl;
  reflpp::module mod = refl.module_cur ();

  mod.object_named ("ddd").as <int (*)(int)> () (17);

  {
    reflpp::type mystruct = mod.type_named ("mystruct");
    std::cerr << "mystruct is: " << mystruct << std::endl;
    reflpp::object obj = mystruct.create ();

    obj["i"] = 1;
    obj["o"]["j"] = 7;
    obj["k"] = 9;
    d (obj.as<struct mystruct> ());

    obj["o"] = (struct other){ 13 };
    d (obj.as<struct mystruct> ());

    obj.as<struct mystruct> ().o.j = 15;
    d (obj.as<struct mystruct> ());
  }

  {
    reflpp::object vppp = mod.object_named ("valueppp");
    while (vppp.type ().is_pointer ())
      vppp = *vppp;
    std::cerr << "v = " << vppp.as<int> () << std::endl;
  }

  {
    mod.method_named ("something");
  }

  return 0;
}
