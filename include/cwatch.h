//==========================================================================
//  CWATCH.H - part of
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//
//  Declaration of the following classes:
//    cWatchBase etc: make primitive types, structs etc inspectable
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2005 Andras Varga

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __CWATCH_H
#define __CWATCH_H

#include <iostream>
#include <sstream>
#include "cobject.h"


/**
 * Utility class to make primitive types and non-cObject objects
 * inspectable in Tkenv. To be used only via the WATCH, WATCH_PTR,
 * WATCH_OBJ, WATCH_VECTOR etc macros.
 *
 * @ingroup Internals
 */
class SIM_API cWatchBase : public cObject
{
  public:
    /** @name Constructors, destructor, assignment */
    //@{
    /**
     * Initialize the shell to hold the given variable.
     */
    cWatchBase(const char *name)  : cObject(name) {}

    /**
     * Copy constructor not supported: it will raise an error via dup().
     */
    cWatchBase(const cWatchBase& v) : cObject(v.name()) {operator=(v);}

    /**
     * Assignment not supported: it will raise an error.
     */
    cWatchBase& operator=(const cWatchBase&) {copyNotSupported();return *this;}

    /**
     * dup() not supported: it will raise an error.
     */
    virtual cObject *dup() const {copyNotSupported(); return NULL;}
    //@}

    /** @name New methods */
    //@{
    /**
     * Tells if changing the variable's value via assign() is supported.
     */
    virtual bool supportsAssignment() const = 0;

    /**
     * Changes the watched variable's value. May only be called if
     * supportsAssignment() returns true.
     */
    virtual void assign(const char *s) {}
    //@}
};


/**
 * Template Watch class, for any type that supports operator<<.
 * @ingroup Internals
 */
template<typename T>
class SIM_API cGenericReadonlyWatch : public cWatchBase
{
  private:
    const T& r;
  public:
    cGenericReadonlyWatch(const char *name, const T& x) : cWatchBase(name), r(x) {}
    virtual const char *className() const {return opp_typename(typeid(T));}
    virtual bool supportsAssignment() const {return false;}
    virtual std::string info() const
    {
        std::stringstream out;
        out << r;
        return out.str();
    }
};


/**
 * Template Watch class, for any type that supports operator<<,
 * and operator>> for assignment.
 * @ingroup Internals
 */
template<typename T>
class SIM_API cGenericAssignableWatch : public cWatchBase
{
  private:
    T& r;
  public:
    cGenericAssignableWatch(const char *name, T& x) : cWatchBase(name), r(x) {}
    virtual const char *className() const {return opp_typename(typeid(T));}
    virtual bool supportsAssignment() const {return true;}
    virtual std::string info() const
    {
        std::stringstream out;
        out << r;
        return out.str();
    }
    virtual void assign(const char *s)
    {
        std::stringstream in(s);
        in >> r;
    }
};

/**
 * Watch class, specifically for bool.
 * @ingroup Internals
 */
class SIM_API cWatch_bool : public cWatchBase
{
  private:
    bool& r;
  public:
    cWatch_bool(const char *name, bool& x) : cWatchBase(name), r(x) {}
    virtual const char *className() const {return "bool";}
    virtual bool supportsAssignment() const {return true;}
    virtual std::string info() const
    {
        return r ? "true" : "false";
    }
    virtual void assign(const char *s)
    {
        r = *s!='0' && *s!='n' && *s!='N' && *s!='f' && *s!='F';
    }
};

/**
 * Watch class, specifically for char.
 * @ingroup Internals
 */
class SIM_API cWatch_char : public cWatchBase
{
  private:
    char& r;
  public:
    cWatch_char(const char *name, char& x) : cWatchBase(name), r(x) {}
    virtual const char *className() const {return "char";}
    virtual bool supportsAssignment() const {return true;}
    virtual std::string info() const
    {
        std::stringstream out;
        out << "'" << r << "' (" << int(r) << ")";
        return out.str();
    }
    virtual void assign(const char *s)
    {
        if (s[0]=='\'')
            r = s[1];
        else
            r = atoi(s);
    }
};

/**
 * Watch class, specifically for std::string.
 * @ingroup Internals
 */
class SIM_API cWatch_stdstring : public cWatchBase
{
  private:
    std::string& r;
  public:
    cWatch_stdstring(const char *name, std::string& x) : cWatchBase(name), r(x) {}
    virtual const char *className() const {return "std::string";}
    virtual bool supportsAssignment() const {return true;}
    virtual std::string info() const
    {
        std::stringstream out;
        out << "\"" << r << "\"";
        return out.str();
    }
    virtual void assign(const char *s)
    {
        r = s;
    }
};

