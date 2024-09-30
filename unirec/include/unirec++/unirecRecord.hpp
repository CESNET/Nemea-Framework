/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines the UnirecRecord class.
 *
 * This file contains the declaration of the UnirecRecord class, which provides a C++
 * interface for working with UniRec records and their fields.
 */

#pragma once

#include "unirecArray.hpp"
#include "unirecRecordView.hpp"
#include "unirecTypeTraits.hpp"
#include "unirecTypes.hpp"

#include <cstddef>
#include <exception>
#include <numeric>
#include <string>
#include <type_traits>
#include <unirec/unirec.h>
#include <vector>

namespace Nemea {

/**
 * @brief A class for working with UniRec records and their fields.
 *
 * The UnirecRecord class facilitates the manipulation of UniRec records and their individual
 * fields. It provides functions to access, set, and transform data within the UniRec record.
 */
class UnirecRecord {
public:
	/**
	 * @brief Default constructor.
	 *
	 * Constructs an uninitialized UnirecRecord object.
	 */
	UnirecRecord()
		: m_recordSize(0)
		, m_recordData(nullptr)
		, m_unirecTemplate(nullptr)
	{
	}

	/**
	 * @brief Constructor with template and maximum variable fields size.
	 *
	 * Constructs a UnirecRecord object with a specified UniRec template and maximum size for
	 * variable fields.
	 *
	 * @param unirecTemplate The UniRec template to associate with the record.
	 * @param maxVariableFieldsSize The maximum size for variable fields in the record.
	 */
	UnirecRecord(ur_template_t* unirecTemplate, size_t maxVariableFieldsSize = UR_MAX_SIZE)
		: m_recordSize(maxVariableFieldsSize)
		, m_unirecTemplate(unirecTemplate)
	{
		m_recordData = ur_create_record(unirecTemplate, maxVariableFieldsSize);
		if (!m_recordData) {
			throw std::runtime_error("Allocation of UniRec record failed");
		}
	}

	void copyFieldsFrom(const UnirecRecord& otherRecord)
	{
		if (!m_unirecTemplate || !m_recordData) {
			throw std::runtime_error(
				"UnirecRecord::copyFieldsFrom() has failed. Record has no template or allocated "
				"memory!");
		}

		// TODO: check sufficient record size

		if (otherRecord.m_recordData != nullptr) {
			ur_copy_fields(
				m_unirecTemplate,
				m_recordData,
				otherRecord.m_unirecTemplate,
				otherRecord.m_recordData);
		}
	}

	void copyFieldsFrom(const UnirecRecordView& otherRecordView)
	{
		if (!m_unirecTemplate || !m_recordData) {
			throw std::runtime_error(
				"UnirecRecord::copyFieldsFrom() has failed. Record has no template or allocated "
				"memory!");
		}

		// TODO: check sufficient record size

		if (otherRecordView.m_recordData != nullptr) {
			ur_copy_fields(
				m_unirecTemplate,
				m_recordData,
				otherRecordView.m_unirecTemplate,
				otherRecordView.m_recordData);
		}
	}

	/**
	 * @brief Destructor.
	 *
	 * Frees the memory associated with the UniRec record.
	 */

	~UnirecRecord() { ur_free_record(m_recordData); }

	/**
	 * @brief Returns a pointer to the data of the UniRec record.
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
	 * @brief Gets the value of a UniRec field and converts it to the specified type.
	 *
	 * This function retrieves the value of a UniRec field, converts it to the specified type,
	 * and returns the result.
	 *
	 * @tparam T The type to which the field value should be converted.
	 * @param fieldID The ID of the UniRec field.
	 * @return The value of the field, converted to the specified type.
	 * @throws std::runtime_error If there is a data type format mismatch or other error.
	 *
	 * @code
	 * // memory allocation in std::string
	 * std::string stringValue = urRecord.getFieldAsType<std::string>(fieldID);
	 *
	 * // no memory allocation
	 * std::string_view stringValue = urRecord.getFieldAsType<std::string_view>(fieldID);
	 *
	 * // Get pointer to unirec field value
	 * int* intValuePtr = urRecord.getFieldAsType<int*>(fieldID);
	 *
	 * // Get reference to unirec field value
	 * int& intValueRef = urRecord.getFieldAsType<int&>(fieldID);
	 *
	 * // Get value of unirec field
	 * int intValue = urRecord.getFieldAsType<int>(fieldID);
	 * @endcode
	 */
	template<typename T>
	T getFieldAsType(ur_field_id_t fieldID) const
	{
		using BaseType = typename std::remove_cv_t<
			typename std::remove_pointer_t<typename std::remove_reference_t<T>>>;

		checkDataTypeCompatibility<T>(fieldID);

		if constexpr (is_string_v<T>) {
			return getFieldAsStringType<T>(fieldID);
		} else if constexpr (std::is_pointer_v<T>) {
			return getFieldAsPointer<T>(fieldID);
		} else if constexpr (std::is_reference_v<T>) {
			return getFieldAsReference<BaseType>(fieldID);
		} else {
			return getFieldAsValue<T>(fieldID);
		}
	}

