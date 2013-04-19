/*
 * cdtproject.cpp
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#include "cdtproject.h"
#include <stdexcept>
#include <sstream>
#include <iterator>
#include "tixml_iterator.h"

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
	for(auto nature : elements_named(natures, "nature"))
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
	for(auto storageModule : elements_named(root, "storageModule"))
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
	for(auto cconfiguration : elements_named(cdt_settings, "cconfiguration"))
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

std::string cdt_project::configuration_t::build_folder::compiler_t::str() const
{
	std::stringstream s;
	s << "{\n";
	s << "   includes: ";
	std::copy(includes.begin(), includes.end(), std::ostream_iterator<std::string>(s, ", "));
	s << "\n";
	s << "   options: " << options << "\n";
	s << "}\n";
	return s.str();
}
std::string cdt_project::configuration_t::build_folder::linker_t::str() const
{
	std::stringstream s;
	s << "{\n";

	s << "   flags: " << flags << "\n";

	s << "   libs: ";
	std::copy(libs.begin(), libs.end(), std::ostream_iterator<std::string>(s, ", "));
	s << "\n";

	s << "   lib_paths: ";
	std::copy(lib_paths.begin(), lib_paths.end(), std::ostream_iterator<std::string>(s, ", "));
	s << "\n";

	s << "}\n";
	return s.str();
}

std::string cdt_project::configuration_t::str() const
{
	std::stringstream s;

	s << "name: '" << name << "'\n";
	s << "artifact: '" << artifact << "'\n";

	s << "prebuild: '" << prebuild << "'\n";
	s << "postbuild: '" << postbuild << "'\n";

	switch(type)
	{
		case Type::Executable:
			s << "type: '" << "Executable" << "'\n";
			break;
		case Type::StaticLibrary:
			s << "type: '" << "Static Library" << "'\n";
			break;
		case Type::SharedLibrary:
			s << "type: '" << "Shared Library" << "'\n";
			break;
	}

	for(auto& bf : build_folders)
	{
		s << "folder: '" << bf.path << "' {\n";
		s << "   cpp.compiler: " << bf.cpp.compiler.str() << "\n";
		s << "   c.compiler: " << bf.c.compiler.str() << "\n";
		s << "   cpp.linker: " << bf.cpp.linker.str() << "\n";
		s << "   c.linker: " << bf.c.linker.str() << "\n";
		s << "}\n";
	}

	for(auto& bf : build_files)
	{
		s << "file: '" << bf.file << "' {\n";
		s << "}\n";
	}

	return s.str();
}

cdt_project::configuration_t cdt_project::configuration(const std::string& cconfiguration_id)
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
