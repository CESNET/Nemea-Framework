/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Header file containing the definition of the IpAddress class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <ostream>
#include <string>

#include <unirec/ipaddr.h>

namespace Nemea {

/**
 * @brief Define a constant for an empty IP address
 * 0.0.0.0 for IPv4, :: for IPv6
 */
// C++ designated initializers is available with -std=c++2a
static const ip_addr_t EMPTY_IP_ADDRESS
    = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff } };

/**
 * @brief A struct representing an IP address with associated operations.
 */
struct IpAddress {
    ip_addr_t ip;

    /**
     * @brief Constructor to initialize IpAddress with an ip_addr_t value.
     * @param ip The ip_addr_t value.
     */
    IpAddress(ip_addr_t ip = EMPTY_IP_ADDRESS);

    /**
     * @brief Constructor to create IpAddress from a string representation of an IP address.
     * @param ipAddressAsString The string representation of the IP address.
     * @throw std::runtime_error if the IP address string is invalid.
     */
    IpAddress(const std::string& ipAddressAsString);

    /**
     * @brief Check if the stored IP address is an IPv4 address.
     * @return True if the IP address is IPv4, false otherwise.
     */
    bool isIpv4() const;

    /**
     * @brief Check if the stored IP address is an IPv6 address.
     * @return True if the IP address is IPv6, false otherwise.
     */
    bool isIpv6() const;

    /**
     * @brief Equality operator to compare two IpAddress objects.
     * @param other The IpAddress to compare with.
     * @return True if the IpAddress objects are equal, false otherwise.
     */
    bool operator==(const IpAddress& other) const;

    /**
     * @brief Bitwise AND operator to perform a bitwise AND operation on two IpAddress objects.
     * @param other The IpAddress to AND with.
     * @return A new IpAddress object containing the result of the bitwise AND operation.
     */
    IpAddress operator&(const IpAddress& other) const;

    /**
     * @brief Output stream operator to stream an IpAddress object to an output stream.
     * @param os The output stream.
     * @return The modified output stream.
     */
    std::ostream& operator<<(std::ostream& os);
};

static_assert(sizeof(IpAddress) == sizeof(ip_addr_t), "Invalid header definition");

} // namespace Nemea
