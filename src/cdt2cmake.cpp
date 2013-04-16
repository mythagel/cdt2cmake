/*
 * cdt2cmake.cpp
 *
 *  Created on: 10/04/2013
 *      Author: nicholas
 *     License: New BSD License
 */
#include "Project.h"
#include "cdtproject.h"
#include <tinyxml.h>
#include <cstdio>
#include <string>
#include <cstdarg>
#include <set>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#include "sourcediscovery.h"

void die_if(bool cond, const char* format, ...);

int main(int argc, char* argv[])
{
	die_if(argc < 2, "%s: /path/to/eclipse/project/\n", argv[0]);

	std::string project_base = argv[1];

	die_if(project_base.empty(), "%s: /path/to/eclipse/project/\n", argv[0]);

	if(project_base.back() != '/')
		project_base += '/';

	cdt_project cdtproject(project_base);

	auto settings = cdtproject.settings();
	die_if(!settings, "%s: Unable to find settings storageModule\n", argv[0]);

	Project master_project;
	master_project.name = cdtproject.name();

	for(const TiXmlElement* cconfiguration = settings->FirstChildElement("cconfiguration"); cconfiguration; cconfiguration = cconfiguration->NextSiblingElement("cconfiguration"))
	{
		Project project = master_project;
		Project::Artifact& artifact = project.artifact;
		if(!cconfiguration->Attribute("id"))
		{
			fprintf(stderr, "%s: cconfiguration without 'id'; skipping\n", argv[0]);
			continue;
		}
		const std::string id = cconfiguration->Attribute("id");

		const TiXmlElement* cdtBuildSystem(NULL);
		for(const TiXmlElement* storageModule = cconfiguration->FirstChildElement("storageModule"); storageModule; storageModule = storageModule->NextSiblingElement("storageModule"))
		{
			const char* moduleId  = storageModule->Attribute("moduleId");
			if(moduleId && std::string(moduleId) == "cdtBuildSystem")
			{
				cdtBuildSystem = storageModule;
				break;
			}
		}

		if(!cdtBuildSystem)
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem storageModule; skipping\n", argv[0]);
			continue;
		}

		const TiXmlElement* configuration = cdtBuildSystem->FirstChildElement("configuration");
		if(!configuration)
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration; skipping\n", argv[0]);
			continue;
		}

		if(!configuration->Attribute("name"))
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration['name']; skipping\n", argv[0]);
			continue;
		}
		if(!configuration->Attribute("artifactName"))
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration['artifactName']; skipping\n", argv[0]);
			continue;
		}
		if(!configuration->Attribute("buildArtefactType"))
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration['buildArtefactType']; skipping\n", argv[0]);
			continue;
		}

		std::string configuration_name = configuration->Attribute("name");

		{
			artifact.name = configuration->Attribute("artifactName");
			if(artifact.name == "${ProjName}")
				artifact.name = project.name;
		}

		std::string buildArtefactType = configuration->Attribute("buildArtefactType");

		if(buildArtefactType == "org.eclipse.cdt.build.core.buildArtefactType.exe")
		{
			artifact.type = Project::Artifact::type_Executable;
		}
		else if(buildArtefactType == "org.eclipse.cdt.build.core.buildArtefactType.staticLib")
		{
			artifact.type = Project::Artifact::type_StaticLibrary;
		}
		else if(buildArtefactType == "org.eclipse.cdt.build.core.buildArtefactType.sharedLib")
		{
			artifact.type = Project::Artifact::type_SharedLibrary;
		}
		else
		{
			fprintf(stderr, "%s: Unknown artifact type '%s'\n", argv[0], buildArtefactType.c_str());
			return 1;
		}

		const TiXmlElement* folderInfo = configuration->FirstChildElement("folderInfo");
		die_if(!folderInfo, "%s: Unable to find cdtBuildSystem/configuration/folderInfo\n", argv[0]);

		const TiXmlElement* toolChain = folderInfo->FirstChildElement("toolChain");
		die_if(!toolChain, "%s: Unable to find cdtBuildSystem/configuration/folderInfo/toolChain\n", argv[0]);

		for(const TiXmlElement* tool = toolChain->FirstChildElement("tool"); tool; tool = tool->NextSiblingElement("tool"))
		{
			for(const TiXmlElement* option = tool->FirstChildElement("option"); option; option = option->NextSiblingElement("option"))
			{
				if(!option->Attribute("superClass"))
				{
					fprintf(stderr, "%s: tool/option without 'superClass'; skipping\n", argv[0]);
					continue;
				}
				const std::string superClass = option->Attribute("superClass");

				if(superClass == "gnu.c.compiler.option.include.paths" || superClass == "gnu.cpp.compiler.option.include.paths")
				{
					for(const TiXmlElement* listOptionValue = option->FirstChildElement("listOptionValue"); listOptionValue; listOptionValue = listOptionValue->NextSiblingElement("listOptionValue"))
					{
						if(!listOptionValue->Attribute("value"))
						{
							fprintf(stderr, "%s: tool/option/listOptionValue without 'value'; skipping\n", argv[0]);
							continue;
						}
						const std::string value = listOptionValue->Attribute("value");
						artifact.includes.insert(value);
					}
				}
				else if(superClass == "gnu.cpp.link.option.libs" || superClass == "gnu.c.link.option.libs")
				{
					for(const TiXmlElement* listOptionValue = option->FirstChildElement("listOptionValue"); listOptionValue; listOptionValue = listOptionValue->NextSiblingElement("listOptionValue"))
					{
						if(!listOptionValue->Attribute("value"))
						{
							fprintf(stderr, "%s: tool/option/listOptionValue without 'value'; skipping\n", argv[0]);
							continue;
						}
						const std::string value = listOptionValue->Attribute("value");
						artifact.libs.push_back(value);
					}
				}
				else if(superClass == "gnu.cpp.link.option.paths" || superClass == "gnu.c.link.option.paths")
				{
					for(const TiXmlElement* listOptionValue = option->FirstChildElement("listOptionValue"); listOptionValue; listOptionValue = listOptionValue->NextSiblingElement("listOptionValue"))
					{
						if(!listOptionValue->Attribute("value"))
						{
							fprintf(stderr, "%s: tool/option/listOptionValue without 'value'; skipping\n", argv[0]);
							continue;
						}
						const std::string value = listOptionValue->Attribute("value");
						artifact.lib_paths.insert(value);
					}
				}
				else if(superClass == "gnu.cpp.compiler.option.other.other")
				{
					if(!option->Attribute("value"))
					{
						fprintf(stderr, "%s: tool/option without 'value'; skipping\n", argv[0]);
						continue;
					}
					const std::string value = option->Attribute("value");

					if(artifact.other_flags.find(value) == std::string::npos)
						artifact.other_flags += value;
				}
				else
				{
//					option->Print(stdout, 3);
				}
			}
		}

		std::vector<std::string> sources;
//		find_sources(project_base, "", sources);
		project.artifact.sources = std::set<std::string>(sources.begin(), sources.end());

		{
			auto sources = find_sources(project_base);
			for(auto x : sources)
			{
				std::cout << x.name << " : " << x.path << "\n";
			}
		}

//		mkdir(project.name.c_str(), 0700);
//		std::string s = project.name + "/" + configuration_name;
//		mkdir(s.c_str(), 0700);
//
		char buf[2048];
		snprintf(buf, sizeof(buf), "%s/%s/CMakeLists.txt", project.name.c_str(), configuration_name.c_str());

//		FILE* f = fopen(buf, "w");
		FILE* f = stdout;
		if(f)
		{
			project.clean();
			project.generate(f);
			fclose(f);
		}
		else
		{
			fprintf(stderr, "Unable to open %s\n", buf);
		}
	}
	return 0;
}

void die_if(bool cond, const char* format, ...)
{
	if(cond)
	{
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		exit(1);
	}
}
