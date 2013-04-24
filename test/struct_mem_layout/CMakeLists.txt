file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( struct_mem_layout ${SAMPLE_SRC} )
set_target_properties( struct_mem_layout PROPERTIES FOLDER "TestCases" )

install( TARGETS struct_mem_layout RUNTIME DESTINATION bin)
install( FILES "struct_mem_layout.ls" DESTINATION bin)
# Specify the dependencies of library
target_link_libraries( struct_mem_layout ${KSC_MODULE_NAME} )



