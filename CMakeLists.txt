cmake_minimum_required (VERSION 3.11)
project (ripple-offline)

#[===========================================[
  The tool depends on the libxrpl
  library which is defined by the rippled
  project. This looks for installed rippled
  libs and, if not found, pulls them in with
  FetchContent.
  (See also: https://github.com/ripple/validator-keys-tool/blob/master/CMakeLists.txt)
#]===========================================]
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(OfflineNIH)

get_directory_property(has_parent PARENT_DIRECTORY)
if(coverage)
  # Rippled also responds to the "coverage" flag, so clear it if it's set until
  # the dependency is set up.
  set(offline_coverage ${coverage})
  set(coverage OFF CACHE BOOL "gcc/clang only" FORCE)
endif()
if (NOT has_parent)
  find_package(xrpl REQUIRED)
  if (NOT TARGET xrpl::libxrpl)

    find_package(Git)

    if (NOT GIT_FOUND)
      message (FATAL_ERROR "git is required to determine branch name")
    endif ()

    execute_process (COMMAND ${GIT_EXECUTABLE} "rev-parse" "--abbrev-ref" "HEAD"
                     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                     RESULT_VARIABLE _git_exit_code
                     OUTPUT_VARIABLE _branch
                     OUTPUT_STRIP_TRAILING_WHITESPACE
                     ERROR_QUIET)
    if (NOT _git_exit_code EQUAL 0)
      message (WARNING "git command failed - deduced branch might be incorrect")
    endif ()

    # rippled_tag is cache string and can be overriden when configuring
    # with -Drippled_tag=commit_or_tag in order to pick a specific
    # rippled version to download. Default tag is develop/master/release as
    # determined by the branch of this project
    if (NOT (_branch STREQUAL "master" OR _branch STREQUAL "release"))
      set (rippled_tag "develop" CACHE STRING
        "tag/commit of rippled to fetch from if a local install is not found")
    else ()
      set (rippled_tag "${_branch}" CACHE STRING
        "tag/commit of rippled to fetch from if a local install is not found")
    endif ()
    message(STATUS "Installed rippled not found... \
      using local copy from tag/commit [${rippled_tag}]")
    include (FetchContent)
    FetchContent_Declare(
      rippled_src
      GIT_REPOSITORY https://github.com/ripple/rippled.git
      GIT_TAG ${rippled_tag}
    )
    FetchContent_GetProperties(rippled_src)
    if(NOT rippled_src_POPULATED)
      message (STATUS "Pausing to download rippled source...")
      FetchContent_Populate(rippled_src)
      add_subdirectory(${rippled_src_SOURCE_DIR} ${rippled_src_BINARY_DIR})
    endif()
  endif ()
endif ()
if(offline_coverage)
  set(coverage ${offline_coverage} CACHE BOOL "gcc/clang only" FORCE)
endif()
include(OfflineSanity)
include(OfflineCov)
include(OfflineInterface)

add_executable (ripple-offline
  src/RippleKey.cpp
  src/Serialize.cpp
  src/OfflineTool.cpp
  # UNIT TESTS:
  src/test/RippleKey_test.cpp
  src/test/Serialize_test.cpp
  src/test/OfflineTool_test.cpp)
target_include_directories (ripple-offline PRIVATE src)
target_link_libraries (ripple-offline xrpl::libxrpl Offline::opts)

if (has_parent)
  set_target_properties (ripple-offline PROPERTIES EXCLUDE_FROM_ALL ON)
  set_target_properties (ripple-offline PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
endif ()

include(CTest)
if(BUILD_TESTING)
  add_test(test ripple-offline --unittest)
endif()
