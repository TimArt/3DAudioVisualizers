/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Oscilloscope1.h"
#include "Oscilloscope.h"
#include "RingBuffer.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   :  public AudioAppComponent,
                                public ChangeListener,
                                public Button::Listener
{
public:
    //==============================================================================
    MainContentComponent()
    {   
        // Setup Audio
        audioTransportState = AudioTransportState::Stopped;
        formatManager.registerBasicFormats();
        audioTransportSource.addChangeListener (this);
        setAudioChannels (2, 2); // Initially Stereo Input to Stereo Output
        
        // Setup GUI
        addAndMakeVisible (&openButton);
        openButton.setButtonText ("Open...");
        openButton.addListener (this);
        
        addAndMakeVisible (&playButton);
        playButton.setButtonText ("Play");
        playButton.addListener (this);
        playButton.setColour (TextButton::buttonColourId, Colours::green);
        playButton.setEnabled (false);

        addAndMakeVisible (&stopButton);
        stopButton.setButtonText ("Stop");
        stopButton.addListener (this);
        stopButton.setColour (TextButton::buttonColourId, Colours::red);
        stopButton.setEnabled (false);
        
        //addAndMakeVisible (&oscilloscope);
        //oscilloscope.start();
        
        setSize (800, 600); // Set Component Size
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }
    
    //==============================================================================
    // Audio Callbacks
    
    /** Called before rendering Audio. 
    */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        // Setup Audio Source
        audioTransportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
        
        // Setup Ring Buffer of GLfloat's for the visualizer to use
        ringBuffer = new RingBuffer<GLfloat> (samplesPerBlockExpected * 10, 2);  // Set default of 2 channels in the RingBuffer
        
        // Setup Visualizers
        //oscilloscope1 = new Oscilloscope1 (ringBuffer);
        //addAndMakeVisible (oscilloscope1);
        //oscilloscope1->start();
        
        oscilloscope = new Oscilloscope (ringBuffer);
        addAndMakeVisible (oscilloscope);
        oscilloscope->start();
    }
    
    /** Called after rendering Audio. 
    */
    void releaseResources() override
    {
        // DELETE Oscilloscope here (so make it not a scoped pointer so this works
        /*if (oscilloscope1 != nullptr)
        {
            oscilloscope1->stop();
        }
  
        delete oscilloscope1;*/
        
        if (oscilloscope != nullptr)
        {
            oscilloscope->stop();
        }
        
        delete oscilloscope;
        
        audioTransportSource.releaseResources();
        delete ringBuffer;
    }
    
    /** The audio rendering callback.
    */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        if (audioReaderSource == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        
        audioTransportSource.getNextAudioBlock (bufferToFill);
        
        // Write to Ring Buffer
        int numChannels = bufferToFill.buffer->getNumChannels();
        for (int i = 0; i < numChannels; ++i)
        {
            const float * audioData = bufferToFill.buffer->getReadPointer (i, bufferToFill.startSample);
            ringBuffer->writeSamples (audioData, bufferToFill.numSamples, i);
        }
    }
    
    
    //==============================================================================
    // GUI Callbacks
    
    /** Paints UI elements and various graphics on the screen. NOT OpenGL.
        This will draw on top of the OpenGL background.
     */
    void paint (Graphics& g) override
    {
        g.fillAll (Colour (0xFF252831)); // Set background color (below any GL Visualizers)
    }

    /** Resizes various JUCE Components (UI elements, etc) placed on the screen. NOT OpenGL.
    */
    void resized() override
    {
        const int w = getWidth();
        const int h = getHeight();

        openButton.setBounds (10, 10, w - 20, 20);
        playButton.setBounds (10, 40, w - 20, 20);
        stopButton.setBounds (10, 70, w - 20, 20);
        
        //oscilloscope1->setBounds (0, 100, w, h - 100);
        oscilloscope->setBounds (0, 100, w, h - 100);
    }
    
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &audioTransportSource)
        {
            if (audioTransportSource.isPlaying())
                changeAudioTransportState (Playing);
            else if ((audioTransportState == Stopping) || (audioTransportState == Playing))
                changeAudioTransportState (Stopped);
            else if (audioTransportState == Pausing)
                changeAudioTransportState (Paused);
        }
    }

    void buttonClicked (Button* button) override
    {
        if (button == &openButton)  openButtonClicked();
        if (button == &playButton)  playButtonClicked();
        if (button == &stopButton)  stopButtonClicked();
    }
    
private:
    //==============================================================================
    // PRIVATE MEMBERS
    
    /** Describes one of the states of the audio transport.
    */
    enum AudioTransportState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };
    
    /** Changes audio transport state.
    */
    void changeAudioTransportState (AudioTransportState newState)
    {
        if (audioTransportState != newState)
        {
            audioTransportState = newState;
            
            switch (audioTransportState)
            {
                case Stopped:
                    playButton.setButtonText ("Play");
                    stopButton.setButtonText ("Stop");
                    stopButton.setEnabled (false);
                    audioTransportSource.setPosition (0.0);
                    break;
                    
                case Starting:
                    audioTransportSource.start();
                    break;
                    
                case Playing:
                    playButton.setButtonText ("Pause");
                    stopButton.setButtonText ("Stop");
                    stopButton.setEnabled (true);
                    break;
                    
                case Pausing:
                    audioTransportSource.stop();
                    break;
                    
                case Paused:
                    playButton.setButtonText ("Resume");
                    stopButton.setButtonText ("Return to Zero");
                    break;
                    
                case Stopping:
                    audioTransportSource.stop();
                    break;
            }
        }
    }
    
    /** Triggered the openButton is clicked. It opens an audio file selected by the user.
    */
    void openButtonClicked()
    {
        FileChooser chooser ("Select a Wave file to play...",
                             File::nonexistent,
                             "*.wav");
        
        if (chooser.browseForFileToOpen())
        {
            File file (chooser.getResult());
            AudioFormatReader* reader = formatManager.createReaderFor (file);
            
            if (reader != nullptr)
            {
                ScopedPointer<AudioFormatReaderSource> newSource = new AudioFormatReaderSource (reader, true);
                audioTransportSource.setSource (newSource, 0, nullptr, reader->sampleRate);
                playButton.setEnabled (true);
                audioReaderSource = newSource.release();
            }
        }
    }
    

    void playButtonClicked()
    {
        if ((audioTransportState == Stopped) || (audioTransportState == Paused))
            changeAudioTransportState (Starting);
        else if (audioTransportState == Playing)
            changeAudioTransportState (Pausing);
    }
    
    void stopButtonClicked()
    {
        if (audioTransportState == Paused)
            changeAudioTransportState (Stopped);
        else
            changeAudioTransportState (Stopping);
    }

    
    //==============================================================================
    // PRIVATE MEMBER VARIABLES

    // GUI Buttons
    TextButton openButton;
    TextButton playButton;
    TextButton stopButton;
    
    // Audio File Reading Variables
    AudioFormatManager formatManager;
    ScopedPointer<AudioFormatReaderSource> audioReaderSource;
    AudioTransportSource audioTransportSource;
    AudioTransportState audioTransportState;
    
    // Audio & GL Audio Buffer
    RingBuffer<float> * ringBuffer;
    
    // Visualizers
    //Oscilloscope1 * oscilloscope1;
    
    Oscilloscope * oscilloscope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()    { return new MainContentComponent(); }
