//
// Copyright (c) 2012 Li-Cheng (Andy) Tai, atai@atai.org
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
/* test demonstrating Sourceforge bug 3507590 */
   
using namespace Sqrat;

class C
{
    
public:
    int suspend()
    {
        return sq_suspendvm(DefaultVM::Get());
    }
};


TEST_F(SqratTest, SuspendVM)
{
    DefaultVM::Set(vm);
    int i; 
    Class<C> cclass(vm, _SC("C"));
    cclass.Func(_SC("suspend"), &C::suspend);
    
    RootTable().Bind(_SC("C"), cclass);
    Script script;
    script.CompileString(_SC("\
        c <- C(); \
        //c.suspend(); /* this would fail in the curent Sqrat; no solution yet */\
        ::suspend(); \
        gTest.EXPECT_INT_EQ(1, 0); /* should not reach here */ \
        "));
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Compile Failed: ") << Sqrat::Error::Message(vm);        
    }

    script.Run();
    if (Sqrat::Error::Occurred(vm)) {
        FAIL() << _SC("Run Failed: ") << Sqrat::Error::Message(vm);
    }
    
}