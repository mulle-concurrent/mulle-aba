cmake_minimum_required( VERSION 3.0)

project( mulle-aba C)


### mulle-sde environment

# add cmake module paths to search path
list( INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake/share")
list( INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake")

include( Environment)


### Files

include( PreFiles OPTIONAL)

set( MULLE_ABA_LIBRARY ON)

#
# Include all non underscored .cmake files in ./cmake
#
file( GLOB LOCAL_CMAKE_INCLUDES
   "cmake/[A-Za-z0-9]*.cmake"
)

foreach( FILE ${LOCAL_CMAKE_INCLUDES} )
   include( "${FILE}")
endforeach()


include_directories( ${INCLUDE_DIRS})

include( PostFiles OPTIONAL)

### Library

include( PreLibrary OPTIONAL)

add_library( "mulle-aba"
# definitions from cmake/HeadersAndSources.cmake
${SOURCES}
${PUBLIC_HEADERS}
${PRIVATE_HEADERS}
# definitions from cmake/share/Environment.cmake
${CMAKE_EDITABLE_FILES}
)



if( BUILD_SHARED_LIBS)
   target_link_libraries( mulle-aba
# definitions from cmake/share/DependenciesAndLibraries.cmake
${DEPENDENCY_LIBRARIES}
${OS_SPECIFIC_LIBRARIES}
)
endif()

include( PostLibrary OPTIONAL)

option( STANDALONE "Create standalone library for debugging" OFF)

set( INSTALL_LIBRARY_TARGETS
"mulle-aba"
)

if( STANDALONE)
   set( STANDALONE_BASE_NAME "mulle-aba")
   set( STANDALONE_NAME "mulle-aba-standalone")
   set( STANDALONE_DEFINITIONS ${MULLE_ABA_DEFINITIONS})
   set( STANDALONE_DEPENDENCIES
   mulle-aba
   )

   set( STANDALONE_ALL_LOAD_LIBRARIES
   $<TARGET_FILE:mulle-aba>
   ${DEPENDENCY_LIBRARIES}
   ${OPTIONAL_DEPENDENCY_LIBRARIES}
   ${OS_SPECIFIC_LIBRARIES}
   )

   include( Standalone)

   set( INSTALL_LIBRARY_TARGETS
   ${INSTALL_LIBRARY_TARGETS}
   "mulle-aba-standalone"
   )
endif()

### Install

include( PreInstall OPTIONAL)

install( TARGETS ${INSTALL_LIBRARY_TARGETS} DESTINATION "lib")
install( FILES ${PUBLIC_HEADERS}  DESTINATION "include/mulle-aba")
install( FILES ${PRIVATE_HEADERS} DESTINATION "include/mulle-aba/private")

# definitions from cmake/share/Environment.cmake
install( FILES ${CMAKE_INCLUDES}  DESTINATION "include/mulle-aba/cmake")

# definitions from cmake/share/LinkManifest.cmake
install( FILES ${LINK_MANIFEST_FILES}  DESTINATION "include/mulle-aba/link")

include( PostInstall OPTIONAL)
