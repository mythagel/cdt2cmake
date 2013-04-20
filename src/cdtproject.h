/*
 * cdtproject.h
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#ifndef CDTPROJECT_H_
#define CDTPROJECT_H_
#include <string>
#include <tinyxml.h>
#include <vector>
#include "cdtconfiguration.h"

namespace cdt
{

/*
 * TODO Implement interface that reads different cdt / eclipse
 * project file formats and extracts enough information to
 * create a cmake project from it.
 * Expand the Project structure to represent a cmake project
 * more completely.
 */

class project
{
private:
	std::string project_path;

	TiXmlDocument project_doc;
	TiXmlDocument cproject_doc;
public:
	project(const std::string& project_base);

	std::string path() const;

	// .project properties
	std::string name();
	std::string comment();
	std::vector<std::string> referenced_projects();
	std::vector<std::string> natures();

	// .cproject properties
	TiXmlElement* settings();
	std::vector<std::string> cconfigurations();
	TiXmlElement* cconfiguration(const std::string& id);

	configuration_t configuration(const std::string& cconfiguration_id);

	TiXmlElement* cdtBuildSystem_configuration(const std::string& cconfiguration_id);
};

}

#endif /* CDTPROJECT_H_ */
