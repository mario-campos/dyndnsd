# dyndnsd [![builds.sr.ht status](https://builds.sr.ht/~mariocampos/dyndnsd.svg)](https://builds.sr.ht/~mariocampos/dyndnsd?) [![code-inspector-status](https://www.code-inspector.com/project/1616/score/svg)](https://www.code-inspector.com/public/project/1616/dyndnsd/dashboard)
dyndnsd is a dynamic DNS daemon for OpenBSD. It is minimal, lightweight, intuitive, and generic/extensible enough to support any dynamic-dns provider.

## Example

First, create the configuration file, */etc/dyndnsd.conf*:

```
run curl https://www.duckdns.org/update?domains=${DYNDNSD_FQDN}&token=sometoken&ip=${DYNDNSD_IPADDR}

interface em0 {
	domain www.example.com
}

interface em1 {
	domain ftp.example.com
}
```

Test the configuration file:

```bash
$ dyndnsd -n
$
```

Then, start the daemon:

```bash
$ dyndnsd
$
```

## Build

```bash
make CONF_PATH=/etc/dyndnsd.conf USER=nobody GROUP=nobody
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
