# dyndnsd
Dynamic DNS Daemon for OpenBSD

## Example

First, create the configuration file, */etc/dyndnsd.conf*:

```
user dyndnsd
group dyndnsd

update https://www.duckdns.org/update?domains=${FQDN}&token=sometoken&ip=${IPv4_ADDRESS}

interface em0 {
	domain example.com
	domain www.example.com
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

## TODO

### Features

- [x] route(4)
- [x] kqueue(2)
- [x] IPv4
- [ ] IPv6
- [ ] Unicode
- [x] pledge(2)
- [x] drop privilege

### Code Quality

- [ ] Unit Testing
- [ ] Fuzz Testing
- [ ] Valgrind
- [ ] Other Static Analyzers 
