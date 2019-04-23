// Copyright (C) 1993-2002 David R. Cheriton.  All rights reserved.

#ifndef FWK_PTR_H
#define FWK_PTR_H

template <class T>
class Ptr {
  public:
    Ptr( T *ptr = 0 );
    Ptr( const Ptr<T>& mp );
    ~Ptr();
    void operator=( const Ptr<T>& mp );
    void operator=( Ptr<T>& mp );
    bool operator==( const Ptr<T>& mp ) const { return mp.value_ == value_; }
    const T * operator->() const { return value_; }
    T * operator->() { return (T *) value_; }
    T * value() const { return (T *) value_; }
    operator bool() const { return value_ ? 1 : 0; }

    template <class OtherType>
    operator Ptr<OtherType>() const {
        return Ptr<OtherType>( value_ ); }

protected:
    T *value_;
};

#endif /* PTR_h */
