#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "lexbor::lexbor" for configuration "Debug"
set_property(TARGET lexbor::lexbor APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(lexbor::lexbor PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/lexbor.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/lexbor.dll"
  )

list(APPEND _cmake_import_check_targets lexbor::lexbor )
list(APPEND _cmake_import_check_files_for_lexbor::lexbor "${_IMPORT_PREFIX}/debug/lib/lexbor.lib" "${_IMPORT_PREFIX}/debug/bin/lexbor.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
