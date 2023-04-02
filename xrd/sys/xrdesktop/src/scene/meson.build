scene_api_path = 'xrdesktop-scene-' + api_version

scene_sources = [
  'xrd-scene-model.c',
  'xrd-scene-client.c',
  'xrd-scene-window.c',
  'xrd-scene-pointer.c',
  'xrd-scene-object.c',
  'xrd-scene-selection.c',
  'xrd-scene-background.c',
  'xrd-scene-pointer-tip.c',
  'xrd-scene-desktop-cursor.c',
  'xrd-scene-renderer.c',
]

scene_headers = [
  'xrd-scene-model.h',
  'xrd-scene-client.h',
  'xrd-scene-window.h',
  'xrd-scene-pointer.h',
  'xrd-scene-object.h',
  'xrd-scene-selection.h',
  'xrd-scene-background.h',
  'xrd-scene-pointer-tip.h',
  'xrd-scene-desktop-cursor.h',
  'xrd-scene-renderer.h',
]

c_args = ['-DXRD_COMPILATION']
deps = [gxr_dep]

renderdoc = get_option('renderdoc')
if renderdoc
  c_args += '-DRENDERDOC'
  dl_dep = meson.get_compiler('c').find_library('dl', required : true)
  deps += dl_dep
endif

scene_lib = static_library(scene_api_path,
  scene_sources,
  dependencies: deps,
  include_directories: xrdesktop_inc,
  c_args : c_args
)

install_headers(scene_headers, subdir: api_path)
