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

			std::string str() const;
		};
		struct linker_t
		{
			std::string flags;

			std::vector<std::string> libs;
			std::vector<std::string> lib_paths;

			std::string str() const;
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

	std::string str() const;
};

configuration_t::Type resolve_artifact_type(const std::string& artifact_type);

}

#endif /* CDTCONFIGURATION_H_ */
