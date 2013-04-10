file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( for_expression ${SAMPLE_SRC} )
set_target_properties( for_expression PROPERTIES FOLDER "TestCases" )

install( TARGETS for_expression RUNTIME DESTINATION bin)
install( FILES "for_expression.ls" DESTINATION bin)
# Specify the dependencies of library
target_link_libraries( for_expression ${KSC_MODULE_NAME} )



