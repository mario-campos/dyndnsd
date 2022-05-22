# dyndnsd [![C/C++ CI](https://github.com/mario-campos/dyndnsd/actions/workflows/c.yml/badge.svg)](https://github.com/mario-campos/dyndnsd/actions/workflows/c.yml)
dyndnsd is a Dynamic-DNS daemon for OpenBSD. It is minimal, lightweight, intuitive, and generic/extensible enough to support any Dynamic-DNS provider.

## Example

First, create the configuration file, */etc/dyndnsd.conf*:

```
run "curl https://www.duckdns.org/update?domains=${DYNDNSD_FQDN}&token=sometoken&ip=${DYNDNSD_IPADDR}"

interface em0 {
	domain www.example.com
}

interface em1 {
	domain ftp.example.com
}
```

Test the configuration file:

```shell
$ dyndnsd -n
$
```

Then, start the daemon:

```shell
$ dyndnsd
$
```

## Build

dyndnsd has no external dependencies&mdash;it's only dependency is libevent, but that's included in OpenBSD base&mdash;on so compiling dyndnsd is straightforward:

```shell
make
```

## TODO

### Features

- [x] route(4)
- [x] kqueue(2)
- [x] pledge(2)
- [x] drop privilege

### Code Quality

- [ ] Fuzz Testing
- [x] Valgrind
- [x] cppcheck
