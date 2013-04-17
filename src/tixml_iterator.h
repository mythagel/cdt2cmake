/*
 * tixml_iterator.h
 *
 *  Created on: 17/04/2013
 *      Author: nicholas
 */

#ifndef TIXML_ITERATOR_H_
#define TIXML_ITERATOR_H_
#include <tinyxml.h>

struct element_iterator_adapter_t
{
	TiXmlElement* parent;
	const char* element_name;

	struct iterator
	{
		element_iterator_adapter_t& a;
		TiXmlElement* node;

		TiXmlElement* operator*();
		iterator& operator++();
		bool operator!=(const iterator& o) const;
	};
};

element_iterator_adapter_t::iterator begin(element_iterator_adapter_t& a);
element_iterator_adapter_t::iterator end(element_iterator_adapter_t& a);

element_iterator_adapter_t elements_named(TiXmlElement* parent, const char* element_name);

#endif /* TIXML_ITERATOR_H_ */
