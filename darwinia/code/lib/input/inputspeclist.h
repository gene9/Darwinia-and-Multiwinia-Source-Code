#ifndef INCLUDED_INPUTSPECLIST_H
#define INCLUDED_INPUTSPECLIST_H

#include <memory>

#include "lib/input/inputspec.h"
#include "lib/auto_vector.h"


typedef auto_vector<const InputSpec> InputSpecList;

typedef InputSpecList::const_iterator InputSpecIt;

typedef std::auto_ptr<const InputSpec> InputSpecPtr;

typedef std::auto_ptr<const InputSpecList> InputSpecListPtr;


#endif // INCLUDED_INPUTSPECLIST_H
