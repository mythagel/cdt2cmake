/*
 * tixml_iterator.cpp
 *
 *  Created on: 17/04/2013
 *      Author: nicholas
 */
#include "tixml_iterator.h"

TiXmlElement* element_iterator_adapter_t::iterator::operator*()
{
	return node;
}
element_iterator_adapter_t::iterator& element_iterator_adapter_t::iterator::operator++()
{
	node = node->NextSiblingElement(a.element_name);
	return *this;
}
bool element_iterator_adapter_t::iterator::operator!=(const iterator& o) const
{
	return node != o.node;
}

element_iterator_adapter_t::iterator begin(element_iterator_adapter_t& a)
{
	return {a, a.parent->FirstChildElement(a.element_name)};
}
element_iterator_adapter_t::iterator end(element_iterator_adapter_t& a)
{
	return {a, nullptr};
}

element_iterator_adapter_t elements_named(TiXmlElement* parent, const char* element_name)
{
	return {parent, element_name};
}

