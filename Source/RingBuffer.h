//
//  RingBuffer.h
//  RingBuffer
//
//  Created by Tim Arterbury on 4/20/17.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <memory>

/** A circular, lock-free buffer for multiple channels of audio.
 
    Supports a single writer (producer) and any number of readers (consumers).
 
    Make sure that the number of samples read from the RingBuffer in every
    readSamples() call is less than the bufferSize specified in the constructor.
 
    Also, ensure that the number of samples read from the RingBuffer at any time
    plus the number of samples written to the RingBuffer at any time never exceed
    the buffer size. This prevents read/write overlap.
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
        
        audioBuffer = std::make_unique<AudioBuffer<Type>> (numChannels, bufferSize);
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
            const int curWritePosition = writePosition.get();
            
            // If we need to loop around the ring
            if (curWritePosition + numSamples > bufferSize - 1)
            {
                int samplesToEdgeOfBuffer = bufferSize - curWritePosition;
                
                audioBuffer->copyFrom (i, curWritePosition, newAudioData, i,
                                       startSample, samplesToEdgeOfBuffer);
                
                audioBuffer->copyFrom (i, 0, newAudioData, i,
                                       startSample + samplesToEdgeOfBuffer,
                                       numSamples - samplesToEdgeOfBuffer);
            }
            // If we stay inside the ring
            else
            {
                audioBuffer->copyFrom (i, curWritePosition, newAudioData, i,
                                       startSample, numSamples);
            }
        }
        
        writePosition += numSamples;
        writePosition = writePosition.get() % bufferSize;
        
        /*
            Although it would seem that the above two lines could cause a
            problem for the consumer calling readsSamples() since it uses the
            value of writePosition to calculate the readPosition, this is not
            the case.
            
            1.  Since writePosition is Atomic, there will be no torn reads.
         
            2.  In the case that the producer only executes the line:
        
                    writePosition += numSamples;
         
                then writePosition has the posibility of being outside the
                bounds of the buffer. If the consumer then calls readsSamples()
                and executes the line:
         
                    int readPosition = (writePosition.get() % bufferSize) - readSize;
         
                it protects itself from an out-of-bounds writePosition by
                calculating the modulus with the buffer size. If the
                writePosition is outside the bounds of the buffer momentarily,
                the consumer's call to readSamples() stays protected and accurate.
         */
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
        // Ensure readSize does not exceed bufferSize
        jassert (readSize < bufferSize);
        
        /*
            Further, as stated in the class comment, it is also bad to have a
            read zone that overlaps with the writing zone, but this wont't cause
            too much of a visual problem unless it happens often.
            To combat this, the user should avoid having any read size and any
            write size that when added together, exceed the bufferSize of the
            RingBuffer.
         */
        
        // Calculate readPosition based on writePosition
        int readPosition = (writePosition.get() % bufferSize) - readSize;
        
        // If read position goes into negative bounds, loop it around the ring
        if (readPosition < 0)
            readPosition = bufferSize + readPosition;
        
        for (int i = 0; i < numChannels; ++i)
        {
            // If we need to loop around the ring
            if (readPosition + readSize > bufferSize - 1)
            {
                int samplesToEdgeOfBuffer = bufferSize - readPosition;
                
                bufferToFill.copyFrom (i, 0, *audioBuffer, i, readPosition,
                                       samplesToEdgeOfBuffer);
                
                bufferToFill.copyFrom (i, samplesToEdgeOfBuffer, *audioBuffer,
                                       i, 0,
                                       readSize - samplesToEdgeOfBuffer);
            }
            // If we stay inside the ring
            else
            {
                bufferToFill.copyFrom (i, 0, *audioBuffer, i, readPosition, readSize);
            }
        }
    }
    
private:
    int bufferSize;
    int numChannels;
    std::unique_ptr<AudioBuffer<Type>> audioBuffer;
    Atomic<int> writePosition; // This must be atomic so the conumer does
                               // not read it in a torn state as it is being
                               // changed.
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingBuffer)
};
