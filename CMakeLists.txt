cmake_minimum_required(VERSION 3.20)
project(QLogueLibrarian
    VERSION 0.1.0
    DESCRIPTION "Cross-platform GUI wrapper for KORG logue-sdk/logue-cli tools"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Static build option ───────────────────────────────────────────────────────
# Usage: cmake -B build -DBUILD_STATIC=ON
option(BUILD_STATIC "Link Qt and libc++ statically to produce a standalone binary" OFF)

if(BUILD_STATIC)
    set(BUILD_SHARED_LIBS OFF)
    set(Qt6_USE_STATIC_LIBS ON)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    message(STATUS "Static build enabled — output will be a standalone binary")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick QuickControls2)

# ── miniz (vendored submodule, pinned to tag 3.0.2) ───────────────────────────
# Provides ZIP archive reading without external `unzip` dependency.
add_subdirectory(external/miniz)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/model/UnitInfo.h
    src/model/UnitInfo.cpp
    src/model/PluginListModel.h
    src/model/PluginListModel.cpp
    src/controller/LogueCLIWrapper.h
    src/controller/LogueCLIWrapper.cpp
    src/controller/UnitHeaderParser.h
    src/controller/UnitHeaderParser.cpp
    src/controller/AppController.h
    src/controller/AppController.cpp
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    miniz
)

# GCC/Clang static runtime (applied after target exists)
if(BUILD_STATIC AND NOT MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()
