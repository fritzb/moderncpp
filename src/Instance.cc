/*
 * $Id: Instance.cc,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Instance.cc -- implementation of Glue layer
 *
 * Fritz Budiyanto, October 2005
 *
 */

#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>

#include "Instance.h"
#include "Gore.h"
#include "Log.h"

/*
 * TODO
 * todolist:
 */

Ptr<Instance::Manager> NetworkFactory();

namespace NetworkImpl
{

Log logGlue("GLUE");

#define GLUE_ERR(format, args...) \
logGlue.entryNew(Log::Error, this->name(), __FUNCTION__, format, ##args)
#define GLUE_TRACE(format, args...) \
logGlue.entryNew(Log::Debug, this->name(), __FUNCTION__, format, ##args)

//---------------------------------------------------------------------------//
// Glue classes:

class ManagerImpl : public Instance::Manager {
public:
    // Types
    static const unsigned int MAX_NAME_LENGTH = 64;
    class InstanceCount : public Nominal<class InstanceCount__, int> {
    public:
        InstanceCount() :Nominal<class InstanceCount__, int>(0) {}
        InstanceCount(const int &i) :Nominal<class InstanceCount__, int>(i) {}
    };

    // Accessors
    Ptr<Instance>   instance(const string &name) const;
    InstanceCount   instances(string type) const;
    string          name() const { return "ManagerImpl"; }

    // Mutator
    Ptr<Instance>   instanceNew(const string &name, const string &type);
    void            instanceDel(const string &name);

    // Callback handler
    void onNetworkUpdate();

    // Constructor/Destructor
    ManagerImpl();
    ~ManagerImpl() { GLUE_TRACE("Destroyed\n"); }

private:
    /*
     * constant
     */
    static const string CONFIG_NAME;
    static const string CONN_NAME;

    map<string,Ptr<Instance> > instance_;
    map<string,InstanceCount > instanceCount_; // count number of instance
    Ptr<Instance> config_;
    Ptr<Instance> conn_;

    void instancesInc(string name);
    void instancesDec(string name);
    bool badName(const string &name); // reject invalid instance naming
};
const string ManagerImpl::CONFIG_NAME   = "config";
const string ManagerImpl::CONN_NAME     = "conn";


class NodeGlue : public Instance {
public:
    // Accessor
    string      attribute(const string &attributeName) const;
    Ptr<Node>   node() const { return node_; }

    // Mutator
    void        attributeIs(const string &attributeName, const string &newValueString);
    void        nodeIs(Ptr<Node> node) { node_  = node; }

    // Callback handler
    void        handleNetworkUpdate() { node_->handleNetworkUpdate(); }

    // Constructor/Destructor
    NodeGlue(const string &name, ManagerImpl *manager) 
        :Instance(name), manager_(manager) {}

protected:
    ManagerImpl* manager_;

private:
    Ptr<Node> node_;
};


class InterfaceGlue : public Instance {
public:
    // Accessor
    string          attribute(const string &attributeName) const;
    Ptr<Interface>  interface() const { return interface_; }

    // Mutator
    void            attributeIs(const string &attributeName, const string &newValueString);
    void            interfaceIs(Ptr<Interface> intf) { interface_ = intf; }

    // Constructor/Destructor
    InterfaceGlue(const string &name, ManagerImpl *manager) 
        :Instance(name), manager_(manager) {}

private:
    ManagerImpl* manager_;
    Ptr<Interface> interface_;
};

class ConnectionGlue : public Instance {
public:
    ConnectionGlue(const string &name, ManagerImpl *manager) 
        :Instance(name), manager_(manager) {}

    string attribute(const string &attributeName) const;
    void attributeIs(const string &attributeName, const string &newValueString);

private:

    ManagerImpl* manager_;
};

class ConfigGlue : public Instance {
public:
    ConfigGlue(const string &name, ManagerImpl *manager) :
        Instance(name), manager_(manager) {}

    string attribute(const string &attributeName) const;
    void attributeIs(const string &attributeName, const string &newValueString);

private:
    ManagerImpl* manager_;
};

                                                                                                  
class ATMSwitchGlue : public NodeGlue {
public:
    ATMSwitchGlue(const string &name, ManagerImpl *manager) 
    :NodeGlue(name, manager) { 
        Ptr<Node> node;
        node = new ATMSwitch(name); 
        if (!node) throw ResourceException();
        nodeIs(node);
    }
};

class EthernetSwitchGlue : public NodeGlue {
public:
    EthernetSwitchGlue(const string &name, ManagerImpl *manager) 
    :NodeGlue(name, manager) { 
        Ptr<Node> node;
        node = new EthernetSwitch(name); 
        if (!node) throw ResourceException();
        nodeIs(node);
    }
};

class IPRouterGlue : public NodeGlue {
public:
    IPRouterGlue(const string &name, ManagerImpl *manager) 
    :NodeGlue(name, manager) { 
        Ptr<Node> node;
        node = new IPRouter(name); 
        if (!node) throw ResourceException();
        nodeIs(node);
    }
};


class IPHostGlue : public NodeGlue {
public:
    IPHostGlue(const string &name, ManagerImpl *manager) 
    :NodeGlue(name, manager) { 
        Ptr<Node> node;
        node = new IPHost(name); 
        if (!node) throw ResourceException();
        nodeIs(node);
    }
    string attribute(const string &attributeName) const;
    void attributeIs(const string &attributeName, const string &newValueString);
};

class ATMInterfaceGlue : public InterfaceGlue {
public:
    ATMInterfaceGlue(const string &name, ManagerImpl *manager) 
    :InterfaceGlue(name, manager) { 
        Ptr<Interface> intf;
        intf = new ATMInterface(name); 
        if (!intf) throw ResourceException();
        interfaceIs(intf);
    }
};

class EthernetInterfaceGlue : public InterfaceGlue {
public:
    EthernetInterfaceGlue(const string &name, ManagerImpl *manager) 
    :InterfaceGlue(name, manager) { 
        Ptr<Interface> intf;
        intf = new EthernetInterface(name);
        if (!intf) throw ResourceException();
        interfaceIs(intf);
    }
};

//---------------------------------------------------------------------------//
// Glue methods:

/**
 * ManagerImpl:
 *
 * pre-allocate conn/config object, this is used to recover from 
 * no memory during instance accessor
 */

ManagerImpl::ManagerImpl()
{
    conn_ = new ConnectionGlue("conn", this);
    if (!conn_) throw ResourceException();

    config_ = new ConfigGlue("config", this);
    if (!config_) throw ResourceException();
}

void 
ManagerImpl::onNetworkUpdate ()
{
    /*
     * go through instances, find NodeGlue
     */
    map<string, Ptr<Instance> >::iterator i;

    for (i = instance_.begin(); i != instance_.end(); i++) {
        Ptr<Instance> instance;

        instance = (*i).second;
        Ptr<NodeGlue> node = dynamic_cast<NodeGlue *>(instance.value());
        if (node) {
            node->handleNetworkUpdate();
        }
    }
}


/**
 * instanceNew:
 *
 * avoid overwriting the old object
 * verify if name is a bad string
 */

Ptr<Instance>
ManagerImpl::instanceNew(const string &name, const string &type)
{
    map<string,Ptr<Instance> >::iterator old = instance_.find(name);

    if (badName(name)) {
        GLUE_ERR("only alphanumeric and '_' is allowed\n");
        throw ParserException();
    }

    // avoid overwriting an object
    if (old != instance_.end()) {
        GLUE_ERR("'%s' already exists\n");
        throw NameInUseException(name);
    }

    if (type == "ATM switch") {
        Ptr<ATMSwitchGlue> t = new ATMSwitchGlue(name, this);
        if (!t) throw ResourceException();
        instance_[name] = t;
        instancesInc(type);
        return t;
    }

    if (type == "Ethernet switch") {
        Ptr<EthernetSwitchGlue> t = new EthernetSwitchGlue(name, this);
        if (!t) throw ResourceException();
        instance_[name] = t;
        instancesInc(type);
        return t;
    }

    if (type == "IP router") {
        Ptr<IPRouterGlue> t = new IPRouterGlue(name, this);
        if (!t) throw ResourceException();
        instance_[name] = t;
        instancesInc(type);
        return t;
    }

    if (type == "IP host") {
        Ptr<IPHostGlue> t = new IPHostGlue(name, this);
        if (!t) throw ResourceException();
        instance_[name] = t;
        instancesInc(type);
        return t;
    }

    if (type == "ATM interface") {
        Ptr<ATMInterfaceGlue> t = new ATMInterfaceGlue(name, this);
        if (!t) throw ResourceException();
        instance_[name] = t;
        instancesInc(type);
        return t;
    }

    if (type == "Ethernet interface") {
        Ptr<EthernetInterfaceGlue> t = new EthernetInterfaceGlue(name, this);
        if (!t) throw ResourceException();
        instance_[name] = t;
        instancesInc(type);
        return t;
    }

    if (type == "config") {
        // reject config name
        if (name != CONFIG_NAME) {
            GLUE_ERR("config instance must be named '%s'\n",
                     CONFIG_NAME.c_str());
            throw ParserException();
        }

        instance_[name] = config_;
        instancesInc(type);
        return config_;
    }

    if (type == "conn") {
         // reject bad name
        if (name != CONN_NAME) {
            GLUE_ERR("connection instance must be named '%s'\n", 
                     CONN_NAME.c_str());
            throw ParserException();
        }

        instance_[name] = conn_;
        instancesInc(type);
        return conn_;
    }

    GLUE_ERR("invalid instance type '%s'\n", type.c_str());
    throw ParserException();
}

/**
 * instance:
 *
 * lookup the object name and return
 * special case for connection and config object, return the
 * pre-allocated object, if not in the instance_ table
 */

Ptr<Instance>
ManagerImpl::instance(const string &name) const
{
    map<string,Ptr<Instance> >::const_iterator t = instance_.find(name);

    if (t == instance_.end()) {
        // if instance not found, check if the instance is 'conn' or 'config'
        ManagerImpl *thisPtr = const_cast<ManagerImpl *>(this);
        Ptr<Instance> instance;

        if (name == CONFIG_NAME) {
            instance = config_;
            thisPtr->instance_[name] = instance;
        }
        if (name == CONN_NAME) {
            instance = conn_;
            thisPtr->instance_[name] = instance;
        }

        return instance;
    }
    else return (*t).second;
}

/**
 * instanceDel:
 *
 * dissosiate all reference to other object
 * set the gore object to NULL in the glue object
 * decrement instance count
 * erase it from the map hastable
 */

void
ManagerImpl::instanceDel(const string &name)
{
    map<string,Ptr<Instance> >::iterator t = instance_.find(name);

    if (t == instance_.end()) {
        GLUE_ERR("%s not found\n", name.c_str());
        throw ResourceException();
    }

    Ptr<Instance> instance = (*t).second;

    // dissosiate all reference to other object
    Ptr<InterfaceGlue> intfGlue = dynamic_cast<InterfaceGlue *>(instance.value());
    if (intfGlue) {
        Ptr<Interface> intf = intfGlue->interface();
        intf->~Interface();
        intfGlue->interfaceIs(NULL);
    }

    Ptr<NodeGlue> nodeGlue = dynamic_cast<NodeGlue *>(instance.value());
    if (nodeGlue) {
        Ptr<Node>  node = nodeGlue->node();
        node->~Node();
        nodeGlue->nodeIs(NULL);
    }

    instancesDec(name);
    instance_.erase(t);
    onNetworkUpdate();
}

/**
 * instances:
 *
 * return the number of count for the specified type
 * this is modeled as a collection attribute, where the key
 * is the queryType
 *
 * queryType is a string indicating the instance type, ie. "ATM interface"
 */

ManagerImpl::InstanceCount
ManagerImpl::instances(string queryType) const
{
    map<string,InstanceCount>::const_iterator t = instanceCount_.find(queryType);
    if (t == instanceCount_.end()) return InstanceCount(0);
    else return (*t).second;
}

/**
 * instancesInc:
 *
 * increment an instance count for the given instance type 'name'
 */

void
ManagerImpl::instancesInc(string name) {
    InstanceCount count = 0;

    map<string,InstanceCount>::iterator t = instanceCount_.find(name);

    if (t == instanceCount_.end()) {
        instanceCount_[name] = count;
    }
    count = instanceCount_[name];
    count = count.value() + 1;
    instanceCount_[name] = count;
}

/**
 * instancesDec:
 *
 * decrement an instance count for the given instance type 'name'
 */

void 
ManagerImpl::instancesDec(string name) {
    InstanceCount count = 0;

    map<string,InstanceCount>::iterator t = instanceCount_.find(name);

    if (t == instanceCount_.end()) {
        instanceCount_[name] = count;
    }
    count = instanceCount_[name];
    count = count.value() - 1;
    instanceCount_[name] = count;
}

/**
 * badName:
 *
 * return TRUE if the name is invalid
 * (longer than 64, empty name, only accept alphanumeric, and _
 */

bool 
ManagerImpl::badName(const string &name)
{
    if (name.empty() || (name == "") || (name.length() > MAX_NAME_LENGTH)) {
        return true;
    }
    for (unsigned i = 0; i < name.length(); i++) {
        if (!isalpha(name[i])) {
            if (name[i] != '_') {
                return false;
            }
        }
    }
    return false;
}

/**
 * attribute:
 *
 * check if a selected attribute is an interface
 */

string
NodeGlue::attribute(const string &attributeName) const
{
    if (attributeName.substr(0, 9) != "interface") {
        GLUE_ERR("invalid node attribute '%s'\n", attributeName.c_str());
        return "";
    }
    const char *t = attributeName.c_str() + 9;
    int i = atoi(t);

    Ptr<Node> node = this->node();
    Ptr<Interface> intf = node->interface(i);
    if (!intf) {
        GLUE_ERR("attribute %s not found\n", attributeName.c_str());
        return "";
    }
    return intf->name();
}


/**
 * attributeIs:
 *
 * set node's attribute, and does proper instance type test using RTTI
 */

void
NodeGlue::attributeIs(const string &attributeName,
                      const string &newValueString)
{
    GLUE_TRACE("%s->%s: %s\n", 
               name().c_str(), attributeName.c_str(), newValueString.c_str());

    if (attributeName.substr(0, 9) != "interface") {
        GLUE_ERR("invalid node attribute '%s'\n", attributeName.c_str());
        throw ParserException();
    }
    const char *t = attributeName.c_str() + 9;
    int i = atoi(t);

    Ptr<Node> node = this->node();
    if (newValueString == "") {
        node->interfaceIs(i, NULL);
        return;
    }

    Ptr<Instance> instance = manager_->instance(newValueString);
    if (!instance) {
        GLUE_ERR("Instance %s not found\n", newValueString.c_str());
        throw ResourceException();
    }
    Ptr<InterfaceGlue> intfGlue = dynamic_cast<InterfaceGlue *>(instance.value());
    if (!intfGlue) {
        GLUE_ERR("Instance %s is not an Interface type\n", newValueString.c_str());
        throw ResourceException();
    }

    Ptr<Interface> intf = intfGlue->interface();

    node->interfaceIs(i, intf);
}

string
InterfaceGlue::attribute(const string &attributeName) const
{
    if (attributeName == "node") {
        Ptr<Node> node = interface()->node();
        if (!node) {
            return "";
        }
        return node->name();
    }

    if (attributeName == "data rate") {
        char temp[100];
        sprintf(temp, "%u", interface()->dataRate().value());
        return temp;
    }

    if (attributeName == "other side") {
        Ptr<Interface> intf = interface()->otherSide();
        if (!intf) {
            return "";
        }

        return intf->name();
    }

    if (attributeName == "filters") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%u", interface()->filters().value());
        return buf;
    }

    if (attributeName == "Packets Received") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%u", interface()->packetsReceived().value());
        return buf;
    }
    
    if (attributeName == "Packets Dropped") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%u", interface()->packetsDropped().value());
        return buf;
    }

