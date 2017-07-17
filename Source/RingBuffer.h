//
//  RingBuffer.h
//  RingBuffer
//
//  Created by Tim Arterbury on 4/20/17.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <assert.h>

/** A RingBuffer for multiple channels of audio. Currenly this only safely
    supports one reader and one writer. If I later add multiple readers, locks
    will be needed, since the read position would be moved around in an incorrect
    fashion otherwise.
 
    Potentially I could make a slightly more optimized solution using a single
    dimensional array and then just access it using arithmetic. But with compiler
    optimizations and stuff, this is probably fine. Plus it is easier to read
    this way.
*/
template <class Type>
class RingBuffer
{
public:
    
    /** Initializes the RingBuffer with the specified channels and size.
     */
    RingBuffer (int numChannels, int bufferSize)
    {
        this->bufferSize = bufferSize;
        this->numChannels = numChannels;
        audioBuffer = new AudioBuffer<Type> (numChannels, bufferSize);
        
        writePosition = 0;
        readPosition = 0;
    }
    
    
    /** Writes samples to all channels in the RingBuffer.
     */
    void writeSamples (AudioBuffer<Type> & newAudioData, int startSample, int numSamples)
    {
        int tempWritePos = writePosition.get();
        
        for (int i = 0; i < numChannels; ++i)
        {
            // If we need to loop around the ring
            if (tempWritePos + numSamples > bufferSize - 1)
            {
                int samplesToEdgeOfBuffer = bufferSize - tempWritePos;
                
                audioBuffer->copyFrom (i, tempWritePos, newAudioData, i,
                                       startSample, samplesToEdgeOfBuffer);
                
                audioBuffer->copyFrom (i, 0, newAudioData, i,
                                       startSample + samplesToEdgeOfBuffer,
                                       numSamples - samplesToEdgeOfBuffer);
            }
            // If we stay inside the ring
            else
            {
                audioBuffer->copyFrom (i, tempWritePos,
                                       newAudioData, i, startSample, numSamples);
            }
        }
        
        writePosition += numSamples;
        writePosition = writePosition.get() % bufferSize;
    }
    
    /** Reads readSize number of samples in front of the write position from all
        channels in the RingBuffer into the bufferToFill.
    */
    void readSamples (AudioBuffer<Type> & bufferToFill, int readSize)
    {
        assert (readSize < bufferSize); // Still bad to have a read size that overlaps with writing position though
        //assert (channel < numChannels && channel >= 0);
        
        int tempWritePos = writePosition.get();
        
        // Calculate readPosition based on write position
        readPosition = tempWritePos - readSize;
        
        if (readPosition.get() < 0)
            readPosition = bufferSize + readPosition.get();
        else
            readPosition = readPosition.get() % bufferSize;
        
        int tempReadPos = readPosition.get();
        
        for (int i = 0; i < numChannels; ++i)
        {
            // If we need to loop around the ring
            if (tempReadPos + readSize > bufferSize - 1)
            {
                int samplesToEdgeOfBuffer = bufferSize - tempReadPos;
                
                bufferToFill.copyFrom (i, 0, *(audioBuffer.get()), i, tempReadPos,
                                       samplesToEdgeOfBuffer);
                
                bufferToFill.copyFrom (i, samplesToEdgeOfBuffer, *(audioBuffer.get()), i,
                                       tempReadPos + samplesToEdgeOfBuffer,
                                       readSize - samplesToEdgeOfBuffer);
            }
            // If we stay inside the ring
            else
            {
                bufferToFill.copyFrom (i, 0, *(audioBuffer.get()), i, tempReadPos, readSize);
            }
        }
    }
    
private:
    
    int bufferSize;
    int numChannels;
    ScopedPointer<AudioBuffer<Type>> audioBuffer;
    
    Atomic<int> readPosition;
    Atomic<int> writePosition;
};
