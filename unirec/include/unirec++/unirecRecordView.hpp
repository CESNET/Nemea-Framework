/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Provides a view into a UniRec record.
 *
 * This file contains the declaration of the `UnirecRecordView` class, which offers a lightweight
 * way to access and inspect the contents of a UniRec record. It provides methods for retrieving
 * field values as various types, including arrays, strings, and other fundamental types.
 * The class does not own the record data and is intended for read-only operations.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "unirecArray.hpp"
#include "unirecTypeTraits.hpp"
#include "unirecTypes.hpp"

#include <cstddef>
#include <exception>
#include <numeric>
#include <string>
#include <type_traits>
#include <unirec/unirec.h>

namespace Nemea {

class UnirecRecord;

/**
 * @class UnirecRecordView
 * @brief Provides a view into a UniRec record.
 *
 * The `UnirecRecordView` class offers a lightweight way to access and inspect the contents of a
 * UniRec record. It provides methods for retrieving field values as various types, including
 * arrays, strings, and other fundamental types. The class does not own the record data and is
 * intended for read-only operations.
 */
class UnirecRecordView {
public:
	/**
	 * @brief Constructs a UnirecRecordView object.
	 * @param unirecRecordData Pointer to the UniRec record data.
	 * @param unirecTemplate Pointer to the UniRec template for the record.
	 * @param sequenceNumber The sequence number of the record.
	 */
	UnirecRecordView(const void* unirecRecordData, ur_template_t* unirecTemplate, uint64_t sequenceNumber = 0)
		: m_recordData(unirecRecordData)
		, m_unirecTemplate(unirecTemplate)
		, m_sequenceNumber(sequenceNumber)
	{
	}

	/**
	 * @brief Returns a const pointer to the data of the UniRec record.
	 *
	 * @return A pointer to the data of the UniRec record.
	 */
	const void* data() const noexcept { return m_recordData; }

	/**
	 * @brief Returns the size of the UniRec record.
	 *
	 * @return The size of the UniRec record.
	 */
	size_t size() const noexcept { return ur_rec_size(m_unirecTemplate, m_recordData); }

	/**
	 * @brief Gets the sequence number of the record.
	 *
	 * The sequence number represents the order of UniRec records when they are processed.
	 * Sequential numbers are currently supported by libtrap only for the Unix socket interface.
	 * If there is a gap between sequential numbers, it indicates data loss, which can occur
	 * when the internal buffers overflow due to the data not being retrieved in a timely manner.
	 * A sequential number of 0 signifies that sequential numbering is not supported.
	 * Sequential numbers start from 1 and increase for each new record.
	 *
	 * @return The sequence number of the record.
	 *
	 * @note 0 signifies that sequential numbering is not supported by the interface.
	 */
	uint64_t getSequenceNumber() const noexcept
	{
		return m_sequenceNumber;
	}

	/**
	 * @brief Gets the value of a field as a type T.
	 *
	 * This function retrieves the value of a field and converts it to the specified type `T`.
	 * It performs type checking to ensure that the field type matches the expected type.
	 *
	 * @tparam T The type of the field to get.
	 * @param fieldID The ID of the field to get.
	 * @return The value of the field as a type `T` object.
	 * @throws std::runtime_error If the field type does not match the expected type.
	 *
	 * @code
	 * // memory allocation in std::string
	 * std::string stringValue = urRecordView.getFieldAsType<std::string>(fieldID);
	 *
	 * // no memory allocation
	 * std::string_view stringValue = urRecordView.getFieldAsType<std::string_view>(fieldID);
	 *
	 * // Get pointer to unirec field value
	 * int* intValuePtr = urRecordView.getFieldAsType<int*>(fieldID);
	 *
	 * // Get reference to unirec field value
	 * int& intValueRef = urRecordView.getFieldAsType<int&>(fieldID);
	 *
	 * // Get value of unirec field
	 * int intValue = urRecordView.getFieldAsType<int>(fieldID);
	 * @endcode
	 */
	template <typename T>
	add_const_t<T> getFieldAsType(ur_field_id_t fieldID) const
	{
		using BaseType = typename std::remove_cv_t<
			typename std::remove_pointer_t<typename std::remove_reference_t<T>>>;
		using RequiredType = typename std::conditional_t<is_string_v<BaseType>, T, BaseType>;

		if (getExpectedUnirecType<RequiredType>() != ur_get_type(fieldID)) {
			throw std::runtime_error(
				"UnirecRecord data type format mismatch: " + std::string(typeid(T).name()));
		}

		if constexpr (is_string_v<T>) {
			return {
				static_cast<const char*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)),
				ur_get_var_len(m_unirecTemplate, m_recordData, fieldID)
			};
		} else if constexpr (std::is_pointer_v<T>) {
			return static_cast<T>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID));
		} else if constexpr (std::is_reference_v<T>) {
			return *reinterpret_cast<BaseType*>(
				ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID));
		} else {
			return *static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID));
		}
	}

	/**
	 * @brief Gets the value of a field as a UnirecArray.
	 *
	 * This function retrieves the value of a field as a `UnirecArray` object, which provides a view
	 * into a contiguous array of values associated with the specified unirec field ID.
	 *
	 * @tparam T The element type of the array.
	 * @param fieldID The ID of the field to get.
	 * @return A `UnirecArray<T>` object representing the array.
	 *
	 * @code
	 * // Get array of int values as UnirecArray
	 * UnirecArray<int> array = urRecordView.getFieldAsUnirecArray<int>(fieldID);
	 * @endcode
	 */
	template <typename T>
	add_const_t<UnirecArray<T>> getFieldAsUnirecArray(ur_field_id_t fieldID) const
	{
		return UnirecArray<T>(
			static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)),
			ur_array_get_elem_cnt(m_unirecTemplate, m_recordData, fieldID),
			fieldID);
	}

private:
	template <typename T>
	add_const_t<T> getFieldAsStringType(ur_field_id_t fieldID) const
	{
		return T(
			static_cast<const char*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)),
			ur_get_var_len(m_unirecTemplate, m_recordData, fieldID));
	}

	const void* m_recordData;
	ur_template_t* m_unirecTemplate;
	uint64_t m_sequenceNumber;

	friend class UnirecRecord;
};

} // namespace Nemea
