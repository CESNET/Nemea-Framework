/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of the IpAddress class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unirec++/ipAddress.hpp"

#include <stdexcept>

namespace NemeaPlusPlus {

IpAddress::IpAddress(ip_addr_t ip)
    : ip(ip)
{
}

IpAddress::IpAddress(const std::string& ipAddressAsString)
{
    int ret = ip_from_str(ipAddressAsString.c_str(), &ip);
    if (ret != 1) {
        throw std::runtime_error("Invalid IP address");
    }
}

bool IpAddress::isIpv4() const { return ip_is4(&ip); }

bool IpAddress::isIpv6() const { return ip_is6(&ip); }

bool IpAddress::operator==(const IpAddress& other) const
{
    return ip.ui64[0] == other.ip.ui64[0] && ip.ui64[1] == other.ip.ui64[1];
}

IpAddress IpAddress::operator&(const IpAddress& other) const
{
    ip_addr_t result;
    result.ui64[0] = ip.ui64[0] & other.ip.ui64[0];
    result.ui64[1] = ip.ui64[1] & other.ip.ui64[1];
    return result;
}

std::ostream& IpAddress::operator<<(std::ostream& os)
{
    char buffer[INET6_ADDRSTRLEN];
    ip_to_str(&ip, buffer);
    os << buffer;
    return os;
}

} // namespace NemeaPlusPlus
