whisper_dep = dependency('whisper', include_type: 'system')
pipewire_dep = dependency('libpipewire-0.3', include_type: 'system')
math_dep = meson.get_compiler('c').find_library('m')

whisper_model = get_option('whisper_model')
vad_model = get_option('vad_model')
if whisper_model == '' or vad_model == ''
  error('Set -Dwhisper_model and -Dvad_model (nix develop exports WHISPER_MODEL and VAD_MODEL).')
endif

src_dir = include_directories('.')

command_lib = static_library('command',
  files('command/fold.c', 'command/intent.c', 'command/lexicon.c'),
  include_directories: src_dir,
)

speech_lib = static_library('speech',
  files('speech/endpoint.c', 'speech/listener.c', 'speech/transcribe.c'),
  include_directories: src_dir,
  link_with: command_lib,
  dependencies: [whisper_dep, math_dep],
)

executable('nixly-voice',
  files('main.c', 'audio/capture.c', 'audio/ring.c'),
  c_args: [
    '-DWHISPER_MODEL="' + whisper_model + '"',
    '-DVAD_MODEL="' + vad_model + '"',
  ],
  include_directories: src_dir,
  link_with: speech_lib,
  dependencies: [whisper_dep, pipewire_dep],
  install: true,
)
