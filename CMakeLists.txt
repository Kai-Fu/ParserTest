cmake_minimum_required (VERSION 2.6)
project (KShaderCompiler)

# Let Visual Studio arrange the projects by folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# supported build types
set( ${CMAKE_BUILD_TYPE} "Debug" "Release" )

# The version number.
set( KSC_VERSION_MAJOR 1 )
set( KSC_VERSION_MINOR 0 )

# module names of KRT
set( KSC_MODULE_NAME "KShaderCompiler" )

include_directories( "src" )
# Add the KSC modules
add_subdirectory( src )
add_subdirectory( test/external_function )
add_subdirectory( test/recursive_call )
add_subdirectory( test/for_expression )
add_subdirectory( test/basic_expressions )
add_subdirectory( test/generic_tests )
add_subdirectory( test/struct_mem_layout )




