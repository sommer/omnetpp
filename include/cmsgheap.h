//==========================================================================
//  CMSGHEAP.H - part of
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//
//  Declaration of the following classes:
//    cMessageHeap : future event set, implemented as heap
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2005 Andras Varga

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __CMSGHEAP_H
#define __CMSGHEAP_H

#include "cobject.h"

class cMessage;


/**
 * Stores the future event set. The underlying data structure is heap;
 * the array used to store the heap expands as needed.
 *
 * @see Iterator
 * @ingroup Internals
 */
class SIM_API cMessageHeap : public cObject
{
  public:
    /**
     * Walks along a cMessageHeap. Note that objects in cMessageHeap are not
     * necessarily iterated ordered by arrival time. Use msgheap->sort()
     * if necessary before using the iterator.
     */
    class Iterator
    {
      private:
        cMessageHeap *q;
        int pos;

      public:
        /**
         * Constructor.
         */
        Iterator(const cMessageHeap& mh)  {q=const_cast<cMessageHeap*>(&mh);pos=1;}

        /**
         * Reinitializes the iterator object.
         */
        void init(const cMessageHeap& mh) {q=const_cast<cMessageHeap*>(&mh);pos=1;}

        /**
         * Returns the current object.
         */
        cMessage *operator()()      {return q->h[pos];}

        /**
         * Returns the current object, then moves the iterator to the next item.
         * If the iterator has reached the end of the list, NULL is returned.
         */
        cMessage *operator++(int)   {return pos<=q->n ? q->h[++pos] : NULL;}

        /**
         * Returns true if the iterator has reached the end of the list.
         */
        bool end() const   {return (bool)(pos>q->n);}
    };

    friend class Iterator;

  private:
    cMessage **h;             // pointer to the 'heap'  (h[0] always empty)
    int n;                    // number of elements in the heap
    int size;                 // size of allocated h array
    unsigned long insertcntr; // counts insertions

    // internal
    void shiftup(int from=1);

  public:
    /** @name Constructors, destructor, assignment */
    //@{

    /**
     * Copy constructor.
     */
    cMessageHeap(const cMessageHeap& msgq);

    /**
     * Constructor.
     */
    cMessageHeap(const char *name=NULL, int size=128);

    /**
     * Destructor.
     */
    virtual ~cMessageHeap();

    /**
     * Assignment operator. The name member doesn't get copied;
     * see cObject's operator=() for more details.
     */
    cMessageHeap& operator=(const cMessageHeap& msgqueue);
    //@}

    /** @name Redefined cObject member functions. */
    //@{

    /**
     * Creates and returns an exact copy of this object.
     * See cObject for more details.
     */
    virtual cPolymorphic *dup() const  {return new cMessageHeap(*this);}

    /**
     * Produces a one-line description of object contents into the buffer passed as argument.
     * See cObject for more details.
     */
    virtual std::string info() const;

    /**
     * Calls v->visit(this) for each contained object.
     * See cObject for more details.
     */
    virtual void forEachChild(cVisitor *v);

    // no netPack() and netUnpack()
    //@}

    /** @name Container functions. */
    //@{

    /**
     * Insert a new message into the heap.
     */
    void insert(cMessage *event);

    /**
     * Peek the first message in the heap (the one with the smallest timestamp.)
     * If the heap is empty, it returns NULL.
     */
    cMessage *peekFirst() const;

    /**
     * Removes and return the first message in the heap (the one
     * with the smallest timestamp.) If the heap is empty, it returns NULL.
     */
    cMessage *getFirst();

    /**
     * Removes and returns the given message in the heap. If the message is
     * not in the heap, returns NULL.
     */
    cMessage *get(cMessage *event);

    /**
     * Returns the mth message in the heap if 0 <= m < length(), and NULL
     * otherwise. Note that iteration does not necessarily return messages
     * in increasing timestamp (arrivalTime()) order unless you called
     * sort() before.
     */
    cMessage *peek(int m);

    /**
     * Sorts the contents of the heap. This is only necessary if one wants
     * to iterate through in the FES in strict timestamp order.
     */
    void sort();

    /**
     * Deletes all messages in the heap.
     */
    void clear();

    /**
     * Returns the number of messages in the heap.
     */
    int length() const {return n;}

    /**
     * Returns true if the heap is empty.
     */
    bool empty() const {return n==0;}
    //@}
};

#endif

