project('Tri', ['c', 'cpp'], meson_version : '>= 1.1.0')

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

deps = [dependency('glfw3'), dependency('glm')]

vulkan_sdk_root = get_option('vulkan_sdk_root')

if (vulkan_sdk_root == '')
  error('`vulkan_sdk_root` must be defined')
endif

# message('Assuming Vulkan located at @0@/lib'.format(vulkan_sdk_root))

vulkan_dep = cpp.find_library('vulkan', dirs : [vulkan_sdk_root + '/lib'])
vulkan_headers = include_directories(vulkan_sdk_root + '/include')

deps += vulkan_dep
tri_args = []

use_vulkan_validation = (get_option('buildtype') == 'debug' or
                         get_option('vulkan_force_disable_validation'))

conf = configuration_data()
conf.set('TRI_WITH_VULKAN_VALIDATION', use_vulkan_validation ? 1 : 0)
conf.set('TRI_COLORED_LOG', get_option('colored_log') ? 1 : 0)
configure_file(output : 'TriConfig.hpp', configuration : conf)

executable('tri', ['main.cpp', 'TriApp.cpp', 'TriLog.cpp',
                   'VkExtLibrary.cpp', 'TriFileUtils.cpp'],
           include_directories : vulkan_headers,
           dependencies : deps,
           cpp_args : tri_args)

# Try to check for glslc
glslc = find_program('glslc', native : true, required : true)

shader_output_dir = meson.current_build_dir() / 'Shaders'
run_command('mkdir', '-p', shader_output_dir, check : true)

# HACK(42): Somehow this must be present otherwise the folder will not be created
#           before the custom targets
message('mkdir @0@'.format(shader_output_dir))

shaders = ['triangle.vert',
           'triangle.frag']

foreach shader : shaders 
  custom_target('Shader @0@'.format(shader),
                input: 'Shaders/@0@'.format(shader),
                output : '@PLAINNAME@.svc',
                command : [
                  glslc, '@INPUT@', '-o', 'Shaders/@OUTPUT@'
                ],
               build_by_default : true)
endforeach

