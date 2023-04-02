project('gxr', 'c', version: '0.15.2',
        meson_version: '>= 0.49.0',
        default_options : [
          'c_std=c11',
          'warning_level=3',
        ],
)

gnome = import('gnome')

config_h = configuration_data()

project_args = ['-Wno-overlength-strings']

compiler = meson.get_compiler('c')
compiler_id = compiler.get_id()

if compiler_id == 'clang'
  project_args += [
    '-Weverything',
    '-Wno-float-equal',
    '-Wno-reserved-id-macro',
    '-Wno-documentation',
    '-Wno-documentation-unknown-command',
    '-Wno-padded',
    '-Wno-disabled-macro-expansion',
    '-Wno-atomic-implicit-seq-cst',
    '-Wno-gnu-empty-initializer',
    '-Wno-covered-switch-default',
    '-Wno-switch-enum',
    '-Wno-used-but-marked-unused',
    '-Wno-double-promotion',
    '-Wno-format-nonliteral',
    # variadic %f
    '-Wno-gnu-folding-constant',
    '-Wno-cast-qual' # G_DEFINE_TYPE produces this
  ]
endif

add_project_arguments([project_args], language: ['c'])

# Paths
gxr_prefix = get_option('prefix')
gxr_libdir = join_paths(gxr_prefix, get_option('libdir'))
gxr_includedir = join_paths(gxr_prefix, get_option('includedir'))
gxr_datadir = join_paths(gxr_prefix, get_option('datadir'))
src_inc = include_directories('src')

### Dependencies
c = meson.get_compiler('c')

## Required
m_dep = c.find_library('m')

gulkan_dep = dependency('gulkan-0.15', version: '>= 0.15.0')
gdk_dep = dependency('gdk-3.0', version: '>= 3.22')
gmodule_dep = dependency('gmodule-2.0')
json_glib_dep = dependency('json-glib-1.0')

config_backends = get_option('backends')
backends = []

## Optional
if 'auto' in config_backends
  config_backends += [ 'openvr', 'openxr' ]
endif

if 'openvr' in config_backends
  openvr_dep = dependency('openvr', version: '>= 1.4.18', required: false)
  if openvr_dep.found()
    backends += 'openvr'
  else
    if 'auto' in config_backends
      message('OpenVR dep not found, disabling OpenVR backend')
    else
      error('OpenVR backend enabled, but not found.')
    endif
  endif
endif

if 'openxr' in config_backends
  openxr_dep = dependency('openxr', required: false)

  if openxr_dep.found()
    backends += 'openxr'
  else
    missing_deps = []
    if not openxr_dep.found()
      missing_deps += 'openxr'
    endif

    if 'auto' in config_backends
      message('Deps ' + ', '.join(missing_deps) + ' not found, disabling OpenXR backend')
    else
      error('OpenXR backend enabled, but deps ' + ', '.join(missing_deps) + ' not found.')
    endif
  endif
endif

if backends.length() == 0
  warning('Gxr will be built without back ends!')
endif

message('Enabled backends: ' + ', '.join(backends))

if 'openvr' in backends
  openvr_version_str_list = openvr_dep.version().split('.')
  config_h.set('OPENVR_VERSION_MAJOR', openvr_version_str_list[0].to_int())
  config_h.set('OPENVR_VERSION_MINOR', openvr_version_str_list[1].to_int())
  config_h.set('OPENVR_VERSION_MICRO', openvr_version_str_list[2].to_int())

  # For a statically linked libopenvr_api we also need to link to the
  # Standard C++ library and the implementation of dlopen()
  stdcpp_dep = c.find_library('stdc++')
  dl_dep = c.find_library('dl')
endif

if 'openvr' in backends
  config_h.set('GXR_DEFAULT_API', 'GXR_API_OPENVR')
elif 'openxr' in backends
  config_h.set('GXR_DEFAULT_API', 'GXR_API_OPENXR')
else
  config_h.set('GXR_DEFAULT_API', 'GXR_API_NONE')
endif

gtk3_dep = dependency('gtk+-3.0', version: '>= 3.22', required : false)
x11_dep = dependency('x11', required : false)
xtst_dep = dependency('xtst', required : false)
cairo_dep = dependency('cairo', required : false)
libdrm_dep = dependency('libdrm', required : false)
libdrm_amdgpu_dep = dependency('libdrm_amdgpu', required : false)
glew_dep = dependency('glew', required : false)
glfw_dep = dependency('glfw3', required : false)
egl_dep = dependency('egl', required : false)

subdir('src')
