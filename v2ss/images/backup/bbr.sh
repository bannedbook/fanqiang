#!/usr/bin/env bash
#
# Auto install latest kernel for TCP BBR
#
# System Required:  CentOS 6+, Debian7+, Ubuntu12+
#
# Copyright (C) 2016-2017 Teddysun <i@teddysun.com>
#
# URL: https://teddysun.com/489.html
#

red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
plain='\033[0m'

cur_dir=$(pwd)

[[ $EUID -ne 0 ]] && echo -e "${red}Error:${plain} This script must be run as root!" && exit 1

[[ -d "/proc/vz" ]] && echo -e "${red}Error:${plain} Your VPS is based on OpenVZ, not be supported." && exit 1

if [ -f /etc/redhat-release ]; then
    release="centos"
elif cat /etc/issue | grep -Eqi "debian"; then
    release="debian"
elif cat /etc/issue | grep -Eqi "ubuntu"; then
    release="ubuntu"
elif cat /etc/issue | grep -Eqi "centos|red hat|redhat"; then
    release="centos"
elif cat /proc/version | grep -Eqi "debian"; then
    release="debian"
elif cat /proc/version | grep -Eqi "ubuntu"; then
    release="ubuntu"
elif cat /proc/version | grep -Eqi "centos|red hat|redhat"; then
    release="centos"
fi

