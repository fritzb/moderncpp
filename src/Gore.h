/*
 * $Id: Gore.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Gore.h -- Gore interface prototype
 *
 * Fritz Budiyanto, October 2005
 *
 * Copyright (c) 2005, Stanford University.
 * All rights reserved.
 */

#ifndef __GORE_H__
#define __GORE_H__

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <limits.h>

#include "PtrInterface.h"
#include "Ptr.h"
#include "Nominal.h"
#include "Notifiee.h"
#include "Activity.h"
#include "Exception.h"

using namespace std;

extern Ptr<Activity::Manager> ActivityManager();

namespace NetworkImpl {

/**
 * NamedObject:
 *
 * NamedObject is the mother of all Gore objects, which can return
 * the name of an object as a string
 */
class NamedObject : public PtrInterface<NamedObject> {
public:
    // Accessor
    string name() const { return name_; }

    // Constructor/Destructor
    NamedObject(string name) :name_(name) {}

private:
    string name_;
};

class Node;
class InterfaceReactor;
class Packet;
class Interface : public NamedObject {
public:
    // Types
    typedef Ptr<Interface> Ptr;
    class Notifiee;
    typedef Nominal<class DataRate__, unsigned int> DataRate;
    typedef Nominal<class FilterCount__, unsigned int> FilterCount;
    class QueueSize : public Nominal<class QueueSize__, unsigned int> {
    public:
        QueueSize(int i) :Nominal<class QueueSize__, unsigned int>((unsigned int)i) {
            if (i < 0) { throw RangeException(); }
        }
    };
    typedef Numeric<class PacketCount__, unsigned int> PacketCount;

    // Accessor
    virtual Ptr<Node>       node() const;
    virtual DataRate        dataRate() const = 0;
    virtual Ptr<Interface>  otherSide() const { return otherSide_; }
    virtual FilterCount     filters() const { return filters_; }
    virtual PacketCount     packetsReceived() const { return packetsReceived_; }
    virtual PacketCount     packetsDropped() const { return packetsDropped_; }
    virtual QueueSize       queueSize() const { return queueSize_; }
    Notifiee                *notifiee() const { return notifiee_; }
    Ptr<Activity>           activity() const { return activity_; }

    // Mutator
    virtual void            nodeIs(Ptr<Node>);
    virtual void            dataRateIs(DataRate) = 0;
    virtual void            otherSideIs(Ptr<Interface> intf);
    virtual void            filtersIs(FilterCount count) { filters_ = count; }
    virtual void            queueSizeIs(QueueSize size) { queueSize_ = size; }
    void                    notifieeIs(Notifiee *n) { notifiee_ = n; }
    virtual void            lastOutputPacketIs(Ptr<Packet> packet);
    virtual void            lastInputPacketIs(Ptr<Packet> packet);

    // Constructor/Destructor
    virtual ~Interface();

protected:
    Ptr<Node> node_;
    Interface(string name);

private:
    friend class InterfaceReactor;
    Notifiee                *notifiee_;
    Ptr<Interface>          otherSide_;
    FilterCount             filters_;
    QueueSize               queueSize_;
    PacketCount             packetsReceived_;
    PacketCount             packetsDropped_;
    vector<Ptr<Packet> >    queue_;
    Ptr<Activity>           activity_;
    Ptr<InterfaceReactor>   reactor_;
};

class Interface::Notifiee : public BaseNotifiee<Interface> {
public:
    Notifiee(Interface *i) :BaseNotifiee<Interface>(i) {}
    virtual void onQueue() = 0;
};

class InterfaceReactor : public Interface::Notifiee {
public:
    InterfaceReactor(Interface *intf)
        :Interface::Notifiee(intf), owner_(intf) {}

    void handleNotification(Activity *a);
    void onQueue();
    string name() const { return "InterfaceReactor"; }

private:
    Interface *owner_;

    Ptr<Activity> activity() const { return owner_->activity(); }
};

class Node : public NamedObject {
public:
    // Types
    typedef Nominal<class Degree__, unsigned int> Degree;
    class Slot : public Nominal<class Slot__, unsigned int> {
    public:
        static const unsigned int Default = (unsigned int)-1;

        Slot() :Nominal<class Slot__, unsigned int>(Default) {}
        Slot(int i) :Nominal<class Slot__, unsigned int>((unsigned int)i) {
            if (i < 0) {
                throw RangeException();
            }
        }
    };

    // Accessor
    Ptr<Interface>      interface(Slot slot) const;
    vector<Ptr<Node> >  directNeighbor() const;
    vector<Ptr<Node> >  distanceNeighbor(Degree degree) const;
    Ptr<Interface>      route(Ptr<Node> n) const;

    // Mutator
    virtual void        interfaceIs(Slot slot, Ptr<Interface> intf);
    virtual void        lastPacketIs(Ptr<Packet> packet);

    // Callback handler
    void                handleNetworkUpdate() { routeUpdate(); }

    // Constructor/destructor
    virtual ~Node();

protected:
    Node(string name) :NamedObject(name) { }

private:
    // Private types
    struct SPF {
        Slot    slot;
        Node    *host;
        int     cost;
        SPF() :slot(0), host(0), cost(0) {}
    };

    // Member variables
    map<Node *, Slot>       routeTable_;
    vector<Ptr<Interface> > interface_;

