cmake_minimum_required (VERSION 3.0)

project (mulle-aba)

#
# dont forget to wipe all cmake produced files before
# changing this
#

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   set (CMAKE_C_FLAGS "-Wno-parantheses")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
   set (CMAKE_C_FLAGS "-fno-color-diagnostics")
endif()


#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -DDEBUG -DTRACE_LIST -DTRACE_SWAP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g -DNDEBUG")


add_library(mulle_aba
src/mulle_callback_types.c
src/mulle_aba_linked_list.c
src/mulle_aba_storage.c
src/mulle_aba.c
mintomic/src/mintomic/mintomic_gcc.c
)


add_executable(mulle_aba_test
src/main.c)

add_executable(mulle_aba_ll_test
src/ll_main.c)

include_directories( AFTER SYSTEM mintomic/include)
target_include_directories( mulle_aba_test PUBLIC mintomic/include)
target_include_directories( mulle_aba_ll_test PUBLIC mintomic/include)

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries( mulle_aba_test LINK_PUBLIC mulle_aba
pthread)
target_link_libraries( mulle_aba_ll_test LINK_PUBLIC mulle_aba
pthread)
