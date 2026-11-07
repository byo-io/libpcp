#include <netinet/in.h>
#include <pcp/ip.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

constexpr char IPV4_MAPPED_PREFIX[] = "\0\0\0\0\0\0\0\0\0\0\xFF\xFF";

bool pcp_ip_address_is_ipv4(const pcp_ip_address_t *ip) {
  assert(ip);
  return memcmp(ip->addr, IPV4_MAPPED_PREFIX, 12) == 0;
}

static struct in_addr ip_address_to_in_addr(const pcp_ip_address_t *ip) {
  assert(ip);
  assert(pcp_ip_address_is_ipv4(ip));
  struct in_addr result = {0};
  memcpy(&result.s_addr, ip->addr + 12, sizeof(result.s_addr));
  return result;
}

static struct in6_addr ip_address_to_in6_addr(const pcp_ip_address_t *ip) {
  assert(ip);
  assert(!pcp_ip_address_is_ipv4(ip));
  struct in6_addr result = {0};
  memcpy(result.s6_addr, ip->addr, sizeof(result.s6_addr));
  return result;
}

pcp_ip_address_t
pcp_ip_address_from_octets(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return (pcp_ip_address_t){
      .addr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, a, b, c, d}};
}

pcp_ip_address_t pcp_ip_address_from_in_addr(const struct in_addr *addr) {
  assert(addr);
  pcp_ip_address_t result = {0};
  memcpy(result.addr, IPV4_MAPPED_PREFIX, 12);
  memcpy(result.addr + 12, &addr->s_addr, sizeof(addr->s_addr));
  return result;
}

pcp_ip_address_t pcp_ip_address_from_in6_addr(const struct in6_addr *addr) {
  assert(addr);
  pcp_ip_address_t result = {0};
  static_assert(sizeof(result.addr) == sizeof(addr->s6_addr));
  memcpy(result.addr, addr->s6_addr, sizeof(result.addr));
  return result;
}

pcp_optional_ip_address_t pcp_ip_address_from_string(const char *ip) {
  struct in_addr ipv4 = {0};
  if (1 == inet_pton(AF_INET, ip, &ipv4)) {
    return (pcp_optional_ip_address_t){.has_value = true,
                                       .value =
                                           pcp_ip_address_from_in_addr(&ipv4)};
  }

  struct in6_addr ipv6 = {0};
  if (1 == inet_pton(AF_INET6, ip, &ipv6)) {
    return (pcp_optional_ip_address_t){.has_value = true,
                                       .value =
                                           pcp_ip_address_from_in6_addr(&ipv6)};
  }

  return (pcp_optional_ip_address_t){.has_value = false, .value = {}};
}

typedef typeof(errno) errno_t;

struct size_or_error {
  bool has_value;
  union {
    size_t value;
    errno_t error;
  };
};

static struct size_or_error
ip_address_to_string_v4(size_t size,
                        char dst[size],
                        const pcp_ip_address_t *src) {
  if (size < (INET_ADDRSTRLEN + 1)) {
    return (struct size_or_error){.has_value = false, .error = ENOBUFS};
  }

  memset(dst, 0, INET_ADDRSTRLEN + 1);

  struct in_addr addr = ip_address_to_in_addr(src);
  if (!inet_ntop(AF_INET, &addr, dst, (socklen_t)size)) {
    return (struct size_or_error){.has_value = false, .error = errno};
  }

  return (struct size_or_error){.has_value = true, .value = strlen(dst)};
}

static struct size_or_error
ip_address_to_string_v6(size_t size,
                        char dst[size],
                        const pcp_ip_address_t *src) {
  if (size < (INET6_ADDRSTRLEN + 1)) {
    return (struct size_or_error){.has_value = false, .error = ENOBUFS};
  }

  memset(dst, 0, INET6_ADDRSTRLEN + 1);

  struct in6_addr addr = ip_address_to_in6_addr(src);
  if (!inet_ntop(AF_INET6, &addr, dst, (socklen_t)size)) {
    return (struct size_or_error){.has_value = false, .error = errno};
  }

  return (struct size_or_error){.has_value = true, .value = strlen(dst)};
}

size_t
pcp_ip_address_to_string(size_t size, char dst[], const pcp_ip_address_t *src) {
  assert(src);

  struct size_or_error result;
  if (pcp_ip_address_is_ipv4(src)) {
    result = ip_address_to_string_v4(size, dst, src);
  } else {
    result = ip_address_to_string_v6(size, dst, src);
  }

  return result.has_value ? result.value : 0;
}

pcp_ip_network_t pcp_ip_network_from_address(pcp_ip_address_t address,
                                             uint8_t prefix) {
  return (pcp_ip_network_t){
      .address = address,
      .prefix =
          pcp_ip_address_is_ipv4(&address) ? (uint8_t)(96 + prefix) : prefix,
  };
}

pcp_socket_address_t
pcp_socket_address_from_sockaddr_in(const struct sockaddr_in *addr) {
  return (pcp_socket_address_t){
      .ip = pcp_ip_address_from_in_addr(&addr->sin_addr),
      .port = ntohs(addr->sin_port)};
}