    // Private member functions
    void candidateAdd (vector<SPF> &candidate, SPF elem, Node *node);
    void routeUpdate (void);
    void spf(vector<SPF> &candidate);
    void addInterface(Slot slot, Ptr<Interface> intf);
    void deleteInterface(Slot slot);
    void distanceNeighbor(vector<Ptr<Node> > &result, 
                          vector<Ptr<Node> > &visited,
                          const Ptr<Node> node, Degree degree) const;
};

class Packet : public PtrInterface<Packet> {
public:
    // Types
    class Age : public Numeric<class Age_, unsigned char> {
    public:
        static const unsigned Min = 1;
        static const unsigned Default = 64;
        Age() :Numeric<class Age_, unsigned char>(Default) {}
    };
    class Size : public Nominal<class Size__, int> {
    public:
        static const unsigned Default = 0;

        Size(int s) :Nominal<class Size__, int>(s) {
            if (s < 0) {
                throw RangeException();
            }
        }
    };

    // Accessors
    Size        size() const { return size_; }
    Time        timestamp() const { return timestamp_; }
    Node*       destination() const { return destination_; }
    Node*       source() const { return source_; }
    Age         age() const { return age_; }

    // Mutators
    void        sizeIs(Size size) { size_ = size; }
    void        timestampIs (Time t) { timestamp_ = t; }
    void        ageDec() { if (age_.value() == 0) throw ResourceException(); --age_; }

    // Constructor/Destructor
    Packet(Size size, Ptr<Node> src, Ptr<Node> dest) 
        :destination_(dest.value()), source_(src.value()), size_(size) {}

private:
    Node*       destination_;
    Node*       source_;
    Size        size_;
    Time        timestamp_;
    Age         age_;
};


class ATMInterface : public Interface {
public:
    // Accessor
    DataRate    dataRate() const { return dataRate_; }

    // Mutator
    void        dataRateIs(Interface::DataRate rate);
    void        otherSideIs(Ptr<Interface> intf);

    // Constructor/Destructor
    ATMInterface(string name) :Interface(name) {}

private:
    class ATMDataRate : public DataRate {
    public:
        static const unsigned int Default = 155;

        ATMDataRate() :DataRate(Default) {}
        ATMDataRate(const DataRate &rate) :DataRate(Default) {
            if (rate.value() != 155 &&
                rate.value() != 25) {
                throw RangeException();
            }

            value_ = rate.value();
        }
    };

    ATMDataRate dataRate_;
};

class EthernetInterface : public Interface {
public:
    // Accessor
    DataRate    dataRate() const { return dataRate_; }

    // Mutator
    void        dataRateIs(DataRate rate);
    void        otherSideIs(Ptr<Interface> intf);

    // Constructor/Destructor
    EthernetInterface(string name) :Interface(name) {}

private:
    // Private Types
    class EthernetDataRate : public DataRate {
    public:
        static const unsigned int Default = 10;

        EthernetDataRate() :DataRate(Default) {}
        EthernetDataRate(const DataRate &rate) :DataRate(Default) {
            if (rate.value() != 10 && 
                rate.value() != 100 &&
                rate.value() != 1000) {
                throw RangeException();
            }
            value_ = rate.value();
            return;
        }
    };

    // Private member variables
    EthernetDataRate dataRate_;
};

class ATMSwitch : public Node {
public:
    void interfaceIs(Slot slot, Ptr<Interface> intf);
    ATMSwitch(string nameString) :Node(nameString) {}
};

class EthernetSwitch : public Node {
public:
    void interfaceIs(Slot slot, Ptr<Interface> intf);
    EthernetSwitch(string nameString) :Node(nameString) {}
};

class IPHostReactor;
class IPHost : public Node {
public:
    // Types
    class TransmitRate : public Nominal<class TransmitRate__, int> {
    public:
        TransmitRate(int i) :Nominal<class TransmitRate__, int>(i) {
            if (i < 0) {
                throw RangeException();
            }
        }
    };
    typedef Numeric<class Latency__, double> Latency;
    typedef Ptr<IPHost> Ptr;
    class Notifiee : public BaseNotifiee<IPHost> {
    public:
        virtual void onGeneratePacket() = 0;

        Notifiee(IPHost *host) :BaseNotifiee<IPHost>(host) {}
    };

    // Accessor
    TransmitRate            transmitRate() const { return transmitRate_; }
    Packet::Size            packetSize() const { return packetSize_; }
    Node*                   destination() const { return destination_; }
    Interface::PacketCount  packetsReceived() const { return packetCount_; }
    Ptr<Activity>           activity() const { return activity_; } 
    Notifiee                *notifiee() const { return notifiee_; }
    Latency                 averageLatency() const;

    // Mutator
    void                    transmitRateIs(TransmitRate rate);
    void                    packetSizeIs(Packet::Size size);
    void                    destinationIs(Ptr<Node> destination);
    void                    notifieeIs(Notifiee *n) { notifiee_ = n; }
    void                    lastPacketIs (Ptr<Packet> packet);

    // Constructor/Destructor
    IPHost(string nameString);

private:
    TransmitRate            transmitRate_;
    Packet::Size            packetSize_;
    Node*                   destination_;
    Notifiee*               notifiee_;
    Latency                 sumLatency_;
    Ptr<Activity>           activity_;
    Interface::PacketCount  packetCount_;
    Ptr<IPHostReactor>      reactor_;
};

class IPHostReactor : public IPHost::Notifiee {
public:
    void handleNotification(Activity *act);
    // used for handle timeout only

    void onGeneratePacket();
    // used to generate packet when dest, rate, and size has valid value

    IPHostReactor(IPHost *host) 
        :IPHost::Notifiee(host), packet_(NULL), owner_(host) {}
    string name() const { return "IPHostReactor"; }

private:
    Ptr<Packet>     packet_;
    IPHost          *owner_;

    Ptr<Activity>   activity() const { return owner_->activity(); }
};

class IPRouter : public Node {
public:
    IPRouter(string nameString) :Node(nameString) {}
};

} /* end namespace */

#include "Ptr.in"

#endif /* __GORE_H__ */

/* end-of-file */
