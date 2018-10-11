//
//  MTRingBuffer.hpp
//  MTAudioController
//
//  Created by Mihail Shevchuk on 08.08.17.
//  Copyright © 2017 Zeus Group LLP. All rights reserved.
//

#ifndef MTRingBuffer_hpp
#define MTRingBuffer_hpp

#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>

#include "MTAudioControllerGlobals.h"

/*!
 Provides a ring-buffer (or circular-buffer) that can be written to and read from
 asynchronously (blocking) or synchronously (non-blocking).
 
 The RingBuffer is an array of \b NumSlots slots of memory
 each of which is of size \b SlotSize bytes (8-bits). Slots can be read and
 written asynchronously/synchronously by multiple threads.
*/
class MTRingBuffer {
public:
    /*!
     The class constructor.
     @param SlotSize Size of one slot in bytes.
     @param NumSlots Number of slots.
    */
    MTRingBuffer(int SlotSize, int NumSlots);
    
    /*! The class destructor. */
    virtual ~MTRingBuffer();
    
    /*!
     Insert a slot into the RingBuffer from ptrToSlot. This method will block 
     until there's space in the buffer.
     
     The caller is responsible to make sure sizeof(WriteSlot) = SlotSize. This
     method should be use when the caller can block against its output, like
     sending/receiving UDP packets. It shouldn't be used by audio. For that, use the
     insertSlotNonBlocking.
     @param ptrToSlot Pointer to slot to insert into the RingBuffer.
    */
    void insertSlotBlocking(const byte* ptrToSlot);
    
    /*!
     Read a slot from the RingBuffer into ptrToReadSlot. This method will block until
     there's space in the buffer.
     
     The caller is responsible to make sure sizeof(ptrToReadSlot) = SlotSize. This
     method should be use when the caller can block against its input, like
     sending/receiving UDP packets. It shouldn't be used by audio. For that, use the
     readSlotNonBlocking.
     @param ptrToReadSlot Pointer to read slot from the RingBuffer.
    */
    void readSlotBlocking(byte* ptrToReadSlot);
    
    /*!
     Same as insertSlotBlocking but non-blocking (asynchronous)
     @param ptrToSlot Pointer to slot to insert into the RingBuffer.
    */
    void insertSlotNonBlocking(const byte* ptrToSlot);
    
    /*!
     Same as readSlotBlocking but non-blocking (asynchronous)
     @param ptrToReadSlot Pointer to read slot from the RingBuffer.
    */
    void readSlotNonBlocking(byte* ptrToReadSlot);
    
protected:
    /*!
     Sets the memory in the Read Slot when uderrun occurs. By default,
     this sets it to 0. Override this method in a subclass for a different behavior.
     @param ptrToReadSlot Pointer to read slot from the RingBuffer.
    */
    virtual void setUnderrunReadSlot(byte* ptrToReadSlot);
    
private:
    /*! Resets the ring buffer for reads under-runs non-blocking. */
    void underrunReset();
    
    /*! Resets the ring buffer for writes over-flows non-blocking. */
    void overflowReset();
    
    /*! Helper method to debug, prints member variables to terminal. */
    void debugDump() const;
    
    const int mSlotSize;   // The size of one slot in byes
    const int mNumSlots;   // Number of Slots
    const int mTotalSize;  // Total size of the mRingBuffer = mSlotSize*mNumSlotss
    int mReadPosition;     // Read Positions in the RingBuffer (Tail)
    int mWritePosition;    // Write Position in the RingBuffer (Head)
    int mFullSlots;        // Number of used (full) slots, in slot-size
    byte* mRingBuffer;     // 8-bit array of data (1-byte)
    byte* mLastReadSlot;   // Last slot read
    
    // Thread Synchronization Private Members
    QMutex mMutex;                    // Mutex to protect read and write operations
    QWaitCondition mBufferIsNotFull;  // Buffer not full condition to monitor threads
    QWaitCondition mBufferIsNotEmpty; // Buffer not empty condition to monitor threads
};

#endif /* MTRingBuffer_hpp */
