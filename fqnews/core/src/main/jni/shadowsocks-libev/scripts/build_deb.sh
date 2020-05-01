#!/bin/sh
# Copyright 2017-2018 Roger Shimizu <rosh@debian.org>
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

help_usage() {
cat << EOT

Build shadowsocks-libev and its dependencies
Usage:
	$(basename $0) [--help|-h] [lib|bin|all]

	--help|-h	Show this usage.
	kcp		Build kcptun package (with its dependencies) only.
	lib		Build library packages only.
	bin		Build binary packages only.
			However, you need the libraries built previously, in current working directory.
			For advanced user only.
	all		Build both binary and library packages (default).
			The safe choice for everyone.

Please run this script in a clean place.
e.g.
	mkdir -p ~/build-area
	cd ~/build-area
	ln -s $(readlink -f $0) .
	./$(basename $0)

EOT
exit
}

help_lib() {
cat << EOT

Failed to install required library:
	$1
You can try to fix it by:

	$0 lib

EOT
exit
}

apt_init() {
	DEPS="$1"
	DEPS_BPO="$2"
	DEPS_EXTRA="$3"
	if [ -n "$DEPS_BPO" ]; then
		BPO=${OSVER}-backports
		case "$OSID" in
		debian)
			REPO=http://deb.debian.org/debian
			;;
		ubuntu)
			REPO=http://archive.ubuntu.com/ubuntu
			;;
		esac
		sudo sh -c "printf \"deb $REPO ${OSVER}-backports main\" > /etc/apt/sources.list.d/${OSVER}-backports.list"
		[ -n "$DEPS_EXTRA" -a "$DEPS_EXTRA" = "sloppy" ] &&
			sudo sh -c "printf \"\\ndeb $REPO ${OSVER}-backports-sloppy main\" >> /etc/apt/sources.list.d/${OSVER}-backports.list"
		sudo apt-get update
		sudo apt-get install --no-install-recommends -y -t $BPO $DEPS_BPO
		[ -n "$DEPS_EXTRA" -a "$DEPS_EXTRA" = "sloppy" ] &&
			sudo apt-get install --no-install-recommends -y -t ${BPO}-sloppy $DEPS_BPO
	else
		sudo apt-get update
	fi
	sudo apt-get install -y $DEPS
}

# Cleanup
apt_clean() {
	sudo apt-get purge -y $DEPS $DEPS_BPO debhelper \
		libbloom-dev libcork-dev libcorkipset-dev libmbedtls-dev libsodium-dev
	sudo apt-get purge -y libcork-build-deps libcorkipset-build-deps \
		libsodium-build-deps mbedtls-build-deps
	sudo apt-get purge -y libbloom-build-deps
	sudo apt-get purge -y simple-obfs-build-deps
	sudo apt-get purge -y shadowsocks-libev-build-deps

if [ $BUILD_KCP -eq 1 ]; then
	sudo apt-get purge -y golang-github-klauspost-reedsolomon-build-deps \
		golang-github-xtaci-kcp-build-deps golang-github-xtaci-smux-build-deps \
		kcptun-build-deps
	sudo apt-get purge -y golang-github-urfave-cli-build-deps
	sudo apt-get purge -y golang-github-golang-snappy-build-deps \
		dh-golang-build-deps golang-github-pkg-errors-build-deps

	sudo apt-get purge -y golang-github-klauspost-reedsolomon-dev \
		golang-github-xtaci-kcp-dev golang-github-xtaci-smux-dev
	sudo apt-get purge -y golang-github-urfave-cli-dev
	sudo apt-get purge -y golang-github-pkg-errors-dev \
		golang-github-golang-snappy-dev dh-golang
fi

	sudo apt-get autoremove -y
}

