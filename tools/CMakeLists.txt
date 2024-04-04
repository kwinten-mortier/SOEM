
set(SOURCES out.c)
add_executable(replay ${SOURCES})
target_link_libraries(replay soem)
install(TARGETS replay DESTINATION bin)
