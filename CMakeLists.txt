cmake_minimum_required(VERSION 3.7)
project(chip8emu VERSION 0.1.0 LANGUAGES C)

if (WIN32)
    set(SDL2_DIR "./SDL2/cmake/")
endif()
    
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})    

add_executable(chip8emu ./src/main.c ./src/chip8.c ./src/tinyfiledialogs.c)
target_link_libraries(chip8emu ${SDL2_LIBRARIES})

if (WIN32)
    set_target_properties(chip8emu PROPERTIES WIN32_EXECUTABLE ON)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
