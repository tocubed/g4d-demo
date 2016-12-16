cmake_minimum_required(VERSION 3.5)
project(g4d-demo LANGUAGES CXX)

add_executable(g4d-demo "src/main.cpp")

add_subdirectory(deps)

target_link_libraries(g4d-demo PRIVATE ${G4D_LIBRARIES})
target_link_libraries(g4d-demo PRIVATE ${GLFW_LIBRARIES})
target_link_libraries(g4d-demo PRIVATE ${STB_LIBRARIES})
