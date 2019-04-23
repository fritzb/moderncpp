// Copyright (C) 1993-2002 David R. Cheriton.  All rights reserved.

#ifndef FWK_PTRINTERFACE_H
#define FWK_PTRINTERFACE_H

typedef long RefCount;

/**
 * PtrInterface defines a template interface for top-level reference-managed
 * classes.  The template parameter T is the class itself.
 * <p>
 * PtrInterface, or PtrInterfaceFrom should be used for all entity classes
 * that provide shared access, and therefore need reference management.
 * <p>
 * - Use PtrInterface for top-level interfaces.
 * - Use PtrInterfaceFrom for derived interfaces.
 *
 * @author David Cheriton, modified by Ed Swierk
 * @version Stanford CS 249 Winter 2002 version 1.0
 */
template <class T>
class PtrInterface {
  public:
    PtrInterface(): references_(0) {}

    RefCount references() const { return references_; }

    const PtrInterface<T> * newRef() const {
        PtrInterface<T> * me = (PtrInterface<T> *) this;
	++me->references_;
	return this;
    }

    void deleteRef() const {
        PtrInterface<T> * me = (PtrInterface<T> *) this;
        if ( --me->references_ == 0 ) me->onZeroReferences();
    }

  protected:
    virtual ~PtrInterface() {}

  private:
    virtual void onZeroReferences() { delete this; }

    RefCount references_;
  };


/**
 * PtrInterfaceFrom defines a template interface for derived
 * reference-managed classes.  The template parameter T is the base class.
 */
template <class T>
class PtrInterfaceFrom : public T {
  public:
    PtrInterfaceFrom(): references_(0) {}

    RefCount references() const { return references_; }

    const PtrInterfaceFrom<T> * newRef() const {
        PtrInterfaceFrom<T> * me = (PtrInterfaceFrom<T> *) this;
	++me->references_;
	return this;
    }

    void deleteRef() const {
        PtrInterfaceFrom<T> * me = (PtrInterfaceFrom<T> *) this;
        if ( --me->references_ == 0 ) me->onZeroReferences();
    }

  protected:
    virtual ~PtrInterfaceFrom() {}

  private:
    virtual void onZeroReferences() { delete this; }

    RefCount references_;
  };

#endif
