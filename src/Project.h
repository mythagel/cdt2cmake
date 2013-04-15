/*
 * Project.h
 *
 *  Created on: 15/04/2013
 *      Author: buildbot
 */

#ifndef PROJECT_H_
#define PROJECT_H_
#include <string>
#include <set>
#include <vector>
#include <cstdio>

struct Project
{
	std::string name;

	struct Artifact
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
	} artifact;

	void clean();
	void generate(FILE* stream) const;
};

#endif /* PROJECT_H_ */