gbp_build() {
	REPO=$1
	BRANCH=$2
	PROJECT_NAME=$(basename $1|sed s/\.git$//)
	gbp clone --pristine-tar $REPO
	cd $PROJECT_NAME
	[ -n "$BRANCH" ] && git checkout $BRANCH
	[ -n "$BRANCH" -a "$BRANCH" = "trusty" ] && # try to rebase the trusty patch
		if ! git rebase master; then git rebase --abort; git rebase debian; fi
	[ -n "$DEPS_BPO" ] && BPO_REPO="-t ${OSVER}-backports"
	mk-build-deps --root-cmd sudo --install --tool "apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y $BPO_REPO"
	rm -f ${PROJECT_NAME}-build-deps_*.deb
	gbp buildpackage -us -uc --git-ignore-branch --git-pristine-tar
	git clean -fdx
	git reset --hard HEAD
	cd -
}

git_build() {
	REPO=$1
	BRANCH=$2
	PROJECT_NAME=$(basename $1|sed s/\.git$//)
	git clone $REPO
	cd $PROJECT_NAME
	[ -n "$BRANCH" ] && git checkout $BRANCH
	mk-build-deps --root-cmd sudo --install --tool "apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y"
	rm ${PROJECT_NAME}-build-deps_*.deb
	gbp buildpackage -us -uc --git-ignore-branch
	git clean -fdx
	git reset --hard HEAD
	cd -
}

dsc_build() {
	DSC=$1
	DSC_FILE=$(basename $1)
	dget -ux $DSC
	PROJECT_NAME=$(grep ^Source: $DSC_FILE|cut -d" " -f2)
	echo cd ${PROJECT_NAME}-*
	cd ${PROJECT_NAME}-*
	mk-build-deps --root-cmd sudo --install --tool "apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y"
	rm ${PROJECT_NAME}-build-deps_*.deb
	dpkg-buildpackage -us -uc
	cd -
}

# Build and install libcork deb
build_install_libcork() {
if [ $BUILD_LIB -eq 1 -o $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	if [ $BUILD_LIB -eq 1 ]; then
		gbp_build https://github.com/rogers0/libcork $BRANCH
	else
		ls libcork-dev_*.deb libcork16_*.deb 2>&1 > /dev/null ||
			help_lib "libcork-dev libcork16"
	fi
	sudo dpkg -i libcork-dev_*.deb libcork16_*.deb
fi
}

# Build and install libcorkipset deb
build_install_libcorkipset() {
if [ $BUILD_LIB -eq 1 -o $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	if [ $BUILD_LIB -eq 1 ]; then
		gbp_build https://github.com/rogers0/libcorkipset $BRANCH
	else
		ls libcorkipset-dev_*.deb libcorkipset1_*.deb 2>&1 > /dev/null ||
			help_lib "libcorkipset-dev libcorkipset1"
	fi
	sudo dpkg -i libcorkipset-dev_*.deb libcorkipset1_*.deb
fi
}

# Build libmbedtls deb
build_install_libmbedtls() {
if [ $BUILD_LIB -eq 1 -o $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	if [ $BUILD_LIB -eq 1 ]; then
		gbp_build https://salsa.debian.org/debian/mbedtls.git $BRANCH
	else
		ls libmbed*.deb 2>&1 > /dev/null ||
			help_lib libmbedtls
	fi
	sudo dpkg -i libmbed*.deb
fi
}

# Build libsodium deb
build_install_libsodium() {
if [ $BUILD_LIB -eq 1 -o $BUILD_BIN -eq 1 ]; then
	if [ $BUILD_LIB -eq 1 ]; then
		git clone https://github.com/gcsideal/debian-libsodium.git libsodium
		cd libsodium; LIBSODIUM=$(dpkg-parsechangelog --show-field Version); cd -
		dget -ud https://deb.debian.org/debian/pool/main/libs/libsodium/libsodium_${LIBSODIUM}.dsc
		DHVER=$(dpkg -l debhelper|grep debhelper|awk '{print $3}'|head -n1)
		cd libsodium
		if dpkg --compare-versions $DHVER lt 10; then
			sed -i 's/debhelper ( >= 11)/debhelper (>= 9), dh-autoreconf/' debian/control;
			sed -i 's/debhelper ( >= 10)/debhelper (>= 9), dh-autoreconf/' debian/control;
			echo 9 > debian/compat;
			dch -D unstable -l~bpo~ "Rebuild as backports"
			git add -u;
			git commit -m "Patch to work with ubuntu"
		elif dpkg --compare-versions $DHVER lt 11; then
			sed -i 's/debhelper ( >= 11)/debhelper (>= 10)/' debian/control;
			echo 10 > debian/compat;
			dch -D unstable -l~bpo~ "Rebuild as backports"
			git add -u;
			git commit -m "Patch to work with ubuntu"
		fi
		mk-build-deps --root-cmd sudo --install --tool "apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y"
		rm libsodium-build-deps_*.deb
		gbp buildpackage -us -uc --git-ignore-branch --git-tarball-dir=.. --git-export-dir=.. --git-overlay
		cd -
	else
		ls libsodium*.deb 2>&1 > /dev/null ||
			help_lib libsodium
	fi
	sudo dpkg -i libsodium*.deb
fi
}

# Build libbloom deb
build_install_libbloom() {
if [ $BUILD_LIB -eq 1 -o $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	if [ $BUILD_LIB -eq 1 ]; then
		gbp_build https://salsa.debian.org/bridges-team/libbloom.git $BRANCH
	else
		ls libbloom-dev_*.deb libbloom1_*.deb 2>&1 > /dev/null ||
			help_lib "libbloom-dev libbloom1"
	fi
	sudo dpkg -i libbloom-dev_*.deb libbloom1_*.deb
fi
}

# Add patch to work on system with debhelper 9 only
patch_sslibev_dh9() {
if [ $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	gbp clone --pristine-tar https://salsa.debian.org/bridges-team/shadowsocks-libev.git
	cd shadowsocks-libev
	[ -n "$BRANCH" ] && git checkout $BRANCH
	sed -i 's/dh $@/dh $@ --with systemd,autoreconf/' debian/rules
	sed -i 's/debhelper (>= 10)/debhelper (>= 9), dh-systemd, dh-autoreconf/' debian/control
	echo 9 > debian/compat
	dch -D unstable -l~bpo~ "Rebuild as backports"
	git add -u
	git commit -m "Patch to work with ubuntu trusty (14.04)"
	cd -
fi
}

# Build and install shadowsocks-libev deb
build_install_sslibev() {
if [ $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/bridges-team/shadowsocks-libev.git $BRANCH
	sudo dpkg -i shadowsocks-libev_*.deb
	sudo apt-get install -fy
fi
}

# Build and install simple-obfs
build_install_simpleobfs() {
if [ $BUILD_BIN -eq 1 ]; then
	BRANCH=$1
	git_build https://salsa.debian.org/bridges-team/simple-obfs.git $BRANCH
	sudo dpkg -i simple-obfs_*.deb
	sudo apt-get install -fy
fi
}

# Build and install dh-golang deb
build_install_dhgolang() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/dh-golang.git $BRANCH
	sudo dpkg -i dh-golang_*.deb
	sudo apt-get install -fy
fi
}

# Build and install golang-github-klauspost-reedsolomon deb
build_install_reedsolomondev() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/golang-github-klauspost-reedsolomon.git $BRANCH
	sudo dpkg -i golang-github-klauspost-reedsolomon-dev_*.deb
	sudo apt-get install -fy
fi
}

# Build and install golang-github-pkg-errors deb
build_install_errorsdev() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/golang-github-pkg-errors.git $BRANCH
	sudo dpkg -i golang-github-pkg-errors-dev_*.deb
	sudo apt-get install -fy
fi
}

# Add patch to work on system with xenial
patch_urfaveclidev_xenial() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp clone --pristine-tar https://salsa.debian.org/go-team/packages/golang-github-urfave-cli.git
	cd golang-github-urfave-cli
	[ -n "$BRANCH" ] && git checkout $BRANCH
	sed -i 's/golang-github-burntsushi-toml-dev/golang-toml-dev/; s/golang-gopkg-yaml.v2-dev/golang-yaml.v2-dev/' debian/control
	dch -D unstable -l~bpo~ "Rebuild as backports"
	git add -u
	git commit -m "Patch to work with ubuntu xenial (16.04)"
	cd -
fi
}

# Build and install golang-github-urfave-cli-dev deb
build_install_urfaveclidev() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/golang-github-urfave-cli.git $BRANCH
	sudo dpkg -i build-area/golang-github-urfave-cli-dev_*.deb
	sudo apt-get install -fy
fi
}

# Build and install golang-github-golang-snappy deb
build_install_snappydev() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/golang-github-golang-snappy.git $BRANCH
	sudo dpkg -i golang-github-golang-snappy-dev_*.deb
	sudo apt-get install -fy
fi
}

# Build and install golang-github-xtaci-kcp deb
build_install_kcpdev() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/golang-github-xtaci-kcp.git $BRANCH
	sudo dpkg -i golang-github-xtaci-kcp-dev_*.deb
	sudo apt-get install -fy
fi
}

# Build and install golang-github-xtaci-smux deb
build_install_smuxdev() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/golang-github-xtaci-smux.git $BRANCH
	sudo dpkg -i golang-github-xtaci-smux-dev_*.deb
	sudo apt-get install -fy
fi
}

