/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Provides an interface for receiving UniRec records using the TRAP protocol.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "unirecException.hpp"
#include "unirecRecordView.hpp"

#include <optional>
#include <string>
#include <unirec/unirec.h>

namespace Nemea {

/**
 * @brief A class that provides an interface for receiving data in the Unirec format using the TRAP
 * library.
 */
class UnirecInputInterface {
public:
	/**
	 * @brief Receives data from the interface and returns an optional UnirecRecordView object.
	 *
	 * If data is received successfully, an UnirecRecordView object is returned that provides a view
	 * into the received data. If no data is available or a timeout occurs, std::nullopt is
	 * returned.
	 *
	 * @return An optional UnirecRecordView object.
	 * @throws EoFException if the end of the input stream is reached.
	 * @throws FormatChangeException if the record format changes.
	 */
	std::optional<UnirecRecordView> receive();

	/**
	 * @brief Changes the Unirec template used by the input interface.
	 *
	 * This method should be called every time when the FormatChangeException is thrown.
	 *
	 * This method changes the UniRec record template used for decoding records received on the
	 * interface.
	 *
	 * @throws std::runtime_error if the data format was not loaded or the template could not be
	 * edited.
	 */
	void changeTemplate();

	/**
	 * @brief Sets the required Unirec format specification.
	 *
	 * This method sets the required Unirec format specification for the input interface.
	 * Format: "uint64 BYTES, string SNI" (unirecDataType NAME)
	 *
	 * @param templateSpecification The required Unirec format specification.
	 * @throws std::runtime_error if the required format could not be set.
	 */
	void setRequieredFormat(const std::string& templateSpecification);

	/**
	 * @brief Sets the receive timeout for the interface.
	 * This method sets the timeout for receiving UniRec records on the interface. If no record is
	 * received within the specified timeout, the receive method returns an empty optional.
	 *
	 * @param timeout The timeout value in microseconds.
	 *  - `TRAP_WAIT`: Blocking mode, wait for client's connection, for message transport to/from
	 * internal system buffer.
	 *  - `TRAP_NO_WAIT`: Non-Blocking mode, do not wait ever.
	 *  - `timeout`: Wait max for specific time.
	 */
	void setTimeout(int timeout);

	/**
	 * @brief Destructor for UnirecInputInterface class.
	 * This method frees the memory used by the Unirec template.
	 */
	~UnirecInputInterface();

	/**
	 * @brief Gets the Unirec template used by the input interface.
	 *
	 * This method returns a pointer to the Unirec template used by the input interface.
	 *
	 * @return A pointer to the Unirec template used by the input interface.
	 */
	ur_template_t* getTemplate() const noexcept { return m_template; }

private:
	UnirecInputInterface(uint8_t interfaceID);
	void handleReceiveErrorCodes(int errorCode) const;
	void changeInternalTemplate(const std::string& templateSpecification);

	ur_template_t* m_template = nullptr;
	uint8_t m_interfaceID;
	const void* m_prioritizedDataPointer;

	friend class Unirec;
};

} // namespace Nemea
