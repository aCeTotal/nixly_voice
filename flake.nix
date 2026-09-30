{
  description = "Nixly Voice - instant offline voice commands for nixlytile";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAll = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});

      models = pkgs: {
        whisper = pkgs.fetchurl {
          url = "https://huggingface.co/ggerganov/whisper.cpp/resolve/5359861c739e955e79d9a303bcbc70fb988958b1/ggml-large-v3-turbo-q8_0.bin";
          hash = "sha256-MX62nBFnPJ3h4fDUWbJTmZgE7HGsTCPBfs9fviTiWaE=";
        };
        vad = pkgs.fetchurl {
          url = "https://huggingface.co/ggml-org/whisper-vad/resolve/9ffd54a1e1ee413ddf265af9913beaf518d1639b/ggml-silero-v6.2.0.bin";
          hash = "sha256-KqJpt4XutTqCmDogUB3ffB2cSOM6tjpBORrGyff7aYc=";
        };
      };
    in {
      packages = forAll (pkgs:
        let m = models pkgs; in {
          default = pkgs.stdenv.mkDerivation {
            pname = "nixly-voice";
            version = "0.1";

            src = pkgs.lib.fileset.toSource {
              root = ./.;
              fileset = pkgs.lib.fileset.unions [ ./meson.build ./meson.options ./src ./tests ];
            };

            strictDeps = true;
            nativeBuildInputs = [ pkgs.meson pkgs.ninja pkgs.pkg-config ];
            buildInputs = [ pkgs.whisper-cpp-vulkan pkgs.pipewire ];

            mesonFlags = [
              "-Dwhisper_model=${m.whisper}"
              "-Dvad_model=${m.vad}"
            ];
            doCheck = true;

            meta = {
              description = "Instant offline voice commands for nixlytile";
              license = pkgs.lib.licenses.gpl3Plus;
              platforms = pkgs.lib.platforms.linux;
              mainProgram = "nixly-voice";
            };
          };
        });

      apps = forAll (pkgs: {
        default = {
          type = "app";
          program = "${self.packages.${pkgs.stdenv.hostPlatform.system}.default}/bin/nixly-voice";
          meta.description = "Listen for voice commands";
        };
      });

      devShells = forAll (pkgs:
        let m = models pkgs; in {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.default ];
            WHISPER_MODEL = m.whisper;
            VAD_MODEL = m.vad;
          };
        });
    };
}
