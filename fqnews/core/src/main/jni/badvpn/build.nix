with import <nixpkgs> {};
rec {
    badvpnFunc = import ./badvpn.nix;
    badvpn = pkgs.callPackage badvpnFunc {};
    badvpnDebug = pkgs.callPackage badvpnFunc { debug = true; };
}
