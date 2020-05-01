# NOTE: Must be used with patched nixpkgs:
# https://github.com/ambrop72/nixpkgs/tree/cross-mingw-nss

let
    pkgsFun = import <nixpkgs>;
    
    crossSystem = {
        config = "i686-w64-mingw32";
        arch = "x86";
        libc = "msvcrt";
        platform = {};
        openssl.system = "mingw";
        is64bit = false;
    };
    
    pkgs = pkgsFun {
        inherit crossSystem;
    };
    
in
rec {
    inherit pkgs;
    
    badvpnPkgsFunc = import ./badvpn-win32.nix;
    
    badvpnPkgs = pkgs.callPackage badvpnPkgsFunc {};
    badvpnDebugPkgs = pkgs.callPackage badvpnPkgsFunc { debug = true; };
}