# Build and install kcptun deb
build_install_kcptun() {
if [ $BUILD_KCP -eq 1 ]; then
	BRANCH=$1
	gbp_build https://salsa.debian.org/go-team/packages/kcptun.git $BRANCH
	sudo dpkg -i kcptun_*.deb
	sudo apt-get install -fy
fi
}

export XZ_DEFAULTS=--memlimit=128MiB

OSID=$(grep ^ID= /etc/os-release|cut -d= -f2)
OSVER=$(lsb_release -cs)
BUILD_KCP=0
BUILD_LIB=0
BUILD_BIN=0

case "$1" in
--help|-h)
	help_usage
	;;
kcp)
	BUILD_KCP=1
	;;
lib)
	BUILD_LIB=1
	;;
bin)
	BUILD_BIN=1
	;;
all|"")
	BUILD_LIB=1
	BUILD_BIN=1
	;;
*)
	echo Parameter error, exiting ...
	exit
esac

# Exit if in a git repo
[ -d .git ] && help_usage

case "$OSVER" in
jessie)
	BPO="debhelper libbloom-dev libsodium-dev"
	BPOEXTRA=sloppy
	;;
stretch)
	BPO=libbloom-dev
	;;
xenial)
	BPO=debhelper
	;;
esac
apt_init "git-buildpackage pristine-tar equivs" "$BPO" $BPOEXTRA

