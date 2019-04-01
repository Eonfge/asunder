
project('asunder',
  'c',
  version: '2.9.3',
  default_options: [
    'c_std=gnu11',
    'warning_level=3',
  ],
)

i18n = import('i18n')

###############################################################################
# Dependencies

gtk2    = dependency('gtk+-2.0',    version: '>=2.4.0')
gthread = dependency('gthread-2.0', version: '>=2.4.0')
libcddb = dependency('libcddb',     version: '>=0.9.5')


datadir_abs = get_option('prefix') / get_option('datadir')
add_project_arguments(
  '-DPACKAGE_DATA_DIR="@0@"'  .format(datadir_abs),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(datadir_abs / 'locale'),
  '-Wno-deprecated-declarations',
  '-Wno-unused-parameter',
  '-DHAVE_CONFIG_H',
  language: 'c',
)


subdir('src')
# subdir('po')
