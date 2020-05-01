add_executable(dostest-server
    dostest-server.c
    StreamBuffer.c
)
target_link_libraries(dostest-server base system)

add_executable(dostest-attacker
    dostest-attacker.c
)
target_link_libraries(dostest-attacker base system)
