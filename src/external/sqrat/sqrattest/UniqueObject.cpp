#include <gtest/gtest.h>
#include <sqrat.h>
#include "Fixture.h"

struct CxxObject {
  CxxObject& cxx_ptr() {
    return *this;
  }
};

char const script_code[] = "\
v <- CxxObject();\
gTest.EXPECT_STR_EQ(v.tostring, v.cxx_ptr().tostring);\
\
target <- null;\
function set_target(v) {\
  ::target = v;\
}\
function test_target(v) {\
  gTest.EXPECT_STR_EQ(::target.tostring, v.tostring);\
}\
";

using namespace Sqrat;

TEST_F(SqratTest, UniqueObject) {
  DefaultVM::Set(vm);

  Class<CxxObject> cls(vm, _SC("CxxObject"));
  cls.Func<CxxObject& (CxxObject::*)()>(_SC("cxx_ptr"), &CxxObject::cxx_ptr);
  RootTable(vm).Bind(_SC("CxxObject"), cls);

  Script script;
  std::string err;
  if (!script.CompileString(script_code, err)) {
    FAIL() << _SC("Compile failed: ") << err;
  }
  if (!script.Run(err)) {
    FAIL() << _SC("Script failed: ") << err;
  }

  Function set_target(RootTable(vm), "set_target"), test_target(RootTable(vm), "test_target");
  CxxObject* obj = new CxxObject;
  set_target(obj);
  test_target(obj);
  delete obj;
}
