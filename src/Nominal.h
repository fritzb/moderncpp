/*
 * $Id: Nominal.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Nominal.h -- 
 *
 * Fritz Budiyanto, October 2005
 *
 */

#ifndef __NOMINAL_H__
#define __NOMINAL_H__

template<class UnitType, class RepType>
class Nominal {
public:
    Nominal(RepType v) : value_(v) {}

    bool operator==(const Nominal<UnitType, RepType>& v) const
    {
        return value_ == v.value_;
    }

    bool operator!=(const Nominal<UnitType, RepType>& v) const
    {
        return value_ != v.value_;
    }

    const Nominal<UnitType, RepType>&
    operator=(const Nominal<UnitType, RepType>& v)
    {
        value_ = v.value_;
        return *this;
    }

    RepType value() const
    {
        return value_;
    }

protected:
    RepType value_;
};

#endif /* __NOMINAL_H__ */

/* end-of-file */
