/*
 * $Id: Exception.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Exception.h -- 
 *
 * Fritz Budiyanto, November 2005
 *
 */

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

class Exception {
public:
    virtual const char * what() const { return what_.c_str(); }
    Exception() {}
    virtual ~Exception() {}

protected:
    void whatIs(std::string w) { what_ = w; }

private:
    std::string what_;
};

class ResourceException : public Exception {
public:
    ResourceException() { whatIs(std::string("ResourceException")); }
    ResourceException(const std::string &msg) { whatIs(msg); }
};

class RangeException : public Exception {
public:
    RangeException() { whatIs("RangeException"); }
};

class InternalException : public ResourceException {
public:
    InternalException() { whatIs("InternalException"); }
};

class ParserException : public ResourceException {
public:
    ParserException() { whatIs("ParserException"); }
};

class NameInUseException : public ResourceException {
public:

    const char * name() const { return name_.c_str(); }

    NameInUseException(const std::string &n) :name_(n) {
        whatIs(n);
    }

private:
    std::string name_;
};

class PermissionException : public ResourceException {
public:
    PermissionException() { whatIs("PermissionException"); }
    PermissionException(const std::string &msg) { whatIs(msg); }
};

#endif /* __EXCEPTION_H__ */

/* end of file */
