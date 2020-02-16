libgmpack_sources = [
  'mpack.c',
  'gmpackclient.c',
  'gmpackmessage.c',
  'gmpackpacker.c',
  'gmpackserver.c',
  'gmpacksession.c',
  'gmpackunpacker.c'
]

libgmpack_headers = [
  'gmpackclient.h',
  'gmpackmessage.h',
  'gmpackpacker.h',
  'gmpackserver.h',
  'gmpacksession.h',
  'gmpackunpacker.h'
]

libgmpack_deps = [
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gio-2.0'),
]

libgmpack = library('gmpack-' + meson.project_version(),
  libgmpack_sources + libgmpack_headers,
  dependencies: libgmpack_deps,
  install: true,
)

libgmpack_as_dep = declare_dependency(
  dependencies: libgmpack_deps,
  link_with: libgmpack,
  include_directories: include_directories('.'),
)
