file( GLOB_RECURSE SAMPLE_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_executable( generic_tests ${SAMPLE_SRC} )
set_target_properties( generic_tests PROPERTIES FOLDER "TestCases" )

install( TARGETS generic_tests RUNTIME DESTINATION bin)

install( FILES "test_00.ls" DESTINATION bin)
install( FILES "test_01.ls" DESTINATION bin)
install( FILES "test_02.ls" DESTINATION bin)

# Specify the dependencies of library
target_link_libraries( generic_tests ${KSC_MODULE_NAME} )



