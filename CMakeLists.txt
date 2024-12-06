cmake_minimum_required(VERSION 3.15)
#set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
set(CMP0091 NEW)
set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(PyMesh)

include(Settings.cmake)

# Process code in each subdirectories
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(python)

add_subdirectory(tests EXCLUDE_FROM_ALL)
#add_subdirectory(examples EXCLUDE_FROM_ALL)
