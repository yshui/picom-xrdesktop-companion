api_version = '0.15'
api_path = 'xrdesktop-' + api_version

xrdesktop_sources = [
  'xrd-window-manager.c',
  'xrd-window.c',
  'xrd-container.c',
  'xrd-settings.c',
  'xrd-input-synth.c',
  'xrd-shake-compensator.c',
  'xrd-client.c',
  'xrd-math.c',
  'xrd-desktop-cursor.c',
  'xrd-button.c',
  'xrd-render-lock.c',
  binding_resources,
  icon_resources,
  shader_resources
]

xrdesktop_headers = [
  'xrd.h',
  'xrd-window-manager.h',
  'xrd-window.h',
  'xrd-container.h',
  'xrd-settings.h',
  'xrd-input-synth.h',
  'xrd-shake-compensator.h',
  'xrd-client.h',
  'xrd-math.h',
  'xrd-desktop-cursor.h',
  'xrd-button.h',
  'xrd-types.h',
  'xrd-enums.h',
  'xrd-render-lock.h',
]

version_split = meson.project_version().split('.')
MAJOR_VERSION = version_split[0]
MINOR_VERSION = version_split[1]
MICRO_VERSION = version_split[2]

version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
version_conf.set('MAJOR_VERSION', MAJOR_VERSION)
version_conf.set('MINOR_VERSION', MINOR_VERSION)
version_conf.set('MICRO_VERSION', MICRO_VERSION)

configure_file(
  input: 'xrd-version.h.in',
  output: 'xrd-version.h',
  configuration: version_conf,
  install_dir: join_paths(get_option('includedir'), api_path)
)

xrdesktop_deps = [
  m_dep,
  gxr_dep
]

xrdesktop_inc = include_directories('.', 'scene', 'overlay')

subdir('scene')
subdir('overlay')

xrdesktop_lib = static_library(api_path,
  xrdesktop_sources,
  dependencies: xrdesktop_deps,
  include_directories: xrdesktop_inc,
  install: true,
  link_whole: [overlay_lib, scene_lib],
  c_args : ['-DXRD_COMPILATION']
)

xrdesktop_dep = declare_dependency(
  sources: [],
  link_with: xrdesktop_lib,
  include_directories: xrdesktop_inc,
  dependencies: xrdesktop_deps,
)

install_headers(xrdesktop_headers, subdir: api_path)

pkg = import('pkgconfig')

pkg.generate(
  description: 'A library for XR interaction with classical desktop compositors.',
    libraries: xrdesktop_lib,
         name: 'xrdesktop',
     filebase: api_path,
      version: meson.project_version(),
      subdirs: api_path,
     requires: 'gxr-' + api_version,
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

if get_option('introspection')
  xrdesktop_gir = gnome.generate_gir(
    xrdesktop_lib,
    sources: xrdesktop_sources + xrdesktop_headers,
    namespace: 'Xrd',
    nsversion: api_version,
    identifier_prefix: 'Xrd',
    symbol_prefix: 'xrd',
    export_packages: api_path,
    includes: [ 'cairo-1.0', 'GdkPixbuf-2.0', 'Gdk-3.0'],
    header: 'xrdesktop/xrd.h',
    install: true,
  )
endif
