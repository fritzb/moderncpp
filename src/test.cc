/*
 * $Id: test.cc,v 1.1 2005/12/05 02:07:20 fzb Exp $
 *
 * test.cc -- 
 *
 * Fritz Budiyanto, October 2005
 *
 */

#include <string>
#include <iostream>

#include "Instance.h"
#include "Notifiee.h"
#include "Activity.h"

extern Ptr<Instance::Manager> NetworkFactory();
extern Ptr<Activity::Manager> ActivityFactory();
extern Ptr<Activity::Manager> RealTimeActivityManager();
extern Ptr<Activity::Manager> ActivityManager();

class Reactor : public RootNotifiee
{
public:
    Reactor(Ptr<Activity::Manager> am) :am_(am) {}
    void handleNotification(Activity *a) { 
        cout << a->name() << " at 2 sec\n"; 
        Time t;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        t = Time(1);
        t += am_->now();
        a->nextTimeIs(t); 
    }

private:
    Ptr<Activity::Manager> am_;
};

int
main(int argc, char *argv[])
{
    Ptr<Activity::Manager> realAM, virtualAM;
    Ptr<Reactor> reactor;
    struct timeval tv;


    realAM = RealTimeActivityManager();
    virtualAM = ActivityManager();
    // Adjust current time
    gettimeofday(&tv, NULL);
    realAM->runningIs(false);
    realAM->nowIs(tv);

    /*
     * run for 20 seconds
     */

    Activity::Ptr activity;
    Activity::Ptr a2;

    activity    = realAM->activityNew("foo");
    a2          = realAM->activityNew("bar");
    reactor     = new Reactor(realAM);

    Time t      = realAM->now() + Time(5);
    Time t2     = t + Time(2);

    activity->nextTimeIs(t);
    activity->timeoutNotifieeIs(reactor);

    a2->nextTimeIs(t2);
    a2->timeoutNotifieeIs(reactor);

    // Run for 10 second
    realAM->runningIs(true);
    realAM->nowIs(realAM->now() + Time(10));

    Time endTime = realAM->now() - Time(tv);
    cout << "Last Virtual Time: " << endTime << endl;

    realAM->activityDel("foo");
    realAM->activityDel("bar");
}
