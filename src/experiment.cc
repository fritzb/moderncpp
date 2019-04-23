/*
 * $Id: experiment.cc,v 1.1 2005/12/05 02:07:20 fzb Exp $
 *
 * experiment.cc -- 
 *
 * Fritz Budiyanto, November 2005
 *
 * Copyright (c) 2005, Stanford University.
 * All rights reserved.
 */


#include <string>
#include <iostream>
#include <stdlib.h>

#include "Exception.h"
#include "Instance.h"
#include "Notifiee.h"
#include "Activity.h"
#include "Log.h"

Log logApp("GLUE");

#define APP_ERR(format, args...) \
logGlue.entryNew(Log::Error, "", __FUNCTION__, format, ##args)
#define APP_TRACE(format, args...) \
logGlue.entryNew(Log::Debug, "", __FUNCTION__, format, ##args)


extern Ptr<Instance::Manager> NetworkFactory();
extern Ptr<Activity::Manager> RealTimeActivityManager();
extern Ptr<Activity::Manager> ActivityManager();

class Parameter {
public:
    enum RunningMode {
        VirtualTime,
        RealTime,
    };
    static const long   PacketRandomMin     = 100;
    static const long   PacketRandomMax     = 10 * 1024;
    static const long   PacketRandomRange   = (PacketRandomMax - PacketRandomMin);
    static const int    SwitchTotal         = 10;
    static const int    SwitchPort          = 10;
    static const int    TransmitRate        = 10; // in Mbps
    static const int    PacketSize          = 1024; // in bytes
    static const int    DataRate            = 1000; // in Mbps
    static const int    SimulationTime      = 10; // in seconds

    string      packetSize() const;
    string      transmitRate() const { return stringify(transmitRate_); }
    string      dataRate() const { return stringify(dataRate_); }
    Time        simulationTime() const { return simulationTime_; }
    int         switchTotal() const { return switchTotal_; }
    int         switchPort() const { return switchPort_; }
    RunningMode runningMode() const { return runningMode_; }

    Parameter(int argc, char **argv);

private:
    bool    random_;
    int     packetSize_;
    int     switchTotal_;
    int     switchPort_;
    int     dataRate_;
    int     transmitRate_;
    Time    simulationTime_;
    RunningMode runningMode_;

    bool    random() const { return random_; }
    string  randomPacketSize() const;
    string  stringify(int i) const;
};

string  
Parameter::stringify(int i) const
{
    char buf[1024];

    sprintf(buf, "%d", i);
    return string(buf);
}

string
Parameter::randomPacketSize() const
{
    long r = rand();
    r = r % PacketRandomRange;
    r += PacketRandomMin;

    return stringify(r);
}

string
Parameter::packetSize() const {
    if (!random()) {
        return stringify(PacketSize);
    }
    return randomPacketSize();
}

Parameter::Parameter(int argc, char **argv)
    :random_(false), packetSize_(PacketSize), switchTotal_(SwitchTotal),
    switchPort_(SwitchPort), dataRate_(DataRate), transmitRate_(TransmitRate),
    simulationTime_(Time(SimulationTime)), runningMode_(RealTime)
{
    int c;

    while ((c = getopt(argc, argv, "hrs:p:l:t:d:x:v")) > 0) {
        switch (c) {
        case 'h':
            cout << "h  help" << endl;
            cout << "r  random packet size" << endl;
            cout << "s  switch total" << endl;
            cout << "p  port per switch total" << endl;
            cout << "l  packet size (in bytes)" << endl;
            cout << "t  transmit rate (in mbps)" << endl;
            cout << "d  data rate (in mbps)" << endl;
            cout << "x  simulation time (in second)" << endl;
            cout << "v  running in virtual time" << endl;
            exit(0);
            break;

        case 'r':
            random_ = true;
            break;

        case 's':
            switchTotal_ = atoi(optarg);
            break;

        case 'p':
            switchPort_ = atoi(optarg);
            break;
            
        case 'l':
            packetSize_ = atoi(optarg);
            break;

        case 't':
            transmitRate_ = atoi(optarg);
            break;

        case 'd':
            dataRate_ = atoi(optarg);
            break;

        case 'x':
            simulationTime_ = Time(atoi(optarg));
            break;

        case 'v':
            runningMode_ = VirtualTime;
            break;
        }
    }
#if 0
    if (runningMode_ == VirtualTime) {
        simulationTime_ = Time(simulationTime_.value() / 1000.0);
    }
#endif
}

void
displayStatistic (Ptr<Instance::Manager> m, Ptr<Instance> inst)
{
    char buf[1024];
    int i;
    cout << inst->name() << endl;
    cout << inst->name() << " packets received: " << inst->attribute("Packets Received") << endl;
    cout << inst->name() << " average latency: " << inst->attribute("Average Latency") << endl;

    i = 0;
    do {
        sprintf(buf, "interface%d", i);
        string intfname = inst->attribute(buf);
        Ptr<Instance> intf = m->instance(intfname);
        if (!intf) {
            break;
        }
        cout << "[" << i << "] " << intf->name() << " received: " << intf->attribute("Packets Received") << " dropped: " << intf->attribute("Packets Dropped") << endl;
        i++;
    } while (1);
    cout << endl;
}

void
display_exception()
{
    try {
        throw;
    }
    catch (Exception &e) {
        cout << e.what() << endl;
    }
}

int
main(int argc, char **argv)
{
    Ptr<Activity::Manager>  realAM, virtualAM;
    Parameter               param(argc, argv);
    Ptr<Instance::Manager>  manager;

    set_terminate(&display_exception);
    set_unexpected(&display_exception);

    // Grab all system wide variable
    manager     = NetworkFactory();
    realAM      = RealTimeActivityManager();
    virtualAM   = ActivityManager();

    // Reset virtual time
    virtualAM->runningIs(false);
    virtualAM->nowIs(0.0);

    cout << "Building instances ..." << endl;

    // create destination
    cout << "Creating dst host ..." << endl;
    Ptr<Instance>    host_dst;
    Ptr<Instance>    host_dst_intf;
    host_dst       = manager->instanceNew("host_dst", "IP host");
    host_dst_intf  = manager->instanceNew("host_dst_eth0", "Ethernet interface");
    host_dst_intf->attributeIs("data rate", param.dataRate());
    host_dst->attributeIs("interface0", "host_dst_eth0");

    // creates 100 hosts
    cout << "Creating " << param.switchPort() * param.switchTotal() << " src hosts ..." << endl;
    vector<Ptr<Instance> >  host_src;
    vector<Ptr<Instance> >  host_src_intf;
    for (int i = 0; i < param.switchPort() * param.switchTotal(); i++) {
        Ptr<Instance>   host;
        Ptr<Instance>   intf;
        char            buf[100];

        sprintf(buf, "host_src%d", i);
        host = manager->instanceNew(buf, "IP host");

        sprintf(buf, "host_src%d_eth0", i);
        intf = manager->instanceNew(buf, "Ethernet interface");
        intf->attributeIs("data rate", param.dataRate());
        host->attributeIs("interface0", intf->name());
        host->attributeIs("Transmit Rate", param.transmitRate());
        host->attributeIs("Packet Size", param.packetSize());
        host->attributeIs("Destination", "host_dst");

        host_src.push_back(host);
        host_src_intf.push_back(intf);
    }

    /*
     * create 10 switches
     */
    cout << "Creating " << param.switchTotal() << " switches ..." << endl;
    vector<Ptr<Instance> > sw;
    vector<Ptr<Instance> > sw_intf;
    for (int i = 0; i < param.switchTotal(); i++) {
        Ptr<Instance>   eswitch;
        Ptr<Instance>   intf;
        char            buf[100];

        sprintf(buf, "switch%d", i);
        eswitch = manager->instanceNew(buf, "Ethernet switch");

        sprintf(buf, "switch%d_eth0", i);
        intf = manager->instanceNew(buf, "Ethernet interface");
        intf->attributeIs("data rate", param.dataRate());
        eswitch->attributeIs("interface0", intf->name());

        sw.push_back(eswitch);
        sw_intf.push_back(intf);

        /*
         * link to host
         */
        for (int j = 0; j < param.switchPort(); j++) {
            Ptr<Instance>   intf;
            char            intf_name[100];

            sprintf(intf_name, "switch%d_eth%d", i, j+1);
            intf = manager->instanceNew(intf_name, "Ethernet interface");
            intf->attributeIs("data rate", param.dataRate());
            sprintf(buf, "interface%d", j+1);
            eswitch->attributeIs(buf, intf->name());

            intf->attributeIs("other side", host_src_intf[i*param.switchPort() + j]->name());
        }
    }

    /*
     * create 1 master switch
     */
    cout << "Creating master switch ..." << endl;
    Ptr<Instance> master_switch;
    Ptr<Instance> master_switch_intf;
    master_switch = manager->instanceNew("master_switch", "Ethernet switch");
    master_switch_intf = manager->instanceNew("master_switch_eth0", "Ethernet interface");
    master_switch_intf->attributeIs("data rate", param.dataRate());
    master_switch->attributeIs("interface0", "master_switch_eth0");

    for (int i = 0; i < param.switchTotal(); i++) {
        Ptr<Instance>   intf;
        char            buf[1024];

        sprintf(buf, "master_switch_eth%d", i+1);
        intf = manager->instanceNew(buf, "Ethernet interface");
        intf->attributeIs("data rate", param.dataRate());
        intf->attributeIs("other side", sw_intf[i]->name());

        sprintf(buf, "interface%d", i+1);
        master_switch->attributeIs(buf, intf->name());
    }

    /*
     * Link master switch to destination
     */
    host_dst_intf->attributeIs("other side", "master_switch_eth0"); 
    cout << "Running Simulation ..." << endl;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    switch (param.runningMode()) {
    case Parameter::RealTime: {
        //Adjust current time
        realAM->runningIs(false);
        realAM->nowIs(tv);

        // Run for 10 second
        virtualAM->runningIs(true);
        realAM->runningIs(true);
        realAM->nowIs(realAM->now() + param.simulationTime());
        cout << "elapsed virtual time: " << virtualAM->now() << endl;
        cout << "elapsed real time: " << (realAM->now() - Time(tv)) << endl;
        break;
    }

    case Parameter::VirtualTime:
        virtualAM->runningIs(true);
        //virtualAM->nowIs(Time(8800001.0));
        //virtualAM->nowIs(Time(8800001.0));
        //virtualAM->nowIs(Time(100000000.0));
        //virtualAM->nowIs(Time(200000000.0));
        virtualAM->nowIs(param.simulationTime());
        cout << "elapsed virtual time: " << virtualAM->now() << endl;
        break;
    }

    struct timeval current;
    gettimeofday(&current, NULL);
    cout << "elapsed actual time: " << Time(current) - Time(tv) << endl;

    cout << "Collecting Statistics ..." << endl;

    cout << "host_dst" << endl;
    cout << "host_dst packets received: " << host_dst->attribute("Packets Received") << endl;
    cout << "host_dst average latency: " << host_dst->attribute("Average Latency") << endl;
    cout << "host_dst_eth0 packets received: " << host_dst_intf->attribute("Packets Received") << endl;
    cout << "host_dst_eth0 packets dropped: " << host_dst_intf->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "switch0" << endl;
    cout << "switch0_eth0 packets received: " << sw_intf[0]->attribute("Packets Received") << endl;
    cout << "switch0_eth0 packets dropped: " << sw_intf[0]->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "master_switch" << endl;
    cout << "master_switch_eth0 packets received: " << master_switch_intf->attribute("Packets Received") << endl;
    cout << "master_switch_eth0 packets dropped: " << master_switch_intf->attribute("Packets Dropped") << endl;
    cout << endl;
}

/* end of file */
