# generate gresources
test_resources = gnome.compile_resources(
  'test_resources', 'tests.gresource.xml',
  source_dir : '.')

binding_resources = gnome.compile_resources(
  'binding_resources', 'bindings.gresource.xml',
  source_dir : '.')

install_data('org.xrdesktop.gschema.xml', install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'))
meson.add_install_script('meson_post_install.py')

subdir('icons')
