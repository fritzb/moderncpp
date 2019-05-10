/*
 * $Id: ActivityImpl.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * ActivityImpl.h -- Activity Implementation
 *
 * Fritz Budiyanto, October 2005
 *
 */

#ifndef __ACTIVITY_IMPL_H__
#define __ACTIVITY_IMPL_H__

#include "Activity.h"
#include <sys/types.h>
#include <sys/times.h>


Ptr<Activity::Manager> ActivityFactory();

namespace ActivityImpl {

extern Log logActivity;

#define ACTIVITY_ERR(format, args...) \
logActivity.entryNew(Log::Error, this->name(), __FUNCTION__, format, ##args)
#define ACTIVITY_TRACE(format, args...) \
logActivity.entryNew(Log::Debug, this->name(), __FUNCTION__, format, ##args)


class ManagerImpl;
class ActivityImpl : public Activity {
public:
    // Types
    typedef Ptr<ActivityImpl> Ptr;

    // Mutator
    void statusIs (Status s);
    void lastNotifieeIs(Ptr<RootNotifiee> p);
    void nextTimeIs(Time t);

    // Constructor/Destructor
    ActivityImpl(string name, ManagerImpl *manager)
        :Activity(name), manager_(manager) {}

private:
    ManagerImpl *manager_;

    void execute();
};

class ManagerImpl : public Activity::Manager {
public:
    // Types
    typedef Ptr<ManagerImpl> Ptr;

    // Accessor
    Activity::Ptr   activity(const string &name) const;
    Time            now() const { return now_; }
    string          name() const { return "Activity::ManagerImpl"; }

    // Mutator
    Activity::Ptr   activityNew(const string &name);
    void            activityDel(const string &name);
    void            nowIs(Time t);
    void            waitingQueueIs(Ptr<Activity> act);
    void            readyQueueIs(Ptr<Activity> act);

    // Constructor/Destructor
    ManagerImpl() {}
    ~ManagerImpl() { ACTIVITY_TRACE("Destroyed\n"); }

protected:
    Time nextTimeout() const;
    void reschedule();
    void runReadyQueue();

private:
    friend class ActivityImpl;

    Time                        now_;
    map<string, Activity::Ptr>  activity_;
    vector<Activity::Ptr>       waitingQueue_;
    vector<Activity::Ptr>       readyQueue_;

    void runActivity(Activity::Ptr activity);
    void runActivityFromThread(Activity::Ptr activity);
    void reschedule(Ptr<Activity> act);
    void insertToWaitingQueue(Activity::Ptr activity);
    void insertToReadyQueue(Ptr<Activity> activity);
    void removeFromWaitingQueue(Activity::Ptr activity);
    void removeFromReadyQueue(Activity::Ptr activity);
};

class RealTimeManagerImpl : public ManagerImpl {
public:
    // Types
    typedef Ptr<RealTimeManagerImpl> Ptr;
    class Ratio : public Nominal<class Ratio__, unsigned int> {
    public:
        static const unsigned int Default = 1000;

        Ratio() :Nominal<class Ratio__, unsigned int>(Default) {}
        Ratio(const Ratio &r) :Nominal<class Ratio__, unsigned int>(Default) {
            if (r.value() == 0) {
                throw RangeException();
            }
            value_ = r.value();
        }
    };

    // Accessor
    Ratio ratio() const { return ratio_; }
    string name() const { return "RealTimeManagerImpl"; }

    // Mutator
    void nowIs(Time t);
    void ratioIs(Ratio ratio) { ratio_ = ratio; }
    void virtualManagerIs(Ptr<Activity::Manager> am) 
        { virtualManager_ = dynamic_cast<ManagerImpl *>(am.value()); }

    // Constructor/Destructor
    RealTimeManagerImpl(Ptr<Activity::Manager> vm=0) { 
        struct timeval tv;

        virtualManagerIs(vm); 
        gettimeofday(&tv, NULL);
        runningIs(false);
        nowIs(Time(tv));
    }
    ~RealTimeManagerImpl() { ACTIVITY_TRACE("Destroyed\n"); }

private:
    Ptr<ManagerImpl>    virtualManager_;
    Ratio               ratio_;
    Time                startTime_;
    Time                startVirtualTime_;
};

} //end of namespace

#endif /* __ACTIVITY_IMPL_H__ */

/* end of file */
