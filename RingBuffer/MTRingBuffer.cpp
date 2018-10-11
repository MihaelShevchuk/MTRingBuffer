//
//  MTRingBuffer.cpp
//  MTAudioController
//
//  Created by Mihail Shevchuk on 08.08.17.
//  Copyright © 2017 Zeus Group LLP. All rights reserved.
//

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "MTRingBuffer.hpp"

//******************************************************************************
MTRingBuffer::MTRingBuffer(int SlotSize, int NumSlots) :
mSlotSize     (SlotSize),
mNumSlots     (NumSlots),
mTotalSize    (mSlotSize * mNumSlots),
mReadPosition (0),
mWritePosition(0),
mFullSlots    (0),
mRingBuffer   (new byte[mTotalSize]),
mLastReadSlot (new byte[mSlotSize]) {
    // Verify if there's enough space to for the buffers
    if ((mRingBuffer == NULL) || (mLastReadSlot == NULL)) {
        throw std::length_error("RingBuffer out of memory!");
    }
    // Set the buffers to zeros
    std::memset(mRingBuffer,   0, mTotalSize); // set buffer to 0
    std::memset(mLastReadSlot, 0, mSlotSize);  // set buffer to 0

    // Advance write position to half of the RingBuffer
    mWritePosition = ( (NumSlots / 2) * SlotSize ) % mTotalSize;
    
    // Udpate Full Slots accordingly
    mFullSlots = (NumSlots / 2);
}

//******************************************************************************
MTRingBuffer::~MTRingBuffer() {
    // Free memory
    delete[] mRingBuffer;
    delete[] mLastReadSlot;
    
    // Clear to prevent using invalid memory reference
    mRingBuffer = NULL;
    mLastReadSlot = NULL;
}

//******************************************************************************
void MTRingBuffer::insertSlotBlocking(const byte* ptrToSlot) {
    // Lock the mutex
    QMutexLocker locker(&mMutex);
    
    // Check if there is space available to write a slot
    // If the Ringbuffer is full, it waits for the bufferIsNotFull condition
    while (mFullSlots == mNumSlots) {
        mBufferIsNotFull.wait(&mMutex);
    }
    
    // Copy mSlotSize bytes to mRingBuffer
    std::memcpy(mRingBuffer + mWritePosition, ptrToSlot, mSlotSize);
    
    // Update write position
    mWritePosition = (mWritePosition + mSlotSize) % mTotalSize;
    mFullSlots++; //update full slots
    
    // Wake threads waitng for bufferIsNotFull condition
    mBufferIsNotEmpty.wakeAll();
}

//******************************************************************************
void MTRingBuffer::readSlotBlocking(byte* ptrToReadSlot) {
    // Lock the mutex
    QMutexLocker locker(&mMutex);
    
    // Check if there are slots available to read
    // If the Ringbuffer is empty, it waits for the bufferIsNotEmpty condition
    while (mFullSlots == 0) {
        mBufferIsNotEmpty.wait(&mMutex);
    }
    
    // Copy mSlotSize bytes to ReadSlot
    std::memcpy(ptrToReadSlot, mRingBuffer + mReadPosition, mSlotSize);
    
    // Always save memory of the last read slot
    std::memcpy(mLastReadSlot, mRingBuffer + mReadPosition, mSlotSize);
    
    // Update write position
    mReadPosition = (mReadPosition + mSlotSize) % mTotalSize;
    mFullSlots--; //update full slots
    
    // Wake threads waitng for bufferIsNotFull condition
    mBufferIsNotFull.wakeAll();
}

//******************************************************************************
void MTRingBuffer::insertSlotNonBlocking(const byte* ptrToSlot) {
    // Lock the mutex
    QMutexLocker locker(&mMutex);
    
    /* Check if there is space available to write a slot
     If the Ringbuffer is full, it returns without writing anything
     and resets the buffer
    */
    if (mFullSlots == mNumSlots) {
        overflowReset();
        return;
    }
    
    // Copy mSlotSize bytes to mRingBuffer
    std::memcpy(mRingBuffer + mWritePosition, ptrToSlot, mSlotSize);
    
    // Update write position
    mWritePosition = (mWritePosition + mSlotSize) % mTotalSize;
    mFullSlots++; //update full slots
    
    // Wake threads waitng for bufferIsNotFull condition
    mBufferIsNotEmpty.wakeAll();
}

//*******************************************************************************
void MTRingBuffer::readSlotNonBlocking(byte* ptrToReadSlot) {
    // Lock the mutex
    QMutexLocker locker(&mMutex);
    
    /* 
     Check if there are slots available to read
     If the Ringbuffer is empty, it returns a buffer of zeros and rests the buffer
    */
    if (mFullSlots == 0) {
        // Returns a buffer of zeros if there's nothing to read
        setUnderrunReadSlot(ptrToReadSlot);
        underrunReset();
        return;
    }
    // Copy mSlotSize bytes to ReadSlot
    std::memcpy(ptrToReadSlot, mRingBuffer + mReadPosition, mSlotSize);
    
    // Always save memory of the last read slot
    std::memcpy(mLastReadSlot, mRingBuffer + mReadPosition, mSlotSize);
    
    // Update write position
    mReadPosition = (mReadPosition + mSlotSize) % mTotalSize;
    mFullSlots--; //update full slots
    
    // Wake threads waitng for bufferIsNotFull condition
    mBufferIsNotFull.wakeAll();
}

//******************************************************************************
void MTRingBuffer::setUnderrunReadSlot(byte* ptrToReadSlot) {
    std::memset(ptrToReadSlot, 0, mSlotSize);
}

//******************************************************************************
// Under-run happens when there's nothing to read.
void MTRingBuffer::underrunReset() {
    // There's nothing new to read, so we clear the whole buffer (Set the entire buffer to 0)
    std::memset(mRingBuffer, 0, mTotalSize);
}

//******************************************************************************
// Over-flow happens when there's no space to write more slots.
void MTRingBuffer::overflowReset() {
    // Advance the read pointer 1/2 the ring buffer
    mReadPosition = ( mReadPosition + ( (mNumSlots/2) * mSlotSize ) ) % mTotalSize;
    mFullSlots -= mNumSlots/2;
}

//******************************************************************************
void MTRingBuffer::debugDump() const {
    std::cout << "mTotalSize = "     << mTotalSize     << std::endl;
    std::cout << "mReadPosition = "  << mReadPosition  << std::endl;
    std::cout << "mWritePosition = " << mWritePosition << std::endl;
    std::cout <<  "mFullSlots = "    << mFullSlots     << std::endl;
}
