/*
 * cdtproject.cpp
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#include "cdtproject.h"
#include <stdexcept>

cdt_project::configuration_t::Type resolve_artifact_type(const std::string& artifact_type)
{
	if(artifact_type == "org.eclipse.cdt.build.core.buildArtefactType.exe")
		return cdt_project::configuration_t::Type::Executable;
	else if(artifact_type == "org.eclipse.cdt.build.core.buildArtefactType.staticLib")
		return cdt_project::configuration_t::Type::StaticLibrary;
	else if(artifact_type == "org.eclipse.cdt.build.core.buildArtefactType.sharedLib")
		return cdt_project::configuration_t::Type::SharedLibrary;
	else
		throw std::runtime_error("Unknown artifact type: " + artifact_type);
}

template <typename ex = std::runtime_error>
void throw_if(bool cond, const std::string& what)
{
	if(cond)
		throw ex(what);
}

cdt_project::cdt_project(const std::string& project_base)
{
	const std::string project_file = project_base + ".project";
	const std::string cproject_file = project_base + ".cproject";

	throw_if(!project.LoadFile(project_file), "Unable to parse file " + project_file);
	throw_if(!cproject.LoadFile(cproject_file), "Unable to parse file " + cproject_file);

	auto project_root = project.RootElement();
	throw_if(project_root->ValueStr() != "projectDescription", "Unrecognised root node in" + project_file);

	auto cproject_root = cproject.RootElement();
	throw_if(cproject_root->ValueStr() != "cproject", "Unrecognised root node in" + cproject_file);
}

std::string cdt_project::name()
{
	TiXmlHandle doc(&project);

	auto name = doc.FirstChildElement("projectDescription").FirstChildElement("name").ToElement();
	throw_if(!name, "Missing /projectDescription/name");

	auto project_name = name->GetText();
	throw_if(!project_name, "Missing /projectDescription/name/CDATA");

	return project_name;
}

std::string cdt_project::comment()
{
	TiXmlHandle doc(&project);

	auto comment = doc.FirstChildElement("projectDescription").FirstChildElement("comment").ToElement();
	if(!comment)
		return {};

	auto cmt = comment->GetText();
	if(!cmt)
		return {};

	return cmt;
}

std::vector<std::string> cdt_project::referenced_projects()
{
	TiXmlHandle doc(&project);

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

std::vector<std::string> cdt_project::natures()
{
	TiXmlHandle doc(&project);

	auto natures = doc.FirstChildElement("projectDescription").FirstChildElement("natures").ToElement();
	if(!natures)
		return {};

	std::vector<std::string> nature_list;
	for(auto nature = natures->FirstChildElement("nature"); nature; nature = nature->NextSiblingElement("nature"))
	{
		auto name = nature->GetText();
		if(!name)
			continue;
		nature_list.emplace_back(name);
	}
	return nature_list;
}

TiXmlElement* cdt_project::settings()
{
	auto root = cproject.RootElement();
	for(auto storageModule = root->FirstChildElement("storageModule"); storageModule; storageModule = storageModule->NextSiblingElement("storageModule"))
	{
		auto moduleId  = storageModule->Attribute("moduleId");
		if(moduleId && std::string(moduleId) == "org.eclipse.cdt.core.settings")
			return storageModule;
	}

	return nullptr;
}

std::vector<std::string> cdt_project::cconfigurations()
{
	auto cdt_settings = settings();
	if(!cdt_settings)
		return {};

	std::vector<std::string> configs;
	for(auto cconfiguration = cdt_settings->FirstChildElement("cconfiguration"); cconfiguration; cconfiguration = cconfiguration->NextSiblingElement("cconfiguration"))
	{
		auto id = cconfiguration->Attribute("id");
		if(!id)
			continue;

		configs.emplace_back(id);
	}

	return configs;
}

TiXmlElement* cdt_project::cconfiguration(const std::string& id)
{
	auto cdt_settings = settings();
	if(!cdt_settings)
		return nullptr;

	for(auto cconfiguration = cdt_settings->FirstChildElement("cconfiguration"); cconfiguration; cconfiguration = cconfiguration->NextSiblingElement("cconfiguration"))
	{
		auto cid = cconfiguration->Attribute("id");
		if(!cid)
			continue;

		if(id == cid)
			return cconfiguration;
	}

	return nullptr;
}

cdt_project::configuration_t cdt_project::configuration(const std::string& cconfiguration_id)
{
	configuration_t conf;
	auto configuration = cdtBuildSystem_configuration(cconfiguration_id);
	throw_if(!configuration, "Unable to read configuration");

	configuration->QueryStringAttribute("name", &conf.name);

	configuration->QueryStringAttribute("artifactName", &conf.artifact);
	if(conf.artifact == "${ProjName}")
		conf.artifact = name();

	configuration->QueryStringAttribute("preBuild", &conf.prebuild);
	configuration->QueryStringAttribute("postBuild", &conf.postbuild);

	std::string buildArtefactType;
	configuration->QueryStringAttribute("buildArtefactType", &buildArtefactType);
	conf.type = resolve_artifact_type(buildArtefactType);

	for(auto build_instr = configuration->FirstChildElement(); build_instr; build_instr = build_instr->NextSiblingElement())
	{
		if(build_instr->ValueStr() == "folderInfo")
		{
			configuration_t::build_folder bf;
			build_instr->QueryStringAttribute("resourcePath", &bf.path);

			auto toolChain = build_instr->FirstChildElement("toolChain");
			throw_if(!toolChain, "Unable to find toolChain node");
		}
		else if(build_instr->ValueStr() == "fileInfo")
		{
			// TODO
		}
		else
		{
			throw std::runtime_error("Unknown build node: " + build_instr->ValueStr());
		}
	}

	return conf;
}

TiXmlElement* cdt_project::cdtBuildSystem_configuration(const std::string& cconfiguration_id)
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
