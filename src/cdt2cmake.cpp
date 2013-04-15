/*
 * cdt2cmake.cpp
 *
 *  Created on: 10/04/2013
 *      Author: nicholas
 *     License: New BSD License
 */
#include "Project.h"
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

void find_sources(const std::string& base_path, const std::string& path, std::vector<std::string>& sources)
{
	std::string abs_path = base_path;

	if(!path.empty())
		abs_path += "/" + path;

	DIR* d = opendir(abs_path.c_str());
	if (d)
	{
		struct dirent *dir;
		while ((dir = readdir(d)) != NULL)
		{
			std::string name = dir->d_name;
			if(dir->d_type == DT_DIR)
			{
				if(name == "." || name == "..")
					continue;
				std::string rel_path;
				if(!path.empty())
					rel_path = path + "/" + name;
				else
					rel_path = name;
				find_sources(base_path, rel_path, sources);
			}
			else if(dir->d_type == DT_REG)
			{
				std::string::size_type pos = name.rfind('.');
				if(pos != std::string::npos)
				{
					std::string ext = name.substr(pos);
					if
					(
							ext == ".cpp" ||
							ext == ".cc" ||
							ext == ".cxx" ||
							ext == ".c" ||
							ext == ".C"
					)
					{
						std::string rel_source;
						if(!path.empty())
							rel_source = path + "/" + name;
						else
							rel_source = name;

						sources.push_back(rel_source);
					}
				}
			}
		}
		closedir(d);
	}
}

