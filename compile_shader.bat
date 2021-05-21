mkdir "./assets/shaders/out"
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V ./assets/shaders/static_scene.vert -o ./assets/shaders/out/static_scene.vert.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V ./assets/shaders/static_scene.frag -o ./assets/shaders/out/static_scene.frag.spv

C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V ./assets/shaders/particles.vert -o ./assets/shaders/out/particles.vert.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V ./assets/shaders/particles.geom -o ./assets/shaders/out/particles.geom.spv
C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V ./assets/shaders/particles.frag -o ./assets/shaders/out/particles.frag.spv

C:/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe -V ./assets/shaders/dir_shadow.vert -o ./assets/shaders/out/dir_shadow.vert.spv