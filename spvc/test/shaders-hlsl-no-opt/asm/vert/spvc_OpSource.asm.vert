          OpCapability Shader
          OpCapability VulkanMemoryModelKHR
          OpExtension "SPV_KHR_vulkan_memory_model"
          OpMemoryModel Logical VulkanKHR
          OpEntryPoint Vertex %func "shader"
          OpSource HLSL 500
%void   = OpTypeVoid
%main = OpTypeFunction %void
%func   = OpFunction %void None %main
%label  = OpLabel
          OpReturn
          OpFunctionEnd
