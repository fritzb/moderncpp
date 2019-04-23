/*
 * $Id: Notifiee.h,v 1.1 2005/12/05 02:07:20 fzb Exp $
 *
 * Notifiee.h -- Notifiee prototype
 *
 * Fritz Budiyanto, October 2005
 *
 * Copyright (c) 2005, Stanford University.
 * All rights reserved.
 */

#ifndef __NOTIFIEE_H__
#define __NOTIFIEE_H__

#include <string>

#include "PtrInterface.h"
#include "Ptr.h"

using namespace std;

class Activity;

class RootNotifiee : public PtrInterface<RootNotifiee> {
public:
    virtual void handleNotification(Activity *) {}
    /* Deliberately empty */
};

template<typename Notifier>
class BaseNotifiee : public RootNotifiee {

public:
    BaseNotifiee(Notifier * n = 0) : notifier_(n) {
        if(n) n->notifieeIs(static_cast<typename Notifier::Notifiee *>(this));
    }

    typename Notifier::Ptr notifier() const { return notifier_; }

    void notifierIs(typename Notifier::Ptr n) {
        if(notifier_ == n) return;
        if(notifier_) notifier_->notifieeIs(0);
        notifier_ = n;
        if(n) n->notifieeIs(static_cast<typename Notifier::Notifiee *>(this));
    }

    ~BaseNotifiee() {
        if(notifier_) notifier_->notifieeIs(0);
    }

private:
    typename Notifier::Ptr notifier_;
};

#include "Ptr.in"

#endif /* __NOTIFIEE_H_ */

/* end of file */
