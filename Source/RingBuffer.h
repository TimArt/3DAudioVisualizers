//
//  RingBuffer.h
//  RingBuffer
//
//  Created by Tim Arterbury on 4/20/17.
//  Copyright © 2017 TimArt. All rights reserved.
//

#ifndef RingBuffer_h
#define RingBuffer_h

#include "../JuceLibraryCode/JuceHeader.h"
#include <assert.h>

/** A RingBuffer for multiple channels of audio. Currenly this only safely supports one reader and one writer.
    If we later add multiple readers, locks will be needed.
 
    Potentially we could make a slightly more lightweight solution using a single
    dimensional array and then just access it using arithmetic. But with compiler
    optimizations and stuff, this is probably fine.
*/
template <class T>
class RingBuffer
{
public:
    
    /** Initializes the RingBuffer with the specified size.
     */
    RingBuffer (int bufferSize, int numChannels)
    {
        size = bufferSize;
        this->numChannels = numChannels;
        data = new T* [numChannels];
        for (int i = 0; i < numChannels; ++i)
        {
            data[i] = new T [size];
        }
        writePosition = 0;
        readPosition = 0;
    }
    
    /** Deallocates data from the ring buffer.
    */
    ~RingBuffer ()
    {
        if (data != nullptr)
        {
            for (int i = 0; i < numChannels; ++i)
            {
                delete [] data[i];
            }
            delete [] data;
        }
    }
    
    /** Writes samples to a channel in the RingBuffer.
     */
    // Not sure if I'm writing completely correctly.
    // Data appears in both channels, but when dumping all the data out for both
    // channels, I still get a decent zone of zeroes. (this might just be the samples
    // themselves, but that seems like alot, i dunno?
    // This is consistent for both channels, meaning the general writing method
    // is probably off a bit
    void writeSamples (const T * newData, int numSamples, int channel)
    {
        int tempWritePos = writePosition.get();
        
        // If we need to loop around the ring
        if (tempWritePos + numSamples > size)
        {
            int toEdgeBufferSize = size - tempWritePos;
            memcpy (data[channel] + tempWritePos, newData, toEdgeBufferSize * sizeof (T) );
            memcpy (data[channel], newData + toEdgeBufferSize, (numSamples - toEdgeBufferSize) * sizeof (T));
        }
        // If we stay inside the ring
        else
        {
            memcpy (data[channel] + tempWritePos, newData, numSamples * sizeof (T));
        }
        writePosition += numSamples;
        writePosition = writePosition.get() % size;
    }
    
    /** Reads readSize ammount of data in front of the write position in the
        RingBuffer.
     
        Returns a pointer to an array of samples but it MUST be deleted.
    */
    T * readSamples (int readSize, int channel)
    {

        // This outputs samples correctly for both channels,
        // but this function still does not return the correct buffer for
        // channel 0. wut is happening??
        /*for (int j = 0; j < size; ++j)
        {
            std::cout << data[0][j];
        }*/
        
        assert(readSize < size); // Still bad to have a read size that overlaps with writing position though
        assert (channel < numChannels && channel >= 0);
        
        int tempWritePos = writePosition.get();
        
        // Calculate readPosition based on write position
        readPosition = tempWritePos - readSize;
        if (readPosition.get() < 0)
            readPosition = size + readPosition.get();
        else
            readPosition = readPosition.get() % size;
        
        // Declare sample array to return
        T * samplesToRead = new T [readSize];
        
        int tempReadPos = readPosition.get();
        // If we need to loop around the ring
        if (tempReadPos + readSize > size)
        {
            int toEdgeBufferSize = size - tempReadPos;
            memcpy (samplesToRead, data[channel] + tempReadPos, toEdgeBufferSize * sizeof (T) );
            memcpy (samplesToRead + toEdgeBufferSize, data[channel], (readSize - toEdgeBufferSize) * sizeof (T));
        }
        // If we stay inside the ring
        else
        {
            memcpy (samplesToRead, data[channel] + tempReadPos, readSize * sizeof (T));
        }
        
        return samplesToRead;
    }
    
private:
    
    int size;
    int numChannels;
    T ** data;
    
    Atomic<int> readPosition;
    Atomic<int> writePosition;
};

#endif /* RingBuffer_h */
