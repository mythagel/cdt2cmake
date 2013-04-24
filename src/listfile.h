/*
 * listfile.h
 *
 *  Created on: 24/04/2013
 *      Author: nicholas
 */

#ifndef LISTFILE_H_
#define LISTFILE_H_
#include <string>
#include <iosfwd>

namespace cmake
{

/* TODO simple cmake list file structure for building valid files

std::ostringstream s;
s << comment("This is a cmake comment");
s << comment("it supports\nmultiple\nlines");
s << "\n";
s << command("cmake_minimum_required", arg("VERSION"), arg(2.8));
s << command("project", var("name"));

#This is a cmake comment
#it supports
#multiple
#lines

cmake_minimum_required (VERSION 2.8)
project(${name})
 */

struct comment_t
{
	std::string cmt;
};

struct variable_t
{
	std::string var;
};

struct argument_t
{
	bool quoted;
	std::string arg;
};

struct command_t
{
	std::string name;
	// TODO
};

std::ostream& operator<<(std::ostream& os, const comment_t& cmt);

}


#endif /* LISTFILE_H_ */
