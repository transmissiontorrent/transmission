find_package(${CMAKE_FIND_PACKAGE_NAME} QUIET NO_MODULE)

include(FindPackageHandleStandardArgs)
if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
    if(TARGET simdutf AND NOT TARGET simdutf::simdutf)
        add_library(simdutf::simdutf ALIAS simdutf)
    endif()

    find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME} CONFIG_MODE)
    return()
endif()

if(${CMAKE_FIND_PACKAGE_NAME}_PREFER_STATIC_LIB)
    set(${CMAKE_FIND_PACKAGE_NAME}_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
endif()

if(UNIX)
    find_package(PkgConfig QUIET)
    pkg_check_modules(_SIMDUTF QUIET simdutf)
endif()

find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
    NAMES simdutf.h
    HINTS ${_SIMDUTF_INCLUDEDIR})
find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
    NAMES simdutf
    HINTS ${_SIMDUTF_LIBDIR})

if(_SIMDUTF_VERSION)
    set(${CMAKE_FIND_PACKAGE_NAME}_VERSION ${_SIMDUTF_VERSION})
endif()

find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
    REQUIRED_VARS
        ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
        ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
    VERSION_VAR ${CMAKE_FIND_PACKAGE_NAME}_VERSION)

if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
    set(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIRS ${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR})
    set(${CMAKE_FIND_PACKAGE_NAME}_LIBRARIES ${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY})

    if(NOT TARGET simdutf::simdutf)
        add_library(simdutf::simdutf INTERFACE IMPORTED)
        target_include_directories(simdutf::simdutf
            INTERFACE
                ${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR})
        target_link_libraries(simdutf::simdutf
            INTERFACE
                ${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY})
    endif()
endif()

mark_as_advanced(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)

if(${CMAKE_FIND_PACKAGE_NAME}_PREFER_STATIC_LIB)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${${CMAKE_FIND_PACKAGE_NAME}_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    unset(${CMAKE_FIND_PACKAGE_NAME}_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()