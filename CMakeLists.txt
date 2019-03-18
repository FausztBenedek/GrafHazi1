cmake_minimum_required(VERSION 3.1)

set(projectName "openglInit")
project(${projectName})

set (CMAKE_CXX_STANDARD 11)

add_executable(${projectName} framework.cpp Skeleton.cpp)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)

include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})

target_link_libraries(${projectName} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${GLEW_LIBRARY})
