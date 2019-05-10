/*
 * $Id: Numeric.h,v 1.1 2005/12/05 02:07:20 fzb Exp $
 *
 * Numeric.h -- 
 *
 * Fritz Budiyanto, November 2005
 *
 */

#ifndef __NUMERIC_H__
#define __NUMERIC_H__

template<class UnitType, class RepType>
class Numeric {
public:
    Numeric(RepType v = 0) : value_(v) {}

    bool operator==(const Numeric<UnitType, RepType>& v) const {
        return value_ == v.value_;
    }

    bool operator!=(const Numeric<UnitType, RepType>& v) const {
        return value_ != v.value_;
    }

    Numeric(const Numeric<UnitType, RepType>& v) {
        value_ = v.value_;
    }

    RepType value() const {
        return value_;
    }

    Numeric<UnitType,RepType>& operator=(const Numeric<UnitType, RepType>& v) { value_ = v.value_; return *this;}
    Numeric<UnitType,RepType>& operator=(Numeric<UnitType, RepType>& v) { value_ = v.value_; return *this;}

    int operator<(const Numeric<UnitType,RepType> &right) const { 
        return value_ < right.value_; 
    }
    int operator<=(const Numeric<UnitType,RepType> &right) const { 
        return value_ <= right.value_; 
    }
    int operator>(const Numeric<UnitType,RepType> &right) const { 
        return value_ > right.value_; 
    }
    int operator>=(const Numeric<UnitType,RepType> &right) const { 
        return value_ >= right.value_; 
    }

    Numeric operator+(const Numeric<UnitType,RepType>& t) const { 
        return Numeric<UnitType,RepType>(value_ + t.value_); 
    }
    Numeric operator-(const Numeric<UnitType,RepType>& t) const { 
        return Numeric<UnitType,RepType>(value_ - t.value_); 
    }

    const Numeric<UnitType,RepType>& operator+=(const Numeric<UnitType,RepType> &t) { 
        value_ += t.value_; 
        return *this; 
    }
    const Numeric<UnitType,RepType>& operator++() { 
        ++value_; 
        return *this; 
    }
    const Numeric<UnitType,RepType> operator++(int) { 
        Numeric<UnitType,RepType> old(value_); 
        value_++; 
        return old;
    }

    const Numeric<UnitType,RepType>& operator-=(const Numeric<UnitType,RepType> &t) { 
        value_ -= t.value_; 
        return *this; 
    }
    const Numeric<UnitType,RepType>& operator--() { 
        --value_; 
        return *this; 
    }
    const Numeric<UnitType,RepType> operator--(int) { 
        Numeric<UnitType,RepType> old(value_); 
        value_--; 
        return old; 
    }


protected:
    RepType value_;
};

#endif /* __NUMERIC_H__ */

/* end of file */
