#pragma once

#include <unirec/unirec.h>

namespace NemeaPlusPlus {

struct IpAddress {
	ip_addr_t ip;
};

static_assert(sizeof(IpAddress) == sizeof(ip_addr_t), "Invalid header definition");

} // namespace NemeaPlusPlus
