cmake_minimum_required(VERSION 3.20)
project(paxp2t_cpp VERSION 0.1.0 LANGUAGES CXX)

option(PAXP2T_RELEASE_MINIMAL "Release: optimize for size (-Os, --gc-sections, --as-needed)" OFF)
if(PAXP2T_RELEASE_MINIMAL)
    # Replace default Release -O3 so -Os is not overridden later on the compile line.
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG -ffunction-sections -fdata-sections" CACHE STRING "C++ flags for Release" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,--as-needed" CACHE STRING "Linker flags for Release exe" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(X11 REQUIRED)

file(GLOB CPP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(paxp2t
    ${CPP_SOURCES}
)

target_include_directories(paxp2t PRIVATE include)

target_link_libraries(paxp2t PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    X11::X11
    Xtst
)

install(TARGETS paxp2t RUNTIME DESTINATION bin)
