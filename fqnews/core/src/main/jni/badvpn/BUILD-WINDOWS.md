To build on Windows, you need Visual Studio 2015 or 2017 and CMake.

If you only want to build tun2socks, then no additional dependencies are needed, but pass `-DBUILD_NOTHING_BY_DEFAULT=1 -DBUILD_TUN2SOCKS=1` options to the CMake command line (see below). If the VPN system components are needed, then you will need to build OpenSSL and NSS yourself and we provide no instructions for that.

Create a build directory (either in or outside the source directory).

Open a command line and navigate to the build directory.

Run CMake:

```
cmake "<path-to-source>" -G "<generator>" -DCMAKE_INSTALL_PREFIX="<install-dir>" [options...]
```

See [here](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) for the list of generators, for example you may want "Visual Studio 15 2017 Win64" for a 64-build with VS2017.

Once cmake is successful, build with the following command:

```
cmake --build . --target install
```

If successful you will have the binaries in `<install-dir>/bin`.