    if (attributeName == "Queue Size") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%u", interface()->queueSize().value());
        return buf;
    }

    GLUE_ERR("invalid interface attribute '%s'\n" ,attributeName.c_str());
    return "";
}

void
InterfaceGlue::attributeIs(const string &attributeName,
                           const string &newValueString)
{
    GLUE_TRACE("%s->%s: %s\n", 
               name().c_str(), attributeName.c_str(), newValueString.c_str());

    if (attributeName == "data rate") {
        interface()->dataRateIs(atoi(newValueString.c_str()));
        return;
    }

    if (attributeName == "other side") {
        Ptr<Instance> otherSide = manager_->instance(newValueString);
        if (!otherSide) {
            GLUE_ERR("other side %s not found\n", newValueString.c_str());
            throw ParserException();
        }

        Ptr<InterfaceGlue> otherSideInterfaceGlue = dynamic_cast<InterfaceGlue *>(otherSide.value());
        if (!otherSideInterfaceGlue) {
            GLUE_ERR("%s is not an Interface type\n", newValueString.c_str());
            throw ResourceException();
        }

        /*
         * connect an interface one to another
         */
        interface()->otherSideIs(otherSideInterfaceGlue->interface());
        try {
            manager_->onNetworkUpdate();
        }
        catch(Exception &e) {
            GLUE_ERR("failure on updating the network: %s\n", e.what());
        }
        catch(...) {
            GLUE_ERR("Unexpected failure on updating the network\n");
        }
        return;
    }

    if (attributeName == "filters") {
        interface()->filtersIs(atoi(newValueString.c_str()));
        return;
    }

    if (attributeName == "Queue Size") {
        interface()->queueSizeIs(atoi(newValueString.c_str()));
        return;
    }

    GLUE_ERR("invalid or not-writable attribute %s\n", attributeName.c_str());
    throw ParserException();
}

