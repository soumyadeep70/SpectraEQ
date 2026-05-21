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
          commonPkgs = with pkgs; [
            llvmPackages_22.clang
            llvmPackages_22.clang-tools
            llvmPackages_22.lldb

            cmake
            ninja
            gnumake
            pkg-config
            gtest
          ];
        in
        {
          devShells = {
            default = pkgs.mkShell {
              packages = commonPkgs
                ++ (with pkgs; [
                  valgrind
                  perf
                  cppcheck
                  nixd
                ]);
            };
            ci = pkgs.mkShell {
              packages = commonPkgs;
            };
          };
        };
    };
}
