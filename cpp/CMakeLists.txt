cmake_minimum_required(VERSION 3.20)
project(p2td VERSION 0.1.0 LANGUAGES CXX)

option(p2td_RELEASE_MINIMAL "Release: optimize for size (-Os, --gc-sections, --as-needed)" OFF)

set(p2td_HAVE_RELEASE_IPO FALSE)
if(p2td_RELEASE_MINIMAL)
    # Replace default Release -O3 so -Os is not overridden later on the compile line.
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG -ffunction-sections -fdata-sections" CACHE STRING "C++ flags for Release" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,--as-needed" CACHE STRING "Linker flags for Release exe" FORCE)

    cmake_policy(SET CMP0069 NEW)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT p2td_HAVE_RELEASE_IPO OUTPUT p2td_IPO_DIAG LANGUAGES CXX)
    if(NOT p2td_HAVE_RELEASE_IPO)
        message(STATUS "IPO/LTO not enabled for p2td: ${p2td_IPO_DIAG}")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse-simple)

file(GLOB CPP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(p2td
    ${CPP_SOURCES}
)

target_include_directories(p2td PRIVATE
    include
    "${CMAKE_CURRENT_SOURCE_DIR}/../.github/scripts/vendor"
)

target_link_libraries(p2td PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    X11::X11
    Xtst
    PkgConfig::PULSE
)

if(p2td_RELEASE_MINIMAL AND p2td_HAVE_RELEASE_IPO)
    set_property(TARGET p2td PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS p2td RUNTIME DESTINATION bin)
