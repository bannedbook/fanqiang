#!/bin/sh
# Copyright 2018 Roger Shimizu <rosh@debian.org>
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

set -e

help_usage() {
cat << EOT

Call build_deb.sh script in a chrooted environment
Usage:
	sudo $(basename $0) [--help|-h] [codename]

	--help|-h	Show this usage.
	[code name]	Debian/Ubuntu release codename
			e.g. jessie/stretch/trusty/xenial

EOT
exit
}

# POSIX-compliant maint function recommend by devref
# to check for the existence of a command
# https://www.debian.org/doc/manuals/developers-reference/ch06.html#bpp-debian-maint-scripts
pathfind() {
	OLDIFS="$IFS"
	IFS=:
	for p in $PATH; do
		if [ -x "$p/$*" ]; then
		IFS="$OLDIFS"
		return 0
		fi
	done
	IFS="$OLDIFS"
	return 1
}

case "$1" in
wheezy|precise)
	echo Sorry, the system $1 is not supported.
	;;
jessie|stretch|buster|testing|unstable|sid)
	OSID=debian
	REPO=http://deb.debian.org/debian
	;;
trusty|yakkety|zesty|xenial|artful|bionic)
	OSID=ubuntu
	REPO=http://archive.ubuntu.com/ubuntu
	;;
--help|-h|*)
	help_usage
esac

if ! pathfind debootstrap; then
	echo Please install debootstrap package.
	exit 1
fi

OSVER=$1
CHROOT=/tmp/${OSVER}-build-$(date +%Y%m%d%H%M)
TIMESTAMP0=$(date)

mkdir -p ${CHROOT}/etc
echo en_US.UTF-8 UTF-8 > ${CHROOT}/etc/locale.gen
if ! debootstrap --variant=minbase --include=ca-certificates,git,sudo,wget,whiptail --exclude=upstart,systemd $OSVER $CHROOT $REPO; then
	echo debootstrap failed. Please kindly check whether proper sudo or not.
	exit 1
fi
case "$OSID" in
debian)
	echo deb $REPO ${OSVER} main > ${CHROOT}/etc/apt/sources.list
	echo deb $REPO ${OSVER}-updates main >> ${CHROOT}/etc/apt/sources.list
	echo deb $REPO-security ${OSVER}/updates main >> ${CHROOT}/etc/apt/sources.list
	;;
ubuntu)
	echo deb $REPO $OSVER main universe > ${CHROOT}/etc/apt/sources.list
	echo deb $REPO ${OSVER}-updates main universe >> ${CHROOT}/etc/apt/sources.list
	echo deb $REPO ${OSVER}-security main universe >> ${CHROOT}/etc/apt/sources.list
	;;
esac

cat << EOL | chroot $CHROOT
apt-get purge -y udev
apt-get update
apt-get -fy install
apt-get -y upgrade
apt-get -y install --no-install-recommends lsb-release
# dh_auto_test of mbedtls (faketime) depends on /dev/shm. https://bugs.debian.org/778462
mkdir -p ~ /dev/shm
mount tmpfs /dev/shm -t tmpfs

date > /TIMESTAMP1
git config --global user.email "script@example.com"
git config --global user.name "build script"
if [ -n "$http_proxy" ]; then
	git config --global proxy.http $http_proxy
	echo Acquire::http::Proxy \"$http_proxy\"\; > /etc/apt/apt.conf
	export http_proxy=$http_proxy
	export https_proxy=$https_proxy
	export no_proxy=$no_proxy
fi
cd /tmp
wget https://raw.githubusercontent.com/shadowsocks/shadowsocks-libev/master/scripts/build_deb.sh
chmod 755 build_deb.sh
./build_deb.sh
date > /TIMESTAMP2
./build_deb.sh kcp
umount /dev/shm
EOL

TIMESTAMP1=$(cat ${CHROOT}/TIMESTAMP1)
TIMESTAMP2=$(cat ${CHROOT}/TIMESTAMP2)
TIMESTAMP3=$(date)

printf \\n"All built deb packages:"\\n
ls -l ${CHROOT}/tmp/*.deb
echo
echo Start-Time: $TIMESTAMP0
echo ChrootDone: $TIMESTAMP1
echo SsDeb-Done: $TIMESTAMP2
echo \ Kcp-Done : $TIMESTAMP3
