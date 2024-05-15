# gravel - Utilities for AWS IoT Core clients
# Copyright (C) 2024 Amazon.com, Inc. or its affiliates

{
  description = "Utilities for AWS IoT Core clients.";
  inputs.flakelight.url = "github:accelbread/flakelight";
  outputs = { flakelight, ... }: flakelight ./. rec {
    devShell = {
      shellHook = "export AR=gcc-ar";
      packages = pkgs: with pkgs; [
        pkg-config
        clang-tools
        bear
      ];
    };
    devShells.clang = devShell // {
      stdenv = pkgs: with pkgs; overrideCC llvmPackages.stdenv
        (llvmPackages.stdenv.cc.override { inherit (llvmPackages) bintools; });
    };
    formatters = pkgs: {
      "*.c" = "${pkgs.llvmPackages.clang-unwrapped}/bin/clang-format -i";
      "*.h" = "${pkgs.llvmPackages.clang-unwrapped}/bin/clang-format -i";
    };
  };
}
