/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines the UnirecOutputInterface class.
 *
 * This file contains the declaration of the UnirecOutputInterface class, which provides a C++
 * interface for sending Unirec records to a TRAP interface. The class encapsulates the low-level
 * TRAP functions and provides a higher-level interface for sending records and configuring the
 * interface.
 */

#pragma once

#include "unirecRecord.hpp"
#include "unirecRecordView.hpp"

#include <string>
#include <unirec/unirec.h>

namespace Nemea {
/**
 * @brief A class for sending UniRec records through a Trap interface.
 */
class UnirecOutputInterface {
public:
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
	 * @brief Sets the send timeout for the Trap interface.
	 *
	 * @param timeout The timeout value in microseconds.
	 *  - `TRAP_WAIT`: Blocking mode, wait for client's connection, for message transport
	 * to/from internal system buffer.
	 *  - `TRAP_HALFWAIT`: Blocking only if any client is connected.
	 *  - `TRAP_NO_WAIT`: Non-Blocking mode, do not wait ever.
	 *  - `timeout`: Wait max for specific time.
	 */
	void setTimeout(int timeout);

	/**
	 * @brief Sets the autoflush timeout for the Trap interface.
	 *
	 * @param timeout The timeout value in microseconds.
	 * - `TRAP_NO_AUTO_FLUSH`: Do not autoflush trap buffers
	 */
	void setAutoflushTimeout(int timeout);

	/**
	 * @brief Changes the UniRec template for the Trap interface.
	 *
	 * Format: "uint64 BYTES, string SNI" (unirecDataType NAME)
	 *
	 * @param templateFields A string containing the UniRec template fields.
	 * @throws std::runtime_error if the template could not be created.
	 */
	void changeTemplate(const std::string& templateFields = "");

	/**
	 * @brief Disables sending an end-of-file marker on exit.
	 */
	void doNotsendEoFOnExit();

	/**
	 * @brief Gets the Unirec template used by the output interface.
	 *
	 * This method returns a pointer to the Unirec template used by the output interface.
	 *
	 * @return A pointer to the Unirec template used by the output interface.
	 */
	ur_template_t* getTemplate() const noexcept { return m_template; }

	/**
	 * @brief Destructor for the UnirecOutputInterface class.
	 *
	 * Sends an end-of-file marker if m_sendEoFonExit is true, then frees the memory allocated
	 * for the UniRec template.
	 */
	~UnirecOutputInterface();

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
	UnirecOutputInterface(uint8_t interfaceID);
	bool handleSendErrorCodes(int errorCode) const;
	void sendEoF() const;

	ur_template_t* m_template = nullptr;
	uint8_t m_interfaceID;
	bool m_sendEoFonExit;
	UnirecRecord m_unirecRecord;

	friend class Unirec;
};

} // namespace Nemea
