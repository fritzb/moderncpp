/*
 * $Id: verification.cc,v 1.1 2005/12/05 02:07:20 fzb Exp $
 *
 * verification.cc -- 
 *
 * Fritz Budiyanto, November 2005
 *
 */


#include <string>
#include <iostream>

#include "Instance.h"
#include "Notifiee.h"
#include "Activity.h"

extern Ptr<Instance::Manager> NetworkFactory();
extern Ptr<Activity::Manager> RealTimeActivityManager();
extern Ptr<Activity::Manager> ActivityManager();

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

/*

Diagram

   +--------+                        +--------+
   | Host   |                        | Host   |
   | h4     |                       /| h1     |
   +--------+                      / +--------+
                                  /
                                 /
                                /
                         eth1  /
                   +--------+ /
              eth0 | Router |/
                  /| r0     |\
                 / +--------+ \
                /        eth2  \
               /                \
         eth0 /                  \
  +--------+ /               eth0 \ +--------+
  | Host   |/                      \| Host   |
  | h0     |\                      /| h2     |
  +--------+ \                    / +--------+
         eth1 \                  / eth1
               \                /
                \              /     
                 \ +--------+ / eth1
            eth0  \| Router |/
                   | r1     |\ eth2
                   +--------+ \ 
                               \
                                \
                                 \ +--------+
                             eth0 \| Host   |
                                   | h3     |
                                   +--------+



 Test:
 h0 send 10 packet to h1
 h1 send 10 packet to h2
 h2 send 10 packet to h3
 h3 send 10 packet to h4

*/          
int
main(int argc, char *argv[])
{
    Ptr<Activity::Manager> realAM, virtualAM;

    realAM = RealTimeActivityManager();
    virtualAM = ActivityManager();

    virtualAM->runningIs(false);
    virtualAM->nowIs(0.0);

    Ptr<Instance::Manager> manager = NetworkFactory();
    Ptr<Instance> h0 = manager->instanceNew("h0", "IP host");
    Ptr<Instance> h1 = manager->instanceNew("h1", "IP host");
    Ptr<Instance> h2 = manager->instanceNew("h2", "IP host");
    Ptr<Instance> h3 = manager->instanceNew("h3", "IP host");
    Ptr<Instance> h4 = manager->instanceNew("h4", "IP host");
    Ptr<Instance> r0 = manager->instanceNew("r0", "IP router");
    Ptr<Instance> r1 = manager->instanceNew("r1", "IP router");

    Ptr<Instance> h0eth0 = manager->instanceNew("h0eth0", "Ethernet interface");
    Ptr<Instance> h0eth1 = manager->instanceNew("h0eth1", "Ethernet interface");
    Ptr<Instance> h1eth0 = manager->instanceNew("h1eth0", "Ethernet interface");
    Ptr<Instance> h2eth0 = manager->instanceNew("h2eth0", "Ethernet interface");
    Ptr<Instance> h2eth1 = manager->instanceNew("h2eth1", "Ethernet interface");
    Ptr<Instance> h3eth0 = manager->instanceNew("h3eth0", "Ethernet interface");

    Ptr<Instance> r0eth0 = manager->instanceNew("r0eth0", "Ethernet interface");
    Ptr<Instance> r0eth1 = manager->instanceNew("r0eth1", "Ethernet interface");
    Ptr<Instance> r0eth2 = manager->instanceNew("r0eth2", "Ethernet interface");

    Ptr<Instance> r1eth0 = manager->instanceNew("r1eth0", "Ethernet interface");
    Ptr<Instance> r1eth1 = manager->instanceNew("r1eth1", "Ethernet interface");
    Ptr<Instance> r1eth2 = manager->instanceNew("r1eth2", "Ethernet interface");

    r0->attributeIs("interface0", "r0eth0");
    r0->attributeIs("interface1", "r0eth1");
    r0->attributeIs("interface2", "r0eth2");

    r1->attributeIs("interface0", "r1eth0");
    r1->attributeIs("interface1", "r1eth1");
    r1->attributeIs("interface2", "r1eth2");

    h0->attributeIs("interface0", "h0eth0");
    h0->attributeIs("interface1", "h0eth1");
    h1->attributeIs("interface0", "h1eth0");

    h2->attributeIs("interface0", "h2eth0");
    h2->attributeIs("interface1", "h2eth1");

    h3->attributeIs("interface0", "h3eth0");

    h0eth0->attributeIs("data rate", "10");
    h0eth1->attributeIs("data rate", "10");
    h1eth0->attributeIs("data rate", "10");
    h2eth0->attributeIs("data rate", "10");
    h2eth1->attributeIs("data rate", "10");
    h3eth0->attributeIs("data rate", "10");

    r0eth0->attributeIs("data rate", "10");
    r0eth1->attributeIs("data rate", "10");
    r0eth2->attributeIs("data rate", "10");

    r1eth0->attributeIs("data rate", "10");
    r1eth1->attributeIs("data rate", "10");
    r1eth2->attributeIs("data rate", "10");

    h0eth0->attributeIs("other side", "r0eth0");
    h0eth1->attributeIs("other side", "r1eth0");
    h1eth0->attributeIs("other side", "r0eth1");
    h2eth0->attributeIs("other side", "r0eth2");
    h2eth1->attributeIs("other side", "r1eth1");
    h3eth0->attributeIs("other side", "r1eth2");

    h0->attributeIs("Transmit Rate", "1");
    h0->attributeIs("Packet Size", "100");
    h0->attributeIs("Destination", "h1");

    h1->attributeIs("Transmit Rate", "1");
    h1->attributeIs("Packet Size", "100");
    h1->attributeIs("Destination", "h2");

    h2->attributeIs("Transmit Rate", "1");
    h2->attributeIs("Packet Size", "100");
    h2->attributeIs("Destination", "h3");

    h3->attributeIs("Transmit Rate", "1");
    h3->attributeIs("Packet Size", "100");
    h3->attributeIs("Destination", "h0");

    h4->attributeIs("Transmit Rate", "1");
    h4->attributeIs("Packet Size", "100");
    h4->attributeIs("Destination", "h0");

    virtualAM->runningIs(false);
    virtualAM->nowIs(Time(0.0));
    virtualAM->runningIs(true);
    virtualAM->nowIs(Time(8800001.0));

    cout << "h0" << endl;
    cout << "h0 packets received: " << h0->attribute("Packets Received") << endl;
    cout << "h0 average latency: " << h0->attribute("Average Latency") << endl;
    cout << "h0eth0 packets received: " << h0eth0->attribute("Packets Received") << endl;
    cout << "h0eth0 packets dropped: " << h0eth0->attribute("Packets Dropped") << endl;
    cout << "h0eth1 packets received: " << h0eth1->attribute("Packets Received") << endl;
    cout << "h0eth1 packets dropped: " << h0eth1->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "h1" << endl;
    cout << "h1 packets received: " << h1->attribute("Packets Received") << endl;
    cout << "h1 average latency: " << h1->attribute("Average Latency") << endl;
    cout << "h1eth0 packets received: " << h1eth0->attribute("Packets Received") << endl;
    cout << "h1eth0 packets dropped: " << h1eth0->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "h2" << endl;
    cout << "h2 packets received: " << h2->attribute("Packets Received") << endl;
    cout << "h2 average latency: " << h2->attribute("Average Latency") << endl;
    cout << "h2eth0 packets received: " << h2eth0->attribute("Packets Received") << endl;
    cout << "h2eth0 packets dropped: " << h2eth0->attribute("Packets Dropped") << endl;
    cout << "h2eth1 packets received: " << h2eth1->attribute("Packets Received") << endl;
    cout << "h2eth1 packets dropped: " << h2eth1->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "h3" << endl;
    cout << "h3 packets received: " << h3->attribute("Packets Received") << endl;
    cout << "h3 average latency: " << h3->attribute("Average Latency") << endl;
    cout << "h3eth0 packets received: " << h3eth0->attribute("Packets Received") << endl;
    cout << "h3eth0 packets dropped: " << h3eth0->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "r0" << endl;
    cout << "r0eth0 packets received: " << r0eth0->attribute("Packets Received") << endl;
    cout << "r0eth0 packets dropped: " << r0eth0->attribute("Packets Dropped") << endl;
    cout << "r0eth1 packets received: " << r0eth1->attribute("Packets Received") << endl;
    cout << "r0eth1 packets dropped: " << r0eth1->attribute("Packets Dropped") << endl;
    cout << endl;

    cout << "r1" << endl;
    cout << "r1eth0 packets received: " << r1eth0->attribute("Packets Received") << endl;
    cout << "r1eth0 packets dropped: " << r1eth0->attribute("Packets Dropped") << endl;
    cout << "r1eth1 packets received: " << r1eth1->attribute("Packets Received") << endl;
    cout << "r1eth1 packets dropped: " << r1eth1->attribute("Packets Dropped") << endl;
}


/* end of file */
