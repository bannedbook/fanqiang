#! /bin/bash

echo "get parameters..."

RootPath=$(pwd)
PublishPath="${RootPath}/publish"

HttpSrcPath="${PublishPath}/http"
ConfSrcPath="${PublishPath}/config"
ServiceSrcPath="${PublishPath}/services"

LibDesPath="/usr/lib/eagle-tunnel"
ServiceDesPath="/usr/lib/systemd/system"
ConfDesPath="/etc/eagle-tunnel.d"

echo "done"