get_latest_version() {

    latest_version=$(wget -qO- http://kernel.ubuntu.com/~kernel-ppa/mainline/ | awk -F'\"v' '/v[4-9]./{print $2}' | cut -d/ -f1 | grep -v -  | sort -V | tail -1)

    [ -z ${latest_version} ] && return 1

    if [[ `getconf WORD_BIT` == "32" && `getconf LONG_BIT` == "64" ]]; then
        deb_name=$(wget -qO- http://kernel.ubuntu.com/~kernel-ppa/mainline/v${latest_version}/ | grep "linux-image" | grep "generic" | awk -F'\">' '/amd64.deb/{print $2}' | cut -d'<' -f1 | head -1)
        deb_kernel_url="http://kernel.ubuntu.com/~kernel-ppa/mainline/v${latest_version}/${deb_name}"
        deb_kernel_name="linux-image-${latest_version}-amd64.deb"
    else
        deb_name=$(wget -qO- http://kernel.ubuntu.com/~kernel-ppa/mainline/v${latest_version}/ | grep "linux-image" | grep "generic" | awk -F'\">' '/i386.deb/{print $2}' | cut -d'<' -f1 | head -1)
        deb_kernel_url="http://kernel.ubuntu.com/~kernel-ppa/mainline/v${latest_version}/${deb_name}"
        deb_kernel_name="linux-image-${latest_version}-i386.deb"
    fi

    [ ! -z ${deb_name} ] && return 0 || return 1
}

get_opsy() {
    [ -f /etc/redhat-release ] && awk '{print ($1,$3~/^[0-9]/?$3:$4)}' /etc/redhat-release && return
    [ -f /etc/os-release ] && awk -F'[= "]' '/PRETTY_NAME/{print $3,$4,$5}' /etc/os-release && return
    [ -f /etc/lsb-release ] && awk -F'[="]+' '/DESCRIPTION/{print $2}' /etc/lsb-release && return
}

opsy=$( get_opsy )
arch=$( uname -m )
lbit=$( getconf LONG_BIT )
kern=$( uname -r )

get_char() {
    SAVEDSTTY=`stty -g`
    stty -echo
    stty cbreak
    dd if=/dev/tty bs=1 count=1 2> /dev/null
    stty -raw
    stty echo
    stty $SAVEDSTTY
}

getversion() {
    if [[ -s /etc/redhat-release ]]; then
        grep -oE  "[0-9.]+" /etc/redhat-release
    else
        grep -oE  "[0-9.]+" /etc/issue
    fi
}

centosversion() {
    if [ "${release}" == "centos" ]; then
        local code=$1
        local version="$(getversion)"
        local main_ver=${version%%.*}
        if [ "$main_ver" == "$code" ]; then
            return 0
        else
            return 1
        fi
    else
        return 1
    fi
}

check_bbr_status() {
    local param=$(sysctl net.ipv4.tcp_available_congestion_control | awk '{print $3}')
    if [[ "${param}" == "bbr" ]]; then
        return 0
    else
        return 1
    fi
}

version_ge(){
    test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" == "$1"
}

check_kernel_version() {
    local kernel_version=$(uname -r | cut -d- -f1)
    if version_ge ${kernel_version} 4.9; then
        return 0
    else
        return 1
    fi
}

install_elrepo() {

    if centosversion 5; then
        echo -e "${red}Error:${plain} not supported CentOS 5."
        exit 1
    fi

    rpm --import https://www.elrepo.org/RPM-GPG-KEY-elrepo.org

    if centosversion 6; then
        rpm -Uvh http://www.elrepo.org/elrepo-release-6-8.el6.elrepo.noarch.rpm
    elif centosversion 7; then
        rpm -Uvh http://www.elrepo.org/elrepo-release-7.0-3.el7.elrepo.noarch.rpm
    fi

    if [ ! -f /etc/yum.repos.d/elrepo.repo ]; then
        echo -e "${red}Error:${plain} Install elrepo failed, please check it."
        exit 1
    fi
}

sysctl_config() {
    sed -i '/net.core.default_qdisc/d' /etc/sysctl.conf
    sed -i '/net.ipv4.tcp_congestion_control/d' /etc/sysctl.conf
    echo "net.core.default_qdisc = fq" >> /etc/sysctl.conf
    echo "net.ipv4.tcp_congestion_control = bbr" >> /etc/sysctl.conf
    sysctl -p >/dev/null 2>&1
}

install_config() {
    if [[ "${release}" == "centos" ]]; then
        if centosversion 6; then
            if [ ! -f "/boot/grub/grub.conf" ]; then
                echo -e "${red}Error:${plain} /boot/grub/grub.conf not found, please check it."
                exit 1
            fi
            sed -i 's/^default=.*/default=0/g' /boot/grub/grub.conf
        elif centosversion 7; then
            if [ ! -f "/boot/grub2/grub.cfg" ]; then
                echo -e "${red}Error:${plain} /boot/grub2/grub.cfg not found, please check it."
                exit 1
            fi
            grub2-set-default 0
        fi
    elif [[ "${release}" == "debian" || "${release}" == "ubuntu" ]]; then
        /usr/sbin/update-grub
    fi
}

reboot_os() {
    echo
    echo -e "${green}Info:${plain} The system needs to reboot."
    read -p "Do you want to restart system? [y/n]" is_reboot
    if [[ ${is_reboot} == "y" || ${is_reboot} == "Y" ]]; then
        reboot
    else
        echo -e "${green}Info:${plain} Reboot has been canceled..."
        exit 0
    fi
}

install_bbr() {
    check_bbr_status
    if [ $? -eq 0 ]; then
        echo
        echo -e "${green}Info:${plain} TCP BBR has been installed. nothing to do..."
        exit 0
    fi
    check_kernel_version
    if [ $? -eq 0 ]; then
        echo
        echo -e "${green}Info:${plain} Your kernel version is greater than 4.9, directly setting TCP BBR..."
        sysctl_config
        echo -e "${green}Info:${plain} Setting TCP BBR completed..."
        exit 0
    fi

    if [[ "${release}" == "centos" ]]; then
        install_elrepo
        yum --enablerepo=elrepo-kernel -y install kernel-ml kernel-ml-devel
        if [ $? -ne 0 ]; then
            echo -e "${red}Error:${plain} Install latest kernel failed, please check it."
            exit 1
        fi
    elif [[ "${release}" == "debian" || "${release}" == "ubuntu" ]]; then
        [[ ! -e "/usr/bin/wget" ]] && apt-get -y update && apt-get -y install wget
        get_latest_version
        [ $? -ne 0 ] && echo -e "${red}Error:${plain} Get latest kernel version failed." && exit 1
        wget -c -t3 -T60 -O ${deb_kernel_name} ${deb_kernel_url}
        if [ $? -ne 0 ]; then
            echo -e "${red}Error:${plain} Download ${deb_kernel_name} failed, please check it."
            exit 1
        fi
        dpkg -i ${deb_kernel_name}
        rm -fv ${deb_kernel_name}
    else
        echo -e "${red}Error:${plain} OS is not be supported, please change to CentOS/Debian/Ubuntu and try again."
        exit 1
    fi

    install_config
    sysctl_config
    reboot_os
}


clear
echo "---------- System Information ----------"
echo " OS      : $opsy"
echo " Arch    : $arch ($lbit Bit)"
echo " Kernel  : $kern"
echo "----------------------------------------"
echo " Auto install latest kernel for TCP BBR"
echo
echo " URL: https://teddysun.com/489.html"
echo "----------------------------------------"
echo
echo "Press any key to start...or Press Ctrl+C to cancel"
char=`get_char`

install_bbr 2>&1 | tee ${cur_dir}/install_bbr.log
