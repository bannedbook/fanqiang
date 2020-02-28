


How to to make linux allow non-root user listen on 53 port:
`sudo setcap 'cap_net_bind_service=+ep' /usr/bin/python2.7`

Notice:
setcap is in the debian package libcap2-bin
at least a 2.6.24 kernel




