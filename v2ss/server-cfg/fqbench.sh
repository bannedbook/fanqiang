#!/usr/bin/env bash
#
# Description: Auto test download & I/O speed script
#
# Copyright (C) 2015 - 2020 Teddysun <i@teddysun.com>
# Thanks: LookBack <admin@dwhd.org>
# URL: https://teddysun.com/444.html
#
trap _exit INT QUIT TERM

_red() {
    printf '\033[0;31;31m%b\033[0m' "$1"
}

_green() {
    printf '\033[0;31;32m%b\033[0m' "$1"
}

_yellow() {
    printf '\033[0;31;33m%b\033[0m' "$1"
}

_blue() {
    printf '\033[0;31;36m%b\033[0m' "$1"
}

_exists() {
    local cmd="$1"
    if eval type type > /dev/null 2>&1; then
        eval type "$cmd" > /dev/null 2>&1
    elif command > /dev/null 2>&1; then
        command -v "$cmd" > /dev/null 2>&1
    else
        which "$cmd" > /dev/null 2>&1
    fi
    local rt=$?
    return ${rt}
}

_64bit(){
    if [ $(getconf WORD_BIT) = '32' ] && [ $(getconf LONG_BIT) = '64' ]; then
        return 0
    else
        return 1
    fi
}

_exit() {
    _red "\nThe script has been terminated.\n"
    # clean up
    rm -fr speedtest-cli benchtest_*
    exit 1
}

get_opsy() {
    [ -f /etc/redhat-release ] && awk '{print ($1,$3~/^[0-9]/?$3:$4)}' /etc/redhat-release && return
    [ -f /etc/os-release ] && awk -F'[= "]' '/PRETTY_NAME/{print $3,$4,$5}' /etc/os-release && return
    [ -f /etc/lsb-release ] && awk -F'[="]+' '/DESCRIPTION/{print $2}' /etc/lsb-release && return
}

next() {
    printf "%-70s\n" "-" | sed 's/\s/-/g'
}

speed_test() {
    local nodeName="$2"
    [ -z "$1" ] && ./speedtest-cli/speedtest --progress=no --accept-license --accept-gdpr > ./speedtest-cli/speedtest.log 2>&1 || \
    ./speedtest-cli/speedtest --progress=no --server-id=$1 --accept-license --accept-gdpr > ./speedtest-cli/speedtest.log 2>&1
    if [ $? -eq 0 ]; then
        local dl_speed=$(awk '/Download/{print $3" "$4}' ./speedtest-cli/speedtest.log)
        local up_speed=$(awk '/Upload/{print $3" "$4}' ./speedtest-cli/speedtest.log)
        local latency=$(awk '/Latency/{print $2" "$3}' ./speedtest-cli/speedtest.log)
        if [[ -n "${dl_speed}" && -n "${up_speed}" && -n "${latency}" ]]; then
            printf "\033[0;33m%-18s\033[0;32m%-18s\033[0;31m%-20s\033[0;36m%-12s\033[0m\n" " ${nodeName}" "${up_speed}" "${dl_speed}" "${latency}"
        fi
    fi
}

speed() {
    speed_test '' 'Speedtest.net'
    speed_test '5145'  'Beijing    CU'
    speed_test '3633'  'Shanghai   CT'
    speed_test '24447' 'Shanghai   CU'
    speed_test '27594' 'Guangzhou  CT'
    speed_test '26678' 'Guangzhou  CU'
    speed_test '16192' 'Shenzhen   CU'
    speed_test '4515'  'Shenzhen   CM'
    speed_test '32155' 'Hongkong   CN'
    speed_test '13623' 'Singapore  SG'
    speed_test '15047' 'Tokyo      JP'
}

io_test() {
    (LANG=C dd if=/dev/zero of=benchtest_$$ bs=64k count=16k conv=fdatasync && rm -f benchtest_$$ ) 2>&1 | awk -F, '{io=$NF} END { print io}' | sed 's/^[ \t]*//;s/[ \t]*$//'
}

calc_disk() {
    local total_size=0
    local array=$@
    for size in ${array[@]}
    do
        [ "${size}" == "0" ] && size_t=0 || size_t=`echo ${size:0:${#size}-1}`
        [ "`echo ${size:(-1)}`" == "K" ] && size=0
        [ "`echo ${size:(-1)}`" == "M" ] && size=$( awk 'BEGIN{printf "%.1f", '$size_t' / 1024}' )
        [ "`echo ${size:(-1)}`" == "T" ] && size=$( awk 'BEGIN{printf "%.1f", '$size_t' * 1024}' )
        [ "`echo ${size:(-1)}`" == "G" ] && size=${size_t}
        total_size=$( awk 'BEGIN{printf "%.1f", '$total_size' + '$size'}' )
    done
    echo ${total_size}
}

