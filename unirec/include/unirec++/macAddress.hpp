#pragma once

#include <unirec/unirec.h>

namespace Nemea {

struct MacAddress {
	mac_addr_t mac;
};

static_assert(sizeof(MacAddress) == sizeof(mac_addr_t), "Invalid header definition");

} // namespace Nemea