/**
 * attribute:
 *
 * parse for name:number
 * perform distance neighbor calculation, and stringify the result
 */

string
ConnectionGlue::attribute(const string &attributeName) const
{
    string::size_type idx = attributeName.find(":");
    /*
     * ":" not found return nothing
     */
    if (idx == string::npos) {
        GLUE_ERR("invalid attribute '%s'\n", attributeName.c_str());
        return "";
    }

    string instanceName = attributeName.substr(0, idx);
    Ptr<Instance> instance = manager_->instance(instanceName);
    if (!instance) {
        GLUE_ERR("instance not found %s\n", attributeName.c_str());
        return "";
    }
    /*
     * parse for degree
     */
    string degreeString = attributeName.substr(idx + 1);
    if (degreeString.empty()) {
        GLUE_ERR("invalid attribute name: %s\n", attributeName.c_str());
        return "";
    }
    int degree = atoi(degreeString.c_str());

    /*
     * type check for Node
     */
    Ptr<NodeGlue> nodeGlue = dynamic_cast<NodeGlue *>(instance.value());
    if (!nodeGlue) {
        GLUE_ERR("only instance with type Node is supported\n");
        return "";
    }
    Ptr<Node> node = nodeGlue->node();

    /*
     * calculate distance neighbor
     */
    vector<Ptr<Node> > neighbor = node->distanceNeighbor(degree);

    /*
     * convert the result to string
     */
    string resultString = "";
    for (unsigned int i = 0; i < neighbor.size(); i++) {
        Ptr<Node> elem = neighbor[i];
        if (resultString.length() > 0) {
            resultString += " ";
        }
        resultString += elem->name();
    }
    return resultString;
}

