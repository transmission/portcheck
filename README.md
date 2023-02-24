## About

This repository contains the server component for portcheck.transmissionbt.com, a small service designed to run an Nginx/lighttpd reverse proxy. It attempts TCP connections to the IPs passed through the `X-Forwarded-For` header and returns `0` or `1` responses.

## Deploying updates
Log into the server vm4.transmissionbt.com (at least titer, john and mikedld have access). Then hop into the portcheck container to do the update:
```
titer@transmission4 ~% lxc exec portcheck bash
root@portcheck:~# cd /portcheck/portcheck
root@portcheck:/portcheck/portcheck# git pull
root@portcheck:/portcheck/portcheck# make
root@portcheck:/portcheck/portcheck# /etc/init.d/portcheck restart
```
