dependency('python3', version : '>= 3.6')
dependency('pygobject-3.0', version: '>= 3.26.1')

install_data('xrd-settings', install_dir: bindir)

install_data('org.xrdesktop.settings.desktop', install_dir: desktopdir)
install_data('org.xrdesktop.settings.svg', install_dir: join_paths(icondir, 'hicolor', 'scalable' , 'apps'))

gnome.compile_resources(
  'org.xrdesktop.settings',
  'xrd-settings.gresource.xml',
  gresource_bundle: true,
  source_dir: meson.current_build_dir(),
  install_dir: pkgdatadir,
  install: true,
)

# Config file
python = import('python')

python3 = python.find_installation('python3')
if not python3.found()
  error('Python 3 is required to use xrd-settings.')
endif

config = configuration_data()
config.set('pkgdatadir', pkgdatadir)

GIT_TAG = ''
git = find_program('git', required : false)
if git.found()
  GIT_TAG = run_command('git', 'rev-parse', '--short', 'HEAD').stdout().strip()
endif

config.set('version', '@0@.@1@'.format(meson.project_version(), GIT_TAG))

configure_file(
  input: 'config.py.in',
  output: 'config.py',
  configuration: config,
  install_dir: python3.get_install_dir(pure: true, subdir: meson.project_name()),
)
