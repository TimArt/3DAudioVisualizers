/**
    Graphics Final Project - Audio Visualizer Suite
    Tim Arterbury & Chicago Velez
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Oscilloscope2D.h"
#include "Oscilloscope.h"
#include "Spectrum.h"
#include "RingBuffer.h"


/** The MainContentComponent is the component that holds all the buttons and
    visualizers. This component fills the entire window.
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
        
        addAndMakeVisible (&oscilloscope2DButton);
        oscilloscope2DButton.setButtonText ("2D Oscilloscope");
        oscilloscope2DButton.setColour (TextButton::buttonColourId, Colour (0xFF0C4B95));
        oscilloscope2DButton.addListener (this);
        oscilloscope2DButton.setToggleState (false, NotificationType::dontSendNotification);
        
        
        addAndMakeVisible(&oscilloscope3DButton);
        oscilloscope3DButton.setButtonText ("3D Oscilloscope");
        oscilloscope3DButton.setColour (TextButton::buttonColourId, Colour (0xFF0C4B95));
        oscilloscope3DButton.addListener (this);
        oscilloscope3DButton.setToggleState (false, NotificationType::dontSendNotification);
        
        
        addAndMakeVisible(&spectrumButton);
        spectrumButton.setButtonText ("Spectrum");
        spectrumButton.setColour (TextButton::buttonColourId, Colour (0xFF0C4B95));
        spectrumButton.addListener (this);
        spectrumButton.setToggleState (false, NotificationType::dontSendNotification);
        
        
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
        
        
        // Allocate all Visualizers
        
        oscilloscope2D = new Oscilloscope2D (ringBuffer);
        addChildComponent (oscilloscope2D);
        //addAndMakeVisible (oscilloscope2D);
        //oscilloscope3D->start();
        
        oscilloscope3D = new Oscilloscope (ringBuffer);
        addChildComponent (oscilloscope3D);
        //addAndMakeVisible (oscilloscope3D);
        //oscilloscope3D->start();
        
        spectrum = new Spectrum (ringBuffer);
        addChildComponent (spectrum);
        //addAndMakeVisible (spectrum);
        //spectrum->start();
    }
    
    /** Called after rendering Audio. 
    */
    void releaseResources() override
    {
        // Delete all visualizer allocations
        if (oscilloscope2D != nullptr)
        {
            oscilloscope2D->stop();
            delete oscilloscope2D;
        }
        
        if (oscilloscope3D != nullptr)
        {
            oscilloscope3D->stop();
            delete oscilloscope3D;
        }
        
        if (spectrum != nullptr)
        {
            spectrum->stop();
            delete spectrum;
        }
        
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
        
        // Button Width
        int buttonWidth = (w - 30) / 2;

        openButton.setBounds (10, 10, buttonWidth, 20);
        playButton.setBounds (10, 40, buttonWidth, 20);
        stopButton.setBounds (10, 70, buttonWidth, 20);
        
        oscilloscope2DButton.setBounds (buttonWidth + 20, 10, buttonWidth, 20);
        oscilloscope3DButton.setBounds (buttonWidth + 20, 40, buttonWidth, 20);
        spectrumButton.setBounds (buttonWidth + 20, 70, buttonWidth, 20);
        
        if (oscilloscope2D != nullptr)
            oscilloscope2D->setBounds (0, 100, w, h - 100);
        if (oscilloscope3D != nullptr)
            oscilloscope3D->setBounds (0, 100, w, h - 100);
        if (spectrum != nullptr)
            spectrum->setBounds (0, 100, w, h - 100);
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
        
        if (button == &oscilloscope2DButton)
        {
            button->setToggleState (true, NotificationType::dontSendNotification);
            oscilloscope3DButton.setToggleState (false, NotificationType::dontSendNotification);
            spectrumButton.setToggleState (false, NotificationType::dontSendNotification);
            
            oscilloscope2D->setVisible(true);
            oscilloscope3D->setVisible(false);
            spectrum->setVisible(false);
            
            
            oscilloscope2D->start();
            oscilloscope3D->stop();
            spectrum->stop();
        }
        
        if (button == &oscilloscope3DButton)
        {
            button->setToggleState (true, NotificationType::dontSendNotification);
            oscilloscope2DButton.setToggleState (false, NotificationType::dontSendNotification);
            spectrumButton.setToggleState (false, NotificationType::dontSendNotification);
            
            oscilloscope2D->setVisible(false);
            oscilloscope3D->setVisible(true);
            spectrum->setVisible(false);
            
            oscilloscope3D->start();
            oscilloscope2D->stop();
            spectrum->stop();
        }
        
        if (button == &spectrumButton)
        {
            button->setToggleState (true, NotificationType::dontSendNotification);
            oscilloscope2DButton.setToggleState (false, NotificationType::dontSendNotification);
            oscilloscope3DButton.setToggleState (false, NotificationType::dontSendNotification);
            
            oscilloscope2D->setVisible(false);
            oscilloscope3D->setVisible(false);
            spectrum->setVisible(true);
            
            spectrum->start();
            oscilloscope3D->stop();
            oscilloscope2D->stop();
        }
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
    
    TextButton oscilloscope2DButton;
    TextButton oscilloscope3DButton;
    TextButton spectrumButton;
    
    // Audio File Reading Variables
    AudioFormatManager formatManager;
    ScopedPointer<AudioFormatReaderSource> audioReaderSource;
    AudioTransportSource audioTransportSource;
    AudioTransportState audioTransportState;
    
    // Audio & GL Audio Buffer
    RingBuffer<float> * ringBuffer;
    
    // Visualizers
    Oscilloscope2D * oscilloscope2D;
    Oscilloscope * oscilloscope3D;
    Spectrum * spectrum;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()    { return new MainContentComponent(); }