/**
 * Watch class, specifically for objects subclassed from cPolymorphic.
 * @ingroup Internals
 */
class SIM_API cWatch_cPolymorphic : public cWatchBase
{
  private:
    cPolymorphic& r;
  public:
    cWatch_cPolymorphic(const char *name, cPolymorphic& ref) : cWatchBase(name), r(ref) {}
    virtual const char *className() const {return r.className();}
    virtual std::string info() const {return r.info();}
    virtual bool supportsAssignment() const {return false;}
    virtual cStructDescriptor *createDescriptor() {return r.createDescriptor();}
};

/**
 * Watch class, specifically for pointers to objects subclassed from cPolymorphic.
 * @ingroup Internals
 */
class SIM_API cWatch_cPolymorphicPtr : public cWatchBase
{
  private:
    cPolymorphic *&rp;
  public:
    cWatch_cPolymorphicPtr(const char *name, cPolymorphic *&ptr) : cWatchBase(name), rp(ptr) {}
    virtual const char *className() const {return rp->className() : "n/a";}
    virtual std::string info() const {return rp ? rp->info() : "<null>";}
    virtual bool supportsAssignment() const {return false;}
    virtual cStructDescriptor *createDescriptor() {return rp ? rp->createDescriptor() : NULL;}
};


inline cWatchBase *createWatch(const char *varname, short& d) {
    return new cGenericAssignableWatch<short>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, unsigned short& d) {
    return new cGenericAssignableWatch<unsigned short>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, int& d) {
    return new cGenericAssignableWatch<int>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, unsigned int& d) {
    return new cGenericAssignableWatch<unsigned int>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, long& d) {
    return new cGenericAssignableWatch<long>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, unsigned long& d) {
    return new cGenericAssignableWatch<unsigned long>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, float& d) {
    return new cGenericAssignableWatch<float>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, double& d) {
    return new cGenericAssignableWatch<double>(varname, d);
}

inline cWatchBase *createWatch(const char *varname, bool& d) {
    return new cWatch_bool(varname, d);
}

inline cWatchBase *createWatch(const char *varname, char& d) {
    return new cWatch_char(varname, d);
}

inline cWatchBase *createWatch(const char *varname, unsigned char& d) {
    return new cWatch_char(varname, *(char *)&d);
}

inline cWatchBase *createWatch(const char *varname, signed char& d) {
    return new cWatch_char(varname, *(char *)&d);
}

inline cWatchBase *createWatch(const char *varname, std::string& v) {
    return new cWatch_stdstring(varname, v);
}

// this is the fallback, if none of the above match
template<typename T>
inline cWatchBase *createWatch(const char *varname, T& d) {
    return new cGenericReadonlyWatch<T>(varname, d);
}

// to be used if T also has op>> defined
template<typename T>
inline cWatchBase *createWatch_genericAssignable(const char *varname, T& d) {
    return new cGenericAssignableWatch<T>(varname, d);
}

// for objects
inline cWatchBase *createWatch_cPolymorphic(const char *varname, cPolymorphic *ptr) {
    return new cWatch_cPolymorphic(varname, ptr);
}

// for objects
inline cWatchBase *createWatch_cPolymorphicPtr(const char *varname, cPolymorphic *ptr) {
    return new cWatch_cPolymorphicPtr(varname, ptr);
}


/**
 * @name WATCH macros
 * @ingroup Macros
 */
//@{

/**
 * Makes primitive types and types with operator<< inspectable in Tkenv.
 * See also WATCH_RW(), WATCH_PTR(), WATCH_OBJ(), WATCH_VECTOR(),
 * WATCH_PTRVECTOR(), etc. macros.
 *
 * @hideinitializer
 */
#define WATCH(variable)    createWatch(#variable,(variable))

/**
 * Makes types with operator<< and operator>> inspectable in Tkenv.
 * operator>> is used to enable changing the variable's value interactively.
 *
 * @hideinitializer
 */
#define WATCH_RW(variable) createWatch_genericAssignable(#variable,(variable))

/**
 * Makes classes derived from cPolymorphic inspectable in Tkenv.
 * See also WATCH_PTR().
 *
 * @hideinitializer
 */
#define WATCH_OBJ(variable) createWatch_cPolymorphic(#variable,(variable))

/**
 * Makes pointers to objects derived from cPolymorphic inspectable in Tkenv.
 * See also WATCH_OBJ().
 *
 * @hideinitializer
 */
#define WATCH_PTR(variable) createWatch_cPolymorphicPtr(#variable,(variable))
//@}

#endif


