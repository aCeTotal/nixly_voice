test('intent',
  executable('intent_test', files('intent_test.c'),
    include_directories: src_dir,
    link_with: command_lib,
  ),
)

test('endpoint',
  executable('endpoint_test', files('endpoint_test.c'),
    include_directories: src_dir,
    link_with: speech_lib,
    dependencies: whisper_dep,
  ),
)

test('apps',
  executable('apps_test', files('apps_test.c'),
    include_directories: src_dir,
    link_with: command_lib,
  ),
)

test('presence',
  executable('presence_test', files('presence_test.c'),
    include_directories: src_dir,
    link_with: speech_lib,
    dependencies: math_dep,
  ),
)
