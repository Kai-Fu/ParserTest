file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( recursive_call ${SAMPLE_SRC} )
set_target_properties( recursive_call PROPERTIES FOLDER "TestCases" )

install( TARGETS recursive_call RUNTIME DESTINATION bin)
install( FILES "recursive_call.ls" DESTINATION bin)
# Specify the dependencies of library
target_link_libraries( recursive_call ${KSC_MODULE_NAME} )



