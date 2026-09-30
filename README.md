# Nixly Voice

Offline voice commands for nixlytile. Listens all the time and understands Norwegian, English, Danish, Finnish, Swedish, German, French and Spanish.

| Say | Does |
|---|---|
| «Åpne hjemmemappe» · "Open home folder" · «Öppna hemmappen» | home folder in the file manager |
| «Åpne kalkulator» · "Open calculator" · «Avaa laskin» | Nixly Kalk |
| «Åpne nettleser» · "Open browser" · «Ouvre le navigateur» | default browser |
| «Google været i morgen» · "Google weather in Paris" | Google search |

The command word must start the utterance. Speech is recognised on the GPU with whisper large-v3-turbo (Vulkan) behind a Silero VAD; nothing leaves the machine. An open command fires as soon as its target word is decoded, within about 0.1 s of the end of speech; a search fires 0.4 s after you stop talking.

It listens to the raw microphone (the NixlyMic tap, or the default source without NixlyMic). nixlytile's mute and push-to-talk silence what apps hear, not the commands.

Each recognised command is one line on stdout, which nixlytile reads: `open home`, `open calculator`, `open browser`, `search <query>`.

## Run

```sh
nix run
```

nixlytile starts it with the session and restarts it if it dies.

## Develop

```sh
nix develop
meson setup build -Dwhisper_model=$WHISPER_MODEL -Dvad_model=$VAD_MODEL
ninja -C build && build/src/nixly-voice
meson test -C build
```

## Layout

```
src/audio    PipeWire capture into a lock-free ring
src/speech   VAD endpointing and whisper decoding
src/command  transcript to command, words per language
tests        command parsing and endpointing
```
