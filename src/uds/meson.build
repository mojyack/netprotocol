netprotocol_uds_server_files = files(
  '../socket/server.cpp',
  '../socket/socket.cpp',
  'server.cpp',
)

netprotocol_uds_client_files = files(
  '../socket/client.cpp',
  '../socket/socket.cpp',
  'client.cpp',
)

netprotocol_uds_deps = []
if host_machine.system() == 'windows'
  cpp = meson.get_compiler('cpp')
  netprotocol_uds_deps += [
    cpp.find_library('ws2_32'),
  ]
endif