int main(int argc, char* argv[])
{
	die_if(argc < 2, "%s: .cproject file\n", argv[0]);

	const std::string filename = argv[1];
	std::string project_base;
	{
		char buf[2048];
		strncpy(buf, argv[1], sizeof(buf));
		project_base = dirname(buf);
	}

	TiXmlDocument doc;
	die_if(!doc.LoadFile(filename), "%s: Unable to parse '%s'\n", argv[0], filename.c_str());

	const TiXmlElement* root = doc.RootElement();
	die_if(root->ValueStr() != "cproject", "%s: Unrecognised root node '%s' in '%s'\n", argv[0], root->Value(), filename.c_str());


	const TiXmlElement* settings(NULL);
	{
		for(const TiXmlElement* storageModule = root->FirstChildElement("storageModule"); storageModule; storageModule = storageModule->NextSiblingElement("storageModule"))
		{
			const char* moduleId  = storageModule->Attribute("moduleId");
			if(moduleId && std::string(moduleId) == "org.eclipse.cdt.core.settings")
			{
				settings = storageModule;
				break;
			}
		}

		die_if(!settings, "%s: Unable to find settings storageModule in '%s'\n", argv[0], filename.c_str());
	}

	const TiXmlElement* project_stormod(NULL);
	{
		const TiXmlElement* cdtBuildSystem(NULL);
		for(const TiXmlElement* storageModule = root->FirstChildElement("storageModule"); storageModule; storageModule = storageModule->NextSiblingElement("storageModule"))
		{
			const char* moduleId  = storageModule->Attribute("moduleId");
			if(moduleId && std::string(moduleId) == "cdtBuildSystem")
			{
				cdtBuildSystem = storageModule;
				break;
			}
		}

		die_if(!cdtBuildSystem, "%s: Unable to find cdtBuildSystem storageModule in '%s'\n", argv[0], filename.c_str());

		project_stormod = cdtBuildSystem->FirstChildElement("project");

		die_if(!project_stormod, "%s: Unable to find cdtBuildSystem/project in '%s'\n", argv[0], filename.c_str());
	}

	die_if(!project_stormod->Attribute("id"), "%s: Unable to find cdtBuildSystem/project['id'] in '%s'\n", argv[0], filename.c_str());

	Project master_project;
	{
		master_project.name = project_stormod->Attribute("id");
		std::string::size_type pos = master_project.name.find(".cdt.");
		if(pos != std::string::npos)
		{
			master_project.name = master_project.name.substr(0, pos);
		}
	}

	for(const TiXmlElement* cconfiguration = settings->FirstChildElement("cconfiguration"); cconfiguration; cconfiguration = cconfiguration->NextSiblingElement("cconfiguration"))
	{
		Project project = master_project;
		Project::Artifact& artifact = project.artifact;
		if(!cconfiguration->Attribute("id"))
		{
			fprintf(stderr, "%s: cconfiguration without 'id' in '%s'; skipping\n", argv[0], filename.c_str());
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
			fprintf(stderr, "%s: Unable to find cdtBuildSystem storageModule in '%s'; skipping\n", argv[0], filename.c_str());
			continue;
		}

		const TiXmlElement* configuration = cdtBuildSystem->FirstChildElement("configuration");
		if(!configuration)
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration in '%s'; skipping\n", argv[0], filename.c_str());
			continue;
		}

		if(!configuration->Attribute("name"))
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration['name'] in '%s'; skipping\n", argv[0], filename.c_str());
			continue;
		}
		if(!configuration->Attribute("artifactName"))
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration['artifactName'] in '%s'; skipping\n", argv[0], filename.c_str());
			continue;
		}
		if(!configuration->Attribute("buildArtefactType"))
		{
			fprintf(stderr, "%s: Unable to find cdtBuildSystem/configuration['buildArtefactType'] in '%s'; skipping\n", argv[0], filename.c_str());
			continue;
		}

		std::string configuration_name = configuration->Attribute("name");

		// configuration/folderInfo/toolChain

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
			fprintf(stderr, "%s: Unknown artifact type '%s' in '%s'\n", argv[0], buildArtefactType.c_str(), filename.c_str());
			return 1;
		}

		const TiXmlElement* folderInfo = configuration->FirstChildElement("folderInfo");
		die_if(!folderInfo, "%s: Unable to find cdtBuildSystem/configuration/folderInfo in '%s'\n", argv[0], filename.c_str());

		const TiXmlElement* toolChain = folderInfo->FirstChildElement("toolChain");
		die_if(!toolChain, "%s: Unable to find cdtBuildSystem/configuration/folderInfo/toolChain in '%s'\n", argv[0], filename.c_str());

		for(const TiXmlElement* tool = toolChain->FirstChildElement("tool"); tool; tool = tool->NextSiblingElement("tool"))
		{
			for(const TiXmlElement* option = tool->FirstChildElement("option"); option; option = option->NextSiblingElement("option"))
			{
				if(!option->Attribute("superClass"))
				{
					fprintf(stderr, "%s: tool/option without 'superClass' in '%s'; skipping\n", argv[0], filename.c_str());
					continue;
				}
				const std::string superClass = option->Attribute("superClass");

				if(superClass == "gnu.c.compiler.option.include.paths" || superClass == "gnu.cpp.compiler.option.include.paths")
				{
					for(const TiXmlElement* listOptionValue = option->FirstChildElement("listOptionValue"); listOptionValue; listOptionValue = listOptionValue->NextSiblingElement("listOptionValue"))
					{
						if(!listOptionValue->Attribute("value"))
						{
							fprintf(stderr, "%s: tool/option/listOptionValue without 'value' in '%s'; skipping\n", argv[0], filename.c_str());
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
							fprintf(stderr, "%s: tool/option/listOptionValue without 'value' in '%s'; skipping\n", argv[0], filename.c_str());
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
							fprintf(stderr, "%s: tool/option/listOptionValue without 'value' in '%s'; skipping\n", argv[0], filename.c_str());
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
						fprintf(stderr, "%s: tool/option without 'value' in '%s'; skipping\n", argv[0], filename.c_str());
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
		find_sources(project_base, "", sources);
		project.artifact.sources = std::set<std::string>(sources.begin(), sources.end());

		mkdir(project.name.c_str(), 0700);
		std::string s = project.name + "/" + configuration_name;
		mkdir(s.c_str(), 0700);

		char buf[2048];
		snprintf(buf, sizeof(buf), "%s/%s/CMakeLists.txt", project.name.c_str(), configuration_name.c_str());

		FILE* f = fopen(buf, "w");
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

