/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of interface for receiving UniRec records using the TRAP protocol.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unirec++/inputInterface.hpp"

#include <libtrap/trap.h>

namespace Nemea {

UnirecInputInterface::~UnirecInputInterface()
{
	ur_free_template(m_template);
}

UnirecInputInterface::UnirecInputInterface(uint8_t interfaceID)
	: m_interfaceID(interfaceID)
	, m_sequenceNumber(0)
	, m_prioritizedDataPointer(nullptr)
{
	setRequieredFormat("");
}

std::optional<UnirecRecordView> UnirecInputInterface::receive()
{
	const void* receivedData;
	uint16_t dataSize = 0;

	if (m_prioritizedDataPointer) {
		receivedData = m_prioritizedDataPointer;
		m_prioritizedDataPointer = nullptr;
		return UnirecRecordView(receivedData, m_template, m_sequenceNumber);
	}

	int errorCode = trap_recv_with_seq_number(m_interfaceID, &receivedData, &dataSize, &m_sequenceNumber);
	if (errorCode == TRAP_E_TIMEOUT) {
		return std::nullopt;
	}
	if (errorCode == TRAP_E_FORMAT_CHANGED) {
		m_prioritizedDataPointer = receivedData;
		throw FormatChangeException();
	}
	handleReceiveErrorCodes(errorCode);

	if (dataSize <= 1) {
		throw EoFException();
	}

	return UnirecRecordView(receivedData, m_template, m_sequenceNumber);
}

void UnirecInputInterface::handleReceiveErrorCodes(int errorCode) const
{
	if (errorCode == TRAP_E_OK) {
		return;
	}
	if (errorCode == TRAP_E_NOT_INITIALIZED) {
		throw std::runtime_error(
			"UnirecInputInterface::receive() has failed. Trap interface is not initialized.");
	}
	if (errorCode == TRAP_E_TERMINATED) {
		throw std::runtime_error(
			"UnirecInputInterface::receive() has failed. Trap interface is terminated.");
	}
	if (errorCode == TRAP_E_NOT_SELECTED) {
		throw std::runtime_error(
			"UnirecInputInterface::receive() has failed. Interface ID out of range.");
	}
	throw std::runtime_error(
		"UnirecInputInterface::receive() has failed. Return code: " + std::to_string(errorCode)
		+ ", msg: " + trap_last_error_msg);
}

void UnirecInputInterface::setRequieredFormat(const std::string& templateSpecification)
{
	int ret = trap_set_required_fmt(m_interfaceID, TRAP_FMT_UNIREC, templateSpecification.c_str());
	if (ret != TRAP_E_OK) {
		throw std::runtime_error(
			"UnirecInputInterface::setRequieredFormat() has failed. Unable to set required "
			"format.");
	}

	changeInternalTemplate(templateSpecification);
}

void UnirecInputInterface::changeTemplate()
{
	uint8_t dataType;
	const char* spec = nullptr;

	int ret = trap_get_data_fmt(TRAPIFC_INPUT, m_interfaceID, &dataType, &spec);
	if (ret != TRAP_E_OK) {
		throw std::runtime_error(
			"UnirecInputInterface::changeTemplate() has failed. Data format was not "
			"loaded.");
	}

	changeInternalTemplate(spec);
}

void UnirecInputInterface::changeInternalTemplate(const std::string& templateSpecification)
{
	m_template = ur_define_fields_and_update_template(templateSpecification.c_str(), m_template);
	if (m_template == nullptr) {
		throw std::runtime_error(
			"UnirecInputInterface::changeTemplate() has failed. Template could not be "
			"edited.");
	}

	int ret = ur_set_input_template(m_interfaceID, m_template);
	if (ret != TRAP_E_OK) {
		throw std::runtime_error("UnirecInputInterface::changeTemplate() has failed.");
	}
}

void UnirecInputInterface::setTimeout(int timeout)
{
	trap_ifcctl(TRAPIFC_INPUT, m_interfaceID, TRAPCTL_SETTIMEOUT, timeout);
}

InputInteraceStats UnirecInputInterface::getInputInterfaceStats() const
{
	InputInteraceStats inputStats;

	struct input_ifc_stats ifcStats = {};
	trap_get_input_ifc_stats(m_interfaceID, &ifcStats);

	inputStats.receivedBytes = ifcStats.received_bytes;
	inputStats.receivedRecords = ifcStats.received_records;
	inputStats.missedRecords = ifcStats.missed_records;
	return inputStats;
}

} // namespace Nemea
