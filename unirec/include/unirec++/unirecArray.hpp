/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Header file containing the definition of the UnirecArray class and its Iterator subclass.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "unirecTypes.hpp"

#include <cstddef>
#include <exception>
#include <iterator>
#include <unirec/unirec.h>

namespace Nemea {

/**
 * @brief A wrapper class for a contiguous array of values with the same unirec fieldID.
 *
 * The `UnirecArray` class provides a convenient way to work with a contiguous array of values
 * associated with the same unirec fieldID. It supports iterating over the array and provides
 * methods for element access and bounds checking.
 *
 * @tparam T The type of the values in the array.
 */
template<typename T>
class UnirecArray {
public:
	/**
	 * @brief An iterator for the UnirecArray class.
	 */
	class Iterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = value_type*;
		using reference = value_type&;

		Iterator(pointer ptr)
			: m_ptr(ptr)
		{
		}

		reference operator*() const { return *m_ptr; }
		pointer operator->() { return m_ptr; }
		Iterator& operator++()
		{
			m_ptr++;
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		T* data() { return m_ptr; }

		bool operator==(const Iterator& other) const { return this->m_ptr == other.m_ptr; };
		bool operator!=(const Iterator& other) const { return this->m_ptr != other.m_ptr; };

	private:
		pointer m_ptr;
	};

	/**
	 * @brief Constructs a UnirecArray object.
	 *
	 * @param dataPointer A pointer to the first element of the value array.
	 * @param size The number of elements in the value array.
	 * @param fieldID The unirec fieldID associated with the array.
	 */
	UnirecArray(T* dataPointer, size_t size, ur_field_id_t fieldID)
		: m_data(dataPointer)
		, m_size(size)
	{
		checkDataType(ur_get_type(fieldID));
	}

	/**
	 * @brief Returns the number of elements in the UniRec field array.
	 */
	constexpr size_t size() const noexcept { return m_size; }

	/**
	 * @brief Returns an iterator to the first element of the UniRec field array.
	 */
	constexpr Iterator begin() const noexcept { return Iterator(m_data); }

	/**
	 * @brief Returns an iterator to the element following the last element of the UniRec field
	 * array.
	 */
	constexpr Iterator end() const noexcept { return Iterator(m_data + m_size); }

	/**
	 * @brief Returns a reference to the element at the specified position in the UniRec field
	 * array.
	 * @param pos The position of the element to return.
	 * @return T& The reference to the element at the specified position.
	 */
	constexpr T& operator[](size_t pos) { return m_data[pos]; }

	/**
	 * @brief Returns a reference to the element at the specified position in the UniRec field
	 * array, with bounds checking.
	 * @tparam pos The position of the element to return.
	 * @throw std::out_of_range If pos is out of range of valid element positions in the UniRec
	 * field array.
	 * @return A reference to the element at the specified position in the UniRec field array.
	 */
	constexpr T& at(size_t pos) const
	{
		if (pos >= m_size)
			throw std::out_of_range(
				"UnirecArray::at: pos (which is %zu) "
				">= m_size (which is %zu)"),
				pos, m_size;
		return m_data[pos];
	}

private:
	void checkDataType(ur_field_type_t fieldDataType) const
	{
		if (getExpectedUnirecType<T*>() != fieldDataType) {
			throw std::runtime_error("Unirec array data type format mismatch");
		}
	}

	size_t m_size;
	T* m_data;
};

} // namespace Nemea
