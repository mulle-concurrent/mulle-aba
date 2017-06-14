cmake_minimum_required (VERSION 3.0)

project (mulle-aba)

cmake_policy(SET CMP0054 NEW)

#
# dont forget to wipe all cmake produced files before
# changing this
#

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -DDEBUG -DMULLE_ABA_TRACE_LIST -DMULLE_ABA_TRACE_SWAP")

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/mulle-configuration/cmake)
message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  include( Debug)
else()
  include( Release)
endif()


include_directories(
src
)

set(HEADERS
src/mulle_aba_linkedlist.h
src/mulle_aba_storage.h
src/mulle_aba.h)

add_library(mulle_aba
src/mulle_aba_linkedlist.c
src/mulle_aba_storage.c
src/mulle_aba.c
)

find_library( MULLE_ALLOCATOR_LIBRARY      NAMES mulle_allocator)
message(STATUS "MULLE_ALLOCATOR_LIBRARY is ${MULLE_ALLOCATOR_LIBRARY}")
find_library( MULLE_TEST_ALLOCATOR_LIBRARY NAMES mulle_test_allocator)
message(STATUS "MULLE_TEST_ALLOCATOR_LIBRARY is ${MULLE_TEST_ALLOCATOR_LIBRARY}")
find_library( MULLE_THREAD_LIBRARY         NAMES mulle_thread)
message(STATUS "MULLE_THREAD_LIBRARY is ${MULLE_THREAD_LIBRARY}")


set( STATIC_DEPENDENCIES
${MULLE_ALLOCATOR_LIBRARY}
${MULLE_TEST_ALLOCATOR_LIBRARY}
${MULLE_THREAD_LIBRARY}
)

add_executable(mulle_aba_test EXCLUDE_FROM_ALL
tests/punish/punish.c)

add_executable(mulle_aba_ll_test EXCLUDE_FROM_ALL
tests/linkedlist/ll_main.c)

target_link_libraries( mulle_aba_test LINK_PUBLIC
mulle_aba
${STATIC_DEPENDENCIES}
pthread)

target_link_libraries( mulle_aba_ll_test LINK_PUBLIC
mulle_aba
${STATIC_DEPENDENCIES}
pthread)


INSTALL( TARGETS mulle_aba DESTINATION "lib")
INSTALL( FILES ${HEADERS} DESTINATION "include/mulle_aba")

