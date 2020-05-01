# -*- mode: ruby -*-
# vi: set ft=ruby :

# DESCRIPTION:
# ============
# Vagrant for running libevent tests with:
# - timeout 30min, to avoid hungs
# - run tests in parallel under ctest (10 concurency)
# - if you have uncommited changes, you should commit them first to check
# - unix only, because of some tar'ing to avoid one vm affect another
#
# ENVIRONMENT:
# ============
# - NO_PKG        -- do not install packages
# - NO_CMAKE      -- do not run with cmake
# - NO_AUTOTOOLS  -- do not run with autoconf/automake

Vagrant.configure("2") do |config|
  # to allow running boxes provisions in parallel, we can't share the same dirs
  # via virtualbox, however sometimes it is the only way, so instead let's
  # create an archive of HEAD (this way we will not have any trash there) and
  # extract it for every box to the separate folder.
  #
  # P.S. we will change this --prefix with tar(1) --trasnform
  system('git archive --prefix=libevent/ --output=.vagrant/libevent.tar HEAD')

  config.vm.provider "virtualbox" do |vb|
    vb.memory = "512"

    # otherwise osx fails, anyway we do not need this
    vb.customize ["modifyvm", :id, "--usb", "off"]
    vb.customize ["modifyvm", :id, "--usbehci", "off"]
  end

  # disable /vagrant share, in case we will not use default mount
  config.vm.synced_folder ".", "/vagrant", disabled: true

  config.vm.define "ubuntu" do |ubuntu|
    system('tar --overwrite --transform=s/libevent/libevent-linux/ -xf .vagrant/libevent.tar -C .vagrant/')

    ubuntu.vm.box = "ubuntu/xenial64"
    ubuntu.vm.synced_folder ".vagrant/libevent-linux", "/vagrant",
      type: "rsync"

    if ENV['NO_PKG'] != "true"
      ubuntu.vm.provision "shell", inline: <<-SHELL
        apt-get update
        apt-get install -y zlib1g-dev libssl-dev python2.7
        apt-get install -y build-essential cmake ninja-build
        apt-get install -y autoconf automake libtool
      SHELL
    end

    if ENV['NO_CMAKE'] != "true"
      ubuntu.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        rm -fr .cmake-vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -G Ninja ..

        export CTEST_TEST_TIMEOUT=1800
        export CTEST_OUTPUT_ON_FAILURE=1
        export CTEST_PARALLEL_LEVEL=20
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      ubuntu.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        ./autogen.sh
        ./configure
        make -j20 verify
      SHELL
    end
  end

  config.vm.define "freebsd" do |freebsd|
    system('tar --overwrite --transform=s/libevent/libevent-freebsd/ -xf .vagrant/libevent.tar -C .vagrant/')

    freebsd.vm.box = "freebsd/FreeBSD-11.0-STABLE"
    freebsd.vm.synced_folder ".vagrant/libevent-freebsd", "/vagrant",
      type: "rsync", group: "wheel"

    # otherwise reports error
    freebsd.ssh.shell = "sh"

    if ENV['NO_PKG'] != "true"
      freebsd.vm.provision "shell", inline: <<-SHELL
        pkg install --yes openssl cmake ninja automake autotools
      SHELL
    end

    if ENV['NO_CMAKE'] != "true"
      freebsd.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        rm -fr .cmake-vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -G Ninja ..

        export CTEST_TEST_TIMEOUT=1800
        export CTEST_OUTPUT_ON_FAILURE=1
        export CTEST_PARALLEL_LEVEL=20
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      freebsd.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        ./autogen.sh
        ./configure
        make -j20 verify
      SHELL
    end
  end

  config.vm.define "netbsd" do |netbsd|
    system('tar --overwrite --transform=s/libevent/libevent-netbsd/ -xf .vagrant/libevent.tar -C .vagrant/')

    netbsd.vm.box = "kja/netbsd-7-amd64"
    netbsd.vm.synced_folder ".vagrant/libevent-netbsd", "/vagrant",
      type: "rsync", group: "wheel"

    if ENV['NO_PKG'] != "true"
      netbsd.vm.provision "shell", inline: <<-SHELL
        export PKG_PATH="ftp://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/x86_64/7.0_2016Q2/All/"
        pkg_add ncurses ninja-build automake cmake libtool
      SHELL
    end

    if ENV['NO_CMAKE'] != "true"
      netbsd.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        rm -fr .cmake-vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -G Ninja ..

        export CTEST_TEST_TIMEOUT=1800
        export CTEST_OUTPUT_ON_FAILURE=1
        export CTEST_PARALLEL_LEVEL=20
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      netbsd.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        ./autogen.sh
        ./configure
        make -j20 verify
      SHELL
    end
  end

  config.vm.define "solaris" do |solaris|
    system('tar --overwrite --transform=s/libevent/libevent-solaris/ -xf .vagrant/libevent.tar -C .vagrant/')

    # XXX:
    # - solaris do not have '-or' it only has '-o' for find(1), so we can't use
    #   rsync
    # - and autoconf(1) doesn't work on virtualbox share, ugh
    solaris.vm.synced_folder ".vagrant/libevent-solaris", "/vagrant-vbox",
      type: "virtualbox"

    solaris.vm.box = "tnarik/solaris10-minimal"
    if ENV['NO_PKG'] != "true"
      # TODO: opencsw does not include ninja(1)
      solaris.vm.provision "shell", inline: <<-SHELL
        pkgadd -d http://get.opencsw.org/now
        pkgutil -U
        pkgutil -y -i libssl_dev cmake rsync python gmake gcc5core automake autoconf libtool
      SHELL
    end

    # copy from virtualbox mount to newly created folder
    solaris.vm.provision "shell", privileged: false, inline: <<-SHELL
      rm -fr ~/vagrant
      cp -r /vagrant-vbox ~/vagrant
    SHELL

    if ENV['NO_CMAKE'] != "true"
      # builtin compiler cc(1) is a wrapper, so we should use gcc5 manually,
      # otherwise it will not work.
      # Plus we should set some paths so that cmake/compiler can find tham.
      solaris.vm.provision "shell", privileged: false, inline: <<-SHELL
        export CMAKE_INCLUDE_PATH=/opt/csw/include
        export CMAKE_LIBRARY_PATH=/opt/csw/lib
        export CFLAGS=-I$CMAKE_INCLUDE_PATH
        export LDFLAGS=-L$CMAKE_LIBRARY_PATH

        cd ~/vagrant
        rm -rf .cmake-vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -DCMAKE_C_COMPILER=gcc ..

        export CTEST_TEST_TIMEOUT=1800
        export CTEST_OUTPUT_ON_FAILURE=1
        export CTEST_PARALLEL_LEVEL=20
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      # and we should set MAKE for `configure` otherwise it will try to use
      # `make`
      solaris.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd ~/vagrant
        ./autogen.sh
        MAKE=gmake ./configure
        gmake -j20 verify
      SHELL
    end
  end

  # known failures:
  # - sometimes vm hangs
  config.vm.define "osx" do |osx|
    system('tar --overwrite --transform=s/libevent/libevent-osx/ -xf .vagrant/libevent.tar -C .vagrant/')

    osx.vm.synced_folder ".vagrant/libevent-osx", "/vagrant",
      type: "rsync", group: "wheel"

    osx.vm.box = "jhcook/osx-elcapitan-10.11"
    if ENV['NO_PKG'] != "true"
      osx.vm.provision "shell", privileged: false, inline: <<-SHELL
        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

        brew uninstall libtool
        brew install libtool openssl ninja cmake autoconf automake
      SHELL
    end

    if ENV['NO_CMAKE'] != "true"
      # we should set some paths so that cmake/compiler can find tham
      osx.vm.provision "shell", privileged: false, inline: <<-SHELL
        export OPENSSL_ROOT=$(echo /usr/local/Cellar/openssl/*)
        export CMAKE_INCLUDE_PATH=$OPENSSL_ROOT/include
        export CMAKE_LIBRARY_PATH=$OPENSSL_ROOT/lib

        cd /vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -G Ninja ..

        export CTEST_TEST_TIMEOUT=1800
        export CTEST_OUTPUT_ON_FAILURE=1
        export CTEST_PARALLEL_LEVEL=20
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      osx.vm.provision "shell", privileged: false, inline: <<-SHELL
        export OPENSSL_ROOT=$(echo /usr/local/Cellar/openssl/*)
        export CFLAGS=-I$OPENSSL_ROOT/include
        export LDFLAGS=-L$OPENSSL_ROOT/lib

        cd /vagrant
        ./autogen.sh
        ./configure
        make -j20 verify
      SHELL
    end
  end

  config.vm.define "centos" do |centos|
    system('tar --overwrite --transform=s/libevent/libevent-centos/ -xf .vagrant/libevent.tar -C .vagrant/')

    centos.vm.synced_folder ".vagrant/libevent-centos", "/vagrant",
      type: "rsync", group: "wheel"

    centos.vm.box = "centos/7"
    if ENV['NO_PKG'] != "true"
      centos.vm.provision "shell", inline: <<-SHELL
        echo "[russianfedora]" > /etc/yum.repos.d/russianfedora.repo
        echo name=russianfedora >> /etc/yum.repos.d/russianfedora.repo
        echo baseurl=http://mirror.yandex.ru/fedora/russianfedora/russianfedora/free/el/releases/7/Everything/x86_64/os/ >> /etc/yum.repos.d/russianfedora.repo
        echo enabled=1 >> /etc/yum.repos.d/russianfedora.repo
        echo gpgcheck=0 >> /etc/yum.repos.d/russianfedora.repo
      SHELL
      centos.vm.provision "shell", inline: <<-SHELL
        yum -y install zlib-devel openssl-devel python
        yum -y install gcc cmake ninja-build
        yum -y install autoconf automake libtool
      SHELL
    end

    if ENV['NO_CMAKE'] != "true"
      centos.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        rm -fr .cmake-vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -G Ninja ..

        export CTEST_TEST_TIMEOUT=1800
        export CTEST_OUTPUT_ON_FAILURE=1
        export CTEST_PARALLEL_LEVEL=20
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      centos.vm.provision "shell", privileged: false, inline: <<-SHELL
        cd /vagrant
        ./autogen.sh
        ./configure
        make -j20 verify
      SHELL
    end
  end

  # known failures:
  # - issues with timers (not enough allowed error)
  config.vm.define "win" do |win|
    system('tar --overwrite --transform=s/libevent/libevent-win/ -xf .vagrant/libevent.tar -C .vagrant/')

    # 512MB not enough after libtool install, huh
    win.vm.provider "virtualbox" do |vb|
      vb.memory = "1024"
    end

    # windows does not have rsync builtin, let's use virtualbox for now
    win.vm.synced_folder ".vagrant/libevent-win", "/vagrant",
      type: "virtualbox"

    win.vm.box = "senglin/win-10-enterprise-vs2015community"
    if ENV['NO_PKG'] != "true"
      # box with vs2015 does not have C++ support, so let's install it manually
      # plus chocolatey that includes in this box, can't handle sha1 checksum for
      # cmake.install, so let's update it<
      win.vm.provision "shell", inline: <<-SHELL
        choco upgrade -y chocolatey -pre -f
        choco install -y VisualStudioCommunity2013
        choco install -y openssl.light
        choco install -y cygwin cyg-get
        choco install -y cmake
        choco install -y cmake.install
        choco install -y python2
      SHELL

      # chocolatey openssl.light package does not contains headers
      win.vm.provision "shell", inline: <<-SHELL
        (new-object System.Net.WebClient).DownloadFile('http://strcpy.net/packages/Win32OpenSSL-1_0_2a.exe', '/openssl.exe')
        /openssl.exe /silent /verysilent /sp- /suppressmsgboxes
      SHELL

      # XXX:
      # - cyg-get depends from cygwinsetup.exe
      #   https://github.com/chocolatey/chocolatey-coreteampackages/issues/200
      # - cyg-get only downloads, do not installs them, ugh. so let's do not use
      #   it
      win.vm.provision "shell", privileged: false, inline: <<-SHELL
        (new-object System.Net.WebClient).DownloadFile('https://cygwin.com/setup-x86_64.exe', '/tools/cygwin/cygwinsetup.exe')

        $env:PATH="/tools/cygwin/bin;$($env:PATH);/tools/cygwin"

        cygwinsetup --root c:/tools/cygwin/ --local-package-dir c:/tools/cygwin/packages/ --no-desktop --no-startmenu --verbose --quiet-mode --download --packages automake,autoconf,gcc-core,libtool,make,python,openssl-devel
        cygwinsetup --root c:/tools/cygwin/ --local-package-dir c:/tools/cygwin/packages/ --no-desktop --no-startmenu --verbose --quiet-mode --local-install --packages automake,autoconf,gcc-core,libtool,make,python,openssl-devel
      SHELL
    end

    if ENV['NO_CMAKE'] != "true"
      win.vm.provision "shell", privileged: false, inline: <<-SHELL
        $env:PATH="/Program Files/CMake/bin;/tools/python2;$($env:PATH)"

        cd /vagrant
        Remove-Item -Recurse -Force .cmake-vagrant
        mkdir -p .cmake-vagrant
        cd .cmake-vagrant
        cmake -G "Visual Studio 12" ..

        $env:CTEST_TEST_TIMEOUT = "1800"
        $env:CTEST_OUTPUT_ON_FAILURE = "1"
        $env:CTEST_PARALLEL_LEVEL = "10"
        cmake --build . --target verify
      SHELL
    end

    if ENV['NO_AUTOTOOLS'] != "true"
      win.vm.provision "shell", privileged: false, inline: <<-SHELL
        $env:PATH="/tools/cygwin/bin;$($env:PATH)"

        bash -lc "echo 'C:/tools/mingw64 /mingw ntfs binary 0 0' > /etc/fstab"
        bash -lc "echo 'C:/OpenSSL-Win32 /ssl ntfs binary 0 0' >> /etc/fstab"
        bash -lc "echo 'C:/vagrant /vagrant ntfs binary 0 0' >> /etc/fstab"

        bash -lc "exec 0</dev/null; exec 2>&1; cd /vagrant; bash -x ./autogen.sh && ./configure LDFLAGS='-L/ssl -L/ssl/lib -L/ssl/lib/MinGW' CFLAGS=-I/ssl/include && make -j20 verify"
      SHELL
    end
  end
end
