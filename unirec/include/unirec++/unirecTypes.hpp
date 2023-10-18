/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief This file contains functions for determining the expected UniRec type for various C++
 * types.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "ipAddress.hpp"
#include "macAddress.hpp"
#include "urTime.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <unirec/unirec.h>

namespace NemeaPlusPlus {

/**
 * @brief Determines the expected UniRec field type for a given C++ type T.
 *
 * @tparam T The C++ type to determine the expected UniRec field type for.
 * @return The expected UniRec field type for the given C++ type T.
 */
template<typename T>
constexpr ur_field_type_t getExpectedUnirecType();

template<>
constexpr ur_field_type_t getExpectedUnirecType<std::byte*>()
{
	return UR_TYPE_BYTES;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<char>()
{
	return UR_TYPE_CHAR;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<std::string>()
{
	return UR_TYPE_STRING;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<std::string_view>()
{
	return UR_TYPE_STRING;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<char*>()
{
	return UR_TYPE_STRING;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint8_t>()
{
	return UR_TYPE_UINT8;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint8_t*>()
{
	return UR_TYPE_A_UINT8;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int8_t>()
{
	return UR_TYPE_INT8;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int8_t*>()
{
	return UR_TYPE_A_INT8;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint16_t>()
{
	return UR_TYPE_UINT16;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint16_t*>()
{
	return UR_TYPE_A_UINT16;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int16_t>()
{
	return UR_TYPE_INT16;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int16_t*>()
{
	return UR_TYPE_A_INT16;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint32_t>()
{
	return UR_TYPE_UINT32;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint32_t*>()
{
	return UR_TYPE_A_UINT32;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int32_t>()
{
	return UR_TYPE_INT32;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int32_t*>()
{
	return UR_TYPE_A_INT32;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint64_t>()
{
	return UR_TYPE_UINT64;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<uint64_t*>()
{
	return UR_TYPE_A_UINT64;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int64_t>()
{
	return UR_TYPE_INT64;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<int64_t*>()
{
	return UR_TYPE_A_INT64;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<float>()
{
	return UR_TYPE_FLOAT;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<float*>()
{
	return UR_TYPE_A_FLOAT;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<double>()
{
	return UR_TYPE_DOUBLE;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<double*>()
{
	return UR_TYPE_A_DOUBLE;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<MacAddress>()
{
	return UR_TYPE_MAC;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<MacAddress*>()
{
	return UR_TYPE_A_MAC;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<IpAddress>()
{
	return UR_TYPE_IP;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<IpAddress*>()
{
	return UR_TYPE_A_IP;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<UrTime>()
{
	return UR_TYPE_TIME;
}

template<>
constexpr ur_field_type_t getExpectedUnirecType<UrTime*>()
{
	return UR_TYPE_A_TIME;
}

} // namespace NemeaPlusPlus