	/**
	 * @brief Gets a UniRecArray representing a UniRec field.
	 *
	 * This function returns a UnirecArray object representing a UniRec field that holds an array of
	 * values.
	 *
	 * @tparam T The type of elements in the array.
	 * @param fieldID The ID of the UniRec field.
	 * @return A UnirecArray representing the UniRec field.
	 *
	 * @code
	 * // Get array of int values as UnirecArray
	 * UnirecArray<int> array = urRecord.getFieldAsUnirecArray<int>(fieldID);
	 * @endcode
	 */
	template<typename T>
	UnirecArray<T> getFieldAsUnirecArray(ur_field_id_t fieldID)
	{
		return UnirecArray<T>(
			static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)),
			ur_array_get_elem_cnt(m_unirecTemplate, m_recordData, fieldID),
			ur_get_type(fieldID));
	}

	/**
	 * @brief Reserves memory for a UniRecArray within a UniRec field.
	 *
	 * This function allocates memory for a UniRecArray within a UniRec field and returns a
	 * UnirecArray object that can be used to populate the field.
	 *
	 * @tparam T The type of elements in the array.
	 * @param elementsCount The number of elements to allocate space for.
	 * @param fieldID The ID of the UniRec field.
	 * @return A UnirecArray object for populating the UniRec field.
	 * @throws std::runtime_error If memory allocation fails or other errors occur.
	 *
	 * @code
	 * // Create a UnirecRecord with a template
	 * UnirecRecord urRecord(urTemplate);
	 *
	 * // Reserve memory for a UnirecArray of integers with 10 elements in field with ID 'fieldID'
	 * UnirecArray<int> intArray = urRecord.reserveUnirecArray<int>(10, fieldID);
	 *
	 * // Populate the array with values (directly modifying the Unirec record data)
	 * for (int i = 0; i < 10; ++i) {
	 *     intArray.at(i) = i * 2;
	 * }
	 * @endcode
	 */
	template<typename T>
	UnirecArray<T> reserveUnirecArray(size_t elementsCount, ur_field_id_t fieldID)
	{
		int retCode = ur_array_allocate(m_unirecTemplate, m_recordData, fieldID, elementsCount);
		if (retCode != UR_OK) {
			throw std::runtime_error("Unable to allocate memory for UnirecArray");
		}

		return UnirecArray<T>(
			static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)),
			elementsCount,
			fieldID);
	}

	/**
	 * @brief Sets the value of a UniRec field using data of a specified type.
	 *
	 * This function sets the value of a UniRec field using the provided data of a specified type.
	 *
	 * @tparam T The type of the field data.
	 * @param fieldData The data to set the field to.
	 * @param fieldID The ID of the UniRec field.
	 *
	 * @code
	 * // set string value to unirec field with ID 'stringFieldID'
	 * urRecord.setFieldFromType("example string", stringFieldID);
	 * @endcode
	 */
	template<typename T>
	void setFieldFromType(const T& fieldData, ur_field_id_t fieldID)
	{
		if constexpr (is_string_v<T>) {
			ur_set_string(m_unirecTemplate, m_recordData, fieldID, fieldData.data());
		} else {
			*static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)) = fieldData;
		}
	}

	/**
	 * @brief Sets the value of a UniRec field using a UnirecArray.
	 *
	 * This function sets the value of a UniRec field in Unirec record using a UnirecArray.
	 *
	 * @tparam T The type of elements in the UnirecArray.
	 * @param unirecArray The UnirecArray to set the field to.
	 * @param fieldID The ID of the UniRec field.
	 */
	template<typename T>
	void setFieldFromUnirecArray(const UnirecArray<T>& unirecArray, ur_field_id_t fieldID)
	{
		checkDataTypeCompatibility<T>(fieldID);

		if (ur_is_array(fieldID)) {
			ur_array_allocate(m_unirecTemplate, m_recordData, fieldID, unirecArray.size());
			std::copy(
				unirecArray.begin(),
				unirecArray.end(),
				static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)));
		} else {
			setFieldFromType<T>(unirecArray.at(0), fieldID);
		}
	}

	UnirecRecord(const UnirecRecord& other) { copyFrom(other); }

	UnirecRecord& operator=(const UnirecRecord& other)
	{
		if (&other == this) {
			return *this;
		}

		if (m_recordData != nullptr) {
			ur_free_record(m_recordData);
			m_recordData = nullptr;
		}

		copyFrom(other);
		return *this;
	}

	/**
	 * @brief Sets the value of a UniRec array field using a vector of values.
	 *
	 * @tparam T The type of elements in the vector (and unirec record).
	 * @param sourceVector The vector of values to set the field to.
	 * @param fieldID The ID of the UniRec field.
	 *
	 * @code
	 * std::vector<int> intVector = {1, 2, 3, 4, 5};
	 * urRecord.setFieldFromVector<int>(intVector, fieldID);
	 * @endcode
	 */
	template<typename T>
	void setFieldFromVector(const std::vector<T>& sourceVector, ur_field_id_t fieldID)
	{
		checkDataTypeCompatibility<T>(fieldID);

		if (!ur_is_array(fieldID)) {
			throw std::runtime_error("Cannot set vector to non-array unirec field");
		}

		ur_array_allocate(m_unirecTemplate, m_recordData, fieldID, sourceVector.size());
		std::copy(
			sourceVector.begin(),
			sourceVector.end(),
			static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)));
	}

