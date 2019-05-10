/*
 * $Id: Log.h,v 1.1 2005/12/05 02:07:19 fzb Exp $
 *
 * Log.h -- 
 *
 * Fritz Budiyanto, October 2005
 *
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <stdio.h>
#include <stdarg.h>

#ifdef DEBUG
#define DEBUG_DEFAULT true
#else
#define DEBUG_DEFAULT false
#endif

class Log {
public:
    // Types
    enum Priority {
        Error,
        Debug
    };
    static const bool DebugDefault = DEBUG_DEFAULT;

    // Accessor
    bool debug() const { return debug_; }
   
    // Mutator 
    void debugIs(bool d) { debug_ = d; }
    void entryNew(Priority          prio, 
                  const std::string &name, 
                  const std::string &funcName, 
                  const char        *format, ...) {
        va_list args;

        if (prio == Debug) {
            if (!debug_) {
                return;
            }
        }

        va_start(args, format);
        fprintf(stderr, "[%s] %s.%s: ", 
                moduleName_.c_str(),
                name.c_str(),
                funcName.c_str());
        vfprintf(stderr, format, args);
        va_end(args);
    }

    // Constructor/Destructor
    Log(std::string moduleName) :debug_(DebugDefault) { moduleName_ = moduleName; }

private:
    std::string moduleName_;
    bool        debug_;
};

#endif /* __LOG_H__ */

/* end of file */
