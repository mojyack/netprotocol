if not is_variable('crypto_utils_deps')
  subdir('crypto')
endif

netprotocol_discord_files = files(
  'client.cpp',
  'crypto/base64.cpp',
)

netprotocol_discord_deps = [
  dependency('dpp'),
] + crypto_utils_deps
