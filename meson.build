project('Tri', ['c', 'cpp'], meson_version : '>= 1.1.0')

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

deps = [dependency('glfw3'), dependency('glm')]

vulkan_sdk_root = get_option('vulkan_sdk_root')

if (vulkan_sdk_root == '')
  error('`vulkan_sdk_root` must be defined')
endif

vulkan_dep = cpp.find_library('vulkan', dirs : [vulkan_sdk_root + '/lib'])
vulkan_headers = include_directories(vulkan_sdk_root + '/include')

deps += vulkan_dep

executable('tri', ['main.cpp', 'TriApp.cpp', 'TriLog.cpp'],
           include_directories : vulkan_headers,
           dependencies : deps)
