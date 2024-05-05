project('RHI','cpp','c',default_options: ['cpp_std=c++17','werror=false'])

inc_list = [
    '.','Core/'
    ]
vulkan_src = [
    'Core/Vulkan/Object.cpp',
    'Core/Vulkan/Surface.cpp',
    'Core/Vulkan/VulkanSpecific.cpp',
    'Core/Vulkan/SwapChain.cpp',
    'Core/Vulkan/CommandQueue.cpp',
    'Core/Vulkan/ShaderReflect.cpp',
    'Core/Vulkan/PhysicalDevice.cpp',
    'Core/Vulkan/Barrier.cpp',
    'Core/Vulkan/CommandList.cpp',
    'Core/Vulkan/CommandAllocator.cpp',
    'Core/Vulkan/PipelineStateObject.cpp',
    'Core/Vulkan/Buffer.cpp',
    'Core/Vulkan/Device.cpp',
    'Core/Vulkan/DescriptorHeap.cpp',
    'Core/Vulkan/DebugBuffer.cpp',
    'Core/Vulkan/Fence.cpp',
    'Core/Vulkan/Instance.cpp',
    'Core/Vulkan/volk.c',
    'vendor/spirv_reflect/spirv_reflect.cpp'
]

deps = []
if(get_option('API') == 'Vulkan')

    deps += dependency('vulkan')
    inc_list += ['vendor/spirv_reflect',
    'vendor/VulkanAfterCrash/include',
    'vendor/VulkanMemoryAllocator/include']
endif

inc = include_directories(inc_list)

lib = library('RHI', vulkan_src, include_directories: inc, dependencies: deps)
rhi_dep = declare_dependency(include_directories: inc, link_with: lib)