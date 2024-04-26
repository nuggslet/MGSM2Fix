//
// Copyright (c) 2013 Li-Cheng (Andy) Tai
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#include <gtest/gtest.h>
#include <sqrat.h>
#include "Fixture.h"


using namespace Sqrat;

class Vector2
{
public:
    float value;
    
    //some Vector Math overloads and members here
    operator float() const 
    {
        return 1.0f; /* test only */    
    }
    
    
    Vector2 operator +(const Vector2& v) 
    {
        value += v.value;
        return v; /* test only */    
    }
    
    inline float add(const Vector2& v) 
    {
        return (*this) + v; //it crashes right here as it references to an nonexistent obj
    }

    
    bool boolFunc() 
    {
        return true;
    }
    
    bool boolFunc2() 
    {
        return false;
    }
    
};

static const SQChar *sq_code = _SC("\
    local v = Vector2();\
    local v2 = Vector2();\
    \
    local b = v2.boolFunc(); \
    \
    \
    local b = v2.boolFunc(); \
    \
	gTest.EXPECT_TRUE(b); \
	gTest.EXPECT_INT_EQ(b, 1); \
	gTest.EXPECT_FLOAT_EQ(b, 1.0); \
    b = v2.boolFunc2(); \
    \
	gTest.EXPECT_FALSE(b); \
	gTest.EXPECT_INT_EQ(b, 0); \
	gTest.EXPECT_FLOAT_EQ(b, 0.0); \
    \
    print (b) ; \
    b = v2.boolFunc2(); \
    \
    print (b) ; \
    \
    v.add(v2); /*good*/  \
    print(\"1\\n\");\
    local raised = false;\
    try { \
        v.add(10);  \
		gTest.EXPECT_INT_EQ(0, 1); \
	} catch (ex) {\
        raised = true;\
        print(ex + \"\\n\"); \
    }\
    gTest.EXPECT_TRUE(raised); \
    print(\"2\\n\");\
    raised = false;\
    try { \
        v.add();  \
		gTest.EXPECT_INT_EQ(0, 1); \
	} catch (ex) {\
        raised = true;\
        print(ex + \"\\n\"); \
    }\
    gTest.EXPECT_TRUE(raised); \
    print(\"3\\n\");\
    raised = false;\
    try {\
        v.add(\"text\");  \
		gTest.EXPECT_INT_EQ(0, 1); \
	} catch (ex) {\
        raised = true;\
        print(ex + \"\\n\"); \
    }\
    gTest.EXPECT_TRUE(raised); \
    print(\"4\\n\");\
       ");



TEST_F(SqratTest, NumericArgumentTypeConversionAndCheck) {
    DefaultVM::Set(vm);
    
    Sqrat::Class<Vector2> classVector2(vm, _SC("Vector2"));
    
    classVector2.Func(_SC("add"), &Vector2::add);
    
    classVector2.Func(_SC("boolFunc"), &Vector2::boolFunc);
    classVector2.Func(_SC("boolFunc2"), &Vector2::boolFunc2);
    Sqrat::RootTable(vm).Bind(_SC("Vector2"), classVector2);
            
    Script script;
    script.CompileString(sq_code);
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }

}

class F
{
public:
    int func(int i, char j)
    {
        return 1;
    }
    
    const char * func(char c, const char *s)
    {
        return s;
    }
    
};

static const char *sq_code2 = _SC("\
         f <- F();\
         gTest.EXPECT_INT_EQ(1, f.f1(2. 'v'));\
         gTest.EXPECT_STR_EQ(\"test\", f.f2('t', \"test\")); \
    ");



TEST_F(SqratTest, FunctionOfSameNumberOfArgumentsButDifferentTypesBinding) {
    DefaultVM::Set(vm);
    
    Sqrat::Class<F> Fclass(vm, _SC("F"));
    Fclass.Func<int (F::*)(int, char)>(_SC("f1"), &F::func);
    Fclass.Func<const char * (F::*)(char, const char*)>(_SC("f2"), &F::func);
    
    Sqrat::RootTable(vm).Bind(_SC("F"), Fclass);
            
    Script script;
    script.CompileString(sq_code2);
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }

}
