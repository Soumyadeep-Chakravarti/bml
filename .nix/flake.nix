{
  description = "BML development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              clang
              clang-tools
              cmake
              ninja
              gnumake
              pkg-config
              just

              nixfmt
              statix
              deadnix
            ];

            shellHook = ''
              echo "BML development environment"
              echo "  $(clang --version | head -n 1)"
              echo "  CMake $(cmake --version | head -n 1 | cut -d ' ' -f 3)"
            '';
          };
        }
      );

      formatter = forAllSystems (system: nixpkgs.legacyPackages.${system}.nixfmt);
    };
}