void 
ConnectionGlue::attributeIs(const string &attributeName, 
                            const string &newValueString)
{
    GLUE_ERR("trying to write to a read only instance\n");
}

string 
ConfigGlue::attribute(const string &attributeName) const
{
    char buf[100];

    ManagerImpl::InstanceCount total = manager_->instances(attributeName);
    snprintf(buf, sizeof(buf), "%d", total.value());
    return string(buf);
}

void 
ConfigGlue::attributeIs(const string &attributeName, 
                        const string &newValueString)
{
    GLUE_ERR("trying to write to a read only instance\n");
}

string 
IPHostGlue::attribute(const string &attributeName) const
{
    Ptr<IPHost> host = dynamic_cast<IPHost *>(node().value());
    if (!host) return "";

    if (attributeName == "Transmit Rate") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%d", host->transmitRate().value());
        return buf;
    }

    if (attributeName == "Packet Size") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%d", host->packetSize().value());
        return buf;
    }

    if (attributeName == "Destination") {
        Ptr<Node> dest = host->destination();
        if (!dest) {
            return "";
        }
        return dest->name();
    }

    if (attributeName == "Packets Received") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%d", host->packetsReceived().value());
        return buf;
    }

    if (attributeName == "Average Latency") {
        char buf[100];
        snprintf(buf, sizeof(buf), "%f", host->averageLatency().value());
        return buf;
    }

    return NodeGlue::attribute(attributeName);
}

