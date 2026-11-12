# Build sin motor del núcleo de reglas (ArpgCore) y sus pruebas.
# El juego completo se compila con Unreal Build Tool; ver Docs/BUILD_Y_DISTRIBUCION.md.
cmake_minimum_required(VERSION 3.20)
project(ArpgCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CORE_DIR ${CMAKE_SOURCE_DIR}/Game/Source/ArpgCore)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS ${CORE_DIR}/Private/*.cpp)
list(FILTER CORE_SOURCES EXCLUDE REGEX "ArpgCoreModule\\.cpp$")

add_library(ArpgCore STATIC ${CORE_SOURCES})
target_include_directories(ArpgCore PUBLIC ${CORE_DIR}/Public)

# Mismas restricciones que impone Unreal: sin excepciones ni RTTI.
if(MSVC)
  target_compile_options(ArpgCore PUBLIC /W4 /WX /GR- /permissive-)
  target_compile_definitions(ArpgCore PUBLIC _HAS_EXCEPTIONS=0)
else()
  target_compile_options(ArpgCore PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
                                         -fno-exceptions -fno-rtti)
endif()

enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/Tests/ArpgCore/*.cpp)
add_executable(ArpgCoreTests ${TEST_SOURCES})
target_link_libraries(ArpgCoreTests PRIVATE ArpgCore)
add_test(NAME ArpgCoreTests COMMAND ArpgCoreTests)
