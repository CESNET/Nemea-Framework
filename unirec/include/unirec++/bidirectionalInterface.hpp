/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines a bidirectional interface for sending and receiving unirec records using the TRAP
 * interface provided by the UniRec library. This class provides a simple and easy-to-use
 * bidirectional interface for sending and receiving unirec records with the SAME format. It wraps
 * the TRAP interface provided by the UniRec library and includes methods for sending and receiving
 * records, changing the record template, setting timeouts, and more.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "interfaceStats.hpp"
#include "unirecException.hpp"
#include "unirecRecord.hpp"
#include "unirecRecordView.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <unirec/unirec.h>

namespace Nemea {

/**
 * @brief A class that provides a bidirectional interface for sending and receiving unirec records.
 *
 * This class wraps the TRAP interface provided by the UniRec library to provide a simple
 * and easy-to-use bidirectional interface for sending and receiving unirec records with the SAME
 * FORMAT.
 */
class UnirecBidirectionalInterface {
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
	 * @brief Sends a UniRec record through the Trap interface.
	 *
	 * @param unirecRecord the Unirec record to send
	 * @throws std::runtime_error if an error occurs while sending the record
	 * @return true if the record was sent successfully, false if a timeout occurred
	 */
	bool send(UnirecRecord& unirecRecord) const;

	/**
	 * @brief Sends a UniRec record view through the Trap interface.
	 *
	 * @param unirecRecordView The UniRec record view to send.
	 * @throws std::runtime_error if an error occurs while sending the record
	 * @return true if the record was sent successfully, false if a timeout occurred
	 */
	bool send(UnirecRecordView& unirecRecordView) const;

	/**
	 * @brief Flushes any pending UniRec records in the Trap interface.
	 */
	void sendFlush() const;

	/**
	 * @brief Changes the Unirec template used by the bidirectional interface.
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
	void setReceiveTimeout(int timeout);

	/**
	 * @brief Sets the send timeout for the Trap interface.
	 *
	 * @param timeout The timeout value in microseconds.
	 *  - `TRAP_WAIT`: Blocking mode, wait for client's connection, for message transport
	 * to/from internal system buffer.
	 *  - `TRAP_HALFWAIT`: Blocking only if any client is connected.
	 *  - `TRAP_NO_WAIT`: Non-Blocking mode, do not wait ever.
	 *  - `timeout`: Wait max for specific time.
	 */
	void setSendTimeout(int timeout);

	/**
	 * @brief Sets the autoflush timeout for the output Trap interface.
	 *
	 * @param timeout The timeout value in microseconds.
	 * - `TRAP_NO_AUTO_FLUSH`: Do not autoflush trap buffers
	 */
	void setSendAutoflushTimeout(int timeout);

	/**
	 * @brief Disables sending an end-of-file marker on exit.
	 */
	void doNotsendEoFOnExit();

	/**
	 * @brief Gets the Unirec template used by the bidirectional interface.
	 *
	 * This method returns a pointer to the Unirec template used by the bidirectional interface.
	 *
	 * @return A pointer to the Unirec template used by the bidirectional interface.
	 */
	ur_template_t* getTemplate() const noexcept { return m_template; }

	/**
	 * @brief Gets the statistics for the input interface.
	 *
	 * This method returns the actual statistics for the input interface.
	 *
	 * @return The statistics for the input interface.
	 */
	InputInteraceStats getInputInterfaceStats() const;

	/**
	 * @brief Destructor for the UnirecBidirectionalInterface class.
	 *
	 * Sends an end-of-file marker if m_sendEoFonExit is true, then frees the memory allocated
	 * for the UniRec template.
	 */
	~UnirecBidirectionalInterface();

	/**
	 * @brief Gets a reference to the pre-allocated UniRec record for efficient use.
	 *
	 * This function provides access to the UniRec record instance that has already been
	 * pre-allocated within the UnirecOutputInterface. It allows direct modification of the
	 * record's fields before sending it through the TRAP interface. Using the pre-allocated record
	 * can be faster compared to creating a new record, as there is no memory allocation involved.
	 * However, please note that using the same record in a multithreaded context may not be
	 * thread-safe.
	 *
	 * @note The record accessed through this function is specific to this instance of the
	 * UnirecOutputInterface and should not be shared across multiple instances or threads.
	 *
	 * @return A reference to the pre-allocated UniRec record.
	 */
	UnirecRecord& getUnirecRecord() noexcept { return m_unirecRecord; }

	/**
	 * @brief Creates a new UniRec record with the specified maximum variable fields size.
	 *
	 * This function generates a fresh UniRec record instance, ready to be populated with data
	 * before sending it through the TRAP interface. The maximum size of variable fields can be
	 * specified to suit your data insertion needs. Unlike using the pre-allocated record with the
	 * `getUnirecRecord` function, this function involves memory allocation and may have a slightly
	 * higher overhead.
	 *
	 * @param maxVariableFieldsSize The maximum size for variable fields in the new UniRec record.
	 * @return A newly created UnirecRecord instance.
	 */
	UnirecRecord createUnirecRecord(size_t maxVariableFieldsSize = UR_MAX_SIZE);

private:
	UnirecBidirectionalInterface(uint8_t inputInterfaceID, uint8_t outputInterfaceID);
	void handleReceiveErrorCodes(int errorCode) const;
	bool handleSendErrorCodes(int errorCode) const;
	void changeInternalTemplate(const std::string& templateSpecification);
	bool isEoFReceived() const noexcept;
	void sendEoF() const;

	ur_template_t* m_template;
	uint8_t m_inputInterfaceID;
	uint8_t m_outputInterfaceID;
	uint64_t m_sequenceNumber;
	const void* m_prioritizedDataPointer;
	bool m_sendEoFonExit;
	UnirecRecord m_unirecRecord;

	friend class Unirec;
};

} // namespace Nemea
