#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static const char *get_family_name(int family)
{
	switch (family) {
		case AF_INET:
			return "AF_INET";
		case AF_INET6:
			return "AF_INET6";
		case AF_UNSPEC:
			return "AF_UNSPEC";
		default:
			return "Unknown";
	}
}

static const char *get_socktype_name(int socktype)
{
	switch (socktype) {
		case SOCK_STREAM:
			return "SOCK_STREAM";
		case SOCK_DGRAM:
			return "SOCK_DGRAM";
		case SOCK_RAW:
			return "SOCK_RAW";
		default:
			return "Unknown";
	}
}

static const char *get_protocol_name(int protocol)
{
	struct protoent *proto = getprotobynumber(protocol);
	if (proto == NULL)
		return "Unknown";
	return proto->p_name;
}

int main()
{
	static const char host[] = "en.wikipedia.org";
	//static const char host[] = "yandex.ru";
	struct addrinfo hints = {
		.ai_flags = AI_CANONNAME,
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};
	struct addrinfo *addrinfo = NULL;
	//int err = getaddrinfo(host, "80", &hints, &addrinfo);
	//int err = getaddrinfo(host, "http", &hints, &addrinfo);
	int err = getaddrinfo(host, "https", &hints, &addrinfo);
	if (err) {
		fprintf(stderr, "getaddrinfo() failed: err=%d", err);
		return 1;
	}

	for (struct addrinfo *cur = addrinfo; cur != NULL; cur = cur->ai_next) {
		printf("\nflags=0x%08X\n", cur->ai_flags);
		printf("family=%s (%d)\n", get_family_name(cur->ai_family), cur->ai_family);
		printf("socktype=%s (%d)\n", get_socktype_name(cur->ai_socktype), cur->ai_socktype);
		printf("protocol=%s %d\n", get_protocol_name(cur->ai_protocol), cur->ai_protocol);
		printf("addrlen=%u\n", cur->ai_addrlen);
		printf("addr.family=%u\n", (unsigned int)cur->ai_addr->sa_family);
		printf("canonname='%s'\n", cur->ai_canonname);
	}

	freeaddrinfo(addrinfo);
	return 0;
}
