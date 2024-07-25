/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implements the UnirecOutputInterface class.
 */

#include "unirec++/outputInterface.hpp"

#include <libtrap/trap.h>
#include <stdexcept>

#include <memory>

namespace Nemea {

UnirecOutputInterface::UnirecOutputInterface(uint8_t interfaceID)
	: m_template(nullptr)
	, m_interfaceID(interfaceID)
	, m_sendEoFonExit(true)
	, m_isInitialized(false)
{
}

UnirecOutputInterface::~UnirecOutputInterface()
{
	if (m_sendEoFonExit && m_isInitialized) {
		sendEoF();
	}

	ur_free_template(m_template);
}

bool UnirecOutputInterface::send(UnirecRecord& unirecRecord) const
{
	int errorCode = trap_send(m_interfaceID, unirecRecord.data(), unirecRecord.size());
	return handleSendErrorCodes(errorCode);
}

bool UnirecOutputInterface::send(UnirecRecordView& unirecRecordView) const
{
	int errorCode = trap_send(m_interfaceID, unirecRecordView.data(), unirecRecordView.size());
	return handleSendErrorCodes(errorCode);
}

bool UnirecOutputInterface::handleSendErrorCodes(int errorCode) const
{
	if (errorCode == TRAP_E_TIMEOUT) {
		return false;
	}
	if (errorCode == TRAP_E_OK) {
		return true;
	}
	if (errorCode == TRAP_E_NOT_INITIALIZED) {
		throw std::runtime_error(
			"UnirecOutputInterface::send() has failed. Trap interface is not initialized.");
	}
	if (errorCode == TRAP_E_TERMINATED) {
		throw std::runtime_error(
			"UnirecOutputInterface::send() has failed. Trap interface is terminated.");
	}
	if (errorCode == TRAP_E_BAD_IFC_INDEX) {
		throw std::runtime_error(
			"UnirecOutputInterface::send() has failed. Interface ID out of range.");
	}
	throw std::runtime_error(
		"UnirecOutputInterface::send() has failed. Return code: " + std::to_string(errorCode)
		+ ", msg: " + trap_last_error_msg);
}

void UnirecOutputInterface::sendFlush() const
{
	trap_send_flush(m_interfaceID);
}

void UnirecOutputInterface::doNotsendEoFOnExit()
{
	m_sendEoFonExit = false;
}

void UnirecOutputInterface::setTimeout(int timeout)
{
	trap_ifcctl(TRAPIFC_OUTPUT, m_interfaceID, TRAPCTL_SETTIMEOUT, timeout);
}

void UnirecOutputInterface::setAutoflushTimeout(int timeout)
{
	trap_ifcctl(TRAPIFC_OUTPUT, m_interfaceID, TRAPCTL_AUTOFLUSH_TIMEOUT, timeout);
}

void UnirecOutputInterface::sendEoF() const
{
	char dummy[1] = { 0 };
	trap_send(m_interfaceID, dummy, sizeof(dummy));
}

void UnirecOutputInterface::changeTemplate(const std::string& templateFields)
{
	if (ur_define_set_of_fields(templateFields.c_str()) != UR_OK) {
		throw std::runtime_error(
			"UnirecOutputInterface::changeTemplate() has failed. Template fields could not be "
			"defined!");
	}

	std::unique_ptr<char*> fieldNames
		= std::make_unique<char*>(ur_ifc_data_fmt_to_field_names(templateFields.c_str()));

	if (!fieldNames) {
		throw std::runtime_error(
			"UnirecOutputInterface::changeTemplate() has failed. Field specs could not be "
			"converted.");
	}
	ur_free_template(m_template);
	m_template = ur_create_output_template(m_interfaceID, *fieldNames, nullptr);
	if (!m_template) {
		throw std::runtime_error(
			"UnirecOutputInterface::changeTemplate() has failed. Output template could not be "
			"created.");
	}
	m_unirecRecord = createUnirecRecord();

	m_isInitialized = true;
}

UnirecRecord UnirecOutputInterface::createUnirecRecord(size_t maxVariableFieldsSize)
{
	return UnirecRecord(m_template, maxVariableFieldsSize);
}

} // namespace Nemea
