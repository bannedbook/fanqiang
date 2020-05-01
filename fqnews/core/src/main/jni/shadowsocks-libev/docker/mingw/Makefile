#
# Makefile for building MinGW port
#
# This file is part of the shadowsocks-libev.
#
# shadowsocks-libev is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# shadowsocks-libev is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with shadowsocks-libev; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#

REPO=shadowsocks
REV=master
IMAGE=ss-build-mingw
DIST=ss-libev-win-dist.tar.gz

all: build

build:
	cd ../../ && docker build -t $(IMAGE) \
	  -f docker/mingw/Dockerfile \
	  --build-arg REV=$(REV) --build-arg REPO=$(REPO) \
	  --build-arg REBUILD="$$(date +%Y-%m-%d-%H-%M-%S)" \
	  .
	docker run --rm --entrypoint cat $(IMAGE) /bin.tgz > $(DIST)

clean:
	rm -f $(DIST)
	docker rmi $(IMAGE) || true

.PHONY: all clean build
