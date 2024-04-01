
set(SOURCES loop_test.c)
add_executable(loop_test ${SOURCES})
target_link_libraries(loop_test soem)
install(TARGETS loop_test DESTINATION bin)
