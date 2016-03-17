/*!
 * \file        sccp_netsock.c
 * \brief       SCCP NetSock Class
 * \author      Diederik de Groot < ddegroot@users.sourceforge.net >
 * \note        This program is free software and may be modified and distributed under the terms of the GNU Public License.
 *              See the LICENSE file at the top of the source tree.
 * \since       2016-02-02
 */
#include "config.h"
#include "common.h"
#include "sccp_netsock.h"

SCCP_FILE_VERSION(__FILE__, "");

#include "sccp_session.h"
#include <netinet/in.h>

union sockaddr_union {
	struct sockaddr sa;
	struct sockaddr_storage ss;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};

gcc_inline boolean_t sccp_netsock_is_IPv4(const struct sockaddr_storage *sockAddrStorage)
{
	return (sockAddrStorage->ss_family == AF_INET) ? TRUE : FALSE;
}

gcc_inline boolean_t sccp_netsock_is_IPv6(const struct sockaddr_storage *sockAddrStorage)
{
	return (sockAddrStorage->ss_family == AF_INET6) ? TRUE : FALSE;
}

uint16_t sccp_netsock_getPort(const struct sockaddr_storage * sockAddrStorage)
{
	if (sccp_netsock_is_IPv4(sockAddrStorage)) {
		return ntohs(((struct sockaddr_in *) sockAddrStorage)->sin_port);
	}
	if (sccp_netsock_is_IPv6(sockAddrStorage)) {
		return ntohs(((struct sockaddr_in6 *) sockAddrStorage)->sin6_port);
	}
	return 0;
}

void sccp_netsock_setPort(const struct sockaddr_storage *sockAddrStorage, uint16_t port)
{
	if (sccp_netsock_is_IPv4(sockAddrStorage)) {
		((struct sockaddr_in *) sockAddrStorage)->sin_port = htons(port);
	} else if (sccp_netsock_is_IPv6(sockAddrStorage)) {
		((struct sockaddr_in6 *) sockAddrStorage)->sin6_port = htons(port);
	}
}

int sccp_netsock_is_any_addr(const struct sockaddr_storage *sockAddrStorage)
{
	union sockaddr_union tmp_addr = {
		.ss = *sockAddrStorage,
	};

	return (sccp_netsock_is_IPv4(sockAddrStorage) && (tmp_addr.sin.sin_addr.s_addr == INADDR_ANY)) || (sccp_netsock_is_IPv6(sockAddrStorage) && IN6_IS_ADDR_UNSPECIFIED(&tmp_addr.sin6.sin6_addr));
}

static int __netsock_resolve_first_af(struct sockaddr_storage *addr, const char *name, int flags, int family)
{
	struct addrinfo *ai;
	int result = 0, e;
	if (!name) {
		return 0;
	}
	struct addrinfo hints = {
		.ai_family = family,
		.ai_socktype = SOCK_DGRAM
	};
	if (!(e = getaddrinfo(name, 0, &hints, &ai))) {
		if (ai && ai->ai_next) {
			memcpy(addr, ai->ai_addr, ai->ai_addrlen);
			result = 1;
		}
	} else {
		pbx_log(LOG_ERROR, "getaddrinfo(\"%s\") failed: %s\n", name, gai_strerror(e));
	}
	freeaddrinfo(ai);
	return result;
}

boolean_t sccp_netsock_getExternalAddr(struct sockaddr_storage *sockAddrStorage)
{
	boolean_t result = FALSE;
	if (sccp_netsock_is_any_addr(&GLOB(externip))) {
		if (GLOB(externhost) && strlen(GLOB(externhost)) == 0 && GLOB(externrefresh) > 0) {
			static time_t externexpire = 0;
			if (time(NULL) >= externexpire) {
				if (__netsock_resolve_first_af(sockAddrStorage, GLOB(externhost), 0, sockAddrStorage->ss_family)) {
					pbx_log(LOG_NOTICE, "Warning: Re-lookup of '%s' failed!\n", GLOB(externhost));
					return FALSE;
				}
				externexpire = time(NULL) + GLOB(externrefresh);
				result = TRUE;
			}
		} else {
			sccp_log(DEBUGCAT_CORE) (VERBOSE_PREFIX_3 "SCCP: No externip set in sccp.conf. In case you are running your PBX on a seperate host behind a NATTED Firewall you need to set externip.\n");
		}
	} else {
		memcpy(sockAddrStorage, &GLOB(externip), sizeof(struct sockaddr_storage));
		result = TRUE;
	}
	return result;
}

size_t sccp_netsock_sizeof(const struct sockaddr_storage * sockAddrStorage)
{
	if (sccp_netsock_is_IPv4(sockAddrStorage)) {
		return sizeof(struct sockaddr_in);
	}
	if (sccp_netsock_is_IPv6(sockAddrStorage)) {
		return sizeof(struct sockaddr_in6);
	}
	return 0;
}

