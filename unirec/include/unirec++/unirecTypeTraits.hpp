/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Provides a set of type traits and aliases for working with unirec++.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <string_view>
#include <type_traits>

namespace Nemea {

// NOLINTBEGIN

/**
 * @brief A type trait that checks if a given type is a string type.
 *
 * A type is considered a string type if it is either `std::string` or `std::string_view`.
 * @tparam T The type to check.
 */
template<typename T>
struct is_string : std::false_type {};

template<>
struct is_string<std::string> : std::true_type {};

template<>
struct is_string<std::string_view> : std::true_type {};

template<typename T>
constexpr bool is_string_v = is_string<T>::value;

/**
 * @brief A type trait that adds `const` to a given type if it is a pointer or a reference.
 *
 * If the input type `T` is not a pointer or a reference, `add_const<T>` is equivalent to `T`.
 * Otherwise, `add_const<T>` adds `const` to the pointed-to or referred-to type.
 * @tparam T The input type to add `const` to.
 */
template<typename T>
struct add_const {
	using type = T;
};

template<typename T>
struct add_const<T*> {
	using type = const T*;
};

template<typename T>
struct add_const<T&> {
	using type = const T&;
};

template<typename T>
using add_const_t = typename add_const<T>::type;

// NOLINTEND

} // namespace Nemea
