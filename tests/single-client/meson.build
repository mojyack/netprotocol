subdir('net/mock-c2c')
executable('mock',
  files(
    'mock.cpp',
  ) + netprotocol_mock_c2c_files,
  dependencies : netprotocol_deps + netprotocol_mock_c2c_deps,
)
