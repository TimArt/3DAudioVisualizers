//
//  Spectrum.h
//  3DAudioVisualizers
//
//  Created by Tim Arterbury on 5/3/17.
//
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "RingBuffer.h"

/** Frequency Spectrum visualizer. Uses basic shaders, and calculates all points
    on the CPU as opposed to the OScilloscope3D which calculates points on the
    GPU.
 */

class Spectrum :    public Component,
                    public OpenGLRenderer,
                    public AsyncUpdater
{
    
public:
    Spectrum (RingBuffer<GLfloat> * ringBuffer)
    :   readBuffer (2, RING_BUFFER_READ_SIZE),
        forwardFFT (fftOrder)
    {
        // Sets the version to 3.2
        openGLContext.setOpenGLVersionRequired (OpenGLContext::OpenGLVersion::openGL3_2);
     
        this->ringBuffer = ringBuffer;
        
        // Set default 3D orientation
        draggableOrientation.reset(Vector3D<float>(0.0, 1.0, 0.0));
        
        // Allocate FFT data
        fftData = new GLfloat [2 * fftSize];
        
        // Attach the OpenGL context but do not start [ see start() ]
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        
        // Setup GUI Overlay Label: Status of Shaders, compiler errors, etc.
        addAndMakeVisible (statusLabel);
        statusLabel.setJustificationType (Justification::topLeft);
        statusLabel.setFont (Font (14.0f));
    }
    
    ~Spectrum()
    {
        // Turn off OpenGL
        openGLContext.setContinuousRepainting (false);
        openGLContext.detach();
        
        delete [] fftData;
        
        // Detach ringBuffer
        ringBuffer = nullptr;
    }
    
    void handleAsyncUpdate() override
    {
        statusLabel.setText (statusText, dontSendNotification);
    }
    
    //==========================================================================
    // Oscilloscope Control Functions
    
    void start()
    {
        openGLContext.setContinuousRepainting (true);
    }
    
    void stop()
    {
        openGLContext.setContinuousRepainting (false);
    }
    
    
    //==========================================================================
    // OpenGL Callbacks
    
    /** Called before rendering OpenGL, after an OpenGLContext has been associated
        with this OpenGLRenderer (this component is a OpenGLRenderer).
        Sets up GL objects that are needed for rendering.
     */
    void newOpenGLContextCreated() override
    {
        // Setup Sizing Variables
        xFreqWidth = 3.0f;
        yAmpHeight = 1.0f;
        zTimeDepth = 3.0f;
        xFreqResolution = 50;
        zTimeResolution = 60;

        numVertices = xFreqResolution * zTimeResolution;
        
        // Initialize XZ Vertices
        initializeXZVertices();
        
        // Initialize Y Vertices
        initializeYVertices();
        
        // Setup Buffer Objects
        openGLContext.extensions.glGenBuffers (1, &xzVBO); // Vertex Buffer Object
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, xzVBO);
        openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices * 2, xzVertices, GL_STATIC_DRAW);
        
        
        openGLContext.extensions.glGenBuffers (1, &yVBO);
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, yVBO);
        openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices, yVertices, GL_STREAM_DRAW);
        
        openGLContext.extensions.glGenVertexArrays(1, &VAO);
        openGLContext.extensions.glBindVertexArray(VAO);
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, xzVBO);
        openGLContext.extensions.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, yVBO);
        openGLContext.extensions.glVertexAttribPointer (1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat), NULL);
        
        openGLContext.extensions.glEnableVertexAttribArray (0);
        openGLContext.extensions.glEnableVertexAttribArray (1);
        
        glPointSize (6.0f);
        
        // Setup Shaders
        createShaders();
    }
    
    /** Called when done rendering OpenGL, as an OpenGLContext object is closing.
        Frees any GL objects created during rendering.
     */
    void openGLContextClosing() override
    {
        shader.release();
        uniforms.release();
        
        delete [] xzVertices;
        delete [] yVertices;
    }
    
    
    /** The OpenGL rendering callback.
     */
    void renderOpenGL() override
    {
        jassert (OpenGLHelpers::isContextActive());
        
        // Setup Viewport
        const float renderingScale = (float) openGLContext.getRenderingScale();
        glViewport (0, 0, roundToInt (renderingScale * getWidth()), roundToInt (renderingScale * getHeight()));
        
        // Set background Color
        OpenGLHelpers::clear (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        
        // Enable Alpha Blending
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Use Shader Program that's been defined
        shader->use();
        
        
        // Copy data from ring buffer into FFT
        
        ringBuffer->readSamples (readBuffer, RING_BUFFER_READ_SIZE);
        FloatVectorOperations::clear (fftData, RING_BUFFER_READ_SIZE);
        
        /** Future Feature:
            Instead of summing channels below, keep the channels seperate and
            lay out the spectrum so you can see the left and right channels
            individually on either half of the spectrum.
         */
        // Sum channels together
        for (int i = 0; i < 2; ++i)
        {
            FloatVectorOperations::add (fftData, readBuffer.getReadPointer(i, 0), RING_BUFFER_READ_SIZE);
        }
        
        // Calculate FFT Crap
        forwardFFT.performFrequencyOnlyForwardTransform (fftData);
        
        // Find the range of values produced, so we can scale our rendering to
        // show up the detail clearly
        Range<float> maxFFTLevel = FloatVectorOperations::findMinAndMax (fftData, fftSize / 2);
        
        // Calculate new y values and shift old y values back
        for (int i = numVertices; i >= 0; --i)
        {
            // For the first row of points, render the new height via the FFT
            if (i < xFreqResolution)
            {
                const float skewedProportionY = 1.0f - std::exp (std::log (i / ((float) xFreqResolution - 1.0f)) * 0.2f);
                const int fftDataIndex = jlimit (0, fftSize / 2, (int) (skewedProportionY * fftSize / 2));
                float level = 0.0f;
                
                if (maxFFTLevel.getEnd() != 0.0f)
                    level = jmap (fftData[fftDataIndex], 0.0f, maxFFTLevel.getEnd(), 0.0f, yAmpHeight);
                
                yVertices[i] = level;
            }
            else // For the subsequent rows, shift back
            {
                yVertices[i] = yVertices[i - xFreqResolution];
            }
        }
        
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, yVBO);
        openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices, yVertices, GL_STREAM_DRAW);
        
        
        // Setup the Uniforms for use in the Shader
        if (uniforms->projectionMatrix != nullptr)
            uniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);
        
        if (uniforms->viewMatrix != nullptr)
        {
            Matrix3D<float> scale;
            scale.mat[0] = 2.0;
            scale.mat[5] = 2.0;
            scale.mat[10] = 2.0;
            Matrix3D<float> finalMatrix = scale * getViewMatrix();
            uniforms->viewMatrix->setMatrix4 (finalMatrix.mat, 1, false);
            
        }

        // Draw the points
        openGLContext.extensions.glBindVertexArray(VAO);
        glDrawArrays (GL_POINTS, 0, numVertices);
        
        
        // Zero Out FFT for next use
        zeromem (fftData, sizeof (GLfloat) * 2 * fftSize);
        
        // Reset the element buffers so child Components draw correctly
