/*
 * cdtconfiguration.h
 *
 *  Created on: 19/04/2013
 *      Author: nicholas
 */

#ifndef CDTCONFIGURATION_H_
#define CDTCONFIGURATION_H_
#include <string>
#include <vector>
#include <ostream>

namespace cdt
{

struct configuration_t
{
	std::string name;
	std::string artifact;

	std::string prebuild;
	std::string postbuild;

	enum class Type
	{
		Executable,
		StaticLibrary,
		SharedLibrary
	} type;

	struct build_folder
	{
		std::string path;

		struct compiler_t
		{
			std::vector<std::string> includes;
			std::string options;
		};
		struct linker_t
		{
			std::string flags;
			std::vector<std::string> libs;
			std::vector<std::string> lib_paths;
		};

		struct
		{
			compiler_t compiler;
			linker_t linker;
		} c;

		struct
		{
			compiler_t compiler;
			linker_t linker;
		} cpp;
	};

	std::vector<build_folder> build_folders;

	struct build_file
	{
		std::string file;
		std::string command;
		std::string inputs;
		std::string outputs;
	};

	std::vector<build_file> build_files;
};

configuration_t::Type resolve_artifact_type(const std::string& artifact_type);

std::ostream& operator<<(std::ostream& os, const configuration_t& conf);
std::ostream& operator<<(std::ostream& os, const configuration_t::build_folder& bf);
std::ostream& operator<<(std::ostream& os, const configuration_t::build_file& bf);
std::ostream& operator<<(std::ostream& os, const configuration_t::build_folder::compiler_t& c);
std::ostream& operator<<(std::ostream& os, const configuration_t::build_folder::linker_t& l);

}

#endif /* CDTCONFIGURATION_H_ */
