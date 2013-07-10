/*
 * cdtconfiguration.cpp
 *
 *  Created on: 19/04/2013
 *      Author: nicholas
 */

#include "cdtconfiguration.h"
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace cdt
{

std::string to_string(configuration_t::Type t)
{
	switch(t)
	{
		case configuration_t::Type::Executable:
			return "EXECUTABLE";
		case configuration_t::Type::StaticLibrary:
			return "STATIC";
		case configuration_t::Type::SharedLibrary:
			return "SHARED";
	}
	return "UNKNOWN";
}

configuration_t::Type resolve_artifact_type(const std::string& artifact_type)
{
	if(artifact_type == "org.eclipse.cdt.build.core.buildArtefactType.exe")
		return configuration_t::Type::Executable;
	else if(artifact_type == "org.eclipse.cdt.build.core.buildArtefactType.staticLib")
		return configuration_t::Type::StaticLibrary;
	else if(artifact_type == "org.eclipse.cdt.build.core.buildArtefactType.sharedLib")
		return configuration_t::Type::SharedLibrary;
	else
		throw std::runtime_error("Unknown artifact type: " + artifact_type);
}

std::ostream& operator<<(std::ostream& os, const configuration_t& conf)
{
	os << "{\n";
	os << "name: '" << conf.name << "'\n";
	os << "artifact: '" << conf.artifact << "'\n";

	os << "prebuild: '" << conf.prebuild << "'\n";
	os << "postbuild: '" << conf.postbuild << "'\n";

	switch(conf.type)
	{
		case configuration_t::Type::Executable:
			os << "type: '" << "Executable" << "'\n";
			break;
		case configuration_t::Type::StaticLibrary:
			os << "type: '" << "Static Library" << "'\n";
			break;
		case configuration_t::Type::SharedLibrary:
			os << "type: '" << "Shared Library" << "'\n";
			break;
	}

	for(auto& bf : conf.build_folders)
		os << bf;

	for(auto& bf : conf.build_files)
		os << bf;

	os << "}\n";
	return os;
}
std::ostream& operator<<(std::ostream& os, const configuration_t::build_folder& bf)
{
	os << "folder: '" << bf.path << "' {\n";
	os << "   cpp.compiler: " << bf.cpp.compiler << "\n";
	os << "   c.compiler: " << bf.c.compiler << "\n";
	os << "   cpp.linker: " << bf.cpp.linker << "\n";
	os << "   c.linker: " << bf.c.linker << "\n";
	os << "}\n";
	return os;
}
std::ostream& operator<<(std::ostream& os, const configuration_t::build_file& bf)
{
	os << "file: '" << bf.file << "' {\n";
	os << "   command: " << bf.command << "\n";
	os << "   inputs: " << bf.inputs << "\n";
	os << "   outputs: " << bf.outputs << "\n";
	os << "}\n";
	return os;
}
std::ostream& operator<<(std::ostream& os, const configuration_t::build_folder::compiler_t& c)
{
	os << "{\n";

	os << "      includes: ";
	std::copy(c.includes.begin(), c.includes.end(), std::ostream_iterator<std::string>(os, ", "));
	os << "\n";

	os << "      options: " << c.options << "\n";

	os << "   }\n";
	return os;
}
std::ostream& operator<<(std::ostream& os, const configuration_t::build_folder::linker_t& l)
{
	os << "{\n";

	os << "      flags: " << l.flags << "\n";

	os << "      libs: ";
	std::copy(l.libs.begin(), l.libs.end(), std::ostream_iterator<std::string>(os, ", "));
	os << "\n";

	os << "      lib_paths: ";
	std::copy(l.lib_paths.begin(), l.lib_paths.end(), std::ostream_iterator<std::string>(os, ", "));
	os << "\n";

	os << "   }\n";
	return os;
}

}
