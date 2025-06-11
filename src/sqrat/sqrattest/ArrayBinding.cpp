//
// Copyright 2013 Li-Cheng (Andy) Tai
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

TEST_F(SqratTest, ArrayGet) {

    static const SQChar *sq_code = _SC("\
        local i; \
        for (i = 0; i < 12; i++) \
            a.append(i);\
        \
           ");
    int i;
    DefaultVM::Set(vm);
    
    Array array(vm);
    RootTable(vm).Bind(_SC("a"), array);
        
    Script script;
    script.CompileString(sq_code);
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }
    
    const int length = array.Length();
    EXPECT_EQ(length, 12);
    
    for ( i = 0; i < length; i++)
    {
        SharedPtr<int> t = array.GetValue<int>(i);
        EXPECT_EQ(t != NULL, 1);
        EXPECT_EQ(*t, i);
        SharedPtr<float> f;
        f = array.GetValue<float>(i);
        EXPECT_EQ(f != NULL, 1);        
        EXPECT_EQ((float) i, *f);
    }
    int t[length];
    array.GetArray(t, sizeof(t) / sizeof(t[0]));
    EXPECT_FALSE(Sqrat::Error::Occurred(vm));    
    for (i = 0; i < length; i++)
    {
        EXPECT_EQ(t[i], i);
    }
    double d[length];
    array.GetArray(d, sizeof(d) / sizeof(d[0]));
    EXPECT_FALSE(Sqrat::Error::Occurred(vm));    

    for (i = 0; i < length; i++)
    {
        EXPECT_EQ(d[i], (double) i);
    }    
    double d2[15];
    array.GetArray(d2, sizeof(d2) / sizeof(d2[0]));
    EXPECT_TRUE(Sqrat::Error::Occurred(vm));    
    Sqrat::Error::Clear(vm);
    double d3[5];
    array.GetArray(d3, sizeof(d3) / sizeof(d3[0]));
    EXPECT_FALSE(Sqrat::Error::Occurred(vm));    

        
}


void touch_element(Sqrat::Array & a, int index, int val) 
{
    a.SetValue(index, val);        
}

void touch_element2(Sqrat::Array a, int index, int val) 
{
    a.SetValue(index, val);        
}

TEST_F(SqratTest, PassingArrayIn) {
    static const int SIZE = 56;
    static const SQChar *sq_code = _SC("\
        local i; \
        for (i = 0; i < a.len(); i++) \
            touch_element2(a, i, 5 - i);\
        \
        for (i = 0; i < a.len(); i++) \
            gTest.EXPECT_INT_EQ( a[i], 5 - i);\
        \
        for (i = 0; i < a.len(); i++) \
            touch_element(a, i, -i);\
        \
        for (i = 0; i < a.len(); i++) \
            gTest.EXPECT_INT_EQ( a[i], - i);\
        \
           ");
           
    DefaultVM::Set(vm);
    RootTable().Func(_SC("touch_element"), &touch_element);
    RootTable().Func(_SC("touch_element2"), &touch_element2);
    
    int i;
    Array array(vm, SIZE);
    RootTable(vm).Bind(_SC("a"), array);
    for (i = 0; i < SIZE; i++)
        touch_element(array, i, i);

    int length = array.Length();
    EXPECT_EQ(length, SIZE);
    
    for (i = 0; i < length; i++)
    {
        SharedPtr<int> t = array.GetValue<int>(i);
        EXPECT_EQ(t != NULL, 1);
        EXPECT_EQ(*t, i);
        
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
        
    
    length = array.Length();
    EXPECT_EQ(length, SIZE);
    
    for (i = 0; i < length; i++)
    {
        SharedPtr<int> t = array.GetValue<int>(i);
        EXPECT_EQ(t != NULL, 1);
        EXPECT_EQ(*t, -i);
        
    }
        
    
}


TEST_F(SqratTest, PassingArrayIn2) {
    static const int SIZE = 56;
    static const SQChar *sq_code = _SC("\
        local i; \
        local a2 = array(12); \
        for (i = 0; i < a2.len(); i++) \
            touch_element2(a2, i, 1 - i); \
        \
        for (i = 0; i < a2.len(); i++) \
            gTest.EXPECT_INT_EQ( a2[i], 1 - i); \
        \
        for (i = 0; i < a2.len(); i++) \
            touch_element(a2, i, 1 + i); \
        \
        for (i = 0; i < a2.len(); i++) \
            gTest.EXPECT_INT_EQ( a2[i], 1 + i); \
        \
        ");
    DefaultVM::Set(vm);
    RootTable().Func(_SC("touch_element2"), &touch_element2);
    RootTable().Func(_SC("touch_element"), &touch_element);
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
