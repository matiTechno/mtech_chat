add_executable(mtech_chat client.cpp message.hpp sound_system.cpp
    sound_system.hpp res_class.hpp)
target_link_libraries(mtech_chat -lSDL2_mixer -lSDL2)
add_executable(my_server server.cpp)
file(COPY res DESTINATION .)
