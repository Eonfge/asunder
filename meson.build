
project('asunder',
  'c',
  version: '2.9.3',
  default_options: [
    'c_std=gnu11',
    'warning_level=3',
  ],
)

i18n = import('i18n')
config = configuration_data()

###############################################################################
# Dependencies

gtk3    = dependency('gtk+-3.0',    version: '>=2.4.0')
gthread = dependency('gthread-2.0', version: '>=2.4.0')
libcddb = dependency('libcddb',     version: '>=0.9.5')

gettext_package = meson.project_name()

datadir_abs = get_option('prefix') / get_option('datadir')

config.set_quoted('PACKAGE',            meson.project_name())
config.set_quoted('PACKAGE_DATA_DIR',   datadir_abs)
config.set_quoted('PACKAGE_LOCALE_DIR', datadir_abs / 'locale')
config.set('ENABLE_NLS', true)

add_project_arguments(
  '-Wno-deprecated-declarations',
  '-DHAVE_CONFIG_H',
  language: 'c',
)

# Port to gtk3
add_project_arguments(
  '-DGDK_DISABLE_DEPRECATED',
  '-DGTK_DISABLE_DEPRECATED',
  language: 'c',
)

subdir('src')
subdir('data')
subdir('po')