//        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
//        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
//        openGLContext.extensions.glBindVertexArray(0);
    }
    
    
    //==========================================================================
    // JUCE Callbacks
    
    void paint (Graphics& g) override {}
    
    void resized () override
    {
        draggableOrientation.setViewport (getLocalBounds());
        statusLabel.setBounds (getLocalBounds().reduced (4).removeFromTop (75));
    }
    
    void mouseDown (const MouseEvent& e) override
    {
        draggableOrientation.mouseDown (e.getPosition());
    }
    
    void mouseDrag (const MouseEvent& e) override
    {
        draggableOrientation.mouseDrag (e.getPosition());
    }
    
private:
    
    //==========================================================================
    // Mesh Functions
    
    // Initialize the XZ values of vertices
    void initializeXZVertices()
    {
        
        int numFloatsXZ = numVertices * 2;
        
        xzVertices = new GLfloat [numFloatsXZ];
        
        // Variables when setting x and z
        int numFloatsPerRow = xFreqResolution * 2;
        GLfloat xOffset = xFreqWidth / ((GLfloat) xFreqResolution - 1.0f);
        GLfloat zOffset = zTimeDepth / ((GLfloat) zTimeResolution - 1.0f);
        GLfloat xStart = -(xFreqWidth / 2.0f);
        GLfloat zStart = -(zTimeDepth / 2.0f);
        
        // Set all X and Z values
        for (int i = 0; i < numFloatsXZ; i += 2)
        {
            
            int xFreqIndex = (i % (numFloatsPerRow)) / 2;
            int zTimeIndex = floor (i / numFloatsPerRow);
            
            // Set X Vertex
            xzVertices[i] = xStart + xOffset * xFreqIndex;
            xzVertices[i + 1] = zStart + zOffset * zTimeIndex;
        }
    }
    
    // Initialize the Y valies of vertices
    void initializeYVertices()
    {
        // Set all Y values to 0.0
        yVertices = new GLfloat [numVertices];
        memset(yVertices, 0.0f, sizeof(GLfloat) * xFreqResolution * zTimeResolution);
    }
    
    
    //==========================================================================
    // OpenGL Functions
    
    /** Calculates and returns the Projection Matrix.
     */
    Matrix3D<float> getProjectionMatrix() const
    {
        float w = 1.0f / (0.5f + 0.1f);
        float h = w * getLocalBounds().toFloat().getAspectRatio (false);
        return Matrix3D<float>::fromFrustum (-w, w, -h, h, 4.0f, 30.0f);
    }
    
    /** Calculates and returns the View Matrix.
     */
    Matrix3D<float> getViewMatrix() const
    {
        Matrix3D<float> viewMatrix (Vector3D<float> (0.0f, 0.0f, -10.0f));
        Matrix3D<float> rotationMatrix = draggableOrientation.getRotationMatrix();
        
        return rotationMatrix * viewMatrix;
    }
    
    /** Loads the OpenGL Shaders and sets up the whole ShaderProgram
     */
    void createShaders()
    {
        vertexShader =
        "#version 330 core\n"
        "layout (location = 0) in vec2 xzPos;\n"
        "layout (location = 1) in float yPos;\n"
        // Uniforms
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projectionMatrix * viewMatrix * vec4(xzPos[0], yPos, xzPos[1], 1.0f);\n"
        "}\n";
   
        
        // Base Shader
        fragmentShader =
        "#version 330 core\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "    color = vec4 (1.0f, 0.0f, 2.0f, 1.0f);\n"
        "}\n";
        

        std::unique_ptr<OpenGLShaderProgram> shaderProgramAttempt = std::make_unique<OpenGLShaderProgram> (openGLContext);
        
        if (shaderProgramAttempt->addVertexShader ((vertexShader))
            && shaderProgramAttempt->addFragmentShader ((fragmentShader))
            && shaderProgramAttempt->link())
        {
            uniforms.release();
            shader = std::move (shaderProgramAttempt);
            uniforms.reset (new Uniforms (openGLContext, *shader));
            
            statusText = "GLSL: v" + String (OpenGLShaderProgram::getLanguageVersion(), 2);
        }
        else
        {
            statusText = shaderProgramAttempt->getLastError();
        }
        
        triggerAsyncUpdate();
    }
    
    //==============================================================================
    // This class manages the uniform values that the shaders use.
    struct Uniforms
    {
        Uniforms (OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram)
        {
            projectionMatrix.reset (createUniform (openGLContext, shaderProgram, "projectionMatrix"));
            viewMatrix.reset (createUniform (openGLContext, shaderProgram, "viewMatrix"));
        }
        
        std::unique_ptr<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;
        //ScopedPointer<OpenGLShaderProgram::Uniform> lightPosition;
        
    private:
        static OpenGLShaderProgram::Uniform* createUniform (OpenGLContext& openGLContext,
                                                            OpenGLShaderProgram& shaderProgram,
                                                            const char* uniformName)
        {
            if (openGLContext.extensions.glGetUniformLocation (shaderProgram.getProgramID(), uniformName) < 0)
                return nullptr;
            
            return new OpenGLShaderProgram::Uniform (shaderProgram, uniformName);
        }
    };

    // Visualizer Variables
    GLfloat xFreqWidth;
    GLfloat yAmpHeight;
    GLfloat zTimeDepth;
    int xFreqResolution;
    int zTimeResolution;
    
    int numVertices;
    GLfloat * xzVertices;
    GLfloat * yVertices;
    
    
    // OpenGL Variables
    OpenGLContext openGLContext;
    GLuint xzVBO;
    GLuint yVBO;
    GLuint VAO;/*, EBO;*/
    
    std::unique_ptr<OpenGLShaderProgram> shader;
    std::unique_ptr<Uniforms> uniforms;
    
    const char* vertexShader;
    const char* fragmentShader;
    
    
    // GUI Interaction
    Draggable3DOrientation draggableOrientation;
    
    // Audio Structures
    RingBuffer<GLfloat> * ringBuffer;
    AudioBuffer<GLfloat> readBuffer;    // Stores data read from ring buffer
    juce::dsp::FFT forwardFFT;
    GLfloat * fftData;
    
    // This is so that we can initialize fowardFFT in the constructor with the order
    enum
    {
        fftOrder = 10,
        fftSize  = 1 << fftOrder // set 10th bit to one
    };
    
    // Overlay GUI
    String statusText;
    Label statusLabel;
    
    /** DEV NOTE
        If I wanted to optionally have an interchangeable shader system,
        this would be fairly easy to add. Chack JUCE Demo -> OpenGLDemo.cpp for
        an implementation example of this. For now, we'll just allow these
        shader files to be static instead of interchangeable and dynamic.
        String newVertexShader, newFragmentShader;
     */
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spectrum)
};
