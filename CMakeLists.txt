cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(Vision)
set(CMAKE_CXX_STANDARD 20)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache) # Less useful to do it for linking, see edit2
endif(CCACHE_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O4 -g -fdiagnostics-color -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_LINK_FLAGS} -O4 -g -fdiagnostics-color -Wall -Wextra -fPIC")


# ADD_DEFINITIONS(-DSDL_DISPLAY=1)


find_library(libfftw3-3 REQUIRED)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
# set(SDL_SHARED OFF CACHE BOOL "" FORCE)
# add_subdirectory(external/SDL)

# set(SDL_IMAGE_STATIC_PIC ON CACHE BOOL "" FORCE)
# set(SDL_IMAGE_SHARED OFF CACHE BOOL "" FORCE)
# add_subdirectory(external/SDL_image)


# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE RELEASE CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
# endif()

add_executable(Vision
main.cpp
)

if (UNIX)
  target_link_libraries(Vision
    SDL2 SDL2_image
    ${OpenCV_LIBS}
  )
endif()

if (WIN32)
endif()
