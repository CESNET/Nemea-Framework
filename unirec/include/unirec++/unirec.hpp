#pragma once

#include "bidirectionalInterface.hpp"
#include "inputInterface.hpp"
#include "outputInterface.hpp"
#include "trapModuleInfo.hpp"

#include <libtrap/trap.h>
#include <string>
#include <unirec/unirec.h>

namespace Nemea {

class Unirec {
public:
	/*
	 * @brief Constructs a new Unirec object with the given TrapModuleInfo.
	 * @param trapModuleInfo A reference to a TrapModuleInfo object containing information about the
	 * trap module being used.
	 */
	Unirec(const TrapModuleInfo& trapModuleInfo);

	~Unirec();

	void init(int& argc, char** argv);

	ur_field_id_t defineUnirecField(const std::string& fieldName, ur_field_type_t fieldType);

	/**
	 * @brief Builds and returns a UnirecInputInterface object if an input interface is available.
	 * @throws std::runtime_error if no input interface is available.
	 * @return A UnirecInputInterface object.
	 */
	UnirecInputInterface buildInputInterface();
	UnirecOutputInterface buildOutputInterface();
	UnirecBidirectionalInterface buildBidirectionalInterface();

	bool isInputInterfaceAvailable() const noexcept { return m_availableInputInterfaces; }
	bool isOutputInterfaceAvailable() const noexcept { return m_availableOutputInterfaces; }
	bool isBidirectionalInterfaceAvailable() const noexcept
	{
		return m_availableInputInterfaces && m_availableOutputInterfaces;
	}

private:
	void parseCommandLine(int& argc, char** argv, trap_ifc_spec_t& trapIfcSpec);

	uint8_t m_availableInputInterfaces;
	uint8_t m_availableOutputInterfaces;
	TrapModuleInfo m_trapModuleInfo;
};

} // namespace Nemea
