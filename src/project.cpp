/*
 * project.cpp
 *
 *  Created on: 15/04/2013
 *      Author: buildbot
 */

#include "project.h"
#include "cdtproject.h"
#include "sourcediscovery.h"
#include <map>
#include <vector>

namespace cmake
{

void project_t::clean()
{
	{
		std::set<std::string> includes;
		for(std::set<std::string>::const_iterator it = artifact.includes.begin(); it != artifact.includes.end(); ++it)
		{
			std::string inc = *it;
			if(inc.empty())
				continue;

			if(inc.find("\"${workspace_loc:/") == 0)
			{
				inc = inc.substr(18);
				inc = inc.substr(0, inc.size() - 2);
				inc = "${PROJECT_SOURCE_DIR}/" + inc;

				if(inc[inc.size()-1] != '/')
					inc += '/';

				includes.insert(inc);
			}
			else if(inc.find("../../") == 0)
			{
				inc = inc.substr(6);
				inc = "${PROJECT_SOURCE_DIR}/" + inc;

				if(inc[inc.size()-1] != '/')
					inc += '/';

				includes.insert(inc);
			}
			else
			{
				if(inc[inc.size()-1] != '/')
					inc += '/';

				includes.insert(inc);
			}
		}
		artifact.includes = includes;
	}

	{
		std::set<std::string> lib_paths;
		for(std::set<std::string>::const_iterator it = artifact.lib_paths.begin(); it != artifact.lib_paths.end(); ++it)
		{
			std::string lib = *it;

			if(lib.empty())
				continue;
			if(lib.find("\"${workspace_loc:/") == 0)
			{
				// CMake is smarter than eclipse
			}
			else if(lib.find("../../") == 0)
			{
				// CMake is smarter than eclipse
			}
			else
			{
				if(lib[lib.size()-1] != '/')
					lib += '/';

				lib_paths.insert(lib);
			}
		}
		artifact.lib_paths = lib_paths;
	}

	{
		std::vector<std::string> libs;
		std::set<std::string> seen;
		for(std::vector<std::string>::const_iterator it = artifact.libs.begin(); it != artifact.libs.end(); ++it)
		{
			if(seen.find(*it) == seen.end())
			{
				libs.push_back(*it);
				seen.insert(*it);
			}
		}
		artifact.libs = libs;
	}
}

project_t build_from(cdt::project& cdtproject)
{
	project_t p;
	p.name = cdtproject.name();

	std::map<std::string, std::vector<std::string> > source_paths;
	{
		auto sources = find_sources(cdtproject.path(), is_source_filename);
		for(const auto& source : sources)
			source_paths[source.path].push_back(source.name);
	}

	for(const auto& source_folder : source_paths)
		p.subdirectories.push_back(source_folder.first);

	auto confs = cdtproject.cconfigurations();

	// testing
	for(const auto& conf : confs)
		std::cout << cdtproject.configuration(conf) << "\n";

	return p;
}

std::ostream& operator<<(std::ostream& os, const project_t& p)
{
	os << "cmake_minimum_required (VERSION 2.8)\n";
	os << "project (" << p.name << ")\n";
	os << "\n";

	if(!p.artifact.other_flags.empty())
		os << "set (" << p.name << "_CFLAGS \"" << p.artifact.other_flags << "\")\n\n";

	if(!p.artifact.includes.empty())
	{
		os << "set_target_properties (" << p.artifact.name << " INCLUDE_DIRECTORIES\n";
		for(auto& inc : p.artifact.includes)
			os << "    " << inc << "\n";
		os << ")\n";
		os << "\n";
	}

	switch(p.artifact.type)
	{
		case artifact_t::type_Executable:
			os << "add_executable (" << p.artifact.name << "\n";
			break;
		case artifact_t::type_StaticLibrary:
			os << "add_library (" << p.artifact.name << " STATIC\n";
			break;
		case artifact_t::type_SharedLibrary:
			os << "add_library (" << p.artifact.name << " SHARED\n";
			break;
	}
	for(auto& src : p.artifact.sources)
		os << "    " << src << "\n";
	os << ")\n\n";

	if(!p.artifact.lib_paths.empty())
	{
		os << "link_directories (\n";
		for(auto& path : p.artifact.lib_paths)
			os << "    " << path << "\n";
		os << ")\n";
		os << "\n";
	}

	if(!p.artifact.libs.empty())
	{
		os << "target_link_libraries (" << p.artifact.name << "\n";
		for(auto& lib : p.artifact.libs)
			os << "    " << lib << "\n";
		os << ")\n";
		os << "\n";
	}

	for(auto& dir : p.subdirectories)
		os << "add_subdirectory(" << dir << ")\n";

	os << "\n";
	return os;
}

}
