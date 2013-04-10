file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( basic_expressions ${SAMPLE_SRC} )
set_target_properties( basic_expressions PROPERTIES FOLDER "TestCases" )

install( TARGETS basic_expressions RUNTIME DESTINATION bin)
install( FILES "basic_expressions.ls" DESTINATION bin)
# Specify the dependencies of library
target_link_libraries( basic_expressions ${KSC_MODULE_NAME} )



