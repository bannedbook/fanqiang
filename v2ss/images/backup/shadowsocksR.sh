#!/usr/bin/env bash
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH
#=================================================================#
#   System Required:  CentOS 6,7, Debian, Ubuntu                  #
#   Description: One click Install ShadowsocksR Server            #
#   Author: Teddysun <i@teddysun.com>                             #
#   Thanks: @breakwa11 <https://twitter.com/breakwa11>            #
#   Intro:  https://shadowsocks.be/9.html                         #
#=================================================================#

clear
echo
echo "#############################################################"
echo "# One click Install ShadowsocksR Server                     #"
echo "# Intro: https://shadowsocks.be/9.html                      #"
echo "# Author: Teddysun <i@teddysun.com>                         #"
echo "# Github: https://github.com/shadowsocksr/shadowsocksr      #"
echo "#############################################################"
echo

libsodium_file="libsodium-1.0.15"
libsodium_url="https://github.com/jedisct1/libsodium/releases/download/1.0.15/libsodium-1.0.15.tar.gz"

#Current folder
cur_dir=`pwd`
# Stream Ciphers
ciphers=(
none
aes-256-cfb
aes-192-cfb
aes-128-cfb
aes-256-cfb8
aes-192-cfb8
aes-128-cfb8
aes-256-ctr
aes-192-ctr
aes-128-ctr
chacha20-ietf
chacha20
rc4-md5
rc4-md5-6
)
# Reference URL:
# https://github.com/breakwa11/shadowsocks-rss/blob/master/ssr.md
# https://github.com/breakwa11/shadowsocks-rss/wiki/config.json
# Protocol
protocols=(
origin
verify_deflate
auth_sha1_v4
auth_sha1_v4_compatible
auth_aes128_md5
auth_aes128_sha1
auth_chain_a
auth_chain_b
)
# obfs
obfs=(
plain
http_simple
http_simple_compatible
http_post
http_post_compatible
tls1.2_ticket_auth
tls1.2_ticket_auth_compatible
tls1.2_ticket_fastauth
tls1.2_ticket_fastauth_compatible
)
# Color
red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
plain='\033[0m'

# Make sure only root can run our script
[[ $EUID -ne 0 ]] && echo -e "[${red}Error${plain}] This script must be run as root!" && exit 1

# Disable selinux
disable_selinux(){
    if [ -s /etc/selinux/config ] && grep 'SELINUX=enforcing' /etc/selinux/config; then
        sed -i 's/SELINUX=enforcing/SELINUX=disabled/g' /etc/selinux/config
        setenforce 0
    fi
}

#Check system
check_sys(){
    local checkType=$1
    local value=$2

    local release=''
    local systemPackage=''

    if [[ -f /etc/redhat-release ]]; then
        release="centos"
        systemPackage="yum"
    elif cat /etc/issue | grep -Eqi "debian"; then
        release="debian"
        systemPackage="apt"
    elif cat /etc/issue | grep -Eqi "ubuntu"; then
        release="ubuntu"
        systemPackage="apt"
    elif cat /etc/issue | grep -Eqi "centos|red hat|redhat"; then
        release="centos"
        systemPackage="yum"
    elif cat /proc/version | grep -Eqi "debian"; then
        release="debian"
        systemPackage="apt"
    elif cat /proc/version | grep -Eqi "ubuntu"; then
        release="ubuntu"
        systemPackage="apt"
    elif cat /proc/version | grep -Eqi "centos|red hat|redhat"; then
        release="centos"
        systemPackage="yum"
    fi

    if [[ ${checkType} == "sysRelease" ]]; then
        if [ "$value" == "$release" ]; then
            return 0
        else
            return 1
        fi
    elif [[ ${checkType} == "packageManager" ]]; then
        if [ "$value" == "$systemPackage" ]; then
            return 0
        else
            return 1
        fi
    fi
}

# Get version
getversion(){
    if [[ -s /etc/redhat-release ]]; then
        grep -oE  "[0-9.]+" /etc/redhat-release
    else
        grep -oE  "[0-9.]+" /etc/issue
    fi
}

