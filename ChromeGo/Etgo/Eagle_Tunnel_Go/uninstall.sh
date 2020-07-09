#!/bin/bash

rm -rf /etc/eagle-tunnel.d
rm -f /usr/bin/et
rm -rf /usr/lib/eagle-tunnel 
rm -f /usr/lib/systemd/system/eagle-tunnel-server.service
rm -f /usr/lib/systemd/system/eagle-tunnel-client.service

./scripts/after-uninstall.sh