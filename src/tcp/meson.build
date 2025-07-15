netprotocol_tcp_server_files = files(
  '../socket/server.cpp',
  '../socket/socket.cpp',
  'common.cpp',
  'server.cpp',
)

netprotocol_tcp_client_files = files(
  '../socket/client.cpp',
  '../socket/socket.cpp',
  'client.cpp',
  'common.cpp',
)

netprotocol_tcp_deps = []
if host_machine.system() == 'windows'
  cpp = meson.get_compiler('cpp')
  netprotocol_tcp_deps += [
    cpp.find_library('ws2_32'),
  ]
endif
