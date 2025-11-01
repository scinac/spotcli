cmake_minimum_required(VERSION 3.14)
project(spotcli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

find_package(CURL REQUIRED)
find_package(Curses REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/playlist.cpp
    src/token_repo.cpp
    src/authSpotify.cpp
    src/utils.cpp
    src/playback.cpp
)

include_directories(${JSONCPP_INCLUDE_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    CURL::libcurl
    ${JSONCPP_LIBRARIES}
    ${CURSES_LIBRARIES}
)
