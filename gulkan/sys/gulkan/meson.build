project('gulkan', 'c',
        version: '0.15.1',
        meson_version: '>= 0.45.1',
        default_options : [
          'c_std=c11',
          'warning_level=3',
        ],
)

gnome = import('gnome')

config_h = configuration_data()
configure_file(
  output: 'gulkan-config.h',
  configuration: config_h,
)

project_args = [
  '-Wno-overlength-strings'
]

compiler = meson.get_compiler('c')
compiler_id = compiler.get_id()

if compiler_id == 'clang'
  project_args += [
    '-Weverything',
    '-Wno-reserved-id-macro',
    '-Wno-documentation',
    '-Wno-documentation-unknown-command',
    '-Wno-padded',
    '-Wno-disabled-macro-expansion',
    '-Wno-atomic-implicit-seq-cst',
    '-Wno-switch-enum',
    '-Wno-used-but-marked-unused',
    '-Wno-cast-qual' # G_DEFINE_TYPE produces this
  ]
endif

add_project_arguments([project_args], language: ['c'])

# Paths
gulkan_prefix = get_option('prefix')
gulkan_libdir = join_paths(gulkan_prefix, get_option('libdir'))
gulkan_includedir = join_paths(gulkan_prefix, get_option('includedir'))
gulkan_datadir = join_paths(gulkan_prefix, get_option('datadir'))
src_inc = include_directories('src')

### Dependencies

## Required
gio_dep = dependency('gio-2.0', version: '>= 2.50')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.36')
vulkan_dep = dependency('vulkan')
graphene_dep = dependency('graphene-1.0')
cairo_dep = dependency('cairo')

## Optional
libdrm_dep = dependency('libdrm', required : false)
libdrm_amdgpu_dep = dependency('libdrm_amdgpu', required : false)
glfw_dep = dependency('glfw3', required : false)
glew_dep = dependency('glew', version : '>=2.1.0', required : false)

shaderc_dep = dependency('shaderc', required : false)
json_glib_dep = dependency('json-glib-1.0', required : false)

subdir('src')
