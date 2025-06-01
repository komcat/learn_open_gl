# CMakeList.txt : CMake project for learn_open_gl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("learn_open_gl")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW options (disable unnecessary features for faster build)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add GLFW subdirectory
add_subdirectory(thirdparty/glfw)

# Create GLAD library
add_library(glad STATIC
    thirdparty/glad/src/glad.c
)

# GLAD include directories
target_include_directories(glad PUBLIC
    thirdparty/glad/include
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Add source to this project's executable
add_executable(learn_open_gl "learn_open_gl.cpp" "learn_open_gl.h")

# Link libraries
target_link_libraries(learn_open_gl 
    glfw 
    glad
    ${OPENGL_LIBRARIES}
)

# Include directories
target_include_directories(learn_open_gl PRIVATE
    thirdparty/glfw/include
    thirdparty/glad/include
)

# For Windows, you might also need these system libraries
if(WIN32)
    target_link_libraries(learn_open_gl
        opengl32
        gdi32
        user32
        kernel32
    )
endif()

# TODO: Add tests and install targets if needed.