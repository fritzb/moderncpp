/*
 * $Id: Gore.cc,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Gore.cc -- Gore implementation
 *
 * Fritz Budiyanto, October 2005
 *
 */

#include <iostream>
#include <vector>
#include "Gore.h"
#include "Notifiee.h"
#include "Log.h"
#include "Activity.h"

using namespace std;

namespace NetworkImpl {

Log logGore("GORE");

#define GORE_ERR(format, args...) \
logGore.entryNew(Log::Error, this->name(), __FUNCTION__, format, ##args)

#define GORE_TRACE(format, args...) \
logGore.entryNew(Log::Debug, this->name(), __FUNCTION__, format, ##args)

Interface::Interface(string name) 
    :NamedObject(name), 
    otherSide_(NULL), 
    filters_(0),
    queueSize_(10),
    packetsReceived_(0),
    packetsDropped_(0),
    activity_(ActivityManager()->activityNew(name + string(" transmit packet")))
{
    reactor_ = new InterfaceReactor(this);
    if (!reactor_) {
        throw ResourceException();
    }
}


Ptr<Node> 
Interface::node() const
{
    return node_;
}

void 
Interface::nodeIs(Ptr<Node> n)
{
    node_ = n;
}

/**
 * otherSideIs:
 *
 * link the interface's otherSide to 'intf'
 */

void 
Interface::otherSideIs(Ptr<Interface> intf) 
{
    /*
     * dont allow link to itself
     */
    if (intf.value() == this) {
        throw PermissionException("cannot link interface to itself");
    }

    /*
     * unlink the other side's interface to me
     */
    if (otherSide_) {
        otherSide_->otherSide_ = NULL;
    }

    /*
     * create 2 way connection
     */
    otherSide_ = intf;
    if (intf) {
        intf->otherSide_ = this;
    }
}

/**
 * ~Interface:
 *
 * node node to point to nowhere
 * disconnect otherSide's connection to me
 * disconnect otherSide
 */

Interface::~Interface() 
{
    try {

    Ptr<Interface> otherSide = this->otherSide();
    if (otherSide) {
        otherSide->otherSideIs(NULL);
    }
    otherSideIs(NULL);
    nodeIs(NULL);

    }
    catch (...) {}
}

void 
InterfaceReactor::handleNotification (Activity *a) 
{
    GORE_TRACE("\n");
    Ptr<Interface> intf = notifier();
    Ptr<Packet> packet = notifier()->queue_.front();

    intf->queue_.erase(intf->queue_.begin());

    Ptr<Interface> otherSide = intf->otherSide();
    otherSide->lastInputPacketIs(packet);

    /*
     * schedule another one until we drain all queue
     */
    if (!(notifier()->queue_.empty())) {
        onQueue();
    }
}

void 
InterfaceReactor::onQueue () 
{
    Ptr<Interface> intf = notifier();
    /*
     * schedule a transmit
     */
    Ptr<Packet> packet = intf->queue_.front();

    Time packetTransmitTime((1000000000.0 * packet->size().value() * 8.0) / (intf->dataRate().value() * 1000000.0));
    packetTransmitTime += ActivityManager()->now();

    Ptr<Activity> act = activity();
    act->nextTimeIs(packetTransmitTime);
    act->timeoutNotifieeIs(this);
}


void
Interface::lastOutputPacketIs(Ptr<Packet> packet)
{

    GORE_TRACE("packetsDropped_: %d\n", packetsDropped_.value());
    if (queue_.size() == queueSize().value()) {
        ++packetsDropped_;
        /*
         * drop em
         */
        return;
    }
    queue_.push_back(packet);

    /*
     * schedule transmission
     */
    if (notifiee()) try {
        notifiee()->onQueue();
    }
    catch(...) {}
}

void
Interface::lastInputPacketIs(Ptr<Packet> packet)
{
    GORE_TRACE("packetsDropped_: %d\n", packetsDropped_.value());
    ++packetsReceived_;

    if (packet->age() <= Packet::Age::Min) {
        ++packetsDropped_;
        return;
    }

    /*
     * decrement packet age
     */
    packet->ageDec();

    /*
     * if this packet is not for us, then 
     * check if there is such route
     */
    if (packet->destination() != node().value()) {
        /*
         * if there is no route, drop and count
         */
        if (!(node()->route(packet->destination()))) {
            ++packetsDropped_;
            return;
        }
    }

    /*
     * let node process the packet
     */
    node()->lastPacketIs(packet);
}

/**
 * lastPacketIs:
 *
 * forward the packet
 *
 * put it in the output interface queue
 * and schedule timeout.
 */

void 
Node::lastPacketIs(Ptr<Packet> packet)
{
    /*
     * received a packet
     */
    Ptr<Interface> outgoingIntf;

    /*
     * packet for me?
     */
    if (packet->destination() == this) {
        return;
    }

    outgoingIntf = route(packet->destination());
    if (!outgoingIntf) {
        return;
    }
    outgoingIntf->lastOutputPacketIs(packet);
}


/**
 * interface:
 *
 * return NULL if slot is invalid
 */

Ptr<Interface>
Node::interface(Slot slot) const
{
    if (slot.value() >= interface_.size()) {
        return NULL;
    }

    return interface_[slot.value()];
}

/**
 * interfaceIs:
 *
 * if intf is NULL, delete the interface
 * if intf is not NULL, add the interface
 */

void
Node::interfaceIs(Slot slot, Ptr<Interface> intf)
{
    if (slot.value() > interface_.size()) {
        return;
    }
    if (intf) {
        addInterface(slot, intf);
    } else {
        deleteInterface(slot);
    }
}

/**
 * directNeighbor:
 *
 * return directecly connected neighbor
 */

vector<Ptr<Node> >
Node::directNeighbor() const 
{
    vector<Ptr<Node> > result;

    for (unsigned int i = 0; i < interface_.size(); i++) {
        Ptr<Interface> otherSide = interface_[i]->otherSide();
        if (!otherSide) {
            continue;
        }
        Ptr<Node> node = otherSide->node();
        if (!node) {
            continue;
        }
        result.push_back(node);
    }

    return result;
}

/**
 * distanceNeighbor:
 *
 * compute the neighbor seperated at distance 'degree' from 'node'
 * the result will be stored at 'result'
 * visited contain all node that has been visited, this is to
 * help the function to detect cyclic
 */

void
Node::distanceNeighbor(vector<Ptr<Node> > &result,
                       vector<Ptr<Node> > &visited,
                       const Ptr<Node> node, 
                       Degree degree) const 
{
    vector<Ptr<Node> > neighbor;

    /*
     * collect directly connected neighbor
     */
    neighbor = node->directNeighbor();

    /*
     * we visited this node
     */
    visited.push_back(node);

    /*
     * terminating condition for the recursive function
     * its when the degree reach 0
     */
    if (degree == 0) {
        result.push_back(node);
        return;
    }
    /*
     * Depth First Search traversal
     */
    for (unsigned int i = 0; i < neighbor.size(); i++) {
        vector<Ptr<Node> >::const_iterator pos;

        pos = find(visited.begin(), visited.end(), neighbor[i]);
        if (pos == visited.end()) {
            distanceNeighbor(result, visited, neighbor[i], degree.value()-1);
        }
    }
}


/**
 * distanceNeighbor:
 *
 * return all distance neighbor at degree hop away
 */

vector<Ptr<Node> >
Node::distanceNeighbor(Degree degree) const 
{
    vector<Ptr<Node> > result, visited;
    Ptr<Node> this_ptr = const_cast<Node *>(this);
    distanceNeighbor(result, visited, const_cast<Node *>(this), degree);
    return result;
}

void
Node::candidateAdd (vector<SPF> &candidate, SPF elem, Node *node)
{
    vector<SPF>::iterator i;
    elem.host = node;
    elem.cost++;

    /*
     * sorted add to candidate list
     */
    for (i = candidate.begin(); i < candidate.end(); i++) {
        SPF s;

        s = (*i);
        if (elem.cost < s.cost) {
            break;
        }
    }
    candidate.insert(i, elem);
    return;
}

/**
 * spf:
 *
 * shortest path forwarding algorithm to build routing
 * table based on the optimal path
 */

void
Node::spf (vector<SPF> &candidate)
{
    SPF elem;
    map<Node *, Slot>::iterator rt;
    
    while (!candidate.empty()) {
        elem = candidate.front();
        candidate.erase(candidate.begin());

        /*
         * what out for existing element
         */
        rt = routeTable_.find(elem.host);
        if (rt != routeTable_.end()) {
            continue;
        }

        routeTable_[elem.host] = elem.slot;

        vector<Ptr<Node> > neighbors;
        neighbors = elem.host->directNeighbor();

        /*
         * for each neighbor
         */
        vector<Ptr<Node> >::iterator nodeIterator;

        for (nodeIterator = neighbors.begin(); 
             nodeIterator < neighbors.end();
             nodeIterator++) {
            Ptr<Node> node = (*nodeIterator);
            rt = routeTable_.find(node.value());
            if (rt != routeTable_.end()) {
                continue;
            }
            candidateAdd(candidate, elem, node.value());
        }
    }
}

void
Node::routeUpdate (void)
{
    vector<SPF> candidate;
    SPF         elem;

    GORE_TRACE("\n");
    routeTable_.clear();

    /*
     * add ourself to the routing table
     */
    routeTable_[this] = Slot();

    for (unsigned int i = 0; i < interface_.size(); i++) {
        Ptr<Interface> otherSide = interface_[i]->otherSide();
        if (!otherSide) {
            continue;
        }
        Ptr<Node> node = otherSide->node();
        if (!node || (node.value() == this)) {
            continue;
        }
        elem.host = node.value();
        elem.slot = Slot(i);
        elem.cost = 1;
        candidate.push_back(elem);
    }

    spf(candidate);

    /*
     * remove ourself from the routing table so that packet wont be looping
     */
    map<Node *, Slot>::iterator rt;
    rt = routeTable_.find(this);
    if (rt != routeTable_.end()) {
        routeTable_.erase(rt);
    }
}

Ptr<Interface> 
Node::route(Ptr<Node> dest) const
{
    if (!dest) {
        return NULL;
    }

    map<Node *, Slot>::const_iterator rt = routeTable_.find(dest.value());

    /*
     * if route found, return it
     */
    if (rt != routeTable_.end()) {
        Slot slot;

        slot = (*rt).second;
        return interface(slot);
    }

    return NULL;
}


/**
 * addInterface:
 *
 * add an interface at the given slot
 * and link the owner to ourself
 */

void
Node::addInterface(Slot slot, Ptr<Interface> intf)
{
    /*
     * link to ourself
     */
    intf->nodeIs(this);

    /*
     * adding an interface ?
     */
    if (slot.value() == interface_.size()) { 
        interface_.push_back(intf);
        return;
    }

    /*
     * must be replacing an interface, unlink the ownership of the node
     */
    Ptr<Interface> oldIntf = interface_[slot.value()];
    oldIntf->nodeIs(NULL);
    interface_[slot.value()] = intf;
}

/**
 * deleteInterface:
 *
 * unlink the deleted interface from the node owner
 * delete the interface at the given slot
 */

void 
Node::deleteInterface(Slot slot)
{
    /*
     * skip on unavailable slot
     */
    if (slot.value() == interface_.size()) { 
        return;
    }
    Ptr<Interface> intf = interface_[slot.value()];
    intf->nodeIs(NULL);

    vector<Ptr<Interface> >::iterator i = interface_.begin();
    i += slot.value();
    interface_.erase(i);
}

/**
 * ~Node:
 *
 * iterate each interface, and delink them to the node,
 * and delete them from the interface list
 */

Node::~Node() 
{
    try {

    vector<Ptr<Interface> >::iterator i;
    for (i = interface_.begin(); i < interface_.end(); i++) {
        Ptr<Interface> intf = *i;
        intf->nodeIs(NULL);
        interface_.erase(i);
    } 

    }
    catch (...) {}
}


/**
 * dataRateIs:
 *
 * reject invalid ATM data rate to be installed
 */

void
ATMInterface::dataRateIs (DataRate rate) 
{
    ATMDataRate atmDataRate;
    try {
        atmDataRate = rate;
    }
    catch (RangeException &e) {
        throw PermissionException("invalid ATM data rate");
    }
    dataRate_ = atmDataRate; 
}

/**
 * otherSideIs:
 *
 * reject connecting to non ATM interface
 */

void
ATMInterface::otherSideIs (Ptr<Interface> intf) 
{
    if (intf && (typeid(*(intf.value())) != typeid(ATMInterface))) {
        throw PermissionException("incompatible ATM interface");
    }
    Interface::otherSideIs(intf);
}

/**
 * dataRateIs:
 *
 * reject invalid data rate, and reject changing data
 * rate when interface is already connected
 */

void
EthernetInterface::dataRateIs (DataRate rate) 
{ 
    EthernetInterface::EthernetDataRate ethDataRate;
    try {
        ethDataRate = rate;
    }
    catch (RangeException &e) {
        throw PermissionException("invalid Ethernet data rate");
    }
    /*
     * do not change data rate when connection is connected
     */
    if (dataRate_ != rate) {
        if (otherSide()) {
            throw PermissionException("cannot change data rate when interface"
                                      " is already connected");
        }
    }

    dataRate_ = rate; 
}

/**
 * otherSideIs:
 *
 * disallow connecting to non ethernet interface, and invalid data rate
 */

void
EthernetInterface::otherSideIs (Ptr<Interface> intf) 
{
    if (intf && (typeid(*(intf.value())) != typeid(EthernetInterface))) {
        throw PermissionException("Trying to connect non EthernetInterface "
                                  "to an EthernetInterface");
    }
    if (intf->dataRate() != dataRate()) {
        throw PermissionException("connecting an incompatible "
                                  "EthernetInterface data rate");
    }

    Interface::otherSideIs(intf);
}

/**
 * interfaceIs:
 *
 * reject incompatible ATM interface
 */

void
ATMSwitch::interfaceIs (Slot slot, Ptr<Interface> intf)
{
    if (intf) {
        Ptr<ATMInterface> atmIntf;

        atmIntf = dynamic_cast<ATMInterface *>(intf.value());
        if (!atmIntf) {
            throw PermissionException("incompatible slot, requires ATM "
                                      "interface for ATM switch");
        }
    }
    Node::interfaceIs(slot, intf);
}

/**
 * interfaceIs:
 *
 * reject incompatible ethernet interface
 */

void
EthernetSwitch::interfaceIs (Slot slot, Ptr<Interface> intf)
{
    if (intf) {
        Ptr<EthernetInterface> ethernetIntf;

        ethernetIntf = dynamic_cast<EthernetInterface *>(intf.value());
        if (!ethernetIntf) {
            throw PermissionException("incompatible slot, requires Ethernet "
                                      "interface for Ethernet switch");
        }
    }
    Node::interfaceIs(slot, intf);
}


IPHost::IPHost(string nameString) :Node(nameString),
                                   transmitRate_(0),
                                   packetSize_(0),
                                   sumLatency_(0),
                                   activity_(ActivityManager()->activityNew(nameString + string(" packet generator"))),
                                   packetCount_(0)
{
    reactor_ = new IPHostReactor(this);
    if (!reactor_) {
        throw ResourceException();
    }
}

/**
 * lastPacketIs:
 *
 * host cannot forward packet
 */

void
IPHost::lastPacketIs (Ptr<Packet> packet)
{
    /*
     * if we are sending the packet, let it go
     */
    if (packet->source() == this &&
        packet->destination() != this) {
        /*
         * send the packet that originated from the host
         */
        Node::lastPacketIs(packet);
        return;
    }

    /*
     * drop the packet if it needs to be forwarded
     * to other host.
     */
    if (packet->destination() != this) {
        /*
         * drop silently
         */
        return;
    }

    /*
     * packet received
     */
    ++packetCount_;

    /* 
     * update latency
     */
    sumLatency_ = Latency(sumLatency_.value() + (ActivityManager()->now() - packet->timestamp()).value());
}

IPHost::Latency                 
IPHost::averageLatency() const 
{
    if (packetCount_ == 0) {
        return Latency(0);
    }
    return IPHost::Latency(sumLatency_.value() / packetCount_.value());
}

void 
IPHost::transmitRateIs(TransmitRate rate) 
{
    if (transmitRate_ == rate) return;
    transmitRate_ = rate; 
    if (notifiee()) try {
        notifiee()->onGeneratePacket(); 
    }
    catch(...) {}
}

void 
IPHost::packetSizeIs(Packet::Size size) 
{
    if (size == packetSize_) return;
    packetSize_ = size;

    if (notifiee()) try {
        notifiee()->onGeneratePacket();
    }
    catch(...) {}
}

void                    
IPHost::destinationIs(Ptr<Node> destination)
{
    if (destination_ == destination.value()) {
        return;
    }
    destination_ = destination.value();
    if (notifiee()) try {
        notifiee()->onGeneratePacket();
    }
    catch(...) {}
}

/**
 * handleNotification:
 *
 * this routine will be called by the Activity 
 * when we are ready to transmit
 */

void 
IPHostReactor::handleNotification (Activity *act)
{
    Ptr<IPHost> host;
    /*
     * all needs to be handled here is timeout notification
     */

    GORE_TRACE("\n");
    host = notifier();
    packet_->timestampIs(ActivityManager()->now());
    host->lastPacketIs(packet_);
    packet_ = NULL;

    /*
     * schedule for next packet generation
     */
    onGeneratePacket();
}

void
IPHostReactor::onGeneratePacket ()
{
    Ptr<IPHost> host;
    Time timeout;

    GORE_TRACE("\n");
    host = notifier();
    /*
     * calculate timeout
     */
    Packet::Size packetSize = host->packetSize();
    IPHost::TransmitRate rate = host->transmitRate();

    timeout = Activity::Never;

    /*
     * calculate timeout on a valid case
     */
    if ((packetSize.value() > 0) && 
        (rate.value() > 0) && 
        (host->destination() != NULL)) {
        double timeoutValue = 1000000000.0 * packetSize.value() * 8 / (1000000 * host->transmitRate().value());
        timeout = ActivityManager()->now() + Time(timeoutValue);

        packet_ = new Packet(packetSize, host, host->destination());
        if (!packet_) throw ResourceException();
    }

    Ptr<Activity> act = activity();
    act->nextTimeIs(timeout);
    act->timeoutNotifieeIs(this);
}

} // namespace NetworkImpl

/* end of file */
