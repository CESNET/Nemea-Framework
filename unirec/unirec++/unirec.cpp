#include <unirec++/unirec.hpp>
#include <unirec++/unirecException.hpp>

namespace NemeaPlusPlus {

Unirec::Unirec(const TrapModuleInfo& trapModuleInfo)
	: m_availableInputInterfaces(0)
	, m_availableOutputInterfaces(0)
	, m_trapModuleInfo(trapModuleInfo)
{
}

void Unirec::init(int& argc, char** argv)
{
	trap_ifc_spec_t trapIfcSpec;

	parseCommandLine(argc, argv, trapIfcSpec);

	if (trap_init(&m_trapModuleInfo.m_moduleInfo, trapIfcSpec) != TRAP_E_OK) {
		trap_free_ifc_spec(trapIfcSpec);
		throw std::runtime_error("Unirec::init() has failed. " + std::string(trap_last_error_msg));
	}

	trap_free_ifc_spec(trapIfcSpec);

	m_availableInputInterfaces = m_trapModuleInfo.m_moduleInfo.num_ifc_in;
	m_availableOutputInterfaces = m_trapModuleInfo.m_moduleInfo.num_ifc_out;
}

void Unirec::parseCommandLine(int& argc, char** argv, trap_ifc_spec_t& trapIfcSpec)
{
	int ret = trap_parse_params(&argc, argv, &trapIfcSpec);
	if (ret == TRAP_E_OK) {
		return;
	}

	if (ret == TRAP_E_HELP) {
		trap_print_help(&m_trapModuleInfo.m_moduleInfo);
		throw HelpException();
	}

	throw std::runtime_error(
		"Libtrap::parseCommandLine() has failed. " + std::string(trap_last_error_msg));
}

UnirecInputInterface Unirec::buildInputInterface()
{
	if (!isInputInterfaceAvailable()) {
		throw std::runtime_error("TODO");
	}
	uint8_t interfaceID = --m_availableInputInterfaces;
	return UnirecInputInterface(interfaceID);
}

UnirecOutputInterface Unirec::buildOutputInterface()
{
	if (!isOutputInterfaceAvailable()) {
		throw std::runtime_error("TODO");
	}
	uint8_t interfaceID = --m_availableOutputInterfaces;
	return UnirecOutputInterface(interfaceID);
}

UnirecBidirectionalInterface Unirec::buildBidirectionalInterface()
{
	if (!isBidirectionalInterfaceAvailable()) {
		throw std::runtime_error("TODO");
	}

	uint8_t inputInterfaceID = --m_availableInputInterfaces;
	uint8_t outputInterfaceID = --m_availableOutputInterfaces;
	return UnirecBidirectionalInterface(inputInterfaceID, outputInterfaceID);
}

ur_field_id_t Unirec::defineUnirecField(const std::string& fieldName, ur_field_type_t fieldType)
{
	int ret = ur_define_field(fieldName.c_str(), fieldType);
	if (ret < 0) {
		throw std::runtime_error(
			"Unirec::defineUnirecField() has failed. Error code=[" + std::to_string(ret) + "]");
	}
	return static_cast<ur_field_id_t>(ret);
}

Unirec::~Unirec()
{
	trap_finalize();
	ur_finalize();
}

} // namespace NemeaPlusPlus