void
IPHostGlue::attributeIs (const string &attributeName,
                         const string &newValueString)
{
    GLUE_TRACE("%s->%s: %s\n", 
               name().c_str(), attributeName.c_str(), newValueString.c_str());
    Ptr<IPHost> host = dynamic_cast<IPHost *>(node().value());

    try {
        if (attributeName == "Transmit Rate") {
            host->transmitRateIs(atoi(newValueString.c_str()));
            return;
        }

        if (attributeName == "Packet Size") {
            host->packetSizeIs(atoi(newValueString.c_str()));
            return;
        }
    }
    catch (RangeException &e) {
        throw ParserException();
    }

    if (attributeName == "Destination") {
        Ptr<Instance> dest = manager_->instance(newValueString);
        if (!dest) {
            GLUE_ERR("dest %s not found\n", newValueString.c_str());
            throw ParserException();
        }
        Ptr<NodeGlue> destGlue = dynamic_cast<IPHostGlue *>(dest.value());
        if (!destGlue) {
            GLUE_ERR("dest %s is not a IPHost type\n", newValueString.c_str());
            throw ResourceException();
        }

        host->destinationIs(destGlue->node());
        return;
    }

    NodeGlue::attributeIs(attributeName, newValueString);
}

} /* end namespace */

/*
 * This is the entry point for your library.
 * The client program will call this function to get a handle
 * on the Instance::Manager object, and from there will use
 * that object to interact with the glue layer (which will
 * in turn interact with the gore layer).
 */
Ptr<Instance::Manager>
NetworkFactory()
{
    Ptr<Instance::Manager> m;
    m = new NetworkImpl::ManagerImpl();
    if (!m) throw ResourceException();
    return m;
}

/* end-of-file */
