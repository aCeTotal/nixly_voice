# Nixly Voice

Offline voice commands for nixlytile. Listens all the time and understands Norwegian; the other languages in the word lists are switched off for now.

| Say | Does |
|---|---|
| «Åpne hjemmemappe» · «Åpne hjem» · «Åpne home» | home folder in the file manager |
| «Åpne kalkulator» | Nixly Kalk |
| «Åpne nettleser» | default browser |
| «Åpne Chrome» · «Start pavucontrol» · «Start Arma 3» | any app nixly_launcher lists |
| «Google været i morgen» | Google search |

«Åpne» and «Start» do the same; the command word must start the utterance. Speech is recognised on the GPU with whisper large-v3-turbo (Vulkan) behind a Silero VAD and a 1–4 kHz band detector, so a whisper is enough; nothing leaves the machine. An open command fires as soon as its target word is decoded, within about 0.1 s of the end of speech; a search fires 0.4 s after you stop talking.

Apps come from the same desktop entries, in the same order, as nixly_launcher. An app answers to its name, its Norwegian name, its program, or any run of words in its name («Teams», «Brave»), and misheard spellings still match: «krom», «Pavo kontroll», «all akkritti». A name that another app's name continues («Steam» before «Steam Link») waits 0.25 s of silence first. Installs and removals are picked up the next time you speak.

It listens to the raw microphone (the NixlyMic tap, or the default source without NixlyMic). nixlytile's mute and push-to-talk silence what apps hear, not the commands.

Each recognised command is one line on stdout, which nixlytile reads: `open home`, `open calculator`, `open browser`, `launch <desktop file>`, `search <query>`.

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
src/apps     installed apps and how their names sound
tests        command parsing, endpointing, whisper detection and app matching
```
