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
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iterator>
#include "listfile.h"

namespace cmake
{

/*
TODO logic encoded in the following has to be converted to new interface.
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

std::ostream& operator<<(std::ostream& os, const project_t& p)
{
	os << "cmake_minimum_required (VERSION 2.6)\n";
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

//	for(auto& dir : p.subdirectories)
//		os << "add_subdirectory(" << dir << ")\n";

	os << "\n";
	return os;
}*/

bool has_c_sources(const std::map<std::string, std::vector<std::string> >& sources)
{
	for(const auto& source_folder : sources)
	{
		for(const auto& source : source_folder.second)
		{
			if(is_c_source_filename(source))
				return true;
		}
	}
	return false;
}

bool has_cxx_sources(const std::map<std::string, std::vector<std::string> >& sources)
{
	for(const auto& source_folder : sources)
	{
		for(const auto& source : source_folder.second)
		{
			if(is_cxx_source_filename(source))
				return true;
		}
	}
	return false;
}

void merge(const cdt::configuration_t::build_folder::compiler_t& source, cdt::configuration_t::build_folder::compiler_t& merged)
{
	for(auto inc : source.includes)
	{
		if(inc.find("\"${workspace_loc:/") == 0)
		{
			inc = inc.substr(18);
			inc = inc.substr(0, inc.size() - 2);
			inc = "${PROJECT_SOURCE_DIR}/" + inc;

			if(inc[inc.size()-1] != '/')
				inc += '/';
		}
		else if(inc.find("../../") == 0)
		{
			inc = inc.substr(6);
			inc = "${PROJECT_SOURCE_DIR}/" + inc;

			if(inc[inc.size()-1] != '/')
				inc += '/';
		}
		else
		{
			if(inc[inc.size()-1] != '/')
				inc += '/';
		}
	
		if(std::find(merged.includes.begin(), merged.includes.end(), inc) == merged.includes.end())
		{
			merged.includes.push_back(inc);
		}
	}
	
	if(source.options != merged.options)
	{
		if(merged.options.empty())
			merged.options = source.options;
		else
			merged.options += " " + source.options;
	}
}
void merge(const cdt::configuration_t::build_folder::linker_t& source, cdt::configuration_t::build_folder::linker_t& merged)
{
	for(auto& lib : source.libs)
	{
		if(std::find(merged.libs.begin(), merged.libs.end(), lib) == merged.libs.end())
			merged.libs.push_back(lib);
	}
	for(auto lib : source.lib_paths)
	{
		if(lib.empty())
			continue;
		if(lib.find("\"${workspace_loc:/") == 0)
		{
			// CMake is smarter than eclipse
			lib = "";
		}
		else if(lib.find("../../") == 0)
		{
			// CMake is smarter than eclipse
			lib = "";
		}
		else
		{
			if(lib[lib.size()-1] != '/')
				lib += '/';
		}

		if(lib.empty())
			continue;
		
		if(std::find(merged.lib_paths.begin(), merged.lib_paths.end(), lib) == merged.lib_paths.end())
			merged.lib_paths.push_back(lib);
	}
	if(source.flags != merged.flags)
	{
		if(merged.flags.empty())
			merged.flags = source.flags;
		else
			merged.flags += " " + source.flags;
	}
}

void merge(const cdt::configuration_t::build_folder& source, cdt::configuration_t::build_folder& merged)
{
	merge(source.c.compiler, merged.c.compiler);
	merge(source.c.linker, merged.c.linker);
	merge(source.cpp.compiler, merged.cpp.compiler);
	merge(source.cpp.linker, merged.cpp.linker);
}

void merge(const cdt::configuration_t::build_file& source, cdt::configuration_t::build_file& merged)
{
	if(source.command != merged.command)
	{
		if(merged.command.empty())
			merged.command = source.command;
		else
			merged.command += " / " + source.command;
	}
	if(source.inputs != merged.inputs)
	{
		if(merged.inputs.empty())
			merged.inputs = source.inputs;
		else
			merged.inputs += " / " + source.inputs;
	}
	if(source.outputs != merged.outputs)
	{
		if(merged.outputs.empty())
			merged.outputs = source.outputs;
		else
			merged.outputs += " / " + source.outputs;
	}
}

// one step, take cdt files and write cmakelists.
void generate(cdt::project& cdtproject, bool write_files)
{
	auto project_name = cdtproject.name();
	auto project_path = cdtproject.path();

	std::map<std::string, std::vector<std::string> > sources;
	{
		auto source_files = find_sources(cdtproject.path(), is_source_filename);
		for(const auto& source : source_files)
			sources[source.path].push_back(source.name);
	}

	bool lang_c = has_c_sources(sources);
	bool lang_cxx = has_cxx_sources(sources);

	std::map<std::string, cdt::configuration_t> artifact_configurations;
	
	auto confs = cdtproject.cconfigurations();
	for(const auto& conf_name : confs)
	{
		auto c = cdtproject.configuration(conf_name);
		cdt::configuration_t& a = artifact_configurations[c.artifact + to_string(c.type)];
		a.name = c.artifact + to_string(c.type);
		a.artifact = c.artifact;
		if(a.prebuild != c.prebuild)
		{
			if(a.prebuild.empty())
				a.prebuild = c.prebuild;
			else
				a.prebuild += " / " + c.prebuild;
		}
		if(a.postbuild != c.postbuild)
		{
			if(a.postbuild.empty())
				a.postbuild = c.postbuild;
			else
				a.postbuild += " / " + c.postbuild;
		}
		a.type = c.type;

		for(auto& bf : c.build_folders)
		{
			cdt::configuration_t::build_folder* merged_bf = nullptr;
			for(auto& abf : a.build_folders)
			{
				if(abf.path == bf.path)
					merged_bf = &abf;
			}
			
			if(!merged_bf)
			{
				cdt::configuration_t::build_folder nbf;
				nbf.path = bf.path;
				
				a.build_folders.push_back(nbf);
				merged_bf = &a.build_folders.back();
			}
			merge(bf, *merged_bf);
		}
		for(auto& bf : c.build_files)
		{
			cdt::configuration_t::build_file* merged_bf = nullptr;
			for(auto& abf : a.build_files)
			{
				if(abf.file == bf.file)
					merged_bf = &abf;
			}
			
			if(merged_bf)
			{
				merge(bf, *merged_bf);
			}
			else
			{
				a.build_files.push_back(bf);
			}
		}
	}

	std::streambuf* buf;
	std::ofstream of;
	if(write_files)
	{
		of.open(project_path + "/CMakeLists.txt");
		buf = of.rdbuf();
	}
	else
	{
		buf = std::cout.rdbuf();
	}
	std::ostream master(buf);
	
	master << "cmake_minimum_required (VERSION 2.6)\n";
	master << "project (" << project_name << ")\n";
	master << "\n";

	for(auto& ac : artifact_configurations)
	{
		auto& c = ac.second;
		
		switch(c.type)
		{
			case cdt::configuration_t::Type::Executable:
				master << "add_executable (" << c.artifact;
				break;
			case cdt::configuration_t::Type::StaticLibrary:
				master << "add_library (" << c.artifact << " STATIC";
				break;
			case cdt::configuration_t::Type::SharedLibrary:
				master << "add_library (" << c.artifact << " SHARED";
				break;
		}
		for(const auto& source_folder : sources)
		{
			for(const auto& source : source_folder.second)
			{
				master << (source_folder.second.size() > 3 ? "\n   " : " ") << (source_folder.first.empty() ? std::string{} : source_folder.first + "/") << source;
			}
			master << "\n";
		}
		master << ")\n";
		
		if(!c.prebuild.empty() || !c.postbuild.empty())
		{
			master << "\n";
			master << "# prebuild: " << c.prebuild << "\n";
			master << "# postbuild: " << c.postbuild << "\n";
			master << "\n";
		}
		
		for(auto& bf : c.build_folders)
		{
			if(bf.path.empty())
			{
				// master
				
				{
					if(!bf.cpp.compiler.includes.empty() || !bf.c.compiler.includes.empty())
					{
						master << "set_target_properties (" << c.artifact << " INCLUDE_DIRECTORIES";
						if(lang_cxx)
						{
							for(auto& inc : bf.cpp.compiler.includes)
								master << (bf.cpp.compiler.includes.size() > 3 ? "\n   " : " ") << '"' << inc << '"';
						}
						if(lang_c)
						{
							for(auto& inc : bf.c.compiler.includes)
								master << (bf.c.compiler.includes.size() > 3 ? "\n   " : " ") << '"' << inc << '"';
						}
						master << ")\n\n";
					}
					
					std::vector<std::string> options;
					if(lang_cxx)
					{
						std::stringstream ss(bf.cpp.compiler.options);
						std::vector<std::string> opts(std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{});
						for(auto& o : opts)
						{
							if(std::find(begin(options), end(options), o) == end(options))
								options.push_back(o);
						}
					}
					if(lang_c)
					{
						std::stringstream ss(bf.c.compiler.options);
						std::vector<std::string> opts(std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{});
						for(auto& o : opts)
						{
							if(std::find(begin(options), end(options), o) == end(options))
								options.push_back(o);
						}
					}
					
					if(!options.empty())
					{
						master << "set_target_properties(" << c.artifact << " PROPERTIES COMPILE_FLAGS \"";
						for(auto& o : options)
							master << o << ' ';
						master << "\")\n\n";
					}
				}
				
				if(lang_cxx)
				{
					// use c++ linker settings.
					if(!bf.cpp.linker.flags.empty())
					{
						std::vector<std::string> flags;
						
						std::stringstream ss(bf.cpp.linker.flags);
						std::vector<std::string> opts(std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{});
						for(auto& o : opts)
						{
							if(std::find(begin(flags), end(flags), o) == end(flags))
								flags.push_back(o);
						}
						
						master << "set_target_properties(" << c.artifact << " PROPERTIES LINK_FLAGS \"";
						for(auto& o : flags)
							master << o << ' ';
						master << "\")\n";
					}
				
					if(!bf.cpp.linker.lib_paths.empty())
					{
						master << "link_directories (";
						for(auto& path : bf.cpp.linker.lib_paths)
							master << (bf.cpp.linker.lib_paths.size() > 3 ? "\n   " : " ") << path;
						master << ")\n";
					}

					if(!bf.cpp.linker.libs.empty())
					{
						master << "target_link_libraries (" << c.artifact;
						for(auto& lib : bf.cpp.linker.libs)
							master << (bf.cpp.linker.libs.size() > 3 ? "\n   " : " ") << lib;
						master << ")\n\n";
					}
				}
				else
				{
					if(!bf.c.linker.flags.empty())
					{
						std::vector<std::string> flags;
						
						std::stringstream ss(bf.c.linker.flags);
						std::vector<std::string> opts(std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{});
						for(auto& o : opts)
						{
							if(std::find(begin(flags), end(flags), o) == end(flags))
								flags.push_back(o);
						}
						
						master << "set_target_properties(" << c.artifact << " PROPERTIES LINK_FLAGS \"";
						for(auto& o : flags)
							master << o << ' ';
						master << "\")\n";
					}
				
					if(!bf.c.linker.lib_paths.empty())
					{
						master << "link_directories (";
						for(auto& path : bf.c.linker.lib_paths)
							master << (bf.c.linker.lib_paths.size() > 3 ? "\n   " : " ") << path;
						master << ")\n";
					}

					if(!bf.c.linker.libs.empty())
					{
						master << "target_link_libraries (" << c.artifact;
						for(auto& lib : bf.c.linker.libs)
							master << (bf.c.linker.libs.size() > 3 ? "\n   " : " ") << lib;
						master << ")\n\n";
					}
				}
			}
			else
			{
				master << "add_subdirectory(" << bf.path << ")\n";
				// create subdir file.
				// subdirectory
			}
		}
		
		for(auto& bf : c.build_files)
		{
			// TODO
			master << "# TODO " << bf.file << '\n';
		}
	}
	
	master << '\n';
}

}
