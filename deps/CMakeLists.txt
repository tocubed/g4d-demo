add_subdirectory(g4d)
set(G4D_LIBRARIES ${G4D_LIBRARIES} PARENT_SCOPE)


set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
set(GLFW_VULKAN_STATIC OFF CACHE BOOL "Use the Vulkan loader statically linked into application")

add_subdirectory(glfw)
set(GLFW_LIBRARIES glfw PARENT_SCOPE)


list(APPEND STB_SOURCES 
	"${CMAKE_CURRENT_SOURCE_DIR}/stb_image.cpp"
)
file(GLOB STB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/stb/*.h")

set(STB_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/stb/include")
add_custom_command(OUTPUT ${STB_INCLUDE_DIR}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${STB_INCLUDE_DIR}/stb" 
	COMMAND ${CMAKE_COMMAND} -E copy ${STB_HEADERS} "${STB_INCLUDE_DIR}/stb"
	DEPENDS ${STB_HEADERS}
	COMMENT "Isolate stb headers from its source tree")
add_custom_target(stb_headers DEPENDS ${STB_INCLUDE_DIR})

add_library(stb STATIC ${STB_SOURCES})
add_dependencies(stb stb_headers)
target_include_directories(stb PUBLIC ${STB_INCLUDE_DIR})

set(STB_LIBRARIES stb PARENT_SCOPE)
