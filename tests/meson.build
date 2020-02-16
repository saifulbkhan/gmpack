test_env = [
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  'G_DEBUG=gc-friendly',
]

test_deps = libgmpack_deps + [ libgmpack_as_dep ]

test_unpacker = executable('test-unpacker',
  ['testutils.h', 'testunpacker.c'],
  dependencies: test_deps,
)
test('test-unpacker', test_unpacker, env: test_env)

test_packer = executable('test-packer',
  ['testutils.h', 'testpacker.c'],
  dependencies: test_deps,
)
test('test-packer', test_packer, env: test_env)

test_rpc = executable('test-rpc',
  'testrpc.c',
  dependencies: test_deps,
)
test('test-rpc', test_rpc, env: test_env)

executable('run-server',
  'runserver.c',
  dependencies: test_deps,
)

# add test-stress?
