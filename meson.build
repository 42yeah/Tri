project('TriCpp',
        ['c', 'cpp'],
        meson_version: '>= 1.1')

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

vk_dir = get_option('vulkan_sdk_root')
if (vk_dir == '')
  warning('`vulkan_sdk_root` not provided; searching from system')
  vk_lib = cpp.find_library('vulkan', required: true)
else
  vk_lib = cpp.find_library('vulkan',
                            dirs: [vk_dir / 'lib'],
                            has_headers: ['vulkan/vulkan.h', 'vulkan/vulkan.hpp'],
                            header_include_directories: include_directories(vk_dir / 'include'),
                            required: true)
endif

glfw = cpp.find_library('glfw', required: true)
glm = cpp.find_library('glm', required: true)

vk_force_val = get_option('vulkan_force_validation_layer') or (get_option('buildtype') == 'debug')

conf_data = configuration_data()
conf_data.set('TRI_VK_FORCE_VALIDATION_LAYER', vk_force_val ? 1 : 0)

configure_file(configuration: conf_data,
               output: 'TriConfig.hpp')

executable('Tri', ['main.cpp', 'TriApp.cpp', 'TriUtils.cpp'],
           dependencies: [vk_lib, glfw, glm])
