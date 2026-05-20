cmake_minimum_required(VERSION 3.16)
project(WiimComsWin CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    ArduinoJson
    GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
    GIT_TAG        v7.0.4
)
FetchContent_MakeAvailable(ArduinoJson)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SRC_ROOT  ${REPO_ROOT}/src)
set(INC_ROOT  ${REPO_ROOT}/include)

add_executable(wiim_win
    ${SRC_ROOT}/WiimService.cpp
    ${SRC_ROOT}/models/DeviceStatus.cpp
    ${SRC_ROOT}/platform/win/WinHttpClient.cpp
    ${SRC_ROOT}/platform/win/main_win.cpp
)

target_include_directories(wiim_win PRIVATE
    ${SRC_ROOT}
    ${INC_ROOT}
)

target_link_libraries(wiim_win PRIVATE
    ArduinoJson
    winhttp
)

if(MSVC)
    target_compile_options(wiim_win PRIVATE
        /W3
        /utf-8
        /Zc:__cplusplus
        /Zc:preprocessor
    )
    target_compile_definitions(wiim_win PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
endif()
