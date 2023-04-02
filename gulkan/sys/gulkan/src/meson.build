so_version = 0
api_version = '0.15'
api_path = 'gulkan-' + api_version

gulkan_sources = [
  'gulkan-instance.c',
  'gulkan-device.c',
  'gulkan-texture.c',
  'gulkan-client.c',
  'gulkan-uniform-buffer.c',
  'gulkan-vertex-buffer.c',
  'gulkan-frame-buffer.c',
  'gulkan-geometry.c',
  'gulkan-renderer.c',
  'gulkan-render-pass.c',
  'gulkan-swapchain.c',
  'gulkan-descriptor-pool.c',
  'gulkan-swapchain-renderer.c',
  'gulkan-buffer.c',
  'gulkan-cmd-buffer.c',
  'gulkan-queue.c'
]

gulkan_headers = [
  'gulkan.h',
  'gulkan-instance.h',
  'gulkan-device.h',
  'gulkan-texture.h',
  'gulkan-client.h',
  'gulkan-uniform-buffer.h',
  'gulkan-vertex-buffer.h',
  'gulkan-frame-buffer.h',
  'gulkan-geometry.h',
  'gulkan-renderer.h',
  'gulkan-render-pass.h',
  'gulkan-swapchain.h',
  'gulkan-descriptor-pool.h',
  'gulkan-swapchain-renderer.h',
  'gulkan-buffer.h',
  'gulkan-cmd-buffer.h',
  'gulkan-queue.h'
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
  input: 'gulkan-version.h.in',
  output: 'gulkan-version.h',
  configuration: version_conf,
  install_dir: join_paths(get_option('includedir'), api_path)
)

gulkan_deps = [
  cairo_dep,
  gio_dep,
  gdk_pixbuf_dep,
  vulkan_dep,
  graphene_dep
]

gulkan_lib = static_library(api_path,
  gulkan_sources,
  dependencies: gulkan_deps,
  install: true,
  c_args : ['-DGULKAN_COMPILATION']
)

gulkan_inc = include_directories('.')

gulkan_dep = declare_dependency(
  sources: [],
  link_with: gulkan_lib,
  include_directories: [ gulkan_inc ],
  dependencies: gulkan_deps,
)

install_headers(gulkan_headers, subdir: api_path)

pkg = import('pkgconfig')

pkg.generate(
  description: 'GLib wrapper for Vulkan',
    libraries: gulkan_lib,
         name: 'gulkan',
     filebase: api_path,
      version: meson.project_version(),
      subdirs: api_path,
     requires: ['glib-2.0', 'vulkan', 'cairo',
                'graphene-1.0', 'gdk-pixbuf-2.0', 'gio-2.0'],
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
