{
  description = "Buna's Math Library (BML) - Experimental symbolic mathematics and optimization engine in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f:
        nixpkgs.lib.genAttrs supportedSystems (system:
          f {
            pkgs = import nixpkgs { inherit system; };
          });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            clang
            cmake
            ninja
            pkg-config
            gdb
            valgrind
            cppcheck
            clang-tools
          ];

          CC = "clang";
          CXX = "clang++";
          CMAKE_GENERATOR = "Ninja";
          CMAKE_EXPORT_COMPILE_COMMANDS = "ON";

          shellHook = ''
            echo "BML development environment: clang + ninja"
            echo "Configure with: cmake --preset dev"
          '';
        };
      });
    };
}
