/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Contains the definition of the TrapModuleInfo class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <libtrap/trap.h>
#include <string>

namespace Nemea {

/**
 * @brief Class representing information about a trap module.
 */
class TrapModuleInfo {
public:
	/**
	 * @brief Constructor for TrapModuleInfo class.
	 * @param inputIfcCount The number of input interfaces of the module [-1: variable].
	 * @param outputIfcCount The number of output interfaces of the module [-1: variable].
	 * @param moduleName The name of the module.
	 * @param moduleDescription The description of the module.
	 */
	TrapModuleInfo(
		int inputIfcCount,
		int outputIfcCount,
		const std::string& moduleName = "",
		const std::string& moduleDescription = "")
		: m_moduleName(moduleName)
		, m_moduleDescription(moduleDescription)
	{
		m_moduleInfo.name = m_moduleName.data();
		m_moduleInfo.description = m_moduleDescription.data();
		m_moduleInfo.num_ifc_in = inputIfcCount;
		m_moduleInfo.num_ifc_out = outputIfcCount;
		m_moduleInfo.params = nullptr;
	}

private:
	std::string m_moduleName;
	std::string m_moduleDescription;
	trap_module_info_t m_moduleInfo;

	friend class Libtrap;
	friend class Unirec;
};

} // namespace Nemea
