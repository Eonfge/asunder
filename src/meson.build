
configure_file(
  output: 'config.h',
  configuration: config,
)

asunder = executable('asunder',
  'main.c',
  'callbacks.c',
  'completion.c',
  'interface.c',
  'prefs.c',
  'support.c',
  'threads.c',
  'util.c',
  'wrappers.c',

  dependencies: [
    gtk3,
    libcddb,
  ],
  install: true,
)
