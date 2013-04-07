file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( external_function ${SAMPLE_SRC} )

install( TARGETS external_function RUNTIME DESTINATION bin CONFIGURATIONS)
# Specify the dependencies of library
target_link_libraries( external_function ${KSC_MODULE_NAME} )



