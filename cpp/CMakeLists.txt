cmake_minimum_required(VERSION 3.20)
project(paxp2t_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(X11 REQUIRED)

add_executable(paxp2t-cpp
    src/main.cpp
    src/config_manager.cpp
    src/pulse_audio_controller.cpp
    src/sound_controller.cpp
    src/tray_icon_manager.cpp
    src/mouse_binder.cpp
    src/keyboard_binder.cpp
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