static int sccp_netsock_is_ipv6_link_local(const struct sockaddr_storage *sockAddrStorage)
{
	union sockaddr_union tmp_addr = {
		.ss = *sockAddrStorage,
	};
	return sccp_netsock_is_IPv6(sockAddrStorage) && IN6_IS_ADDR_LINKLOCAL(&tmp_addr.sin6.sin6_addr);
}

boolean_t sccp_netsock_is_mapped_IPv4(const struct sockaddr_storage *sockAddrStorage)
{
	if (sccp_netsock_is_IPv6(sockAddrStorage)) {
		const struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sockAddrStorage;
		return IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr);
	} 
	return FALSE;
}

boolean_t sccp_netsock_ipv4_mapped(const struct sockaddr_storage *sockAddrStorage, struct sockaddr_storage *sockAddrStorage_mapped)
{
	const struct sockaddr_in6 *sin6;
	struct sockaddr_in sin4;

	if (!sccp_netsock_is_IPv6(sockAddrStorage)) {
		return FALSE;
	}

	if (!sccp_netsock_is_mapped_IPv4(sockAddrStorage)) {
		return FALSE;
	}

	sin6 = (const struct sockaddr_in6 *) sockAddrStorage;

	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
	sin4.sin_port = sin6->sin6_port;
	sin4.sin_addr.s_addr = ((uint32_t *) & sin6->sin6_addr)[3];

	memcpy(sockAddrStorage_mapped, &sin4, sizeof(sin4));
	return TRUE;
}

/*!
 * \brief
 * Compares the addresses of two ast_sockaddr structures.
 *
 * \retval -1 \a a is lexicographically smaller than \a b
 * \retval 0 \a a is equal to \a b
 * \retval 1 \a b is lexicographically smaller than \a a
 */
int sccp_netsock_cmp_addr(const struct sockaddr_storage *a, const struct sockaddr_storage *b)
{
	//char *stra = pbx_strdupa(sccp_netsock_stringify_addr(a));
	//char *strb = pbx_strdupa(sccp_netsock_stringify_addr(b));

	const struct sockaddr_storage *a_tmp, *b_tmp;
	struct sockaddr_storage ipv4_mapped;
	size_t len_a = sccp_netsock_sizeof(a);
	size_t len_b = sccp_netsock_sizeof(b);
	int ret = -1;

	a_tmp = a;
	b_tmp = b;

	if (len_a != len_b) {
		if (sccp_netsock_ipv4_mapped(a, &ipv4_mapped)) {
			a_tmp = &ipv4_mapped;
		} else if (sccp_netsock_ipv4_mapped(b, &ipv4_mapped)) {
			b_tmp = &ipv4_mapped;
		}
	}
	if (len_a < len_b) {
		ret = -1;
		goto EXIT;
	} else if (len_a > len_b) {
		ret = 1;
		goto EXIT;
	}

	if (a_tmp->ss_family == b_tmp->ss_family) {
		if (a_tmp->ss_family == AF_INET) {
			ret = memcmp(&(((struct sockaddr_in *) a_tmp)->sin_addr), &(((struct sockaddr_in *) b_tmp)->sin_addr), sizeof(struct in_addr));
		} else {											// AF_INET6
			ret = memcmp(&(((struct sockaddr_in6 *) a_tmp)->sin6_addr), &(((struct sockaddr_in6 *) b_tmp)->sin6_addr), sizeof(struct in6_addr));
		}
	}
EXIT:
	//sccp_log(DEBUGCAT_HIGH)(VERBOSE_PREFIX_2 "SCCP: sccp_netsock_cmp_addr(%s, %s) returning %d\n", stra, strb, ret);
	return ret;
}

/*!
 * \brief
 * Splits a string into its host and port components
 *
 * \param str       [in] The string to parse. May be modified by writing a NUL at the end of
 *                  the host part.
 * \param host      [out] Pointer to the host component within \a str.
 * \param port      [out] Pointer to the port component within \a str.
 * \param flags     If set to zero, a port MAY be present. If set to PARSE_PORT_IGNORE, a
 *                  port MAY be present but will be ignored. If set to PARSE_PORT_REQUIRE,
 *                  a port MUST be present. If set to PARSE_PORT_FORBID, a port MUST NOT
 *                  be present.
 *
 * \retval 1 Success
 * \retval 0 Failure
 */
