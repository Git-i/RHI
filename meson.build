project('RHI','cpp','c',default_options: ['cpp_std=c++20','werror=false'])
pymodule = import('python')
python3 = pymodule.find_installation('python3')

res = run_command(python3, '-c', 'import os; print(os.environ["VULKAN_SDK"])')
vulkan_sdk = ''
has_vulkan_sdk = res.returncode() == 0
if has_vulkan_sdk
    vulkan_sdk = res.stdout().strip()
endif

glfw3_dep= dependency('glfw3')
cpp = meson.get_compiler('cpp')
args = []
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
    'Core/Vulkan/Texture.cpp',
    'Core/Vulkan/Device.cpp',
    'Core/Vulkan/DescriptorHeap.cpp',
    'Core/Vulkan/DebugBuffer.cpp',
    'Core/Vulkan/Fence.cpp',
    'Core/Vulkan/Instance.cpp',
    'Core/Common.cpp',
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
    ]
    assert(has_vulkan_sdk, 'Vulkan SDK must be present to use vulkan')
    inc_list += [
        vulkan_sdk / 'include',
        vulkan_sdk / 'include' / 'vma'
    ]
    vulkan_src += vulkan_sdk / 'include' / 'volk.c'
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
lib = library('RHI', vulkan_src, 
    include_directories: inc, 
    dependencies: deps, 
    cpp_args: args,
    override_options : ['cpp_std=c++20'])
rhi_dep = declare_dependency(include_directories: inc, link_with: lib, dependencies: deps)
meson.override_dependency('pistachio_rhi', rhi_dep)