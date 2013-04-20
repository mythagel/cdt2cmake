/*
 * cdtproject_doc.cpp
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#include "cdtproject.h"
#include <stdexcept>
#include <sstream>
#include <iterator>
#include "tixml_iterator.h"

template <typename ex = std::runtime_error>
void throw_if(bool cond, const std::string& what)
{
	if(cond)
		throw ex(what);
}

namespace cdt
{

project::project(const std::string& project_base)
 : project_path(project_base)
{
	const std::string project_file = project_path + ".project";
	const std::string cproject_file = project_path + ".cproject";

	throw_if(!project_doc.LoadFile(project_file), "Unable to parse file " + project_file);
	throw_if(!cproject_doc.LoadFile(cproject_file), "Unable to parse file " + cproject_file);

	auto project_root = project_doc.RootElement();
	throw_if(project_root->ValueStr() != "projectDescription", "Unrecognised root node in" + project_file);

	auto cproject_root = cproject_doc.RootElement();
	throw_if(cproject_root->ValueStr() != "cproject", "Unrecognised root node in" + cproject_file);
}

std::string project::path() const
{
	return project_path;
}

std::string project::name()
{
	TiXmlHandle doc(&project_doc);

	auto name = doc.FirstChildElement("projectDescription").FirstChildElement("name").ToElement();
	throw_if(!name, "Missing /projectDescription/name");

	auto project_name = name->GetText();
	throw_if(!project_name, "Missing /projectDescription/name/CDATA");

	return project_name;
}

std::string project::comment()
{
	TiXmlHandle doc(&project_doc);

	auto comment = doc.FirstChildElement("projectDescription").FirstChildElement("comment").ToElement();
	if(!comment)
		return {};

	auto cmt = comment->GetText();
	if(!cmt)
		return {};

	return cmt;
}

std::vector<std::string> project::referenced_projects()
{
	TiXmlHandle doc(&project_doc);

	auto projects = doc.FirstChildElement("projectDescription").FirstChildElement("projects").ToElement();
	if(!projects)
		return {};

	std::vector<std::string> project_list;
	for(auto project = projects->FirstChildElement("project"); project; project = project->NextSiblingElement("project"))
	{
		auto name = project->GetText();
		if(!name)
			continue;
		project_list.emplace_back(name);
	}
	return project_list;
}

std::vector<std::string> project::natures()
{
	TiXmlHandle doc(&project_doc);

	auto natures = doc.FirstChildElement("projectDescription").FirstChildElement("natures").ToElement();
	if(!natures)
		return {};

	std::vector<std::string> nature_list;
	for(auto nature : elements_named(natures, "nature"))
	{
		auto name = nature->GetText();
		if(!name)
			continue;
		nature_list.emplace_back(name);
	}
	return nature_list;
}

TiXmlElement* project::settings()
{
	auto root = cproject_doc.RootElement();
	for(auto storageModule : elements_named(root, "storageModule"))
	{
		auto moduleId  = storageModule->Attribute("moduleId");
		if(moduleId && std::string(moduleId) == "org.eclipse.cdt.core.settings")
			return storageModule;
	}

	return nullptr;
}

std::vector<std::string> project::cconfigurations()
{
	auto cdt_settings = settings();
	if(!cdt_settings)
		return {};

	std::vector<std::string> configs;
	for(auto cconfiguration : elements_named(cdt_settings, "cconfiguration"))
	{
		auto id = cconfiguration->Attribute("id");
		if(!id)
			continue;

		configs.emplace_back(id);
	}

	return configs;
}

TiXmlElement* project::cconfiguration(const std::string& id)
{
	auto cdt_settings = settings();
	if(!cdt_settings)
		return nullptr;

	for(auto cconfiguration : elements_named(cdt_settings, "cconfiguration"))
	{
		auto cid = cconfiguration->Attribute("id");
		if(!cid)
			continue;

		if(id == cid)
			return cconfiguration;
	}

	return nullptr;
}

configuration_t project::configuration(const std::string& cconfiguration_id)
{
	configuration_t conf;
	auto configuration = cdtBuildSystem_configuration(cconfiguration_id);
	throw_if(!configuration, "Unable to read configuration");

	configuration->QueryStringAttribute("name", &conf.name);
	configuration->QueryStringAttribute("artifactName", &conf.artifact);
	configuration->QueryStringAttribute("prebuildStep", &conf.prebuild);
	configuration->QueryStringAttribute("postbuildStep", &conf.postbuild);

	std::string buildArtefactType;
	configuration->QueryStringAttribute("buildArtefactType", &buildArtefactType);
	conf.type = resolve_artifact_type(buildArtefactType);

	for(auto build_instr = configuration->FirstChildElement(); build_instr; build_instr = build_instr->NextSiblingElement())
	{
		if(build_instr->ValueStr() == "folderInfo")
		{
			conf.build_folders.emplace_back();
			configuration_t::build_folder& bf = conf.build_folders.back();

			build_instr->QueryStringAttribute("resourcePath", &bf.path);

			auto toolChain = build_instr->FirstChildElement("toolChain");
			throw_if(!toolChain, "Unable to find toolChain node");

			auto extract_option_list = [](TiXmlElement* option, std::vector<std::string>& list)
			{
				for(auto listOptionValue : elements_named(option, "listOptionValue"))
				{
					if(!listOptionValue->Attribute("value"))
						continue;

					const std::string value = listOptionValue->Attribute("value");
					list.push_back(value);
				}
			};

			auto extract_compiler_options = [&extract_option_list](TiXmlElement* tool, configuration_t::build_folder::compiler_t& compiler)
			{
				for(auto option : elements_named(tool, "option"))
				{
					std::string superClass;
					option->QueryStringAttribute("superClass", &superClass);

//					fprintf(stderr, "option: %s\n", superClass.c_str());

					if(superClass.find("compiler.option.include.paths") != std::string::npos)
						extract_option_list(option, compiler.includes);
					else if(superClass.find("compiler.option.other.other") != std::string::npos)
						option->QueryStringAttribute("value", &compiler.options);
				}
			};

			auto extract_linker_options = [&extract_option_list](TiXmlElement* tool, configuration_t::build_folder::linker_t& linker)
			{
				for(auto option : elements_named(tool, "option"))
				{
					std::string superClass;
					option->QueryStringAttribute("superClass", &superClass);

//					fprintf(stderr, "option: %s\n", superClass.c_str());

					if(superClass.find("link.option.libs") != std::string::npos)
						extract_option_list(option, linker.libs);
					else if(superClass.find("link.option.paths") != std::string::npos)
						extract_option_list(option, linker.lib_paths);
					else if(superClass.find("link.option.flags") != std::string::npos)
						option->QueryStringAttribute("value", &linker.flags);
				}
			};

			for(auto tool : elements_named(toolChain, "tool"))
			{
				std::string superClass;
				tool->QueryStringAttribute("superClass", &superClass);

//				fprintf(stderr, "tool: %s\n", superClass.c_str());

				if(superClass.find("cpp.compiler") != std::string::npos)
					extract_compiler_options(tool, bf.cpp.compiler);

				else if(superClass.find("c.compiler") != std::string::npos)
					extract_compiler_options(tool, bf.c.compiler);

				else if(superClass.find("cpp.linker") != std::string::npos)
					extract_linker_options(tool, bf.cpp.linker);

				else if(superClass.find("c.linker") != std::string::npos)
					extract_linker_options(tool, bf.c.linker);
			}
		}
		else if(build_instr->ValueStr() == "fileInfo")
		{
			conf.build_files.emplace_back();
			configuration_t::build_file& bf = conf.build_files.back();

			build_instr->QueryStringAttribute("resourcePath", &bf.file);

			for(auto tool : elements_named(build_instr, "tool"))
			{
				std::string customBuildStep;
				tool->QueryStringAttribute("customBuildStep", &customBuildStep);

				if(customBuildStep == "true")
				{
					tool->QueryStringAttribute("command", &bf.command);

					if(auto inputType = tool->FirstChildElement("inputType"))
					{
						if(auto additionalInput = inputType->FirstChildElement("additionalInput"))
						{
							additionalInput->QueryStringAttribute("paths", &bf.inputs);
						}
					}
					if(auto outputType = tool->FirstChildElement("outputType"))
					{
						outputType->QueryStringAttribute("outputNames", &bf.outputs);
					}
				}
			}
		}
		else
		{
			throw std::runtime_error("Unknown build node: " + build_instr->ValueStr());
		}
	}

	return conf;
}

TiXmlElement* project::cdtBuildSystem_configuration(const std::string& cconfiguration_id)
{
	auto cdt_cconfiguration = cconfiguration(cconfiguration_id);
	if(!cdt_cconfiguration)
		return nullptr;

	for(auto storageModule = cdt_cconfiguration->FirstChildElement("storageModule"); storageModule; storageModule = storageModule->NextSiblingElement("storageModule"))
	{
		auto moduleId = storageModule->Attribute("moduleId");
		if(moduleId && std::string(moduleId) == "cdtBuildSystem")
			return storageModule->FirstChildElement("configuration");
	}

	return nullptr;
}

}
