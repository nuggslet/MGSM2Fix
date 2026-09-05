//
// Copyright (c) 2009 Brandon Jones
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
#include <iostream>
#include <sstream>
#include "Fixture.h"

using namespace Sqrat;

const Sqrat::string GetGreeting()
{
    return _SC("Hello world!");
}

int AddTwo(int a, int b)
{
    return a + b;
}

struct Person
{
    string name;
    int age;
};

static bool get_object_string(HSQUIRRELVM vm, HSQOBJECT obj, string & out_string)
{
    sq_pushobject(vm, obj);
    sq_tostring(vm, -1);
    const SQChar *s;
    SQRESULT res = sq_getstring(vm, -1, &s);
    bool r = SQ_SUCCEEDED(res);
    if (r)
    {
        out_string = string(s);
        sq_pop(vm,1);
    }
    return r;

}

TEST_F(SqratTest, SimpleTableBinding)
{
    DefaultVM::Set(vm);

    string version = _SC("1.0.0");
    string n12 = _SC("N12");

    // Bind table values and functions
    Table test(vm);
    test
    // Global functions
    .Func(_SC("GetGreeting"), &GetGreeting)
    .Func(_SC("AddTwo"), &AddTwo)

    // Variables
    .SetValue(_SC("version"), version) // Changes to this variable in the script will not propagate back to the native variable
    .SetValue(_SC("author"), _SC("Brandon Jones"))
    .SetValue(_SC("count"), 12)
    .SetValue(12, n12);
    ;

    // Bind a class to the table. In this case the table acts somewhat as a namespace
    Class<Person> person(vm, _SC("Person"));
    person
    .Var(_SC("name"), &Person::name)
    .Var(_SC("age"), &Person::age)
    ;

    test.Bind(_SC("Person"), person);

    // Bind the table to the root table
    RootTable().Bind(_SC("Test"), test);

    Table::iterator it;
    string  str1, str2;

    while (test.Next(it))
    {
        EXPECT_TRUE(get_object_string(vm, it.getKey(), str1));
        EXPECT_TRUE(get_object_string(vm, it.getValue(), str2));
#ifndef SQUNICODE
        std::cout << "Key: "
                  << str1 << " Value: "
                  << str2 << std::endl;
#endif
    }
    SharedPtr<string> value = test.GetValue<string>(12);
    EXPECT_EQ(value != NULL, 1);
    EXPECT_STREQ(value->c_str(), n12.c_str());
    value = test.GetValue<string>(_SC("version"));
    EXPECT_EQ(value != NULL, 1);
    EXPECT_STREQ(value->c_str(), version.c_str());
    
    
    Script script;
    script.CompileString(_SC("  \
        gTest.EXPECT_STR_EQ(Test.version, \"1.0.0\"); \
        gTest.EXPECT_STR_EQ(Test.GetGreeting(), \"Hello world!\"); \
        gTest.EXPECT_INT_EQ(Test.AddTwo(1, 2), 3); \
        Test.count += 3; \
        gTest.EXPECT_INT_EQ(Test.count, 15); \
        \
        p <- Test.Person(); \
        p.name = \"Bobby\"; \
        p.age = 25; \
        gTest.EXPECT_STR_EQ(p.name, \"Bobby\"); \
        gTest.EXPECT_STR_EQ(p.age, 25); \
        "));
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }
}

TEST_F(SqratTest, TableGet)
{

    static const SQChar *sq_code = _SC("\
        local i; \
        for (i = 0; i < 12; i++) \
            tb[i.tostring()] <- \"value \" + i;\
        \
        for (i = 100; i < 112; i++) \
            tb[i] <- \"value \" + i;\
        \
           ");
    int i;
    DefaultVM::Set(vm);

    Table table(vm);
    RootTable(vm).Bind(_SC("tb"), table);

    Script script;
    script.CompileString(sq_code);
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }

    const int length = 12;

    for ( i = 0; i < length; i++)
    {
#ifdef SQUNICODE
        std::wstringstream ss1, ss2;
#else
        std::stringstream ss1, ss2;
#endif
        string key, value;
        ss1 << i;
        ss2 << "value " << i;
        key = ss1.str();
        value = ss2.str();
        SharedPtr<string> value2 = table.GetValue<string>(key.c_str());
        EXPECT_EQ(value2 != NULL, 1);
        EXPECT_EQ(value, *value2);
    }

    for ( i = 100; i < 100 + length; i++)
    {
#ifdef SQUNICODE
        std::wstringstream ss2;
#else
        std::stringstream ss2;
#endif
        string value;

        ss2 << "value " << i;

        value = ss2.str();
        SharedPtr<string> value2 = table.GetValue<string>(i);
        EXPECT_EQ(value2 != NULL, 1);
        EXPECT_EQ(value, *value2);
    }

}

