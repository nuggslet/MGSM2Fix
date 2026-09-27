cmake_minimum_required(VERSION 3.22)
project(M2FixGUI LANGUAGES C CXX RC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(miniz STATIC vendor/miniz/miniz.c vendor/miniz/miniz_tdef.c vendor/miniz/miniz_tinfl.c vendor/miniz/miniz_zip.c)
target_compile_definitions(miniz PRIVATE _CRT_SECURE_NO_WARNINGS)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(RECIPE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../integral-english/release-recipes.zip")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/recipes.lzms"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/pack_recipes.py" "${RECIPE_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/recipes.lzms"
    DEPENDS "${RECIPE_SOURCE}" pack_recipes.py VERBATIM)
configure_file(app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/app.rc" PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/recipes.lzms;${CMAKE_CURRENT_SOURCE_DIR}/../../MGSM2Fix.ini;${CMAKE_CURRENT_SOURCE_DIR}/../integral-english/CREDITS.md")
add_executable(M2FixGUI WIN32 main.cpp core_base.hpp archive.hpp image.hpp builder.hpp
    core.hpp settings.hpp "${CMAKE_CURRENT_BINARY_DIR}/app.rc" app.manifest)

target_compile_features(M2FixGUI PRIVATE cxx_std_17)
target_compile_definitions(M2FixGUI PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_include_directories(M2FixGUI PRIVATE ../../src/extern/json/include)
target_compile_options(M2FixGUI PRIVATE /utf-8 /W4 /O2 /Gy /Gw)
target_link_libraries(M2FixGUI PRIVATE miniz bcrypt cabinet comctl32 comdlg32 shell32 ole32 uxtheme dwmapi advapi32)
target_link_options(M2FixGUI PRIVATE /OPT:REF /OPT:ICF /MANIFEST:EMBED)
set(M2FIXGUI_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../dist" CACHE PATH
    "Directory for the M2FixGUI executable")
set_target_properties(M2FixGUI PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${M2FIXGUI_OUTPUT_DIR}")

include(CTest)
if(BUILD_TESTING)
    add_executable(M2FixGUI_tests tests.cpp core_base.hpp archive.hpp image.hpp builder.hpp
        core.hpp settings.hpp "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    target_compile_features(M2FixGUI_tests PRIVATE cxx_std_17)
    target_compile_definitions(M2FixGUI_tests PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(M2FixGUI_tests PRIVATE ../../src/extern/json/include)
    target_compile_options(M2FixGUI_tests PRIVATE /utf-8 /W4)
    target_link_libraries(M2FixGUI_tests PRIVATE miniz bcrypt cabinet advapi32)
    add_test(NAME M2FixGUI_native COMMAND M2FixGUI_tests)
endif()
