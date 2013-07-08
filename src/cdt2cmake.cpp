/*
 * cdt2cmake.cpp
 *
 *  Created on: 10/04/2013
 *      Author: nicholas
 *     License: New BSD License
 */
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

#include "cdtproject.h"
#include "project.h"

void usage(const std::string& program_name);

int main(int argc, char* argv[])
{
	std::vector<std::string> args{argv, argv+argc};
	assert(!args.empty());

	std::string program_name = args[0];
	args.erase(begin(args));

	bool generate(false);
	std::vector<std::string> projects;

	for(auto it = begin(args); it != end(args); ++it)
	{
		auto arg = *it;

		if(arg.empty())
			continue;

		if(arg == "-")
		{
			projects.insert(projects.end(), ++it, end(args));
			break;
		}

		if(arg.find("--") == 0)
		{
			if(arg == "--generate")
			{
				generate = true;
			}
			else if(arg == "--help")
			{
				usage(program_name);
				return 1;
			}
			else
			{
				std::cout << "Unrecognised option " << arg << "\n";
				usage(program_name);
				return 1;
			}
		}
		else
		{
			projects.push_back(arg);
		}
	}

	projects.erase(std::remove_if(begin(projects), end(projects), [](const std::string& project){ return project.empty(); }), projects.end());

	if(projects.empty())
	{
		usage(program_name);
		return 1;
	}

	std::for_each(begin(projects), end(projects), [](std::string& project)
	{
		if(project.back() != '/')
			project += '/';
	});

	for(auto project_base : projects)
	{
		try
		{
			cdt::project cdtproject(project_base);

			auto project = cmake::build_from(cdtproject);
			
			if(generate)
				cmake::generate(project);
			else
				std::cout << project;
		}
		catch(const std::exception& ex)
		{
			std::cout << "Error: " << ex.what() << "\n";
		}
	}
	return 0;
}

void usage(const std::string& program_name)
{
	std::cout << "Usage: " << program_name << " [OPTIONS]... /path/to/eclipse-cdt/project/...\n";
	std::cout << "Converts CDT project file descriptions to CMakeLists.txt files.\n";
	std::cout << "By default no changes are made to the project source path.\n\n";

	std::cout << "  --generate              Generate the CMakeLists.txt files\n";
	std::cout << "                          in their appropriate source locations.\n";
	std::cout << "  --help                  display this help and exit\n";
}
