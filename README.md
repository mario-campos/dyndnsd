# dyndnsd [![builds.sr.ht status](https://builds.sr.ht/~mariocampos/dyndnsd.svg)](https://builds.sr.ht/~mariocampos/dyndnsd?) [![code-inspector-status](https://www.code-inspector.com/project/1616/score/svg)](https://www.code-inspector.com/public/project/1616/dyndnsd/dashboard)
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

dyndnsd compiles with some configurations, such as the default configuration-file path and the user/group to which to drop privilege. The build command below will compile dyndnsd with the default configuration-file path */etc/dyndnsd.conf* and unprivileged user/group *nobody*:

```shell
make
```

To set the the default configuration-file path and/or the unprivileged user/group, modify the build command below as necessary:

```shell
make CPPFLAGS='-DDYNDNSD_CONF_PATH=\"/path/to/dyndnsd.conf\" -DDYNDNSD_USER=\"foo\" -DDYNDNSD_GROUP=\"foo\"'
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
