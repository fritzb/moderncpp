/*
 * $Id: Activity.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Activity.h -- Activity Prototype
 *
 * Fritz Budiyanto, October 2005
 *
 * Copyright (c) 2005, Stanford University.
 * All rights reserved.
 */

#ifndef __ACTIVITY_H__
#define __ACTIVITY_H__
#include <iostream>
#include <string>
#include <queue>
#include "PtrInterface.h"
#include "Ptr.h"

#include "Nominal.h"
#include "Numeric.h"
#include "Notifiee.h"

using namespace std;

class Time : public Numeric<Time, double> {
public:
    // Types
    static const double SEC_TO_NANO = 1000000000;
    static const double USEC_TO_NANO = 1000;
    static const double DEFAULT = 0.0;

    /*
     * convert struct timeval to nanosecond
     */
    operator struct timeval() {
        struct timeval tv;

        tv.tv_usec = (unsigned)(value() / 1000);
        tv.tv_usec %= 1000000;
        tv.tv_sec = (unsigned)(value() / 1000000000.0);

        return tv;
    }

    // Constructor/Destructor
    Time(const Numeric<Time, double> &t) :Numeric<Time, double>(t) {}
    Time() :Numeric<Time, double>(DEFAULT) {}
    Time(const double &t) :Numeric<Time, double>(t) {}
    Time(const int &t) :Numeric<Time, double>(DEFAULT) { value_ = t * SEC_TO_NANO; }
    Time(const struct timeval &t) 
        :Numeric<Time, double>(DEFAULT) {
        value_ = 1000000000.0 * t.tv_sec;
        value_ += 1000.0 * t.tv_usec;
    }
};
extern ostream& operator<<(ostream &s, Time t);

class Activity : public PtrInterface<Activity> {
public:
    // Types
    typedef Ptr<Activity> Ptr;
    enum Status {Free,Waiting,Ready,Executing};
    static const Time Never;
    class Manager;

    // Accessor
    Status          status() const { return status_; }
    virtual Time    nextTime() const { return nextTime_; }
    virtual string  name() const { return name_; }

    // Mutator
    virtual void    statusIs(Status s) { status_ = s; }
    virtual void    nextTimeIs(Time t) { nextTime_ = t; statusIs(Waiting); }
    virtual void    lastNotifieeIs(Ptr<RootNotifiee> p) { lastNotifiee_.push_back(p); statusIs(Ready); }
    virtual void    timeoutNotifieeIs(Ptr<RootNotifiee> p) { timeoutNotifiee_ = p; if (status() != Ready) statusIs(Waiting); }

protected:
    vector<Ptr<RootNotifiee> >  lastNotifiee_;
    Ptr<RootNotifiee>           timeoutNotifiee_;

    Activity(const string &name) 
        :name_(name), status_(Free), nextTime_(Activity::Never) {}
    virtual ~Activity() {}

private:
    string  name_;
    Status  status_;
    Time    nextTime_;
};



class Activity::Manager : public PtrInterface<Activity::Manager> {
public:
    // Accessor
    virtual Activity::Ptr   activity(const string &name) const = 0;
    virtual Time            now() const = 0;
    virtual bool            running() const { return running_; }
    virtual string          name() const = 0;

    // Mutator
    virtual Activity::Ptr   activityNew(const string &name) = 0;
    virtual void            activityDel(const string &name) = 0;
    virtual void            runningIs(bool r) { running_ = r; }
    virtual void            nowIs(Time t) = 0;

    // Constructor/Destructor
    Manager() :running_(false) {}

protected:
    virtual void idle() {}

private:
    bool running_;
};

#include "Ptr.in"

#endif /* __ACTIVITY_H__ */

/* end of file */
