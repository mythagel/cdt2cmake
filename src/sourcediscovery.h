/*
 * sourcediscovery.h
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#ifndef SOURCEDISCOVERY_H_
#define SOURCEDISCOVERY_H_
#include <string>
#include <vector>
#include <functional>

bool is_source_filename(const std::string& filename);
bool is_c_source_filename(const std::string& filename);
bool is_cxx_source_filename(const std::string& filename);

struct source_file
{
	std::string name;
	std::string path;
};

std::vector<source_file> find_sources(const std::string& base_path, const std::function<bool(std::string)>& predicate = is_source_filename);

#endif /* SOURCEDISCOVERY_H_ */
