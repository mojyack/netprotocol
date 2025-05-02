subdir('net/mock-c2c')
executable('mock',
  files(
    'mock.cpp',
  ) + netprotocol_mock_c2c_files,
  dependencies : netprotocol_deps + netprotocol_mock_c2c_deps,
)

subdir('net/discord')
executable('discord',
  files(
    'discord.cpp',
  ) + netprotocol_discord_files,
  dependencies : netprotocol_deps + netprotocol_discord_deps,
)
