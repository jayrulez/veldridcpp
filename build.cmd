@setlocal
@echo off

if not exist "bin" mkdir bin
clang++ -shared src/veldridcpp/*.cpp -o bin/veldridcpp.dll -IC:\VulkanSDK\1.1.70.1\Include\vulkan -LC:\VulkanSDK\1.1.70.1\Lib -lvulkan-1 -D _WINDOWS=1 -Xclang -flto-visibility-public-std
