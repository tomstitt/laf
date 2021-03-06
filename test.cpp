#include <iostream>
#include <functional>
#include <string>

#include "laf.hpp"

using namespace std;

/**
 * What's tested:
 *  lambda           - X
 *  std::function    - X
 *  function pointer - X
 *  member fun ptr   - X
 *
 *  return types  -
 *   double       - X
 *   int          - X
 *   bool         -
 *   const char * - X
 *   std::string  - X
 *   void         - X
 *
 *  args:         - X
 *   double       - X
 *   int          - X
 *   bool         -
 *   const char * - X
 *   std::string  - X
 *   void (none)  -
 *   userdata *   - X
 **/


struct A {
  int val;
  A(int _val) : val(_val) {}
  int getVal() { return val; }
  int addVal(const int & i) { return i + val; }
  void setVal(const int & i) { val = i; }
  bool getBool() const { return true; }
  int overloadedFun() const { return 2; };
  int overloadedFun(std::string) { return 5; };
};

namespace laf {
  template <> const char * identifier<A>() { return "main::A"; }
}

luaL_reg a_lua_methods[] = {
  {"get_val", LAF_WRAP_MEMBER(&A::getVal)},
  {"add_val", LAF_WRAP_MEMBER(&A::addVal)},
  {"set_val", LAF_WRAP_MEMBER(&A::setVal)},
  {"get_bool", LAF_WRAP_MEMBER(&A::getBool)},
  {"overloaded_fun", LAF_WRAP_MEMBER(static_cast<int(A::*)() const>(&A::overloadedFun))},
  {"overloaded_fun2", LAF_WRAP_MEMBER(static_cast<int(A::*)(std::string)>(&A::overloadedFun))},
  {0, 0}
};


const char * fun_ptr_cstr_id(const char * a) {
  return a;
}

int error_count = 0;
int test_count = 0;

void test(lua_State * L, const char * lua_statement) {
  test_count++;
  std::cout << "starting test " << test_count << ": " << lua_statement << std::endl;
  if (luaL_dostring(L, lua_statement) != 0) {
    std::cout << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 1);
    std::cout << "test failed" << std::endl << std::endl;
    error_count++;
    return;
  }
  std::cout << "test passed" << std::endl << std::endl;
}


int main() {
  lua_State * L = luaL_newstate();
  luaL_openlibs(L);

  // test 1
  std::function<int(int, int)> apb = [](int a, int b) {
    return a + b;
	};
  laf::push_function(L, apb);
  lua_setglobal(L, "apb");
  test(L, "assert(apb(5, 10) == 15)");

  // test 2
  double t2_val = 2;
  laf::push_function(L, [t2_val](int a, double x, int b) { return a * x + b + t2_val; });
  lua_setglobal(L, "axpbpv");
  test(L, "assert(axpbpv(4, 3, 8) == 22)");

  // test 3
  laf::push_function(L, [](int a, double b) { std::cout << a << " " << b << std::endl; });
  lua_setglobal(L, "print_a_b");
  test(L, "print_a_b(4, 3.14)");

  // test 4
  laf::push_function(L, [](std::string a) { return a + " world!"; } );
  lua_setglobal(L, "add_world");
  test(L, "assert(add_world('hello') == 'hello world!')");

  // test 5
  laf::push_function(L, fun_ptr_cstr_id);
  lua_setglobal(L, "str_id");
  test(L, "assert(str_id('lua!') == 'lua!')");

  // test 6
  A * a = new(lua_newuserdata(L, sizeof(A))) A(13);
  //luaL_newmetatable(L, laf::lua_type_info<A>::identifier());
  luaL_newmetatable(L, laf::identifier<A>());
  std::cout << "laf's identifier for A is " << laf::identifier<A>() << std::endl;
  lua_newtable(L);
  luaL_register(L, nullptr, a_lua_methods);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -2);
  lua_setglobal(L, "A");

  laf::push_function(L, &A::getVal);
  lua_setglobal(L, "get_val_of_A");
  test(L, "assert(get_val_of_A(A) == 13)");

  // test 7
  laf::push_function(L, &A::addVal);
  lua_setglobal(L, "add_val_of_A");
  test(L, "assert(add_val_of_A(A, 8) == 21)");

  // test 8
  laf::push_function(L, &A::getBool);
  lua_setglobal(L, "get_bool_of_A");
  test(L, "assert(get_bool_of_A(A, 8) == true)");

  // test 8
  test(L, "assert(A:get_val() == 13)");

  // test 9
  test(L, "assert(A:add_val(7.1) == 20)");

  // test 10
  test(L, "assert(A:get_bool() == true)");

  // test 10
  test(L, "assert(A:overloaded_fun() == 2)");

  // test 10
  test(L, "assert(A:overloaded_fun2('meh') == 5)");

  // print status
  std::cout << std::endl << test_count - error_count << " of " << test_count << " tests ran successfully" << std::endl;
  return error_count;
}
