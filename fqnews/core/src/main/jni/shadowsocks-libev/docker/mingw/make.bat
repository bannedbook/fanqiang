@echo off
pushd %~dp0
set "REPO=shadowsocks"
set "REV=master"
set "PLUGIN=true"
set "IMAGE=ss-build-mingw"
set "DIST=ss-libev-win-dist.tar.gz"
docker build --force-rm -t %IMAGE% ^
      --build-arg REV=%REV% --build-arg REPO=%REPO% ^
      --build-arg REBUILD=%RANDOM% ^
      --build-arg PLUGIN=%PLUGIN% .
docker run --rm --entrypoint cat %IMAGE% /bin.tgz > %DIST%
pause
