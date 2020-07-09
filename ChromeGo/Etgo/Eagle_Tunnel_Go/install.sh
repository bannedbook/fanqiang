#!/bin/bash

source ./scripts/env_install.sh

echo "installing libs"
mkdir -p ${LibDesPath}
cp -f ${PublishPath}/et.go.linux ${LibDesPath}
cp -f ${PublishPath}/run.sh ${LibDesPath}
ln -sf ${LibDesPath}/run.sh /usr/bin/et

echo "installing systemd units"
mkdir -p ${ServiceDesPath}
cp -f ${ServiceSrcPath}/* ${ServiceDesPath}

echo "installing config files"
mkdir -p ${ConfDesPath}
if [ -f ${ConfDesPath}/client.conf ]; then
    cp -f ${ConfSrcPath}/client.conf ${ConfDesPath}/client.conf_backup
else
    cp ${ConfSrcPath}/client.conf ${ConfDesPath}
fi
if [ -f ${ConfDesPath}/server.conf ]; then
    cp -f ${ConfSrcPath}/server.conf ${ConfDesPath}/server.conf_backup
else
    cp ${ConfSrcPath}/server.conf ${ConfDesPath}
fi

echo "installing clear domains"
ProxyDomainsDir=proxylists
DirectDomainsDir=directlists

ProxyDomainsDesDir=${ConfDesPath}/${ProxyDomainsDir}
DirectDomainsDesDir=${ConfDesPath}/${DirectDomainsDir}

ProxyDomainsSrcDir=${ConfSrcPath}/${ProxyDomainsDir}
DirectDomainsSrcDir=${ConfSrcPath}/${DirectDomainsDir}

mkdir -p ${ProxyDomainsDesDir}
mkdir -p ${DirectDomainsDesDir}

cp -f ${ProxyDomainsSrcDir}/* ${ProxyDomainsDesDir}
cp -f ${DirectDomainsSrcDir}/* ${DirectDomainsDesDir}

echo "installing hosts"
cp -rf ${ConfSrcPath}/hosts ${ConfDesPath}

./scripts/after-install.sh