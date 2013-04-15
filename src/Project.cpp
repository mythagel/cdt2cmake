/*
 * Project.cpp
 *
 *  Created on: 15/04/2013
 *      Author: buildbot
 */

#include "Project.h"

void Project::clean()
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

void Project::generate(FILE* stream) const
{
	fprintf(stream, "cmake_minimum_required (VERSION 2.8)\n");
	fprintf(stream, "project (%s)\n", name.c_str());
	fprintf(stream, "\n");

	if(!artifact.other_flags.empty())
		fprintf(stream, "set (%s_CFLAGS \"%s\")\n\n", name.c_str(), artifact.other_flags.c_str());

	if(!artifact.includes.empty())
	{
		fprintf(stream, "set_target_properties (%s INCLUDE_DIRECTORIES\n", artifact.name.c_str());
		for(std::set<std::string>::const_iterator it = artifact.includes.begin(); it != artifact.includes.end(); ++it)
			fprintf(stream, "    %s\n", it->c_str());
		fprintf(stream, ")\n");
		fprintf(stream, "\n");
	}

	switch(artifact.type)
	{
		case Artifact::type_Executable:
		{
			fprintf(stream, "add_executable (%s\n", artifact.name.c_str());
			for(std::set<std::string>::const_iterator it = artifact.sources.begin(); it != artifact.sources.end(); ++it)
				fprintf(stream, "    %s\n", it->c_str());
			fprintf(stream, ")\n\n");
			break;
		}
		case Artifact::type_StaticLibrary:
		{
			fprintf(stream, "add_library (%s STATIC\n", artifact.name.c_str());
			for(std::set<std::string>::const_iterator it = artifact.sources.begin(); it != artifact.sources.end(); ++it)
				fprintf(stream, "    %s\n", it->c_str());
			fprintf(stream, ")\n\n");
			break;
		}
		case Artifact::type_SharedLibrary:
		{
			fprintf(stream, "add_library (%s SHARED\n", artifact.name.c_str());
			for(std::set<std::string>::const_iterator it = artifact.sources.begin(); it != artifact.sources.end(); ++it)
				fprintf(stream, "    %s\n", it->c_str());
			fprintf(stream, ")\n\n");
			break;
		}
	}

	if(!artifact.lib_paths.empty())
	{
		fprintf(stream, "link_directories (\n");
		for(std::set<std::string>::const_iterator it = artifact.lib_paths.begin(); it != artifact.lib_paths.end(); ++it)
			fprintf(stream, "    %s\n", it->c_str());
		fprintf(stream, ")\n");
		fprintf(stream, "\n");
	}

	if(!artifact.libs.empty())
	{
		fprintf(stream, "target_link_libraries (%s\n", artifact.name.c_str());
		for(std::vector<std::string>::const_iterator it = artifact.libs.begin(); it != artifact.libs.end(); ++it)
			fprintf(stream, "    %s\n", it->c_str());
		fprintf(stream, ")\n");
		fprintf(stream, "\n");
	}

	fprintf(stream, "\n");
}

