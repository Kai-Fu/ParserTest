cmake_minimum_required (VERSION 2.6)
project (KShaderCompiler)

# The version number.
set( KSC_VERSION_MAJOR 1 )
set( KSC_VERSION_MINOR 0 )

# module names of KRT
set( KSC_MODULE_NAME "KShaderCompiler" )

include_directories( "src" )
# Add the KSC modules
add_subdirectory( src )
add_subdirectory( test/external_function )




