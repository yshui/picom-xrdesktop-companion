project('xrdesktop', 'c', version: '0.15.1',
        meson_version: '>= 0.46.0',
        default_options : [
          'c_std=c11',
          'warning_level=3',
        ],
)

gnome = import('gnome')

config_h = configuration_data()
configure_file(
  output: 'xrdesktop-config.h',
  configuration: config_h,
)

project_args = ['-I' + meson.build_root(),
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
    '-Wno-overlength-strings',
    '-Wno-disabled-macro-expansion',
    '-Wno-atomic-implicit-seq-cst',
    '-Wno-float-equal',
    '-Wno-used-but-marked-unused',
    '-Wno-assign-enum',
    '-Wno-gnu-folding-constant',
    '-Wno-cast-qual', # G_DEFINE_TYPE produces this
    '-Wno-covered-switch-default'
  ]
endif

add_project_arguments([project_args], language: ['c'])


# Paths
prefix = get_option('prefix')
libdir = join_paths(prefix, get_option('libdir'))
includedir = join_paths(prefix, get_option('includedir'))
datadir = join_paths(prefix, get_option('datadir'))
bindir = join_paths(prefix, get_option('bindir'))
pkgdatadir = join_paths(datadir, meson.project_name())
desktopdir = join_paths(datadir, 'applications')
icondir = join_paths(datadir, 'icons')

src_inc = include_directories('src')

### Dependencies
gxr_dep = dependency('gxr-0.15')
c = meson.get_compiler('c')
m_dep = c.find_library('m')

subdir('shaders')
subdir('res')
subdir('src')
subdir('settings')
