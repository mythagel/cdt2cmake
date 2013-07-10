/*
 * project.h
 *
 *  Created on: 15/04/2013
 *      Author: buildbot
 */

#ifndef PROJECT_H_
#define PROJECT_H_
#include <string>
#include <set>
#include <vector>
#include <ostream>

namespace cdt
{
struct project;
}

namespace cmake
{

void generate(cdt::project& cdtproject, bool write_files);

}

#endif /* PROJECT_H_ */

