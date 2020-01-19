gmpack_sources = [
  'mpack.c',
  'gmpackmessage.c',
  'gmpackpacker.c',
  'gmpackserver.c',
  'gmpacksession.c',
  'gmpackunpacker.c',
  'main.c'
]

gmpack_deps = [
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gio-2.0'),
]

executable('gmpack', gmpack_sources,
  dependencies: gmpack_deps,
  install: true,
)
