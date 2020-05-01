# Shadowsocks-libev Docker Image

[shadowsocks-libev][1] is a lightweight secured socks5 proxy for embedded
devices and low end boxes.  It is a port of [shadowsocks][2] created by
@clowwindy maintained by @madeye and @linusyang.

Docker images are built for quick deployment in various computing cloud providers. For more information on docker and containerization technologies, refer to [official document][9].

## Prepare the host

Many cloud providers offer docker-ready environments, for instance the [CoreOS Droplet in DigitalOcean][10] or the [Container-Optimized OS in Google Cloud][11].

If you need to install docker yourself, follow the [official installation guide][12].

## Pull the image

```bash
$ docker pull shadowsocks/shadowsocks-libev
```
This pulls the latest release of shadowsocks-libev.

You can also choose to pull a previous release or to try the bleeding edge build:
```bash
$ docker pull shadowsocks/shadowsocks-libev:<tag>
$ docker pull shadowsocks/shadowsocks-libev:edge
```
> A list of supported tags can be found at [Docker Hub][13].

## Start a container

```bash
$ docker run -p 8388:8388 -p 8388:8388/udp -d --restart always shadowsocks/shadowsocks-libev:latest
```
This starts a container of the latest release with all the default settings, which is equivalent to
```bash
$ ss-server -s 0.0.0.0 -p 8388 -k "$(hostname)" -m aes-256-gcm -t 300 --fast-open -d "8.8.8.8,8.8.4.4" -u
```
> **Note**: It's the hostname in the container that is used as the password, not that of the host.

### With custom port

In most cases you'll want to change a thing or two, for instance the port which the server listens on. This is done by changing the `-p` arguments.

Here's an example to start a container that listens on `28388` (both TCP and UDP):
```bash
$ docker run -p 28388:8388 -p 28388:8388/udp -d --restart always shadowsocks/shadowsocks-libev
```

### With custom password

Another thing you may want to change is the password. To change that, you can pass your own password as an environment variable when starting the container.

Here's an example to start a container with `9MLSpPmNt` as the password:
```bash
$ docker run -e PASSWORD=9MLSpPmNt -p 8388:8388 -p 8388:8388/udp -d --restart always shadowsocks/shadowsocks-libev
```
> :warning: Click [here][6] to generate a strong password to protect your server.

### With other customizations
Besides `PASSWORD`, the image also defines the following environment variables that you can customize:
* `SERVER_ADDR`: the IP/domain to bind to, defaults to `0.0.0.0`
* `SERVER_ADDR_IPV6`: the IPv6 address to bind to, defaults to `::0`
* `METHOD`: encryption method to use, defaults to `aes-256-gcm`
* `TIMEOUT`: defaults to `300`
* `DNS_ADDRS`: DNS servers to redirect NS lookup requests to, defaults to `8.8.8.8,8.8.4.4`

Additional arguments supported by `ss-server` can be passed with the environment variable `ARGS`, for instance to start in verbose mode:
```bash
$ docker run -e ARGS=-v -p 8388:8388 -p 8388:8388/udp -d --restart always shadowsocks/shadowsocks-libev:latest
```

## Use docker-compose to manage (optional)

It is very handy to use [docker-compose][7] to manage docker containers.
You can download the binary at <https://github.com/docker/compose/releases>.

This is a sample `docker-compose.yml` file.

```yaml
shadowsocks:
  image: shadowsocks/shadowsocks-libev
  ports:
    - "8388:8388"
  environment:
    - METHOD=aes-256-gcm
    - PASSWORD=9MLSpPmNt
  restart: always
```

It is highly recommended that you setup a directory tree to make things easy to manage.

```bash
$ mkdir -p ~/fig/shadowsocks/
$ cd ~/fig/shadowsocks/
$ curl -sSLO https://github.com/shadowsocks/shadowsocks-libev/raw/master/docker/alpine/docker-compose.yml
$ docker-compose up -d
$ docker-compose ps
```

## Finish

At last, download shadowsocks client [here][8].
Don't forget to share internet with your friends.

```yaml
{
    "server": "your-vps-ip",
    "server_port": 8388,
    "local_address": "0.0.0.0",
    "local_port": 1080,
    "password": "9MLSpPmNt",
    "timeout": 600,
    "method": "aes-256-gcm"
}
```

[1]: https://github.com/shadowsocks/shadowsocks-libev
[2]: https://shadowsocks.org/en/index.html
[6]: https://duckduckgo.com/?q=password+12&t=ffsb&ia=answer
[7]: https://github.com/docker/compose
[8]: https://shadowsocks.org/en/download/clients.html
[9]: https://docs.docker.com/
[10]: https://www.digitalocean.com/products/linux-distribution/coreos/
[11]: https://cloud.google.com/container-optimized-os/
[12]: https://docs.docker.com/install/
[13]: https://hub.docker.com/r/shadowsocks/shadowsocks-libev/tags/
