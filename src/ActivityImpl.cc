/*
 * $Id: ActivityImpl.cc,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * ActivityImpl.cc -- 
 *
 * Fritz Budiyanto, October 2005
 *
 * Copyright (c) 2005, Stanford University.
 * All rights reserved.
 */

#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <deque>
#include <sys/time.h>
#include <limits.h>

#include "Log.h"
#include "Exception.h"
#include "Activity.h"
#include "ActivityImpl.h"

using namespace std;

ostream& operator<<(ostream &s, Time t)
{
    struct timeval tv;

    tv = t;
    return s << "(" << tv.tv_sec << " sec " << tv.tv_usec << " usec)";
}

namespace ActivityImpl {

const Time Activity::Never = Time(DBL_MAX);

Log logActivity("ACTIVITY");

/**
 * execute:
 *
 * run lastNotifiee if available and return
 * -otherwise-
 * run timeoutNotifiee if available and return
 */

void
ActivityImpl::execute()
{
    Ptr<RootNotifiee> root;
    vector<Ptr<RootNotifiee> >::iterator i;

    /*
     * go through all lastNotifiee_ and invoke handleNotification
     */
    if (!lastNotifiee_.empty()) {
        while (!lastNotifiee_.empty()) {
            i = lastNotifiee_.begin();
            root = (*i);
            try {
                root->handleNotification(this);
            } 
            catch (Exception &e) {
                ACTIVITY_ERR("%s\n", e.what());
            }
            catch(...) {
                ACTIVITY_ERR("error detected while calling handleNotification\n");
            }
            lastNotifiee_.erase(i);
        }
        return;
    }

    /*
     * if there is no lastNotifiee, then it must be timeoutNotifiee
     */
    root = timeoutNotifiee_;
    nextTimeIs(Activity::Never);
    try {
        root->handleNotification(this);
    }
    catch (Exception &e) {
        ACTIVITY_ERR("%s\n", e.what());
    }
    catch(...) {
        ACTIVITY_ERR("error detected when calling handleNotification\n");
    }
}

void
ActivityImpl::statusIs (Status s)
{
    if (status() == s) {
        return;
    }
    Activity::statusIs(s);
    if (s == Executing) {
        execute();
    }
}

void            
ActivityImpl::lastNotifieeIs(Ptr<RootNotifiee> p)
{
    if (!p) {
        ACTIVITY_ERR("trying to add NULL notifiee\n");
        return;
    }
    Activity::lastNotifieeIs(p);
    manager_->readyQueueIs(this);
}

void            
ActivityImpl::nextTimeIs(Time t)
{
    if (nextTime() == t) {
        return;
    }
    Activity::nextTimeIs(t);
    manager_->waitingQueueIs(this);
}

Activity::Ptr 
ManagerImpl::activity (const string &name) const
{
    map<string, Activity::Ptr>::const_iterator t = activity_.find(name);

    if (t == activity_.end()) {
        return NULL;
    }

    return (*t).second;
}

Activity::Ptr 
ManagerImpl::activityNew(const string &name)
{
    Activity::Ptr activity;

    map<string, Activity::Ptr>::const_iterator t = activity_.find(name);
    if (t != activity_.end()) {
        ACTIVITY_ERR("activity '%s' exists\n", name.c_str());
        /*
         * tell caller to use other name
         */
        throw NameInUseException(name);
    }
     
    activity = new ActivityImpl(name, this);
    if (!activity) throw ResourceException();
    activity_[name] = activity;

    return activity;
}

void 
ManagerImpl::activityDel(const string &name)
{
    Activity::Ptr                           activity;
    map<string, Activity::Ptr>::iterator    t;
   
    t = activity_.find(name);
    if (t == activity_.end()) {
        ACTIVITY_ERR("activity '%s' does not exists\n", name.c_str());
        return;
    }
    activity = (*t).second;

    switch (activity->status()) {
    case Activity::Free:
        break;

    case Activity::Waiting:
        removeFromWaitingQueue(activity);
        break;

    case Activity::Ready:
        removeFromReadyQueue(activity);
        if (activity->nextTime() != Activity::Never) {
            removeFromWaitingQueue(activity);
        }
        break;

    case Activity::Executing:
        /*
         * Can't do this, tell the caller to do it some other time
         */
        throw PermissionException();
    }

    activity_.erase(t);
}

void
ManagerImpl::removeFromReadyQueue (Activity::Ptr activity)
{
    vector<Activity::Ptr>::iterator i;

    for (i = readyQueue_.begin(); i < readyQueue_.end(); i++) {
        if ((*i) == activity) {
            readyQueue_.erase(i);
            break;
        }
    }
}

void
ManagerImpl::removeFromWaitingQueue (Activity::Ptr activity)
{
    vector<Activity::Ptr>::iterator i;

    for (i = waitingQueue_.begin(); i < waitingQueue_.end(); i++) {
        if ((*i) == activity) {
            waitingQueue_.erase(i);
            return;
        }
    }
}

void
ManagerImpl::insertToWaitingQueue (Activity::Ptr activity)
{
    /*
     * insert it so its in sorted order
     */
    vector<Activity::Ptr>::iterator i;

    /*
     * if activity is in the waiting queue, remove it
     */
    if (activity->status() != Activity::Free) {
        removeFromWaitingQueue(activity);
    }

    /*
     * set the status
     */
    if (activity->status() != Activity::Ready) {
        activity->statusIs(Activity::Waiting);
    }

    /*
     * optimization case, where we need to put things on the back
     */
    if (waitingQueue_.size() > 0) {
        if (waitingQueue_.back()->nextTime() <= activity->nextTime()) {
            waitingQueue_.push_back(activity);
            return;
        }
    }

    for (i = waitingQueue_.begin(); i < waitingQueue_.end(); i++) {
        Activity::Ptr act;
        act = (*i);
        if (activity->nextTime() < act->nextTime()) {
            break;
        }
    }
    waitingQueue_.insert(i, activity);
}

void
ManagerImpl::insertToReadyQueue (Ptr<Activity> activity)
{
    activity->statusIs(Activity::Ready);
    readyQueue_.push_back(activity);
}

void
ManagerImpl::reschedule()
{
    vector<Activity::Ptr>::iterator i;
    /*
     * go through wait queue and move it to  ready queue
     * if its time to run the activity
     */
    for (i = waitingQueue_.begin(); i < waitingQueue_.end(); i++) {
        Activity::Ptr activity;

        activity    = (*i);

        /*
         * if activity is currently in ready queue, skip
         */
        if (activity->status() == Activity::Ready) {
            continue;
        }

        /*
         * move it to active queue if its due
         */
        if (activity->nextTime() <= now()) {
            waitingQueue_.erase(i);
            insertToReadyQueue(activity);
        }

        /*
         * since this is sorted order, we can break right away
         * if time is greater than now
         */
        if (activity->nextTime() > now()) {
            break;
        }
    }
}

void
ManagerImpl::runActivityFromThread(Ptr<Activity> activity)
{
    activity->statusIs(Activity::Executing);
}

void
ManagerImpl::runActivity(Ptr<Activity> activity)
{
    runActivityFromThread(activity);
}

void
ManagerImpl::reschedule (Ptr<Activity> activity)
{
    if (activity->status() == Activity::Executing) {
        if (activity->nextTime() == Activity::Never) {
            activity->statusIs(Activity::Free);
            return;
        }

        if (activity->nextTime() >= now()) {
            activity->statusIs(Activity::Waiting);
            return;
        }
    }
}

void
ManagerImpl::runReadyQueue()
{
    while (!readyQueue_.empty()) {
        Activity::Ptr   activity;

        activity = readyQueue_[0];
        //readyQueue_.pop_front();
        readyQueue_.erase(readyQueue_.begin());

        runActivity(activity);
        reschedule(activity);
    }
}

Time
ManagerImpl::nextTimeout () const
{
    /*
     * go through wait queue, and find the one that timeout the first
     */
    if (waitingQueue_.empty()) {
        return Activity::Never;
    }

    return waitingQueue_[0]->nextTime();
}

void
ManagerImpl::waitingQueueIs(Ptr<Activity> act)
{
    insertToWaitingQueue(act);
}

void
ManagerImpl::readyQueueIs(Ptr<Activity> act)
{
    insertToReadyQueue(act);
}

/**
 * nowIs:
 *
 * set now_ to the given time.
 *
 * if running is false, change now_, and return
 * if running is true, change now_, and process
 * outstanding activity
 */

void
ManagerImpl::nowIs(Time t)
{
    if (t == now()) return;

    if (!running()) {
        now_ = t;
        return;
    }

    while (now_ < t) {
        reschedule();

        /*
         * if ready queue is none, advance in time
         */
        if (readyQueue_.size() == 0) {
            if (nextTimeout() == Activity::Never) {
                now_ = t;
                break;
            }

            if (nextTimeout() < t) {
                now_ = nextTimeout();
            } else {
                now_ = t;
            }
        }

        runReadyQueue();
    }
}

/**
 * nowIs:
 *
 * if not running, set now_ to t, set startTime_ to t,
 * and record current virtualTime, and return
 *
 * if running, then go into a loop and update now_, and 
 * call virtualManager->nowIs() (if virtualManager exists)
 * loop will terminate when time t has elapsed.
 */

void 
RealTimeManagerImpl::nowIs(Time t)
{
    Time virtualTime;

    if (t == now()) return;

    virtualTime = Time(t.value() / ratio().value());

    if (!running()) {
        startTime_ = t;
        if (virtualManager_) {
            startVirtualTime_ = virtualManager_->now();
        }
        ManagerImpl::nowIs(t);
        return;
    }

    Time current;
    struct timeval tv;
    do {
        gettimeofday(&tv, NULL);
        current = Time(tv);
        if (current > t) {
            break;
        }
        ManagerImpl::nowIs(current);
        if (virtualManager_) {
            virtualTime = startVirtualTime_ + 
                          Time((current - startTime_).value() / ratio().value());
            virtualManager_->nowIs(virtualTime);
        }
        reschedule();
        runReadyQueue();
    } while(1);
}

Ptr<Activity::Manager> vam;
Ptr<Activity::Manager> ram;

} // namespace ActivityImpl

Ptr<Activity::Manager> ActivityManager()
{
    if (!ActivityImpl::vam) {
        ActivityImpl::vam = new ActivityImpl::ManagerImpl();
        if (!ActivityImpl::vam) {
            throw ResourceException();
        }
    }

    return ActivityImpl::vam;
}

Ptr<Activity::Manager> RealTimeActivityManager()
{
    if (!ActivityImpl::ram) {
        ActivityImpl::ram = new ActivityImpl::RealTimeManagerImpl(ActivityManager());
        if (!ActivityImpl::ram) {
            throw ResourceException();
        }
    }

    return ActivityImpl::ram;
}

/* end of file */
