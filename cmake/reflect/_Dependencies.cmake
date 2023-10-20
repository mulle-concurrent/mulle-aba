# This file will be regenerated by `mulle-sourcetree-to-cmake` via
# `mulle-sde reflect` and any edits will be lost.
#
# This file will be included by cmake/share/Files.cmake
#
# Disable generation of this file with:
#
# mulle-sde environment set MULLE_SOURCETREE_TO_CMAKE_DEPENDENCIES_FILE DISABLE
#
if( MULLE_TRACE_INCLUDE)
   message( STATUS "# Include \"${CMAKE_CURRENT_LIST_FILE}\"" )
endif()

#
# Generated from sourcetree: 9DB39403-9DF3-4E29-8871-E8D6023B9334;mulle-allocator;no-all-load,no-cmake-inherit,no-import,no-singlephase;
# Disable with : `mulle-sourcetree mark mulle-allocator no-link`
# Disable for this platform: `mulle-sourcetree mark mulle-allocator no-cmake-platform-${MULLE_UNAME}`
# Disable for a sdk: `mulle-sourcetree mark mulle-allocator no-cmake-sdk-<name>`
#
if( NOT MULLE_ALLOCATOR_LIBRARY)
   find_library( MULLE_ALLOCATOR_LIBRARY NAMES
      ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-allocator${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-allocator${CMAKE_STATIC_LIBRARY_SUFFIX}
      mulle-allocator
      NO_CMAKE_SYSTEM_PATH NO_SYSTEM_ENVIRONMENT_PATH
   )
   if( NOT MULLE_ALLOCATOR_LIBRARY AND NOT DEPENDENCY_IGNORE_SYSTEM_LIBARIES)
      find_library( MULLE_ALLOCATOR_LIBRARY NAMES
         ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-allocator${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
         ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-allocator${CMAKE_STATIC_LIBRARY_SUFFIX}
         mulle-allocator
      )
   endif()
   message( STATUS "MULLE_ALLOCATOR_LIBRARY is ${MULLE_ALLOCATOR_LIBRARY}")
   #
   # The order looks ascending, but due to the way this file is read
   # it ends up being descending, which is what we need.
   #
   if( MULLE_ALLOCATOR_LIBRARY)
      #
      # Add MULLE_ALLOCATOR_LIBRARY to DEPENDENCY_LIBRARIES list.
      # Disable with: `mulle-sourcetree mark mulle-allocator no-cmake-add`
      #
      list( APPEND DEPENDENCY_LIBRARIES ${MULLE_ALLOCATOR_LIBRARY})
      # intentionally left blank
   else()
      # Disable with: `mulle-sourcetree mark mulle-allocator no-require-link`
      message( FATAL_ERROR "MULLE_ALLOCATOR_LIBRARY was not found")
   endif()
endif()


#
# Generated from sourcetree: FDD72DA6-03FE-41C8-AC5E-BDC4E891E0E3;mulle-thread;no-all-load,no-cmake-searchpath,no-import,no-singlephase;
# Disable with : `mulle-sourcetree mark mulle-thread no-link`
# Disable for this platform: `mulle-sourcetree mark mulle-thread no-cmake-platform-${MULLE_UNAME}`
# Disable for a sdk: `mulle-sourcetree mark mulle-thread no-cmake-sdk-<name>`
#
if( NOT MULLE_THREAD_LIBRARY)
   find_library( MULLE_THREAD_LIBRARY NAMES
      ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-thread${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-thread${CMAKE_STATIC_LIBRARY_SUFFIX}
      mulle-thread
      NO_CMAKE_SYSTEM_PATH NO_SYSTEM_ENVIRONMENT_PATH
   )
   if( NOT MULLE_THREAD_LIBRARY AND NOT DEPENDENCY_IGNORE_SYSTEM_LIBARIES)
      find_library( MULLE_THREAD_LIBRARY NAMES
         ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-thread${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
         ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-thread${CMAKE_STATIC_LIBRARY_SUFFIX}
         mulle-thread
      )
   endif()
   message( STATUS "MULLE_THREAD_LIBRARY is ${MULLE_THREAD_LIBRARY}")
   #
   # The order looks ascending, but due to the way this file is read
   # it ends up being descending, which is what we need.
   #
   if( MULLE_THREAD_LIBRARY)
      #
      # Add MULLE_THREAD_LIBRARY to DEPENDENCY_LIBRARIES list.
      # Disable with: `mulle-sourcetree mark mulle-thread no-cmake-add`
      #
      list( APPEND DEPENDENCY_LIBRARIES ${MULLE_THREAD_LIBRARY})
      #
      # Inherit information from dependency.
      # Encompasses: no-cmake-searchpath,no-cmake-dependency,no-cmake-loader
      # Disable with: `mulle-sourcetree mark mulle-thread no-cmake-inherit`
      #
      # temporarily expand CMAKE_MODULE_PATH
      get_filename_component( _TMP_MULLE_THREAD_ROOT "${MULLE_THREAD_LIBRARY}" DIRECTORY)
      get_filename_component( _TMP_MULLE_THREAD_ROOT "${_TMP_MULLE_THREAD_ROOT}" DIRECTORY)
      #
      #
      # Search for "Definitions.cmake" and "DependenciesAndLibraries.cmake" to include.
      # Disable with: `mulle-sourcetree mark mulle-thread no-cmake-dependency`
      #
      foreach( _TMP_MULLE_THREAD_NAME "mulle-thread")
         set( _TMP_MULLE_THREAD_DIR "${_TMP_MULLE_THREAD_ROOT}/include/${_TMP_MULLE_THREAD_NAME}/cmake")
         # use explicit path to avoid "surprises"
         if( IS_DIRECTORY "${_TMP_MULLE_THREAD_DIR}")
            list( INSERT CMAKE_MODULE_PATH 0 "${_TMP_MULLE_THREAD_DIR}")
            # we only want top level INHERIT_OBJC_LOADERS, so disable them
            if( NOT NO_INHERIT_OBJC_LOADERS)
               set( NO_INHERIT_OBJC_LOADERS OFF)
            endif()
            list( APPEND _TMP_INHERIT_OBJC_LOADERS ${NO_INHERIT_OBJC_LOADERS})
            set( NO_INHERIT_OBJC_LOADERS ON)
            #
            include( "${_TMP_MULLE_THREAD_DIR}/DependenciesAndLibraries.cmake" OPTIONAL)
            #
            list( GET _TMP_INHERIT_OBJC_LOADERS -1 NO_INHERIT_OBJC_LOADERS)
            list( REMOVE_AT _TMP_INHERIT_OBJC_LOADERS -1)
            list( REMOVE_ITEM CMAKE_MODULE_PATH "${_TMP_MULLE_THREAD_DIR}")
            #
            unset( MULLE_THREAD_DEFINITIONS)
            include( "${_TMP_MULLE_THREAD_DIR}/Definitions.cmake" OPTIONAL)
            list( APPEND INHERITED_DEFINITIONS ${MULLE_THREAD_DEFINITIONS})
            break()
         else()
            message( STATUS "${_TMP_MULLE_THREAD_DIR} not found")
         endif()
      endforeach()
   else()
      # Disable with: `mulle-sourcetree mark mulle-thread no-require-link`
      message( FATAL_ERROR "MULLE_THREAD_LIBRARY was not found")
   endif()
endif()


#
# Generated from sourcetree: 7ACE8853-C4E5-499A-951D-A9CDCC85785F;mulle-linkedlist;no-all-load,no-cmake-loader,no-cmake-searchpath,no-import;
# Disable with : `mulle-sourcetree mark mulle-linkedlist no-link`
# Disable for this platform: `mulle-sourcetree mark mulle-linkedlist no-cmake-platform-${MULLE_UNAME}`
# Disable for a sdk: `mulle-sourcetree mark mulle-linkedlist no-cmake-sdk-<name>`
#
if( NOT MULLE_LINKEDLIST_LIBRARY)
   find_library( MULLE_LINKEDLIST_LIBRARY NAMES
      ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-linkedlist${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-linkedlist${CMAKE_STATIC_LIBRARY_SUFFIX}
      mulle-linkedlist
      NO_CMAKE_SYSTEM_PATH NO_SYSTEM_ENVIRONMENT_PATH
   )
   if( NOT MULLE_LINKEDLIST_LIBRARY AND NOT DEPENDENCY_IGNORE_SYSTEM_LIBARIES)
      find_library( MULLE_LINKEDLIST_LIBRARY NAMES
         ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-linkedlist${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
         ${CMAKE_STATIC_LIBRARY_PREFIX}mulle-linkedlist${CMAKE_STATIC_LIBRARY_SUFFIX}
         mulle-linkedlist
      )
   endif()
   message( STATUS "MULLE_LINKEDLIST_LIBRARY is ${MULLE_LINKEDLIST_LIBRARY}")
   #
   # The order looks ascending, but due to the way this file is read
   # it ends up being descending, which is what we need.
   #
   if( MULLE_LINKEDLIST_LIBRARY)
      #
      # Add MULLE_LINKEDLIST_LIBRARY to DEPENDENCY_LIBRARIES list.
      # Disable with: `mulle-sourcetree mark mulle-linkedlist no-cmake-add`
      #
      list( APPEND DEPENDENCY_LIBRARIES ${MULLE_LINKEDLIST_LIBRARY})
      #
      # Inherit information from dependency.
      # Encompasses: no-cmake-searchpath,no-cmake-dependency,no-cmake-loader
      # Disable with: `mulle-sourcetree mark mulle-linkedlist no-cmake-inherit`
      #
      # temporarily expand CMAKE_MODULE_PATH
      get_filename_component( _TMP_MULLE_LINKEDLIST_ROOT "${MULLE_LINKEDLIST_LIBRARY}" DIRECTORY)
      get_filename_component( _TMP_MULLE_LINKEDLIST_ROOT "${_TMP_MULLE_LINKEDLIST_ROOT}" DIRECTORY)
      #
      #
      # Search for "Definitions.cmake" and "DependenciesAndLibraries.cmake" to include.
      # Disable with: `mulle-sourcetree mark mulle-linkedlist no-cmake-dependency`
      #
      foreach( _TMP_MULLE_LINKEDLIST_NAME "mulle-linkedlist")
         set( _TMP_MULLE_LINKEDLIST_DIR "${_TMP_MULLE_LINKEDLIST_ROOT}/include/${_TMP_MULLE_LINKEDLIST_NAME}/cmake")
         # use explicit path to avoid "surprises"
         if( IS_DIRECTORY "${_TMP_MULLE_LINKEDLIST_DIR}")
            list( INSERT CMAKE_MODULE_PATH 0 "${_TMP_MULLE_LINKEDLIST_DIR}")
            #
            include( "${_TMP_MULLE_LINKEDLIST_DIR}/DependenciesAndLibraries.cmake" OPTIONAL)
            #
            list( REMOVE_ITEM CMAKE_MODULE_PATH "${_TMP_MULLE_LINKEDLIST_DIR}")
            #
            unset( MULLE_LINKEDLIST_DEFINITIONS)
            include( "${_TMP_MULLE_LINKEDLIST_DIR}/Definitions.cmake" OPTIONAL)
            list( APPEND INHERITED_DEFINITIONS ${MULLE_LINKEDLIST_DEFINITIONS})
            break()
         else()
            message( STATUS "${_TMP_MULLE_LINKEDLIST_DIR} not found")
         endif()
      endforeach()
   else()
      # Disable with: `mulle-sourcetree mark mulle-linkedlist no-require-link`
      message( FATAL_ERROR "MULLE_LINKEDLIST_LIBRARY was not found")
   endif()
endif()
