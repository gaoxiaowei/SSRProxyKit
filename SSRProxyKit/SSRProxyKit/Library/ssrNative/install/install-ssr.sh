#!/usr/bin/env bash
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH
#=================================================================#
#   System Required:  CentOS 7, Debian, Ubuntu                    #
#   Description: Script of Install ShadowsocksR Native Server     #
#   Author: ssrlive                                               #
#=================================================================#

proj_name="ShadowsocksR Native"

daemon_script_url="https://raw.githubusercontent.com/ShadowsocksR-Live/shadowsocksr-native/master/install/ssr-daemon.sh"

target_dir=/usr/bin
config_dir=/etc/ssr-native
service_dir=/lib/systemd/system
service_name=ssr-native
service_stub=/etc/init.d/${service_name}
bin_name=ssr-server

#Current folder
cur_dir=`pwd`

cmd_success=0
cmd_failed=1

# Stream Ciphers
ciphers=(
none
table
rc4
rc4-md5-6
rc4-md5
aes-128-cfb
aes-192-cfb
aes-256-cfb
aes-128-ctr
aes-192-ctr
aes-256-ctr
bf-cfb
camellia-128-cfb
camellia-192-cfb
camellia-256-cfb
salsa20
chacha20
chacha20-ietf
)

# Reference URL:
# https://github.com/shadowsocksr-rm/shadowsocks-rss/blob/master/ssr.md
# Protocol
protocols=(
origin
auth_sha1_v4
auth_aes128_md5
auth_aes128_sha1
auth_chain_a
auth_chain_b
auth_chain_c
auth_chain_d
auth_chain_e
auth_chain_f
)

# obfs
obfs=(
plain
http_simple
http_post
tls1.2_ticket_auth
tls1.2_ticket_fastauth
)

# Color
red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
plain='\033[0m'

# Disable selinux
disable_selinux(){
    if [ -s /etc/selinux/config ] && grep 'SELINUX=enforcing' /etc/selinux/config; then
        sed -i 's/SELINUX=enforcing/SELINUX=disabled/g' /etc/selinux/config
        setenforce 0
    fi
}

#Check system
check_sys(){
    local checkType=${1}
    local value=${2}

    local release=''
    local systemPackage=''

    if [[ -f /etc/redhat-release ]]; then
        release="centos"
        systemPackage="yum"
    elif grep -Eqi "debian" /etc/issue; then
        release="debian"
        systemPackage="apt"
    elif grep -Eqi "ubuntu" /etc/issue; then
        release="ubuntu"
        systemPackage="apt"
    elif grep -Eqi "centos|red hat|redhat" /etc/issue; then
        release="centos"
        systemPackage="yum"
    elif grep -Eqi "debian" /proc/version; then
        release="debian"
        systemPackage="apt"
    elif grep -Eqi "ubuntu" /proc/version; then
        release="ubuntu"
        systemPackage="apt"
    elif grep -Eqi "centos|red hat|redhat" /proc/version; then
        release="centos"
        systemPackage="yum"
    fi

    if [[ "${checkType}" == "sysRelease" ]]; then
        if [ "${value}" == "${release}" ]; then
            return ${cmd_success}
        else
            return ${cmd_failed}
        fi
    elif [[ "${checkType}" == "packageManager" ]]; then
        if [ "${value}" == "${systemPackage}" ]; then
            return ${cmd_success}
        else
            return ${cmd_failed}
        fi
    fi
}

# Get version
get_version(){
    if [[ -s /etc/redhat-release ]]; then
        grep -oE  "[0-9.]+" /etc/redhat-release
    else
        grep -oE  "[0-9.]+" /etc/issue
    fi
}

