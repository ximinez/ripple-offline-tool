
cmake_minimum_required (VERSION 3.9)
project (ripple-offline-tool)

#[===========================================[
  this pulls-in the rippled source code via
  git submodule. The signer exe depends on the
  xrpl_core library which is defined by the
  rippled project.
#]===========================================]
if (MSVC)
  #[=======================================[
    HACK to patch a one-line windows-only
    bug in rippled CMakeLists.txt. This can
    be removed once rippled 1.2 is available
    and we update the submodule
  #]=======================================]
  execute_process( COMMAND powershell -Command
     "(Get-Content extras/rippled/CMakeLists.txt) \
     -replace '^exclude_if_included\\s+\\(rocksdb\\)', '##' \
     | Set-Content extras/rippled/CMakeLists.txt"
     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif ()
add_subdirectory(extras/rippled)

add_executable (ripple-offline-tool
  src/RippleKey.cpp
  src/Serialize.cpp
  src/OfflineTool.cpp
  ## UNIT TESTS:
  src/test/RippleKey_test.cpp
  src/test/Serialize_test.cpp
  src/test/OfflineTool_test.cpp)
target_include_directories (ripple-offline-tool PRIVATE src)
target_link_libraries (ripple-offline-tool Ripple::xrpl_core)