[ $BUILD_KCP -eq 1 ] && case "$OSVER" in
wheezy|precise|trusty)
	echo Sorry, your system $OSID/$OSVER is not supported.
	;;
jessie)
	build_install_urfaveclidev
	build_install_reedsolomondev
	build_install_kcpdev
	build_install_smuxdev
	build_install_kcptun
	;;
stretch|unstable|sid|yakkety|zesty)
	build_install_reedsolomondev
	build_install_kcpdev
	build_install_smuxdev
	build_install_kcptun
	;;
xenial)
	build_install_dhgolang debian/jessie-backports
	build_install_reedsolomondev
	build_install_errorsdev
	patch_urfaveclidev_xenial
	build_install_urfaveclidev
	build_install_snappydev debian/jessie-backports
	build_install_kcpdev
	build_install_smuxdev
	build_install_kcptun
	;;
esac

[ $BUILD_LIB -eq 1 -o $BUILD_BIN -eq 1 ] && case "$OSVER" in
wheezy|precise)
	echo Sorry, your system $OSID/$OSVER is not supported.
	;;
jessie|stretch|buster|testing|unstable|sid)
	build_install_sslibev
	;;
artful|bionic)
	build_install_libbloom
	build_install_sslibev
	build_install_simpleobfs
	;;
zesty)
	build_install_libsodium
	build_install_libbloom
	build_install_sslibev
	build_install_simpleobfs
	;;
xenial|yakkety)
	build_install_libcork debian
	build_install_libcorkipset debian
	build_install_libsodium
	build_install_libbloom
	build_install_sslibev
	build_install_simpleobfs
	;;
trusty)
	build_install_libcork trusty
	build_install_libcorkipset trusty
	build_install_libmbedtls debian/jessie-backports
	build_install_libsodium
	build_install_libbloom trusty
	patch_sslibev_dh9
	build_install_sslibev
	build_install_simpleobfs trusty
	;;
*)
	echo Your system $OSID/$OSVER is not supported yet.
	echo Please report issue:
	echo "    https://github.com/shadowsocks/shadowsocks-libev/issues/new"
	;;
esac

apt_clean
