/*
 * sourcediscovery.cpp
 *
 *  Created on: 16/04/2013
 *      Author: nicholas
 */

#include "sourcediscovery.h"
#include <algorithm>
#include <dirent.h>

bool is_source_filename(const std::string& filename)
{
	static const auto c_types = {".c", ".C", "c++", ".cc", ".cpp", ".cxx"};

	std::string::size_type pos = filename.rfind('.');
	if(pos == std::string::npos)
		return false;

	auto file_type = filename.substr(pos);
	return std::find(begin(c_types), end(c_types), file_type) != end(c_types);
}

void find_sources(const std::string& base_path, const std::string& path, const std::function<bool(std::string)>& predicate, std::vector<source_file>& sources)
{
	auto abs_path = base_path;

	if(!path.empty())
		abs_path += "/" + path;

	auto d = opendir(abs_path.c_str());
	if(d)
	{
		while (auto dir = readdir(d))
		{
			std::string name = dir->d_name;
			if(dir->d_type == DT_DIR)
			{
				if(name == "." || name == "..")
					continue;

				auto rel_path = name;
				if(!path.empty())
					rel_path = path + "/" + name;

				find_sources(base_path, rel_path, predicate, sources);
			}
			else if(dir->d_type == DT_REG)
			{
				if(predicate(name))
				{
					auto rel_source = name;
					if(!path.empty())
						rel_source = path + "/" + name;

					sources.push_back({name, path});
				}
			}
		}
		closedir(d);
	}
}

std::vector<source_file> find_sources(const std::string& base_path, const std::function<bool(std::string)>& predicate)
{
	std::vector<source_file> sources;
	find_sources(base_path, {}, predicate, sources);

	return std::move(sources);
}
