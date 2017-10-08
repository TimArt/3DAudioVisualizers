/**
    Graphics Final Project - Audio Visualizer Suite
    Tim Arterbury
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Oscilloscope2D.h"
#include "Oscilloscope3D.h"
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
    /*
        Future Cleanup:
            - Fix resize method cuz it coule be made simpler with rectangels.
            - Come up with more elegant way to start/stop these visualizers. A
              signle function to eliminate all the code reuse I have in the
              button callbacks and such. The code is very hairy.
     */
    
    //==============================================================================
    MainContentComponent() : audioIOSelector(deviceManager, 1, 2, 0, 0, false, false, true, true)
    {
        audioFileModeEnabled = false;
        audioInputModeEnabled = false;
        
        // Setup Audio
        audioTransportState = AudioTransportState::Stopped;
        formatManager.registerBasicFormats();
        audioTransportSource.addChangeListener (this);
        setAudioChannels (2, 2); // Initially Stereo Input to Stereo Output
        
        // Setup GUI
        addAndMakeVisible (&openFileButton);
        openFileButton.setButtonText ("Open File");
        openFileButton.addListener (this);
        
        addAndMakeVisible (&audioInputButton);
        audioInputButton.setButtonText ("Audio Input");
        audioInputButton.addListener (this);
        
        addAndMakeVisible(&showIOSelectorButton);
        showIOSelectorButton.setButtonText("IO");
        showIOSelectorButton.addListener (this);
        
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
        
        addChildComponent(audioIOSelector);
        
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
        // Uses two channels
        ringBuffer = new RingBuffer<GLfloat> (2, samplesPerBlockExpected * 10);
        
        
        // Allocate all Visualizers
        
        oscilloscope2D = new Oscilloscope2D (ringBuffer);
        addChildComponent (oscilloscope2D);
        
        oscilloscope3D = new Oscilloscope3D (ringBuffer);
        addChildComponent (oscilloscope3D);
        
        spectrum = new Spectrum (ringBuffer);
        addChildComponent (spectrum);
    }
    
    /** Called after rendering Audio. 
    */
    void releaseResources() override
    {
        // Delete all visualizer allocations
        if (oscilloscope2D != nullptr)
        {
            oscilloscope2D->stop();
            removeChildComponent (oscilloscope2D);
            delete oscilloscope2D;
        }
        
        if (oscilloscope3D != nullptr)
        {
            oscilloscope3D->stop();
            removeChildComponent (oscilloscope3D);
            delete oscilloscope3D;
        }
        
        if (spectrum != nullptr)
        {
            spectrum->stop();
            removeChildComponent (spectrum);
            delete spectrum;
        }
        
        audioTransportSource.releaseResources();
        delete ringBuffer;
    }
    
    /** The audio rendering callback.
    */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // If no mode is enabled, do not mess with audio
        if (!audioFileModeEnabled && !audioInputModeEnabled)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        
        if (audioFileModeEnabled)
            audioTransportSource.getNextAudioBlock (bufferToFill);
        
        // Write to Ring Buffer
        ringBuffer->writeSamples (*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
        
        // If using mic input, clear the output so the mic input is not audible
        if (audioInputModeEnabled)
            bufferToFill.clearActiveBufferRegion();
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
        
        // Button Dimenstions
        const int bWidth = (w - 30) / 2;
        const int bHeight = 20;
        const int bMargin = 10;
        
        const int smallBWidth = bWidth / 2 - bMargin / 2;

        openFileButton.setBounds (bMargin, bMargin, smallBWidth, bHeight);
        audioInputButton.setBounds (1.5f * bMargin + bWidth / 2, bMargin, (smallBWidth * 2/3) - bMargin / 2, bHeight);
        showIOSelectorButton.setBounds ((1.5f * bMargin + bWidth / 2) + (smallBWidth * 2/3) + bMargin / 2, bMargin, (smallBWidth / 3) - bMargin / 2, bHeight);
        playButton.setBounds (bMargin, 40, bWidth, 20);
        stopButton.setBounds (bMargin, 70, bWidth, 20);
        
        oscilloscope2DButton.setBounds (bWidth + 2 * bMargin, bMargin, bWidth, bHeight);
        oscilloscope3DButton.setBounds (bWidth + 2 * bMargin, 40, bWidth, bHeight);
        spectrumButton.setBounds (bWidth + 2 * bMargin, 70, bWidth, bHeight);
        
        //Rectangle<int> ioSelectorBounds (bWidth + bMargin, 0, w - (bWidth + bMargin), 100);
        //audioIOSelector.setBounds(ioSelectorBounds);
        audioIOSelector.setBounds (0, 100, w, h - 100);
        
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
        if (button == &openFileButton)  openFileButtonClicked();
        else if (button == &audioInputButton) audioInputButtonClicked();
        else if (button == &playButton)  playButtonClicked();
        else if (button == &stopButton)  stopButtonClicked();
        else if (button == &showIOSelectorButton) showIOSelectorButtonClicked();
        
        else if (button == &oscilloscope2DButton)
        {
            bool buttonToggleState = !button->getToggleState();
            button->setToggleState (buttonToggleState, NotificationType::dontSendNotification);
            oscilloscope3DButton.setToggleState (false, NotificationType::dontSendNotification);
            spectrumButton.setToggleState (false, NotificationType::dontSendNotification);
            
            audioIOSelector.setVisible(false);
            oscilloscope2D->setVisible(buttonToggleState);
            oscilloscope3D->setVisible(false);
            spectrum->setVisible(false);
            
            oscilloscope2D->start();
            oscilloscope3D->stop();
            spectrum->stop();
            resized();
        }
        
        else if (button == &oscilloscope3DButton)
        {
            bool buttonToggleState = !button->getToggleState();
            button->setToggleState (buttonToggleState, NotificationType::dontSendNotification);
            oscilloscope2DButton.setToggleState (false, NotificationType::dontSendNotification);
            spectrumButton.setToggleState (false, NotificationType::dontSendNotification);
            
            audioIOSelector.setVisible(false);
            oscilloscope2D->setVisible(false);
            oscilloscope3D->setVisible(buttonToggleState);
            spectrum->setVisible(false);
            
            oscilloscope3D->start();
            oscilloscope2D->stop();
            spectrum->stop();
            resized();
        }
        
        else if (button == &spectrumButton)
        {
            bool buttonToggleState = !button->getToggleState();
            button->setToggleState (buttonToggleState, NotificationType::dontSendNotification);
            oscilloscope2DButton.setToggleState (false, NotificationType::dontSendNotification);
            oscilloscope3DButton.setToggleState (false, NotificationType::dontSendNotification);
            
            audioIOSelector.setVisible(false);
            oscilloscope2D->setVisible(false);
            oscilloscope3D->setVisible(false);
            spectrum->setVisible(buttonToggleState);
            
            spectrum->start();
            oscilloscope3D->stop();
            oscilloscope2D->stop();
            resized();
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
    
    /** Triggered when the openButton is clicked. It opens an audio file selected by the user.
    */
    void openFileButtonClicked()
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
                audioInputModeEnabled = false;
                audioFileModeEnabled = true;
            }
        }
    }
    
    /** Triggered when the Mic Input (Audio Input Button) is clicked. It pulls
        audio from the computer's first two audio inputs.
     */
    void audioInputButtonClicked()
    {
        changeAudioTransportState(AudioTransportState::Stopping);
        changeAudioTransportState(AudioTransportState::Stopped);
        
        audioFileModeEnabled = false;
        audioInputModeEnabled = true;
        
        playButton.setEnabled (false);
        stopButton.setEnabled (false);
    }
    
    void showIOSelectorButtonClicked()
    {
        oscilloscope2DButton.setToggleState(false, NotificationType::dontSendNotification);
        oscilloscope3DButton.setToggleState(false, NotificationType::dontSendNotification);
        spectrumButton.setToggleState(false, NotificationType::dontSendNotification);
        
        bool audioIOShouldBeVisibile = !audioIOSelector.isVisible();
        
        audioIOSelector.setVisible(audioIOShouldBeVisibile);
        
        if (audioIOShouldBeVisibile)
        {
            if (oscilloscope2D != nullptr)
            {
                oscilloscope2D->setVisible(false);
                oscilloscope2D->stop();
            }
            
            if (oscilloscope3D != nullptr)
            {
                oscilloscope3D->setVisible(false);
                oscilloscope3D->stop();
            }
                
            if (spectrum != nullptr)
            {
                spectrum->setVisible(false);
                spectrum->stop();
            }
        }
        else
        {
            if (oscilloscope2DButton.getToggleState())
            {
                oscilloscope3D->setVisible(true);
                oscilloscope3D->start();
            }
            else if (oscilloscope3DButton.getToggleState())
            {
                oscilloscope3D->setVisible(true);
                oscilloscope3D->start();
            }
            else if (spectrumButton.getToggleState())
            {
                oscilloscope3D->setVisible(true);
                oscilloscope3D->start();
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

    // App State
    bool audioFileModeEnabled;
    bool audioInputModeEnabled;
    
    // GUI Buttons
    TextButton openFileButton;
    TextButton audioInputButton;
    TextButton showIOSelectorButton;
    TextButton playButton;
    TextButton stopButton;
    
    TextButton oscilloscope2DButton;
    TextButton oscilloscope3DButton;
    TextButton spectrumButton;
    
    AudioDeviceSelectorComponent audioIOSelector;
    
    // Audio File Reading Variables
    AudioFormatManager formatManager;
    ScopedPointer<AudioFormatReaderSource> audioReaderSource;
    AudioTransportSource audioTransportSource;
    AudioTransportState audioTransportState;
    
    // Audio & GL Audio Buffer
    RingBuffer<float> * ringBuffer;
    
    // Visualizers
    Oscilloscope2D * oscilloscope2D;
    Oscilloscope3D * oscilloscope3D;
    Spectrum * spectrum;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()    { return new MainContentComponent(); }
