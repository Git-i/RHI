project('RHI','cpp','c',default_options: ['cpp_std=c++17','werror=false'])

cmake = import('cmake')

glfw_opt = cmake.subproject_options()
glfw_opt.add_cmake_defines({'GLFW_BUILD_WAYLAND':get_option('linux_use_wayland')})
glfw_opt.add_cmake_defines({'GLFW_BUILD_X11':get_option('linux_use_x11')})
glfw3 = cmake.subproject('glfw',options: glfw_opt)
glfw3_dep = glfw3.dependency('glfw')

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
    'Util/Vulkan/FormatUtils.cpp',
    'vendor/spirv_reflect/spirv_reflect.cpp'
]

ezr = subproject('ezr')
ezr_dep = ezr.get_variable('ezr_dep')

deps = [ezr_dep]
if(get_option('API') == 'Vulkan')

    deps += dependency('vulkan')
    inc_list += ['vendor/spirv_reflect',
    'vendor/VulkanAfterCrash/include',
    'vendor/VulkanMemoryAllocator/include']
endif

inc = include_directories(inc_list)
debug(get_option('linux_window_subsys'))
if(build_machine.system() == 'linux')
    if(get_option('linux_window_subsys') == 'xcb')
        add_project_arguments('-DUSE_XCB',language: 'cpp')
        add_project_arguments('-DVK_USE_PLATFORM_XCB_KHR',language: 'cpp')
    elif(get_option('linux_window_subsys') == 'wayland')
        add_project_arguments('-DUSE_WAYLAND',language: 'cpp')
        add_project_arguments('-DVK_USE_PLATFORM_WAYLAND_KHR',language: 'cpp')
    elif(get_option('linux_window_subsys') == 'glfw')
        add_project_arguments('-DUSE_GLFW',language: 'cpp')
        deps += glfw3_dep
    endif
endif
if(build_machine.system() == 'windows')
    add_project_arguments('-DVK_USE_PLATFORM_WIN32_KHR',language: 'cpp')
endif
lib = library('RHI', vulkan_src, include_directories: inc, dependencies: deps)
rhi_dep = declare_dependency(include_directories: inc, link_with: lib, dependencies: deps)