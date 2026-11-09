cmake_minimum_required(VERSION 3.21)
project(SomeLensSDL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The demo is built when this is the top project, not when another project takes the library in (FetchContent,
# add_subdirectory).
option(SOMELENS_BUILD_DEMO "Build the demo" ${PROJECT_IS_TOP_LEVEL})

find_package(SDL3 3.4 CONFIG REQUIRED)

# The library: glass lenses over any content drawn with SDL's GPU renderer.
add_library(somelens STATIC
    src/glass_lens.cpp
    src/lens_container.cpp
    src/lens_morph.cpp
    src/lens_shaders.cpp
    src/lens_settings.cpp
    src/shader_program.cpp
)
target_include_directories(somelens
    PUBLIC include
    PRIVATE src
)
target_link_libraries(somelens PUBLIC SDL3::SDL3)
target_compile_features(somelens PUBLIC cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(somelens PRIVATE -Wall -Wextra -Wpedantic)
endif()

# The demo: the same scene as the reference one, to compare against.
if(SOMELENS_BUILD_DEMO)
    find_package(SDL3_ttf CONFIG REQUIRED)
    add_executable(somelens_demo
        demo/main.cpp
        demo/demo_background.cpp
        demo/help_panel.cpp
    )
    target_include_directories(somelens_demo PRIVATE src)
    target_link_libraries(somelens_demo PRIVATE somelens SDL3_ttf::SDL3_ttf)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(somelens_demo PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