private:
	template<typename T>
	void checkDataTypeCompatibility(ur_field_id_t fieldID) const
	{
		using BaseType = typename std::remove_cv_t<
			typename std::remove_pointer_t<typename std::remove_reference_t<T>>>;
		using RequiredType = typename std::conditional_t<is_string_v<BaseType>, T, BaseType>;

		ur_field_type_t expectedType;
		if (ur_is_array(fieldID)) {
			expectedType = getExpectedUnirecType<RequiredType*>();
		} else {
			expectedType = getExpectedUnirecType<RequiredType>();
		}

		if (expectedType != ur_get_type(fieldID)) {
			throw std::runtime_error(
				"UnirecRecord data type format mismatch: " + std::string(typeid(T).name()));
		}
	}

	template<typename T>
	T getFieldAsStringType(ur_field_id_t fieldID) const
	{
		return T(
			static_cast<const char*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID)),
			ur_get_var_len(m_unirecTemplate, m_recordData, fieldID));
	}

	template<typename T>
	T getFieldAsPointer(ur_field_id_t fieldID) const
	{
		return static_cast<T>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID));
	}

	template<typename T>
	T getFieldAsReference(ur_field_id_t fieldID) const
	{
		return *reinterpret_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID));
	}

	template<typename T>
	T getFieldAsValue(ur_field_id_t fieldID) const
	{
		return *static_cast<T*>(ur_get_ptr_by_id(m_unirecTemplate, m_recordData, fieldID));
	}

	void copyFrom(const UnirecRecord& other)
	{
		m_unirecTemplate = other.m_unirecTemplate;
		m_recordSize = other.m_recordSize;
		if (other.m_recordData != nullptr) {
			m_recordData = ur_create_record(m_unirecTemplate, m_recordSize);
			ur_copy_fields(m_unirecTemplate, m_recordData, m_unirecTemplate, other.m_recordData);
			if (!m_recordData) {
				throw std::runtime_error("Allocation of UniRec record failed");
			}
		}
	}

	size_t m_recordSize;

	void* m_recordData;
	ur_template_t* m_unirecTemplate;
};

} // namespace Nemea
