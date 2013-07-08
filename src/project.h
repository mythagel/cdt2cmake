/*
 * project.h
 *
 *  Created on: 15/04/2013
 *      Author: buildbot
 */

#ifndef PROJECT_H_
#define PROJECT_H_
#include <string>
#include <set>
#include <vector>
#include <ostream>

namespace cdt
{
struct project;
}

namespace cmake
{

struct artifact_t
{
	std::string name;

	enum Type
	{
		type_Executable,
		type_StaticLibrary,
		type_SharedLibrary
	} type;

	std::set<std::string> includes;
	std::vector<std::string> libs;
	std::set<std::string> lib_paths;
	std::set<std::string> sources;

	std::string other_flags;
};

struct project_t
{
	std::string name;
	std::string base_path;
	artifact_t artifact;

	std::vector<std::string> subdirectories;
};

project_t build_from(cdt::project& cdtproject);
void generate(const project_t& p);

std::ostream& operator<<(std::ostream& os, const project_t& p);

}

#endif /* PROJECT_H_ */

