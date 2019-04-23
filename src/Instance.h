/*
 * $Id: Instance.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Instance.h -- Glue layer interface
 *
 * Fritz Budiyanto, October 2005
 *
 * Copyright (c) 2005, Stanford University.
 * All rights reserved.
 */

#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <string>

#include "PtrInterface.h"
#include "Ptr.h"

using namespace std;

class Instance : public PtrInterface<Instance>
{
public:
    Instance(const string &name) : name_(name) {}

    virtual string attribute( const string &attributeName ) const = 0;
    // return value of attribute with given name

    virtual void attributeIs( const string &attributeName, 
                              const string &newValueString ) = 0;
    // write attribute to value specified by string

    virtual string name() const { return name_; }

    class Manager;

private:
    string name_;
};

class Instance::Manager : public PtrInterface<Instance::Manager>
{
public:
    virtual Ptr<Instance> instanceNew( const string &name, 
                                       const string &spec ) = 0;
    // create new instance by specified name

    virtual Ptr<Instance> instance( const string &name ) const = 0;
    // get instance by this name, else null.

    virtual void instanceDel( const string &name ) = 0;
    // delete the instance with the given name
};

#include "Ptr.in"

#endif /* INSTANCE_H */

/* end-of-file */