# CentOS version
centosversion(){
    if check_sys sysRelease centos; then
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

# Get public IP address
get_ip(){
    local IP=$( ip addr | egrep -o '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' | egrep -v "^192\.168|^172\.1[6-9]\.|^172\.2[0-9]\.|^172\.3[0-2]\.|^10\.|^127\.|^255\.|^0\." | head -n 1 )
    [ -z ${IP} ] && IP=$( wget -qO- -t1 -T2 ipv4.icanhazip.com )
    [ -z ${IP} ] && IP=$( wget -qO- -t1 -T2 ipinfo.io/ip )
    [ ! -z ${IP} ] && echo ${IP} || echo
}

get_char(){
    SAVEDSTTY=`stty -g`
    stty -echo
    stty cbreak
    dd if=/dev/tty bs=1 count=1 2> /dev/null
    stty -raw
    stty echo
    stty $SAVEDSTTY
}

# Pre-installation settings
pre_install(){
    if check_sys packageManager yum || check_sys packageManager apt; then
        # Not support CentOS 5
        if centosversion 5; then
            echo -e "$[{red}Error${plain}] Not supported CentOS 5, please change to CentOS 6+/Debian 7+/Ubuntu 12+ and try again."
            exit 1
        fi
    else
        echo -e "[${red}Error${plain}] Your OS is not supported. please change OS to CentOS/Debian/Ubuntu and try again."
        exit 1
    fi
    # Set ShadowsocksR config password
    echo "Please enter password for ShadowsocksR:"
    read -p "(Default password: teddysun.com):" shadowsockspwd
    [ -z "${shadowsockspwd}" ] && shadowsockspwd="teddysun.com"
    echo
    echo "---------------------------"
    echo "password = ${shadowsockspwd}"
    echo "---------------------------"
    echo
    # Set ShadowsocksR config port
    while true
    do
    echo -e "Please enter a port for ShadowsocksR [1-65535]:"
    read -p "(Default port: 8989):" shadowsocksport
    [ -z "${shadowsocksport}" ] && shadowsocksport="8989"
    expr ${shadowsocksport} + 1 &>/dev/null
    if [ $? -eq 0 ]; then
        if [ ${shadowsocksport} -ge 1 ] && [ ${shadowsocksport} -le 65535 ] && [ ${shadowsocksport:0:1} != 0 ]; then
            echo
            echo "---------------------------"
            echo "port = ${shadowsocksport}"
            echo "---------------------------"
            echo
            break
        fi
    fi
    echo -e "[${red}Error${plain}] Please enter a correct number [1-65535]"
    done

    # Set shadowsocksR config stream ciphers
    while true
    do
    echo -e "Please select stream cipher for ShadowsocksR:"
    for ((i=1;i<=${#ciphers[@]};i++ )); do
        hint="${ciphers[$i-1]}"
        echo -e "${green}${i}${plain}) ${hint}"
    done
    read -p "Which cipher you'd select(Default: ${ciphers[1]}):" pick
    [ -z "$pick" ] && pick=2
    expr ${pick} + 1 &>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "[${red}Error${plain}] Please enter a number"
        continue
    fi
    if [[ "$pick" -lt 1 || "$pick" -gt ${#ciphers[@]} ]]; then
        echo -e "[${red}Error${plain}] Please enter a number between 1 and ${#ciphers[@]}"
        continue
    fi
    shadowsockscipher=${ciphers[$pick-1]}
    echo
    echo "---------------------------"
    echo "cipher = ${shadowsockscipher}"
    echo "---------------------------"
    echo
    break
    done

    # Set shadowsocksR config protocol
    while true
    do
    echo -e "Please select protocol for ShadowsocksR:"
    for ((i=1;i<=${#protocols[@]};i++ )); do
        hint="${protocols[$i-1]}"
        echo -e "${green}${i}${plain}) ${hint}"
    done
    read -p "Which protocol you'd select(Default: ${protocols[0]}):" protocol
    [ -z "$protocol" ] && protocol=1
    expr ${protocol} + 1 &>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "[${red}Error${plain}] Input error, please input a number"
        continue
    fi
    if [[ "$protocol" -lt 1 || "$protocol" -gt ${#protocols[@]} ]]; then
        echo -e "[${red}Error${plain}] Input error, please input a number between 1 and ${#protocols[@]}"
        continue
    fi
    shadowsockprotocol=${protocols[$protocol-1]}
    echo
    echo "---------------------------"
    echo "protocol = ${shadowsockprotocol}"
    echo "---------------------------"
    echo
    break
    done

    # Set shadowsocksR config obfs
    while true
    do
    echo -e "Please select obfs for ShadowsocksR:"
    for ((i=1;i<=${#obfs[@]};i++ )); do
        hint="${obfs[$i-1]}"
        echo -e "${green}${i}${plain}) ${hint}"
    done
    read -p "Which obfs you'd select(Default: ${obfs[0]}):" r_obfs
    [ -z "$r_obfs" ] && r_obfs=1
    expr ${r_obfs} + 1 &>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "[${red}Error${plain}] Input error, please input a number"
        continue
    fi
    if [[ "$r_obfs" -lt 1 || "$r_obfs" -gt ${#obfs[@]} ]]; then
        echo -e "[${red}Error${plain}] Input error, please input a number between 1 and ${#obfs[@]}"
        continue
    fi
    shadowsockobfs=${obfs[$r_obfs-1]}
    echo
    echo "---------------------------"
    echo "obfs = ${shadowsockobfs}"
    echo "---------------------------"
    echo
    break
    done

    echo
    echo "Press any key to start...or Press Ctrl+C to cancel"
    char=`get_char`
    # Install necessary dependencies
    if check_sys packageManager yum; then
        yum install -y python python-devel python-setuptools openssl openssl-devel curl wget unzip gcc automake autoconf make libtool
    elif check_sys packageManager apt; then
        apt-get -y update
        apt-get -y install python python-dev python-setuptools openssl libssl-dev curl wget unzip gcc automake autoconf make libtool
    fi
    cd ${cur_dir}
}

# Download files
download_files(){
    # Download libsodium file
    if ! wget --no-check-certificate -O ${libsodium_file}.tar.gz ${libsodium_url}; then
        echo -e "[${red}Error${plain}] Failed to download ${libsodium_file}.tar.gz!"
        exit 1
    fi
    # Download ShadowsocksR file
    if ! wget --no-check-certificate -O manyuser.zip https://github.com/teddysun/shadowsocksr/archive/manyuser.zip; then
        echo -e "[${red}Error${plain}] Failed to download ShadowsocksR file!"
        exit 1
    fi
    # Download ShadowsocksR init script
    if check_sys packageManager yum; then
        if ! wget --no-check-certificate https://raw.githubusercontent.com/teddysun/shadowsocks_install/master/shadowsocksR -O /etc/init.d/shadowsocks; then
            echo -e "[${red}Error${plain}] Failed to download ShadowsocksR chkconfig file!"
            exit 1
        fi
    elif check_sys packageManager apt; then
        if ! wget --no-check-certificate https://raw.githubusercontent.com/teddysun/shadowsocks_install/master/shadowsocksR-debian -O /etc/init.d/shadowsocks; then
            echo -e "[${red}Error${plain}] Failed to download ShadowsocksR chkconfig file!"
            exit 1
        fi
    fi
}

# Firewall set
firewall_set(){
    echo -e "[${green}Info${plain}] firewall set start..."
    if centosversion 6; then
        /etc/init.d/iptables status > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            iptables -L -n | grep -i ${shadowsocksport} > /dev/null 2>&1
            if [ $? -ne 0 ]; then
                iptables -I INPUT -m state --state NEW -m tcp -p tcp --dport ${shadowsocksport} -j ACCEPT
                iptables -I INPUT -m state --state NEW -m udp -p udp --dport ${shadowsocksport} -j ACCEPT
                /etc/init.d/iptables save
                /etc/init.d/iptables restart
            else
                echo -e "[${green}Info${plain}] port ${shadowsocksport} has been set up."
            fi
        else
            echo -e "[${yellow}Warning${plain}] iptables looks like shutdown or not installed, please manually set it if necessary."
        fi
    elif centosversion 7; then
        systemctl status firewalld > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            firewall-cmd --permanent --zone=public --add-port=${shadowsocksport}/tcp
            firewall-cmd --permanent --zone=public --add-port=${shadowsocksport}/udp
            firewall-cmd --reload
        else
            echo -e "[${yellow}Warning${plain}] firewalld looks like not running or not installed, please enable port ${shadowsocksport} manually if necessary."
        fi
    fi
    echo -e "[${green}Info${plain}] firewall set completed..."
}

# Config ShadowsocksR
config_shadowsocks(){
    cat > /etc/shadowsocks.json<<-EOF
{
    "server":"0.0.0.0",
    "server_ipv6":"[::]",
    "server_port":${shadowsocksport},
    "local_address":"127.0.0.1",
    "local_port":1080,
    "password":"${shadowsockspwd}",
    "timeout":120,
    "method":"${shadowsockscipher}",
    "protocol":"${shadowsockprotocol}",
    "protocol_param":"",
    "obfs":"${shadowsockobfs}",
    "obfs_param":"",
    "redirect":"",
    "dns_ipv6":false,
    "fast_open":false,
    "workers":1
}
EOF
}

# Install ShadowsocksR
install(){
    # Install libsodium
    if [ ! -f /usr/lib/libsodium.a ]; then
        cd ${cur_dir}
        tar zxf ${libsodium_file}.tar.gz
        cd ${libsodium_file}
        ./configure --prefix=/usr && make && make install
        if [ $? -ne 0 ]; then
            echo -e "[${red}Error${plain}] libsodium install failed!"
            install_cleanup
            exit 1
        fi
    fi

    ldconfig
    # Install ShadowsocksR
    cd ${cur_dir}
    unzip -q manyuser.zip
    mv shadowsocksr-manyuser/shadowsocks /usr/local/
    if [ -f /usr/local/shadowsocks/server.py ]; then
        chmod +x /etc/init.d/shadowsocks
        if check_sys packageManager yum; then
            chkconfig --add shadowsocks
            chkconfig shadowsocks on
        elif check_sys packageManager apt; then
            update-rc.d -f shadowsocks defaults
        fi
        /etc/init.d/shadowsocks start

        clear
        echo
        echo -e "Congratulations, ShadowsocksR server install completed!"
        echo -e "Your Server IP        : \033[41;37m $(get_ip) \033[0m"
        echo -e "Your Server Port      : \033[41;37m ${shadowsocksport} \033[0m"
        echo -e "Your Password         : \033[41;37m ${shadowsockspwd} \033[0m"
        echo -e "Your Protocol         : \033[41;37m ${shadowsockprotocol} \033[0m"
        echo -e "Your obfs             : \033[41;37m ${shadowsockobfs} \033[0m"
        echo -e "Your Encryption Method: \033[41;37m ${shadowsockscipher} \033[0m"
        echo
        echo "Welcome to visit:https://shadowsocks.be/9.html"
        echo "Enjoy it!"
        echo
    else
        echo "ShadowsocksR install failed, please Email to Teddysun <i@teddysun.com> and contact"
        install_cleanup
        exit 1
    fi
}

# Install cleanup
install_cleanup(){
    cd ${cur_dir}
    rm -rf manyuser.zip shadowsocksr-manyuser ${libsodium_file}.tar.gz ${libsodium_file}
}


# Uninstall ShadowsocksR
uninstall_shadowsocksr(){
    printf "Are you sure uninstall ShadowsocksR? (y/n)"
    printf "\n"
    read -p "(Default: n):" answer
    [ -z ${answer} ] && answer="n"
    if [ "${answer}" == "y" ] || [ "${answer}" == "Y" ]; then
        /etc/init.d/shadowsocks status > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            /etc/init.d/shadowsocks stop
        fi
        if check_sys packageManager yum; then
            chkconfig --del shadowsocks
        elif check_sys packageManager apt; then
            update-rc.d -f shadowsocks remove
        fi
        rm -f /etc/shadowsocks.json
        rm -f /etc/init.d/shadowsocks
        rm -f /var/log/shadowsocks.log
        rm -rf /usr/local/shadowsocks
        echo "ShadowsocksR uninstall success!"
    else
        echo
        echo "uninstall cancelled, nothing to do..."
        echo
    fi
}

# Install ShadowsocksR
install_shadowsocksr(){
    disable_selinux
    pre_install
    download_files
    config_shadowsocks
    if check_sys packageManager yum; then
        firewall_set
    fi
    install
    install_cleanup
}

# Initialization step
action=$1
[ -z $1 ] && action=install
case "$action" in
    install|uninstall)
        ${action}_shadowsocksr
        ;;
    *)
        echo "Arguments error! [${action}]"
        echo "Usage: `basename $0` [install|uninstall]"
        ;;
esac
