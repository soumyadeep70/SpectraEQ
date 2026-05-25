{
  description = "Flake dependency pinning for SpectraEQ";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
  outputs =
    inputs@ { flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      perSystem =
        { pkgs, ... }:
        let
          qt6Env = with pkgs.qt6; env "qt6-env" [
            qtbase
            qtdeclarative
            qtwayland
            qtlanguageserver
          ];
          projDeps = with pkgs; [
            llvmPackages_22.clang
            llvmPackages_22.clang-tools
            llvmPackages_22.lldb

            cmake
            ninja
            gnumake

            pkg-config
            gtest
            miniaudio
            alsa-lib
            libpulseaudio
            libjack2
            sndio
            pipewire

            qtcreator
            qt6.qtbase
            qt6.wrapQtAppsHook
            makeWrapper
          ]
          ++ [ qt6Env ];
          projShellHook = ''
            # Set up Qt6 library paths for linking
            export QT_PLUGIN_PATH="${qt6Env}/lib/qt-6/plugins"
            export QML_IMPORT_PATH="${qt6Env}/lib/qt-6/qml"
            export QT_QPA_PLATFORM_PLUGIN_PATH="${qt6Env}/lib/qt-6/plugins/platforms"

            # Additional Qt6 library paths
            export PKG_CONFIG_PATH="${qt6Env}/lib/pkgconfig:$PKG_CONFIG_PATH"
            export QT_QPA_PLATFORM=wayland

            if [ -z "$QT_WRAPPED_SHELL" ]; then
              export QT_WRAPPED_SHELL=1

              zshdir=$(mktemp -d)
              makeWrapper "${pkgs.zsh}/bin/zsh" "$zshdir/zsh" "''${qtWrapperArgs[@]}"

              exec "$zshdir/zsh"
            fi
          '';
          devDeps = with pkgs; [
            valgrind
            perf
            cppcheck
            nixd
          ];
        in
        {
          devShells = {
            default = pkgs.mkShell.override { inherit (pkgs.llvmPackages_22) stdenv; } {
              packages = projDeps ++ devDeps;
              shellHook = projShellHook;
            };
            ci = pkgs.mkShell.override { inherit (pkgs.llvmPackages_22) stdenv; } {
              packages = projDeps;
              shellHook = projShellHook;
            };
          };
        };
    };
}