check_virt(){
    _exists "dmesg" && virtualx="$(dmesg 2>/dev/null)"
    if _exists "dmidecode"; then
        sys_manu="$(dmidecode -s system-manufacturer 2>/dev/null)"
        sys_product="$(dmidecode -s system-product-name 2>/dev/null)"
        sys_ver="$(dmidecode -s system-version 2>/dev/null)"
    else
        sys_manu=""
        sys_product=""
        sys_ver=""
    fi
    if   grep -qa docker /proc/1/cgroup; then
        virt="Docker"
    elif grep -qa lxc /proc/1/cgroup; then
        virt="LXC"
    elif grep -qa container=lxc /proc/1/environ; then
        virt="LXC"
    elif [[ -f /proc/user_beancounters ]]; then
        virt="OpenVZ"
    elif [[ "${virtualx}" == *kvm-clock* ]]; then
        virt="KVM"
    elif [[ "${cname}" == *KVM* ]]; then
        virt="KVM"
    elif [[ "${cname}" == *QEMU* ]]; then
        virt="KVM"
    elif [[ "${virtualx}" == *"VMware Virtual Platform"* ]]; then
        virt="VMware"
    elif [[ "${virtualx}" == *"Parallels Software International"* ]]; then
        virt="Parallels"
    elif [[ "${virtualx}" == *VirtualBox* ]]; then
        virt="VirtualBox"
    elif [[ -e /proc/xen ]]; then
        virt="Xen"
    elif [[ "${sys_manu}" == *"Microsoft Corporation"* ]]; then
        if [[ "${sys_product}" == *"Virtual Machine"* ]]; then
            if [[ "${sys_ver}" == *"7.0"* || "${sys_ver}" == *"Hyper-V" ]]; then
                virt="Hyper-V"
            else
                virt="Microsoft Virtual Machine"
            fi
        fi
    else
        virt="Dedicated"
    fi
}

ipv4_info() {
    local org="$(wget -q -T10 -O- ipinfo.io/org)"
    local city="$(wget -q -T10 -O- ipinfo.io/city)"
    local country="$(wget -q -T10 -O- ipinfo.io/country)"
    local region="$(wget -q -T10 -O- ipinfo.io/region)"
    [[ -n "$org" ]] && echo " Organization          : $(_blue "$org")"
    [[ -n "$city" && -n "country" ]] && echo " Location              : $(_blue "$city / $country")"
    [[ -n "$region" ]] && echo " Region                : $(_blue "$region")"
}

install_speedtest() {
    if  [ ! -e "./speedtest-cli/speedtest" ]; then
        _64bit && sys_bit=x86_64 || sys_bit=i386
        url1="https://dl.bintray.com/ookla/download/ookla-speedtest-1.0.0-${sys_bit}-linux.tgz"
        url2="https://dl.lamp.sh/files/ookla-speedtest-1.0.0-${sys_bit}-linux.tgz"
        wget --no-check-certificate -q -T10 -O speedtest.tgz ${url1}
        if [ $? -ne 0 ]; then
            wget --no-check-certificate -q -T10 -O speedtest.tgz ${url2}
            [ $? -ne 0 ] && _red "Error: Failed to download speedtest-cli.\n" && exit 1
        fi
        mkdir -p speedtest-cli && tar zxf speedtest.tgz -C ./speedtest-cli && chmod +x ./speedtest-cli/speedtest
        rm -f speedtest.tgz
    fi
}

