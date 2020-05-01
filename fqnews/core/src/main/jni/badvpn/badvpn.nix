{ stdenv, cmake, pkgconfig, openssl, nspr, nss, debug ? false }:
let
    compileFlags = "-O3 ${stdenv.lib.optionalString (!debug) "-DNDEBUG"}";
in
stdenv.mkDerivation {
    name = "badvpn";
    nativeBuildInputs = [ cmake pkgconfig ];
    buildInputs = [ openssl nspr nss ];
    src = stdenv.lib.cleanSource ./.;
    preConfigure = ''
        cmakeFlagsArray=( "-DCMAKE_BUILD_TYPE=" "-DCMAKE_C_FLAGS=${compileFlags}" );
    '';
}
