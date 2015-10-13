cmake_minimum_required (VERSION 3.0)

project (mulle-aba)

#
# dont forget to wipe all cmake produced files before
# changing this
#

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -DDEBUG -DMULLE_ABA_TRACE_LIST -DMULLE_ABA_TRACE_SWAP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g -DNDEBUG")

set(HEADERS
src/mulle_aba_linked_list.h
src/mulle_aba_storage.h
src/mulle_aba.h)

add_library(mulle_aba
src/mulle_aba_linked_list.c
src/mulle_aba_storage.c
src/mulle_aba.c
)

add_executable(mulle_aba_test EXCLUDE_FROM_ALL
src/main.c)

add_executable(mulle_aba_ll_test EXCLUDE_FROM_ALL
src/ll_main.c)

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries( mulle_aba_test LINK_PUBLIC mulle_aba
pthread)
target_link_libraries( mulle_aba_ll_test LINK_PUBLIC mulle_aba
pthread)


INSTALL(TARGETS mulle_aba DESTINATION "lib")
INSTALL(FILES ${HEADERS} DESTINATION "include/mulle_aba")