! _exists "wget" && _red "Error: wget command not found. You must be install wget command at first.\n" && exit 1
# Get System information
cname=$( awk -F: '/model name/ {name=$2} END {print name}' /proc/cpuinfo | sed 's/^[ \t]*//;s/[ \t]*$//' )
cores=$( awk -F: '/model name/ {core++} END {print core}' /proc/cpuinfo )
freq=$( awk -F'[ :]' '/cpu MHz/ {print $4;exit}' /proc/cpuinfo )
ccache=$( awk -F: '/cache size/ {cache=$2} END {print cache}' /proc/cpuinfo | sed 's/^[ \t]*//;s/[ \t]*$//' )
tram=$( free -m | awk '/Mem/ {print $2}' )
uram=$( free -m | awk '/Mem/ {print $3}' )
swap=$( free -m | awk '/Swap/ {print $2}' )
uswap=$( free -m | awk '/Swap/ {print $3}' )
up=$( awk '{a=$1/86400;b=($1%86400)/3600;c=($1%3600)/60} {printf("%d days, %d hour %d min\n",a,b,c)}' /proc/uptime )
if _exists "w"; then
    load=$( w | head -1 | awk -F'load average:' '{print $2}' | sed 's/^[ \t]*//;s/[ \t]*$//' )
elif _exists "uptime"; then
    load=$( uptime | head -1 | awk -F'load average:' '{print $2}' | sed 's/^[ \t]*//;s/[ \t]*$//' )
fi
opsy=$( get_opsy )
arch=$( uname -m )
if _exists "getconf"; then
    lbit=$( getconf LONG_BIT )
else
    echo ${arch} | grep -q "64" && lbit="64" || lbit="32"
fi
kern=$( uname -r )
disk_size1=($( LANG=C df -hPl | grep -wvE '\-|none|tmpfs|devtmpfs|by-uuid|chroot|Filesystem|udev|docker' | awk '{print $2}' ))
disk_size2=($( LANG=C df -hPl | grep -wvE '\-|none|tmpfs|devtmpfs|by-uuid|chroot|Filesystem|udev|docker' | awk '{print $3}' ))
disk_total_size=$( calc_disk "${disk_size1[@]}" )
disk_used_size=$( calc_disk "${disk_size2[@]}" )
tcpctrl=$( sysctl net.ipv4.tcp_congestion_control | awk -F ' ' '{print $3}' )
check_virt
clear
next
echo " CPU Model             : $(_blue "$cname")"
echo " CPU Cores             : $(_blue "$cores")"
echo " CPU Frequency         : $(_blue "$freq MHz")"
echo " CPU Cache             : $(_blue "$ccache")"
echo " Total Disk            : $(_blue "$disk_total_size GB ($disk_used_size GB Used)")"
echo " Total Mem             : $(_blue "$tram MB ($uram MB Used)")"
echo " Total Swap            : $(_blue "$swap MB ($uswap MB Used)")"
echo " System uptime         : $(_blue "$up")"
echo " Load average          : $(_blue "$load")"
echo " OS                    : $(_blue "$opsy")"
echo " Arch                  : $(_blue "$arch ($lbit Bit)")"
echo " Kernel                : $(_blue "$kern")"
echo " TCP CC                : $(_blue "$tcpctrl")"
echo " Virtualization        : $(_blue "$virt")"
ipv4_info
# next
# io1=$( io_test )
# echo " I/O Speed(1st run)    : $(_yellow "$io1")"
# io2=$( io_test )
# echo " I/O Speed(2nd run)    : $(_yellow "$io2")"
# io3=$( io_test )
# echo " I/O Speed(3rd run)    : $(_yellow "$io3")"
# ioraw1=$( echo $io1 | awk 'NR==1 {print $1}' )
# [ "`echo $io1 | awk 'NR==1 {print $2}'`" == "GB/s" ] && ioraw1=$( awk 'BEGIN{print '$ioraw1' * 1024}' )
# ioraw2=$( echo $io2 | awk 'NR==1 {print $1}' )
# [ "`echo $io2 | awk 'NR==1 {print $2}'`" == "GB/s" ] && ioraw2=$( awk 'BEGIN{print '$ioraw2' * 1024}' )
# ioraw3=$( echo $io3 | awk 'NR==1 {print $1}' )
# [ "`echo $io3 | awk 'NR==1 {print $2}'`" == "GB/s" ] && ioraw3=$( awk 'BEGIN{print '$ioraw3' * 1024}' )
# ioall=$( awk 'BEGIN{print '$ioraw1' + '$ioraw2' + '$ioraw3'}' )
# ioavg=$( awk 'BEGIN{printf "%.1f", '$ioall' / 3}' )
# echo -e " Average I/O speed     : $(_yellow "$ioavg MB/s")"
next
install_speedtest && printf "%-18s%-18s%-20s%-12s\n" " Node Name" "Upload Speed" "Download Speed" "Latency"
speed && rm -fr speedtest-cli
next
