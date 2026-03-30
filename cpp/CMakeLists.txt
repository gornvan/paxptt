cmake_minimum_required(VERSION 3.20)
project(paxp2t_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(X11 REQUIRED)

file(GLOB CPP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(paxp2t-cpp
    ${CPP_SOURCES}
)

target_include_directories(paxp2t-cpp PRIVATE include)

target_link_libraries(paxp2t-cpp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    X11::X11
    Xtst
)

install(TARGETS paxp2t-cpp RUNTIME DESTINATION bin)
