subdir('net')

common_files = files(
  'cases/backend.cpp',
  'cases/simple-server.cpp',
) + netprotocol_files

subdir('net/tcp')
executable('tcp',
  files(
    'tcp.cpp',
  ) + common_files + netprotocol_tcp_server_files + netprotocol_tcp_client_files,
  dependencies : netprotocol_deps + netprotocol_tcp_deps,
)

subdir('net/uds')
executable('uds',
  files(
    'uds.cpp',
  ) + common_files + netprotocol_uds_server_files + netprotocol_uds_client_files,
  dependencies : netprotocol_deps + netprotocol_uds_deps,
)

subdir('net/enc')
executable('enc',
  files(
    'enc.cpp',
  ) + common_files + netprotocol_enc_server_files + netprotocol_enc_client_files + netprotocol_tcp_server_files + netprotocol_tcp_client_files,
  dependencies : netprotocol_deps + netprotocol_enc_deps + netprotocol_tcp_deps,
)

if get_option('websocket')
  subdir('net/ws')
  executable('ws',
    files(
      'ws.cpp',
    ) + common_files + netprotocol_ws_server_files + netprotocol_ws_client_files,
    dependencies : netprotocol_deps + netprotocol_ws_deps,
  )
endif
