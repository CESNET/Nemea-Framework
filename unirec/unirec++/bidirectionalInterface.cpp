/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of a bidirectional interface for sending and receiving unirec records using
 * the TRAP interface provided by the UniRec library.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unirec++/bidirectionalInterface.hpp"

#include <libtrap/trap.h>

namespace Nemea {

UnirecBidirectionalInterface::~UnirecBidirectionalInterface()
{
	if (m_sendEoFonExit) {
		sendEoF();
	}

	ur_free_template(m_template);
}

UnirecBidirectionalInterface::UnirecBidirectionalInterface(
	uint8_t inputInterfaceID,
	uint8_t outputInterfaceID)
	: m_template(nullptr)
	, m_inputInterfaceID(inputInterfaceID)
	, m_outputInterfaceID(outputInterfaceID)
	, m_prioritizedDataPointer(nullptr)
	, m_sendEoFonExit(true)
	, m_EoFOnNextReceive(false)
{
	setRequieredFormat("");
}

std::optional<UnirecRecordView> UnirecBidirectionalInterface::receive()
{
	const void* receivedData;
	uint16_t dataSize = 0;

	if (isEoFReceived()) {
		throw EoFException();
	}

	if (m_prioritizedDataPointer) {
		receivedData = m_prioritizedDataPointer;
		m_prioritizedDataPointer = nullptr;
		return UnirecRecordView(receivedData, m_template);
	}

	int errorCode = trap_recv(m_inputInterfaceID, &receivedData, &dataSize);
	if (errorCode == TRAP_E_TIMEOUT) {
		return std::nullopt;
	}
	if (errorCode == TRAP_E_FORMAT_CHANGED) {
		m_prioritizedDataPointer = receivedData;
		throw FormatChangeException();
	}
	handleReceiveErrorCodes(errorCode);

	if (dataSize <= 1) {
		m_EoFOnNextReceive = true;
	}

	return UnirecRecordView(receivedData, m_template);
}

bool UnirecBidirectionalInterface::isEoFReceived() const noexcept
{
	return m_EoFOnNextReceive;
}

void UnirecBidirectionalInterface::handleReceiveErrorCodes(int errorCode) const
{
	if (errorCode == TRAP_E_OK) {
		return;
	}
	if (errorCode == TRAP_E_NOT_INITIALIZED) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::receive() has failed. Trap interface is not "
			"initialized.");
	}
	if (errorCode == TRAP_E_TERMINATED) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::receive() has failed. Trap interface is terminated.");
	}
	if (errorCode == TRAP_E_NOT_SELECTED) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::receive() has failed. Interface ID out of range.");
	}
	throw std::runtime_error(
		"UnirecBidirectionalInterface::receive() has failed. Return code: "
		+ std::to_string(errorCode) + ", msg: " + trap_last_error_msg);
}

void UnirecBidirectionalInterface::setRequieredFormat(const std::string& templateSpecification)
{
	int ret
		= trap_set_required_fmt(m_inputInterfaceID, TRAP_FMT_UNIREC, templateSpecification.c_str());
	if (ret != TRAP_E_OK) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::setRequieredFormat() has failed. Unable to set required "
			"format.");
	}
}

void UnirecBidirectionalInterface::setReceiveTimeout(int timeout)
{
	trap_ifcctl(TRAPIFC_INPUT, m_inputInterfaceID, TRAPCTL_SETTIMEOUT, timeout);
}

void UnirecBidirectionalInterface::changeTemplate()
{
	uint8_t dataType;
	const char* spec = nullptr;

	int ret = trap_get_data_fmt(TRAPIFC_INPUT, m_inputInterfaceID, &dataType, &spec);
	if (ret != TRAP_E_OK) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::changeTemplate() has failed. Data format was not "
			"loaded.");
	}

	m_template = ur_define_fields_and_update_template(spec, m_template);
	if (m_template == nullptr) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::changeTemplate() has failed. Template could not be "
			"edited.");
	}

	std::string specCopy = spec;
	trap_set_data_fmt(m_outputInterfaceID, TRAP_FMT_UNIREC, specCopy.c_str());

	ret = ur_set_input_template(m_inputInterfaceID, m_template);
	if (ret != TRAP_E_OK) {
		throw std::runtime_error("UnirecBidirectionalInterface::changeTemplate() has failed.");
	}

	ret = ur_set_output_template(m_outputInterfaceID, m_template);
	if (ret != TRAP_E_OK) {
		throw std::runtime_error("UnirecBidirectionalInterface::changeTemplate() has failed.");
	}

	m_unirecRecord = createUnirecRecord();
}

UnirecRecord UnirecBidirectionalInterface::createUnirecRecord(size_t maxVariableFieldsSize)
{
	return UnirecRecord(m_template, maxVariableFieldsSize);
}

bool UnirecBidirectionalInterface::send(UnirecRecord& unirecRecord) const
{
	int errorCode = trap_send(m_outputInterfaceID, unirecRecord.data(), unirecRecord.size());
	return handleSendErrorCodes(errorCode);
}

bool UnirecBidirectionalInterface::send(UnirecRecordView& unirecRecordView) const
{
	int errorCode
		= trap_send(m_outputInterfaceID, unirecRecordView.data(), unirecRecordView.size());
	return handleSendErrorCodes(errorCode);
}

bool UnirecBidirectionalInterface::handleSendErrorCodes(int errorCode) const
{
	if (errorCode == TRAP_E_TIMEOUT) {
		return false;
	}
	if (errorCode == TRAP_E_OK) {
		return true;
	}
	if (errorCode == TRAP_E_NOT_INITIALIZED) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::send() has failed. Trap interface is not initialized.");
	}
	if (errorCode == TRAP_E_TERMINATED) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::send() has failed. Trap interface is terminated.");
	}
	if (errorCode == TRAP_E_BAD_IFC_INDEX) {
		throw std::runtime_error(
			"UnirecBidirectionalInterface::send() has failed. Interface ID out of range.");
	}
	throw std::runtime_error(
		"UnirecBidirectionalInterface::send() has failed. Return code: " + std::to_string(errorCode)
		+ ", msg: " + trap_last_error_msg);
}

void UnirecBidirectionalInterface::sendFlush() const
{
	trap_send_flush(m_outputInterfaceID);
}

void UnirecBidirectionalInterface::doNotsendEoFOnExit()
{
	m_sendEoFonExit = false;
}

void UnirecBidirectionalInterface::setSendTimeout(int timeout)
{
	trap_ifcctl(TRAPIFC_OUTPUT, m_outputInterfaceID, TRAPCTL_SETTIMEOUT, timeout);
}

void UnirecBidirectionalInterface::setSendAutoflushTimeout(int timeout)
{
	trap_ifcctl(TRAPIFC_OUTPUT, m_outputInterfaceID, TRAPCTL_AUTOFLUSH_TIMEOUT, timeout);
}

void UnirecBidirectionalInterface::sendEoF() const
{
	char dummy[1] = {0};
	trap_send(m_outputInterfaceID, dummy, sizeof(dummy));
}

} // namespace Nemea