int sccp_netsock_split_hostport(char *str, char **host, char **port, int flags)
{
	char *s = str;
	char *orig_str = str;											/* Original string in case the port presence is incorrect. */
	char *host_end = NULL;											/* Delay terminating the host in case the port presence is incorrect. */

	sccp_log(DEBUGCAT_HIGH) (VERBOSE_PREFIX_4 "Splitting '%s' into...\n", str);
	*host = NULL;
	*port = NULL;
	if (*s == '[') {
		*host = ++s;
		for (; *s && *s != ']'; ++s) {
		}
		if (*s == ']') {
			host_end = s;
			++s;
		}
		if (*s == ':') {
			*port = s + 1;
		}
	} else {
		*host = s;
		for (; *s; ++s) {
			if (*s == ':') {
				if (*port) {
					*port = NULL;
					break;
				} else {
					*port = s;
				}
			}
		}
		if (*port) {
			host_end = *port;
			++*port;
		}
	}

	switch (flags & PARSE_PORT_MASK) {
		case PARSE_PORT_IGNORE:
			*port = NULL;
			break;
		case PARSE_PORT_REQUIRE:
			if (*port == NULL) {
				pbx_log(LOG_WARNING, "Port missing in %s\n", orig_str);
				return 0;
			}
			break;
		case PARSE_PORT_FORBID:
			if (*port != NULL) {
				pbx_log(LOG_WARNING, "Port disallowed in %s\n", orig_str);
				return 0;
			}
			break;
	}
	/* Can terminate the host string now if needed. */
	if (host_end) {
		*host_end = '\0';
	}
	sccp_log(DEBUGCAT_HIGH) (VERBOSE_PREFIX_4 "...host '%s' and port '%s'.\n", *host, *port ? *port : "");
	return 1;
}

AST_THREADSTORAGE(sccp_netsock_stringify_buf);
char *__netsock_stringify_fmt(const struct sockaddr_storage *sockAddrStorage, int format)
{
	const struct sockaddr_storage *sockAddrStorage_tmp;
	char host[NI_MAXHOST] = "";
	char port[NI_MAXSERV] = "";
	struct ast_str *str;
	int e;
	static const size_t size = sizeof(host) - 1 + sizeof(port) - 1 + 4;

	if (!sockAddrStorage) {
		return "(null)";
	}

	if (!(str = ast_str_thread_get(&sccp_netsock_stringify_buf, size))) {
		return "";
	}
	//if (sccp_netsock_ipv4_mapped(sockAddrStorage, &sockAddrStorage_tmp_ipv4)) {
	//	struct sockaddr_storage sockAddrStorage_tmp_ipv4;
	//	sockAddrStorage_tmp = &sockAddrStorage_tmp_ipv4;
	//#if DEBUG
	//	sccp_log(0)("SCCP: ipv4 mapped in ipv6 address\n");
	//#endif
	//} else {
	sockAddrStorage_tmp = sockAddrStorage;
	//}

	if ((e = getnameinfo((struct sockaddr *) sockAddrStorage_tmp, sccp_netsock_sizeof(sockAddrStorage_tmp), format & SCCP_SOCKADDR_STR_ADDR ? host : NULL, format & SCCP_SOCKADDR_STR_ADDR ? sizeof(host) : 0, format & SCCP_SOCKADDR_STR_PORT ? port : 0, format & SCCP_SOCKADDR_STR_PORT ? sizeof(port) : 0, NI_NUMERICHOST | NI_NUMERICSERV))) {
		sccp_log(DEBUGCAT_SOCKET) (VERBOSE_PREFIX_3 "SCCP: getnameinfo(): %s \n", gai_strerror(e));
		return "";
	}

	if ((format & SCCP_SOCKADDR_STR_REMOTE) == SCCP_SOCKADDR_STR_REMOTE) {
		char *p;

		if (sccp_netsock_is_ipv6_link_local(sockAddrStorage_tmp) && (p = strchr(host, '%'))) {
			*p = '\0';
		}
	}
	switch ((format & SCCP_SOCKADDR_STR_FORMAT_MASK)) {
		case SCCP_SOCKADDR_STR_DEFAULT:
			ast_str_set(&str, 0, sockAddrStorage_tmp->ss_family == AF_INET6 ? "[%s]:%s" : "%s:%s", host, port);
			break;
		case SCCP_SOCKADDR_STR_ADDR:
			ast_str_set(&str, 0, "%s", host);
			break;
		case SCCP_SOCKADDR_STR_HOST:
			ast_str_set(&str, 0, sockAddrStorage_tmp->ss_family == AF_INET6 ? "[%s]" : "%s", host);
			break;
		case SCCP_SOCKADDR_STR_PORT:
			ast_str_set(&str, 0, "%s", port);
			break;
		default:
			pbx_log(LOG_ERROR, "Invalid format\n");
			return "";
	}

	return ast_str_buffer(str);
}

// kate: indent-width 8; replace-tabs off; indent-mode cstyle; auto-insert-doxygen on; line-numbers on; tab-indents on; keep-extra-spaces off; auto-brackets off;