pcp_socket_address_t
pcp_socket_address_from_sockaddr_in6(const struct sockaddr_in6 *addr) {
  return (pcp_socket_address_t){
      .ip = pcp_ip_address_from_in6_addr(&addr->sin6_addr),
      .port = ntohs(addr->sin6_port)};
}

// Parses the first `length` chars of `src` as an AF_INET or AF_INET6 address
static bool parse_ip_address(int family,
                             const char *src,
                             size_t length,
                             pcp_ip_address_t *out) {
  char node[INET6_ADDRSTRLEN] = {0};
  if (length == 0 || length >= sizeof(node)) {
    return false;
  }
  memcpy(node, src, length);

  if (family == AF_INET) {
    struct in_addr ipv4 = {0};
    if (1 != inet_pton(AF_INET, node, &ipv4)) {
      return false;
    }
    *out = pcp_ip_address_from_in_addr(&ipv4);
  } else {
    struct in6_addr ipv6 = {0};
    if (1 != inet_pton(AF_INET6, node, &ipv6)) {
      return false;
    }
    *out = pcp_ip_address_from_in6_addr(&ipv6);
  }
  return true;
}

// Parses `src` as a port number, nothing else allowed
static bool parse_port(const char *src, pcp_port_t *out) {
  auto const length = strlen(src);
  if (length == 0 || length > 5) {
    return false;
  }

  uint32_t value = 0;
  for (size_t i = 0; i < length; ++i) {
    if (src[i] < '0' || src[i] > '9') {
      return false;
    }
    value = value * 10 + (uint32_t)(src[i] - '0');
  }

  if (value > UINT16_MAX) {
    return false;
  }
  *out = (pcp_port_t)value;
  return true;
}

pcp_optional_socket_address_t
pcp_socket_address_from_string(const char *address) {
  constexpr pcp_optional_socket_address_t none = {.has_value = false,
                                                  .value = {}};
  if (!address) {
    return none;
  }

  // either [ipv6]:port or ipv4:port
  pcp_socket_address_t result = {};
  char const *separator = nullptr;
  if (address[0] == '[') {
    auto const close = strchr(address, ']');
    if (!close || close[1] != ':' ||
        !parse_ip_address(AF_INET6, address + 1, (size_t)(close - address - 1),
                          &result.ip)) {
      return none;
    }
    separator = close + 1;
  } else {
    separator = strchr(address, ':');
    if (!separator ||
        !parse_ip_address(AF_INET, address, (size_t)(separator - address),
                          &result.ip)) {
      return none;
    }
  }

  if (!parse_port(separator + 1, &result.port)) {
    return none;
  }

  return (pcp_optional_socket_address_t){.has_value = true, .value = result};
}

static struct size_or_error
socket_address_to_string_v4(size_t size,
                            char dst[size],
                            const pcp_socket_address_t *address) {
  if (size < (INET_ADDRSTRLEN + 7)) {
    return (struct size_or_error){.has_value = false, .error = ENOBUFS};
  }

  auto const ip_size = ip_address_to_string_v4(size, dst, &address->ip);
  if (!ip_size.has_value) {
    return ip_size;
  }

  size -= ip_size.value;

  int const result = snprintf(dst + ip_size.value, size, ":%hu", address->port);
  if (result < 0) {
    return (struct size_or_error){.has_value = false, .error = errno};
  }

  return (struct size_or_error){.has_value = true,
                                .value = ip_size.value + (size_t)result};
}

static struct size_or_error
socket_address_to_string_v6(size_t size,
                            char dst[size],
                            const pcp_socket_address_t *address) {
  constexpr size_t MIN_SIZE = INET6_ADDRSTRLEN + 3 + 6;
  if (size < MIN_SIZE) {
    return (struct size_or_error){.has_value = false, .error = ENOBUFS};
  }

  dst[0] = '[';

  auto const ip_size =
      ip_address_to_string_v6(INET6_ADDRSTRLEN + 1, dst + 1, &address->ip);
  if (!ip_size.has_value) {
    return ip_size;
  }

  size -= ip_size.value + 1;

  int const result =
      snprintf(dst + ip_size.value + 1, size, "]:%hu", address->port);
  if (result < 0) {
    return (struct size_or_error){.has_value = false, .error = errno};
  }

  return (struct size_or_error){.has_value = true,
                                .value = ip_size.value + 1 + (size_t)result};
}

size_t pcp_socket_address_to_string(size_t size,
                                    char dst[],
                                    const pcp_socket_address_t *address) {

  auto const result = pcp_ip_address_is_ipv4(&address->ip)
                          ? socket_address_to_string_v4(size, dst, address)
                          : socket_address_to_string_v6(size, dst, address);

  return result.has_value ? result.value : 0;
}

int pcp_socket_address_cmp(pcp_socket_address_t const *lhs,
                           pcp_socket_address_t const *rhs) {
  return memcmp(lhs, rhs, sizeof(pcp_socket_address_t));
}
