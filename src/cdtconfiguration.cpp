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

std::string configuration_t::build_folder::compiler_t::str() const
{
	std::stringstream s;
	s << "{\n";
	s << "   includes: ";
	std::copy(includes.begin(), includes.end(), std::ostream_iterator<std::string>(s, ", "));
	s << "\n";
	s << "   options: " << options << "\n";
	s << "}\n";
	return s.str();
}
std::string configuration_t::build_folder::linker_t::str() const
{
	std::stringstream s;
	s << "{\n";

	s << "   flags: " << flags << "\n";

	s << "   libs: ";
	std::copy(libs.begin(), libs.end(), std::ostream_iterator<std::string>(s, ", "));
	s << "\n";

	s << "   lib_paths: ";
	std::copy(lib_paths.begin(), lib_paths.end(), std::ostream_iterator<std::string>(s, ", "));
	s << "\n";

	s << "}\n";
	return s.str();
}

std::string configuration_t::str() const
{
	std::stringstream s;

	s << "name: '" << name << "'\n";
	s << "artifact: '" << artifact << "'\n";

	s << "prebuild: '" << prebuild << "'\n";
	s << "postbuild: '" << postbuild << "'\n";

	switch(type)
	{
		case Type::Executable:
			s << "type: '" << "Executable" << "'\n";
			break;
		case Type::StaticLibrary:
			s << "type: '" << "Static Library" << "'\n";
			break;
		case Type::SharedLibrary:
			s << "type: '" << "Shared Library" << "'\n";
			break;
	}

	for(auto& bf : build_folders)
	{
		s << "folder: '" << bf.path << "' {\n";
		s << "   cpp.compiler: " << bf.cpp.compiler.str() << "\n";
		s << "   c.compiler: " << bf.c.compiler.str() << "\n";
		s << "   cpp.linker: " << bf.cpp.linker.str() << "\n";
		s << "   c.linker: " << bf.c.linker.str() << "\n";
		s << "}\n";
	}

	for(auto& bf : build_files)
	{
		s << "file: '" << bf.file << "' {\n";
		s << "   command: " << bf.command << "\n";
		s << "   inputs: " << bf.inputs << "\n";
		s << "   outputs: " << bf.outputs << "\n";
		s << "}\n";
	}

	return s.str();
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

}