TEST_F(SqratTest, TableCleanup)    // test case for Sourceforge Sqrat Bug 43
{
    static const SQChar *sq_code = _SC("\
        local i; \
        for (i = 0; i < 12; i++) \
            tb[i.tostring()] <- \"value \" + i;\
        \
        for (i = 100; i < 112; i++) \
            tb[i] <- \"value \" + i;\
        \
           ");
    HSQUIRRELVM v = sq_open(1024);

    Table table(v);
    RootTable(v).Bind(_SC("tb"), table);

    Script script(v);
    script.CompileString(sq_code);
    if (Sqrat::Error::Occurred(v)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(v);
    }

    script.Run();
    if (Sqrat::Error::Occurred(v)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(v);
    }
    const int length = 12;
    // do some normal things with the table
    for ( int i = 0; i < length; i++)
    {
#ifdef SQUNICODE
        std::wstringstream ss1, ss2;
#else
        std::stringstream ss1, ss2;
#endif
        string key, value;
        ss1 << i;
        ss2 << "value " << i;
        key = ss1.str();
        value = ss2.str();
        SharedPtr<string> value2 = table.GetValue<string>(key.c_str());
#ifndef SQUNICODE
        std::cout << "Key: "
                  << key << " Value: "
                  << value << " value2: " << *value2 << std::endl;
#endif
        EXPECT_EQ(value2 != NULL, 1);
        EXPECT_EQ(value, *value2);
    }
    script.Release();
    table.Release();
    sq_close(v); // see what happens now


}


void touch_element(Sqrat::Table & t, const char *key, int val) 
{
    t.SetValue(key, val);        
}

void touch_element2(Sqrat::Table t, const char *key, int val) 
{
    t.SetValue(key, val);        
}


void touch_element3(Sqrat::Table t, int index, Sqrat::Table &t2) 
{
    t.SetValue(index, t2);        
}

void touch_element4(Sqrat::Table t, const char *key, Sqrat::Table &t2) 
{
    t.SetValue(key, t2);        
}

TEST_F(SqratTest, PassingTableIn) {
    char buf[200];
    static const int SIZE = 56;
    static const SQChar *sq_code = _SC("\
        local i; \
        for (i = 0; i < SIZE; i++) \
            touch_element2(t, i.tostring(), 5 - i);\
        \
        for (i = 0; i < SIZE; i++) \
            gTest.EXPECT_INT_EQ( t[i.tostring()], 5 - i);\
        \
        for (i = 0; i < SIZE; i++) \
            touch_element(t, i.tostring(), -i);\
        \
        local t2 = {} \
        for (i = 0; i < SIZE; i++) \
            touch_element(t2, i.tostring(), 1 - i);\
        \
        for (i = 0; i < SIZE; i++) \
            gTest.EXPECT_INT_EQ( t2[i.tostring()], 1 - i);\
        \
           ");
    DefaultVM::Set(vm);
    RootTable().Func(_SC("touch_element"), &touch_element);
    RootTable().Func(_SC("touch_element2"), &touch_element2);
    ConstTable().Const(_SC("SIZE"), SIZE);
    
    int i;
    Table table(vm);
    RootTable(vm).Bind(_SC("t"), table);
    
    
    
    for (i = 0; i < SIZE; i++) {
        snprintf(buf, sizeof(buf), "%d", i);
        touch_element(table, buf, i);
    }

    
    
    for (i = 0; i < SIZE; i++)
    {

        snprintf(buf, sizeof(buf), "%d", i);
        SharedPtr<int> j = table.GetValue<int>(buf);
        EXPECT_EQ(j != NULL, 1);
        EXPECT_EQ(*j, i);
        
    }

    Script script;
    script.CompileString(sq_code);
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }
        
    
    for (i = 0; i < SIZE; i++)
    {

        snprintf(buf, sizeof(buf), "%d", i);
        SharedPtr<int> j = table.GetValue<int>(buf);
        EXPECT_EQ(j != NULL, 1);
        EXPECT_EQ(*j, -i);
        
    }
        
    
}

TEST_F(SqratTest, PassingTableIn2) {
    char buf[200];
    static const int SIZE = 56;
    static const SQChar *sq_code = _SC("\
        local i; \
        local t2 = {} \
        for (i = 0; i < SIZE; i++) \
            touch_element(t2, i.tostring(), 1 - i);\
        \
        for (i = 0; i < SIZE; i++) \
            gTest.EXPECT_INT_EQ( t2[i.tostring()], 1 - i);\
        \
        for (i = 0; i < SIZE; i++) \
            touch_element2(t2, i.tostring(), 1 + i);\
        \
        for (i = 0; i < SIZE; i++) \
            gTest.EXPECT_INT_EQ( t2[i.tostring()], 1 + i);\
        \
           ");
    DefaultVM::Set(vm);
    RootTable().Func(_SC("touch_element"), &touch_element);
    RootTable().Func(_SC("touch_element2"), &touch_element2);
    ConstTable().Const(_SC("SIZE"), SIZE);

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


TEST_F(SqratTest, PassingTableIn3) {
    char buf[200];
    static const int SIZE = 56;
    static const SQChar *sq_code = _SC("\
        gTest.EXPECT_INT_EQ( t[1].field, 10);\
        gTest.EXPECT_INT_EQ( t.element.x, 55);\
           ");
    DefaultVM::Set(vm);
    RootTable().Func(_SC("touch_element4"), &touch_element4);
    RootTable().Func(_SC("touch_element3"), &touch_element3);
    RootTable().Func(_SC("touch_element2"), &touch_element2);
    RootTable().Func(_SC("touch_element"), &touch_element);
    ConstTable().Const(_SC("SIZE"), SIZE);
    Table table(vm);
    RootTable(vm).Bind(_SC("t"), table);
    Table table2(vm);
    RootTable(vm).Bind(_SC("t2"), table2);
    touch_element3(table, 1, table2);
    touch_element2(table2, "field", 10);
    touch_element4(table, "element", table2);
    touch_element(table2, "x", 55);
    
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

