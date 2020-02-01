gmpack_sources = [
  'mpack.c',
  'gmpackclient.c',
  'gmpackmessage.c',
  'gmpackpacker.c',
  'gmpackserver.c',
  'gmpacksession.c',
  'gmpackunpacker.c'
]

gmpack_main = [ 'main.c' ]
gmpack_server = [ 'runserver.c' ]

gmpack_deps = [
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gio-2.0'),
]

executable('gmpack', gmpack_sources + gmpack_main,
  dependencies: gmpack_deps,
  install: true,
)

executable('gmpack-server', gmpack_sources + gmpack_server,
  dependencies: gmpack_deps,
  install: true,
)
