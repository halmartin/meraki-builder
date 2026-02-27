# NOTE: this is currently hardcoded to the ms220 device
{ pkgs, ... }:
let
  buildroot = pkgs.applyPatches {
    src = pkgs.fetchzip {
      url = "https://www.buildroot.org/downloads/buildroot-2023.02.4.tar.xz";
      hash = "sha256-7r90TYW5bzC8aTMiEsIhqiJ8FEpUAIJdb3z3hkkZ0B8=";
      extension = "tar.xz";
    };
    patches = with builtins; map (f: ./buildroot/patches + "/${f}")
      (attrNames (readDir ./buildroot/patches));
    patchFlags = [ "-p0" ];
  };

  kernelHeaders = pkgs.runCommand "linux.tar.bz2"
    {
      src = pkgs.fetchFromGitHub {
        owner = "halmartin";
        repo = "switch-11-22-ms220";
        rev = "master";
        hash = "sha256-/Chmog+3fg/RV7iAhZbgd4y7cYw3LvquUg8+Mv/87dk=";
      };
    } ''tar -C $src -cjf $out linux-3.18'';
in
{
  packages = [
    (pkgs.buildFHSEnv {
      name = "buildroot-fhs";
      targetPkgs = p: (with p; [
        gcc13Stdenv.cc
        libxcrypt

        patch
        gawk
        python3
        which
        coreutils
        findutils
        gnugrep
        gnused
        gnutar
        gzip
        xz

        perl
        gnumake
        flex
        bison
        ncurses
        file
        wget
        cpio
        unzip
        rsync
        bc
        git
        cacert
        ubootTools
        xxd
      ]);
      profile = ''
        # Disable nixpkgs hardening flags and GCC warnings that cause -Werror issues
        export NIX_HARDENING_ENABLE=""
        WARN_FLAGS="-Wno-error=format-security -Wno-error=dangling-pointer"
        export CFLAGS="$WARN_FLAGS"
        export CXXFLAGS="$WARN_FLAGS"
        export HOST_CFLAGS="$WARN_FLAGS"
        export HOST_CXXFLAGS="$WARN_FLAGS"
      '';
    })
  ];

  env.LC_ALL = "C";

  scripts = {
    build.exec = ''
      [ ! -z "$1" ] && ARGS="$1-rebuild"
      make -C build/buildroot -j$(nproc) $ARGS
    '';
    clean.exec = ''
      rm -rf build
      devenv shell
    '';
  };

  enterShell = ''
      set -euo pipefail

      # Pull LFS files (kernel modules) if they haven't been fetched yet
      if ${pkgs.git-lfs}/bin/git-lfs ls-files 2>/dev/null | grep -q '^[^ ]* -'; then
        echo "Pulling git LFS files..."
        ${pkgs.git-lfs}/bin/git-lfs pull
      fi

      BR="$DEVENV_ROOT/build/buildroot"
      mkdir -p "$BR"

      # sync patched buildroot from (RO) Nix store
      ${pkgs.rsync}/bin/rsync -a --chmod=u+w --no-owner --no-group ${buildroot}/ "$BR/"

      # Symlink board configuration
      ln -sf "$DEVENV_ROOT/buildroot/board/meraki" "$BR/board/"

      # Symlink custom packages from buildroot/packages/
      ln -sf "$DEVENV_ROOT"/buildroot/packages/* "$BR/package/"

      # Copy default config if needed
      cp --update=none "$DEVENV_ROOT/buildroot/board/meraki/ms220/buildroot-config" "$BR/.config"

      # Update kernel headers location to use Nix store tarball
      sed -i "s|^BR2_KERNEL_HEADERS_CUSTOM_TARBALL_LOCATION=.*|BR2_KERNEL_HEADERS_CUSTOM_TARBALL_LOCATION=\"file://${kernelHeaders}\"|" "$BR/.config"

    echo "
      - build [<pkg>]   build the firmware (or a specific package)
      - clean           remove build and reload dev shell
    "
    exec buildroot-fhs
  '';
}
