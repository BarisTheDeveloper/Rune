cmake_minimum_required(VERSION 3.20)
project(rune-core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

# Required dependencies
pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.1)
pkg_check_modules(JSC REQUIRED javascriptcoregtk-4.1)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JSON REQUIRED nlohmann_json)

add_subdirectory(src)
