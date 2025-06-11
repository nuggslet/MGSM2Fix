#!/bin/sh -ex

#shopt -s nullglob

SQUIRREL_INCLUDE=C:/SQUIRREL3/include
SQUIRREL_LIB=C:/SQUIRREL3/lib
CFLAGS="-g -O0 -I. -I../include -IC:/gtest-1.7.0/include -I${SQUIRREL_INCLUDE}"
LDFLAGS=-L${SQUIRREL_LIB}
LIBS="C:/gtest-1.7.0/build/libgtest.a -lsqstdlib -lsquirrel -lstdc++ -lm"

mkdir -p bin

gcc $CFLAGS \
     ../sqimport/sqratimport.cpp ImportTest.cpp Main.cpp \
     -o bin/ImportTest ${LDFLAGS} ${LIBS}

TEST_CPPS="ClassBinding.cpp\
    ClassInstances.cpp\
    ClassProperties.cpp\
    ConstBindings.cpp\
    FunctionOverload.cpp\
    ScriptLoading.cpp\
    SquirrelFunctions.cpp\
    TableBinding.cpp\
    FunctionParams.cpp \
    RunStackHandling.cpp \
    SuspendVM.cpp \
    NullPointerReturn.cpp\
    FuncInputArgumentType.cpp \
    ArrayBinding.cpp \
    UniqueObject.cpp "

for f in $TEST_CPPS; do
    gcc $CFLAGS \
    ${f} Vector.cpp Main.cpp \
    -o bin/${f%.cpp}  ${LDFLAGS} ${LIBS}
done