# CentOS version
centosversion(){
    if check_sys sysRelease centos; then
        local code=${1}
        local version="$(get_version)"
        local main_ver=${version%%.*}
        if [ "${main_ver}" == "${code}" ]; then
            return ${cmd_success}
        else
            return ${cmd_failed}
        fi
    else
        return ${cmd_failed}
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
        # Not support CentOS 5, 6
        if centosversion 5 || centosversion 6; then
            echo -e "$[{red}Error${plain}] Not supported CentOS 5, 6, please change to CentOS 7+/Debian 7+/Ubuntu 12+ and try again."
            exit 1
        fi
    else
        echo -e "[${red}Error${plain}] Your OS is not supported. please change OS to CentOS/Debian/Ubuntu and try again."
        exit 1
    fi
    # Set ShadowsocksR config password
    echo "Please enter password for ${proj_name}:"
    rnd_psw=$(cat /proc/sys/kernel/random/uuid)
    read -p "(Default password: ${rnd_psw}):" shadowsockspwd
    [ -z "${shadowsockspwd}" ] && shadowsockspwd=${rnd_psw}
    echo
    echo "---------------------------"
    echo "password = ${shadowsockspwd}"
    echo "---------------------------"
    echo

    # Set ShadowsocksR config port
    while true; do
        dport=$(shuf -i 9000-19999 -n 1)
        echo -e "Please enter a port for ${proj_name} [1-65535]"
        read -p "(Default port: ${dport}):" shadowsocksport
        [ -z "${shadowsocksport}" ] && shadowsocksport=${dport}
        expr ${shadowsocksport} + 1 &>/dev/null
        if [ $? -eq 0 ]; then
            if [ ${shadowsocksport} -ge 1 ] && [ ${shadowsocksport} -le 65535 ] && [ ${shadowsocksport:0:1} != 0 ]; then
                break
            fi
        fi
        echo -e "[${red}Error${plain}] Please enter a correct number [1-65535]"
    done
    echo
    echo "---------------------------"
    echo "port = ${shadowsocksport}"
    echo "---------------------------"
    echo

    # Set shadowsocksR config stream ciphers
    while true ; do
        echo -e "Please select stream cipher for ${proj_name}:"
        for (( i=1; i<=${#ciphers[@]}; i++ )); do
            hint="${ciphers[$i-1]}"
            # echo -e "${green}${i}${plain}) ${hint}"
            printf "%2d) %s\n" ${i} ${hint}
        done
        read -p "Which cipher you'd select(Default: ${ciphers[8]}):" pick
        [ -z "$pick" ] && pick=9
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
        break
    done
    echo
    echo "---------------------------"
    echo "cipher = ${shadowsockscipher}"
    echo "---------------------------"
    echo

    # Set shadowsocksR config protocol
    while true ; do
        echo -e "Please select protocol for ${proj_name}:"
        for ((i=1;i<=${#protocols[@]};i++ )); do
            hint="${protocols[$i-1]}"
            # echo -e "${green}${i}${plain}) ${hint}"
            printf "%2d) %s\n" ${i} ${hint}
        done
        read -p "Which protocol you'd select(Default: ${protocols[2]}):" protocol
        [ -z "$protocol" ] && protocol=3
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
        break
    done
    echo
    echo "---------------------------"
    echo "protocol = ${shadowsockprotocol}"
    echo "---------------------------"
    echo

    # Set shadowsocksR config obfs
    while true ; do
        echo -e "Please select obfs for ${proj_name}:"
        for ((i=1;i<=${#obfs[@]};i++ )); do
            hint="${obfs[$i-1]}"
            echo -e "${green}${i}${plain}) ${hint}"
        done
        read -p "Which obfs you'd select(Default: ${obfs[3]}):" r_obfs
        [ -z "$r_obfs" ] && r_obfs=4
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
        break
    done
    echo
    echo "---------------------------"
    echo "obfs = ${shadowsockobfs}"
    echo "---------------------------"
    echo
}

install_build_tools() {
    if check_sys sysRelease debian ; then
        apt-get remove upstart -y
        apt-get remove udev -y
        apt-get autoremove -y
    fi

    # Install necessary dependencies
    if check_sys packageManager yum; then
        yum install wget curl git gcc gcc-c++ autoconf automake libtool make asciidoc xmlto -y
    elif check_sys packageManager apt; then
        apt-get -f install -y
        apt-get -y update
        apt-get -y upgrade
        apt-get -y install --no-install-recommends build-essential autoconf libtool asciidoc xmlto
        apt-get -y install git gcc g++ wget curl automake
    fi
    
    curl https://cmake.org/files/v3.12/cmake-3.12.1-Linux-x86_64.sh -o cmake_pkg.sh
    sh cmake_pkg.sh --prefix=/usr/ --exclude-subdir && rm -rf cmake_pkg.sh
}

build_ssr_native(){
    git clone https://github.com/ShadowsocksR-Live/shadowsocksr-native.git
    cd shadowsocksr-native
    git submodule update --init
    git submodule foreach -q 'git checkout $(git config -f $toplevel/.gitmodules submodule.$name.branch || echo master)'

    # build ShadowsocksR-native
    cmake . && make

    cd ..

    /bin/cp -rfa ./shadowsocksr-native/src/${bin_name} ${target_dir}

    rm -rf shadowsocksr-native
}

# Firewall set
firewall_settings(){
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
write_ssr_config(){
    public_ip=$(get_ip)

    [ ! -d ${config_dir} ] && mkdir ${config_dir}

    cat > ${config_dir}/config.json<<-EOF
{
    "server": "${public_ip}",
    "server_port": ${shadowsocksport},
    "password": "${shadowsockspwd}",
    "method": "${shadowsockscipher}",
    "protocol": "${shadowsockprotocol}",
    "protocol_param": "",
    "obfs": "${shadowsockobfs}",
    "obfs_param": "",
    "local_address": "127.0.0.1",
    "local_port": 1080,
    "udp": true,
    "timeout": 300
}
EOF
}

write_service_description_file(){
    local svc_name=${1}
    local svc_stub=${2}

    cat > ${service_dir}/${svc_name}.service<<-EOF
[Unit]
    Description=${svc_name}
    After=network.target
[Service]
    Type=forking
    ExecStart=${svc_stub} start
    ExecReload=${svc_stub} restart
    ExecStop=${svc_stub} stop
    PrivateTmp=true
[Install]
    WantedBy=multi-user.target
EOF

    chmod 754 ${service_dir}/${svc_name}.service
}

# Install ShadowsocksR Native
install_ssr_service(){
    ldconfig
    # Install ShadowsocksR Native
    cd ${cur_dir}
    if [ -f ${target_dir}/${bin_name} ]; then

        # Download ShadowsocksR Native service script
        if ! wget --no-check-certificate ${daemon_script_url} -O ${service_stub} ; then
            echo -e "[${red}Error${plain}] Failed to download ${proj_name} Native chkconfig file!"
            exit 1
        fi

        chmod +x ${service_stub}
        if check_sys packageManager yum; then
            chkconfig --add ${service_name}
            chkconfig ${service_name} on
        elif check_sys packageManager apt; then
            update-rc.d -f ${service_name} defaults
        fi

        write_service_description_file ${service_name} ${service_stub}

        # ${service_stub} start
        systemctl enable ${service_name}.service
        systemctl start ${service_name}.service

        # clear
        echo
        echo -e "Congratulations, ${proj_name} server install completed!"
        echo -e "Your Server IP        : \033[41;37m $(get_ip) \033[0m"
        echo -e "Your Server Port      : \033[41;37m ${shadowsocksport} \033[0m"
        echo -e "Your Password         : \033[41;37m ${shadowsockspwd} \033[0m"
        echo -e "Your Protocol         : \033[41;37m ${shadowsockprotocol} \033[0m"
        echo -e "Your obfs             : \033[41;37m ${shadowsockobfs} \033[0m"
        echo -e "Your Encryption Method: \033[41;37m ${shadowsockscipher} \033[0m"
        echo
        echo "Enjoy it!"
        echo
    else
        echo "${proj_name} install failed, please contact @ssrlive"
        exit 1
    fi
}

# Uninstall ShadowsocksR
uninstall_shadowsocksr(){
    printf "Are you sure uninstall ${proj_name}? (y/n)\n"
    read -p "(Default: n):" answer
    [ -z ${answer} ] && answer="n"
    if [ "${answer}" == "y" ] || [ "${answer}" == "Y" ]; then
        ${service_stub} status > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            ${service_stub} stop
        fi
        if check_sys packageManager yum; then
            chkconfig --del ${service_name}
        elif check_sys packageManager apt; then
            update-rc.d -f ${service_name} remove
        fi

        systemctl stop ${service_name}.service

        rm -rf ${config_dir}
        rm -f ${service_stub}
        rm -f ${target_dir}/${bin_name}
        rm -f ${service_dir}/${service_name}.service
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

    echo
    echo "Press any key to start...or Press Ctrl+C to cancel"
    char=`get_char`

    cd ${cur_dir}

    install_build_tools

    build_ssr_native

    write_ssr_config

    if check_sys packageManager yum; then
        firewall_settings
    fi

    install_ssr_service
}

main(){
    clear
    echo
    echo "####################################################################"
    echo "# Script of Install ${proj_name} Server                     #"
    echo "# Author: ssrlive                                                  #"
    echo "# Github: https://github.com/ShadowsocksR-Live/shadowsocksr-native #"
    echo "####################################################################"
    echo

    # Make sure only root can run our script
    [[ $EUID -ne 0 ]] && echo -e "[${red}Error${plain}] This script must be run as root!" && exit 1

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

    exit 0
}

#=================================================================#
#                    script begin entry                           #
#=================================================================#

main
