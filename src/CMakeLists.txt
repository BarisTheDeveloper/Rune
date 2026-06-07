# Rune Core — the native runtime engine
# Compiles to `rune-core` binary that the CLI launches

set(RUNE_SOURCES
  core/main.cpp
  core/handlers/fs.cpp
  core/handlers/process.cpp
  ipc/server.cpp
  ui/window.cpp
)

set(RUNE_HEADERS
  # Headers will be added as modules are built
)

add_executable(rune-core ${RUNE_SOURCES} ${RUNE_HEADERS})

target_include_directories(rune-core PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${WEBKIT2_INCLUDE_DIRS}
  ${JSC_INCLUDE_DIRS}
  ${GTK3_INCLUDE_DIRS}
  ${JSON_INCLUDE_DIRS}
)

target_link_libraries(rune-core PRIVATE
  ${WEBKIT2_LIBRARIES}
  ${JSC_LIBRARIES}
  ${GTK3_LIBRARIES}
  pthread
)

target_compile_options(rune-core PRIVATE -Wall -Wextra -O2)
