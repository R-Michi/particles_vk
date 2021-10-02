# requiered CMAKE version to build the project
cmake_minimum_required (VERSION 3.8)

# current project
project("particles")

# use C++ 17
set(CMAKE_CXX_STANDARD 17)

# used include directories for libraries
include_directories(
    "C:/VulkanSDK/1.2.170.0/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw-3.3.3/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/stb_master"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/tiny_obj_loader"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vka-1.1.0/include"
)

# linker search directories for libraries
link_directories(
    "C:/VulkanSDK/1.2.170.0/Lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw-3.3.3/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/glm/glm/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vka-1.1.0/lib"
)

# compile and link executable
add_executable(particles 
    "main.cpp" 
    "VulkanApp.cpp" 
    "random/random.cpp" 
    "application/onscreen.cpp" 
    "application/shadow_map.cpp"
    "application/descriptor_manager.cpp" 
    "particles/particle_types.cpp" 
    "particles/particle_renderer_init.cpp" 
    "particles/particle_renderer_other.cpp"
    "particles/particle_pool.cpp"
 "main_application.cpp" "particles/particle_engine.cpp" "particles/static_particle_engine.cpp")

target_link_libraries(particles PRIVATE
    "-lvulkan_abstraction"
    "-lglm_static" 
    "-lglfw3" 
    "-lvulkan-1"
)

# custom command to compile shaders while compiling the program
add_custom_command(
    TARGET particles
    PRE_BUILD COMMAND "compile_shader.bat"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# export compiler commands
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
