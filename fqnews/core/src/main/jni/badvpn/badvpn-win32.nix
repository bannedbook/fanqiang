{ stdenv, cmake, pkgconfig, openssl, nspr, nss, zlib, sqlite, zip, debug ? false }:

rec {
    badvpn = (
    let
        compileFlags = "-O3 ${stdenv.lib.optionalString (!debug) "-DNDEBUG"}";
    in
    stdenv.mkDerivation {
        name = "badvpn";
        
        src = stdenv.lib.cleanSource ./.;
        
        nativeBuildInputs = [ cmake pkgconfig ];
        buildInputs = [ openssl nspr nss ];
        
        NIX_CFLAGS_COMPILE = "-I${nspr.crossDrv.dev}/include/nspr -I${nss.crossDrv.dev}/include/nss -ggdb";
        NIX_CFLAGS_LINK = ["-ggdb"];
        
        preConfigure = ''
            cmakeFlagsArray=( "-DCMAKE_BUILD_TYPE=" "-DCMAKE_C_FLAGS=${compileFlags}" "-DCMAKE_SYSTEM_NAME=Windows" );
        '';
        
        postInstall = ''
            for lib in eay32; do
                cp ${openssl.crossDrv.bin}/bin/lib$lib.dll $out/bin/
            done
            for lib in nspr4 plc4 plds4; do
                cp ${nspr.crossDrv.out}/lib/lib$lib.dll $out/bin/
            done
            for lib in nss3 nssutil3 smime3 ssl3 softokn3 freebl3; do
                cp ${nss.crossDrv.out}/lib/$lib.dll $out/bin/
            done
            cp ${zlib.crossDrv.out}/bin/zlib1.dll $out/bin/
            cp ${sqlite.crossDrv.out}/bin/libsqlite3-0.dll $out/bin/
            _linkDLLs() { true; }
        '';
        
        dontCrossStrip = true;
    }).crossDrv;
    
    badvpnZip = stdenv.mkDerivation {
        name = "badvpn.zip";
        unpackPhase = "true";
        nativeBuildInputs = [ zip ];
        installPhase = ''
            mkdir badvpn-win32
            ln -s ${badvpn}/bin badvpn-win32/bin
            zip -q -r $out badvpn-win32
        '';
    };
}
