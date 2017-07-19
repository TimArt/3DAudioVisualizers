//
//  RingBuffer.h
//  RingBuffer
//
//  Created by Tim Arterbury on 4/20/17.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

/** A circular buffer for multiple channels of audio.
 
    Supports a single writer and any number of readers.
    Make sure that the number of samples read from the RingBuffer in every
    readSamples() call is less than the buffer size specified in the constructor.
*/
template <class Type>
class RingBuffer
{
public:
    
    /** Initializes the RingBuffer with the specified channels and size.
     
        @param numChannels  number of channels of audio to store in buffer
        @param bufferSize   size of the audio buffer
     */
    RingBuffer (int numChannels, int bufferSize)
    {
        this->bufferSize = bufferSize;
        this->numChannels = numChannels;
        
        audioBuffer = new AudioBuffer<Type> (numChannels, bufferSize);
        writePosition = 0;
    }
    
    
    /** Writes samples to all channels in the RingBuffer.
     
        @param newAudioData     an audio buffer to write into the RingBuffer
                                This AudioBuffer must have the same number of
                                channels as specified in the RingBuffer's constructor.
        @param startSample      the starting sample in the newAudioData to write
                                into the RingBuffer
        @param numSamples       the number of samples from newAudioData to write
                                into the RingBuffer
     */
    void writeSamples (AudioBuffer<Type> & newAudioData, int startSample, int numSamples)
    {
        for (int i = 0; i < numChannels; ++i)
        {
            // If we need to loop around the ring
            if (writePosition + numSamples > bufferSize - 1)
            {
                int samplesToEdgeOfBuffer = bufferSize - writePosition;
                
                audioBuffer->copyFrom (i, writePosition, newAudioData, i,
                                       startSample, samplesToEdgeOfBuffer);
                
                audioBuffer->copyFrom (i, 0, newAudioData, i,
                                       startSample + samplesToEdgeOfBuffer,
                                       numSamples - samplesToEdgeOfBuffer);
            }
            // If we stay inside the ring
            else
            {
                audioBuffer->copyFrom (i, writePosition, newAudioData, i,
                                       startSample, numSamples);
            }
        }
        
        writePosition += numSamples;
        writePosition = writePosition % bufferSize;
    }
    
    /** Reads readSize number of samples in front of the write position from all
        channels in the RingBuffer into the bufferToFill.
     
         @param bufferToFill    buffer to be filled with most recent audio
                                samples from the RingBuffer
         @param readSize        number of samples to read from the RingBuffer.
                                Note, this must be less than the buffer size
                                of the RingBuffer specified in the constructor.
    */
    void readSamples (AudioBuffer<Type> & bufferToFill, int readSize)
    {
        jassert (readSize < bufferSize);    // Further, it is bad to have a read size that overlaps with writing position,
                                            // but wont't make too much of a visual problem unless it happens often.
                                            // to combat this, the buffer size of the RingBuffer should be larger than
                                            // the largest read size used.
        
        // Calculate readPosition based on write position
        int readPosition = writePosition - readSize;
        
        if (readPosition < 0)
            readPosition = bufferSize + readPosition;
        else
            readPosition = readPosition % bufferSize;
        
        for (int i = 0; i < numChannels; ++i)
        {
            // If we need to loop around the ring
            if (readPosition + readSize > bufferSize - 1)
            {
                int samplesToEdgeOfBuffer = bufferSize - readPosition;
                
                bufferToFill.copyFrom (i, 0, *(audioBuffer.get()), i, readPosition,
                                       samplesToEdgeOfBuffer);
                
                bufferToFill.copyFrom (i, samplesToEdgeOfBuffer, *(audioBuffer.get()),
                                       i, readPosition + samplesToEdgeOfBuffer,
                                       readSize - samplesToEdgeOfBuffer);
            }
            // If we stay inside the ring
            else
            {
                bufferToFill.copyFrom (i, 0, *(audioBuffer.get()), i, readPosition, readSize);
            }
        }
    }
    
private:
    int bufferSize;
    int numChannels;
    ScopedPointer<AudioBuffer<Type>> audioBuffer;
    int writePosition;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingBuffer)
};
