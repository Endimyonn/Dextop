cmake_minimum_required(VERSION 3.21)
project(Dextop LANGUAGES CXX)



find_package(Slint QUIET)
if (NOT Slint_FOUND)
  message("Slint could not be located in the CMake module search path. Downloading it from Git and building it locally")
  include(FetchContent)
  FetchContent_Declare(
    Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG release/1
    SOURCE_SUBDIR api/cpp
  )
  FetchContent_MakeAvailable(Slint)
endif (NOT Slint_FOUND)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(${CMAKE_PROJECT_NAME} src/Dextop.cpp src/Dexxor.cpp src/AssetManager.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Slint::Slint)
slint_target_sources(${CMAKE_PROJECT_NAME} ui/DextopPrimaryWindow.slint)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC include)

# console
if(WIN32)
  set_target_properties(${CMAKE_PROJECT_NAME}
  PROPERTIES
    LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
    LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
  )
endif(WIN32)

# On Windows, copy the Slint DLL next to the application binary so that it's found.
if (WIN32)
  add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}> COMMAND_EXPAND_LISTS)
endif()
