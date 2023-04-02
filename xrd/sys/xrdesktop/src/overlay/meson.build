overlay_api_path = 'xrdesktop-overlay-' + api_version

overlay_headers = [
  'xrd-overlay-pointer-tip.h',
  'xrd-overlay-desktop-cursor.h',
  'xrd-overlay-pointer.h',
  'xrd-overlay-client.h',
  'xrd-overlay-window.h',
]

overlay_sources = [
  'xrd-overlay-pointer-tip.c',
  'xrd-overlay-desktop-cursor.c',
  'xrd-overlay-pointer.c',
  'xrd-overlay-client.c',
  'xrd-overlay-window.c',
]

overlay_lib = static_library(overlay_api_path,
  overlay_sources,
  dependencies: [gxr_dep],
  include_directories: xrdesktop_inc,
  c_args : ['-DXRD_COMPILATION']
)

install_headers(overlay_headers, subdir: api_path)
