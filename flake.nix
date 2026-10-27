{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs =
    inputs:
    let
      genSystems = func: builtins.mapAttrs func inputs.nixpkgs.legacyPackages;
    in
    {
      formatter = genSystems (system: pkgs: pkgs.nixfmt-tree);
      devShells = genSystems (
        system: pkgs: {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              cmake
              dosfstools
              efitools
              git
              gnat
              gnumake
              mtools
              ninja
              opencode
              python3
              qemu
              OVMF
              util-linux
            ];
            buildInputs = with pkgs.pkgsStatic; [
            ];

            shellHook = ''
              export __fish_machine="(dev:llvm-ada)"
              if [ -t 0 ] && [ -t 1 ]; then
                export SHELL="${pkgs.fish}/bin/fish"
                exec $SHELL
              fi
            '';
          };
        }
      );
    };
}
