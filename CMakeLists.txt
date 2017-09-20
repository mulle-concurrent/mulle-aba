cmake_minimum_required ( VERSION 3.0)

project( mulle-aba C)

#
# dont forget to wipe all cmake produced files before
# changing this
#

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -DDEBUG -DMULLE_ABA_TRACE_LIST -DMULLE_ABA_TRACE_SWAP")

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/mulle-configuration/cmake)
message( STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")
message( STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  include( Debug)
else()
  include( Release)
endif()


set( MULLE_ABA_LIBRARY ON)

include( CMakeSourcesAndHeaders.txt)
include( CMakeDependencies.txt)


set( PUBLIC_HEADERS
${HEADERS}
CMakeDependencies.txt
)

#
# add definitions for all
#
add_definitions( ${MULLE_ABA_DEFINITIONS})


add_library( mulle_aba
${SOURCES}
)

# PRIVATE is a guess
#target_compile_definitions( mulle_aba
#PRIVATE ${MULLE_ABA_DEFINITIONS}
#)


add_executable( mulle_aba_test EXCLUDE_FROM_ALL
tests/punish/punish.c)

add_executable( mulle_aba_ll_test EXCLUDE_FROM_ALL
tests/linkedlist/ll_main.c)


target_link_libraries( mulle_aba_test LINK_PUBLIC
mulle_aba
${C_DEPENDENCY_LIBRARIES}
${OS_SPECIFIC_LIBRARIES}
)

target_link_libraries( mulle_aba_ll_test LINK_PUBLIC
mulle_aba
${C_DEPENDENCY_LIBRARIES}
${OS_SPECIFIC_LIBRARIES}
)


INSTALL( TARGETS mulle_aba DESTINATION "lib")
INSTALL( FILES ${PUBLIC_HEADERS} DESTINATION "include/mulle_aba")

include( MulleTest)
