//
//  Spectrum.h
//  3DAudioVisualizers
//
//  Created by Tim Arterbury on 5/3/17.
//
//

#ifndef Spectrum_h
#define Spectrum_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "RingBuffer.h"

/** Frequency Spectrum visualizer.
 */
class Spectrum :    public Component,
                    public OpenGLRenderer
{
    
public:
    Spectrum (RingBuffer<GLfloat> * audioBuffer)
    : forwardFFT (fftOrder, false)
    {
        // Set's the version to 3.2
        openGLContext.setOpenGLVersionRequired (OpenGLContext::OpenGLVersion::openGL3_2);
     
        this->audioBuffer = audioBuffer;
        
        // Set default 3D orientation
        draggableOrientation.reset(Vector3D<float>(0.0, 1.0, 0.0));
        
        // Allocate FFT data
        fftData = new GLfloat [2 * fftSize];
        
        // Attach and start OpenGL
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        
        // Setup GUI Overlay Label: Status of Shaders and crap, compiler errors, etc.
        addAndMakeVisible (statusLabel);
        statusLabel.setJustificationType (Justification::topLeft);
        statusLabel.setFont (Font (14.0f));
    }
    
    ~Spectrum()
    {
        // Turn of OpenGL
        openGLContext.setContinuousRepainting (false);
        openGLContext.detach();
        
        delete [] fftData;
        
        // Detach AudioBuffer
        audioBuffer = nullptr;
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
     Implement this method to set up any GL objects that you need for rendering.
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
        openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, xzVBO);
        openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices * 2, xzVertices, GL_STATIC_DRAW);
        
        
        openGLContext.extensions.glGenBuffers (1, &yVBO);
        openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, yVBO);
        openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices, yVertices, GL_STREAM_DRAW);
        
        openGLContext.extensions.glGenVertexArrays(1, &VAO);
        openGLContext.extensions.glBindVertexArray(VAO);
        openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, xzVBO);
        openGLContext.extensions.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
        openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, yVBO);
        openGLContext.extensions.glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat), NULL);
        
        openGLContext.extensions.glEnableVertexAttribArray(0);
        openGLContext.extensions.glEnableVertexAttribArray(1);
        
        glPointSize (6.0f);
        
        // Setup Shaders
        createShaders();
    }
    
    /** Called when done rendering OpenGL, as an OpenGLContext object is closing.
     Implement this method to free any GL objects that you created during rendering.
     */
    void openGLContextClosing() override
    {
        shader->release();
        shader = nullptr;
        //attributes = nullptr;
        uniforms = nullptr;
        
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
        
        // Copy new audio into FFT
        GLfloat * audioSamples = audioBuffer->readSamples (256, 1);
        memcpy (fftData, audioSamples, 256 * sizeof (GLfloat));
        delete [] audioSamples;
        
        // Calculate FFT Crap
        forwardFFT.performFrequencyOnlyForwardTransform (fftData);
        
        // find the range of values produced, so we can scale our rendering to
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
                //const int fftDataIndex = jlimit (0, fftSize / 2, (int) (i * fftSize / 2));
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
        
        openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, yVBO);
        openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices, yVertices, GL_STREAM_DRAW);
        
        
        // Setup the Uniforms for use in the Shader
        if (uniforms->projectionMatrix != nullptr)
            uniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);
        
        if (uniforms->viewMatrix != nullptr)
        {
            //uniforms->viewMatrix->setMatrix4 (getViewMatrix().mat, 1, false);
            // Scale and view matrix
            Matrix3D<float> scale;
            scale.mat[0] = 2.0;
            scale.mat[5] = 2.0;
            scale.mat[10] = 2.0;
            Matrix3D<float> finalMatrix = scale * getViewMatrix();
            uniforms->viewMatrix->setMatrix4 (finalMatrix.mat, 1, false);
            
        }
        
        //if (uniforms->resolution != nullptr)
        //    uniforms->resolution->set ((GLfloat) 100.0, (GLfloat) 100.0);
        //uniforms->resolution->set ((GLfloat) renderingScale * getWidth(), (GLfloat) renderingScale * getHeight());
        
        //if (uniforms->audioSampleData != nullptr)
        //    uniforms->audioSampleData->set (audioBuffer->readSamples (256, 1), 256); // RingBuffer Channel 0 still doesn't work lolz what the crap
        

        // Draw the points
        openGLContext.extensions.glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, numVertices);
        
        
        // Zero Out FFT for next use
        zeromem (fftData, sizeof (GLfloat) * 2 * fftSize);
        
        // Reset the element buffers so child Components draw correctly
//        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
//        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
//        openGLContext.extensions.glBindVertexArray(0);
    }
    
    
    //==========================================================================
    // JUCE Callbacks
    
    void paint (Graphics& g) override
    {
    }
    
    void resized () override
    {
        draggableOrientation.setViewport (getLocalBounds());
        
        // Resize Status Label text
        // This is overdone, make this like 1 line later
        Rectangle<int> area (getLocalBounds().reduced (4));
        Rectangle<int> top (area.removeFromTop (75));
        statusLabel.setBounds (top);
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
    
    // Initialize Vertices
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
        
//        std::cout << "XZVertices\n";
//        for (int i = 0; i < numFloatsXZ; i += 2)
//        {
//            std::cout << xzVertices[i] << " " << xzVertices[i + 1] << '\n';
//        }
    }
    
    // Initialize Vertices
    void initializeYVertices()
    {
        // Set all Y values to 0.0
        yVertices = new GLfloat [numVertices];
        memset(yVertices, 0.0f, sizeof(GLfloat) * xFreqResolution * zTimeResolution);
        
//        std::cout << "YVertices\n";
//        for (int i = 0; i < numVertices; ++i)
//        {
//            std::cout << yVertices[i] << '\n';
//        }
    }
    
    
    //==========================================================================
    // OpenGL Functions
    
    /** Used to Open a Shader file.
     */
    std::string getFileContents(const char *filename) {
        std::ifstream inStream (filename, std::ios::in | std::ios::binary);
        if (inStream) {
            std::string contents;
            inStream.seekg (0, std::ios::end);
            contents.resize((unsigned int) inStream.tellg());
            inStream.seekg (0, std::ios::beg);
            inStream.read (&contents[0], contents.size());
            inStream.close();
            return (contents);
        }
        return "";
    }
    
    Matrix3D<float> getProjectionMatrix() const
    {
        float w = 1.0f / (0.5f + 0.1f);
        float h = w * getLocalBounds().toFloat().getAspectRatio (false);
        return Matrix3D<float>::fromFrustum (-w, w, -h, h, 4.0f, 30.0f);
    }
    
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
        

        ScopedPointer<OpenGLShaderProgram> newShader (new OpenGLShaderProgram (openGLContext));
        String statusText;
        
        if (newShader->addVertexShader ((vertexShader))
            && newShader->addFragmentShader ((fragmentShader))
            && newShader->link())
        {
            //attributes = nullptr;
            uniforms = nullptr;
            
            shader = newShader;
            shader->use();
            
            //attributes = new Attributes (openGLContext, *shader);
            uniforms   = new Uniforms (openGLContext, *shader);
            
            statusText = "GLSL: v" + String (OpenGLShaderProgram::getLanguageVersion(), 2);
        }
        else
        {
            statusText = newShader->getLastError();
        }
        
        statusLabel.setText (statusText, dontSendNotification);
    }
    
    //==============================================================================
    // This struct represents a Vertex and its associated attributes
    /*struct Vertex
     {
     float position[3];
     float normal[3];
     float colour[4];
     float texCoord[2];
     };*/
    
    //==============================================================================
    // This class just manages the vertex attributes that the shaders use.
    /*struct Attributes
     {
     Attributes (OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram)
     {
     position      = createAttribute (openGLContext, shaderProgram, "position");
     normal        = createAttribute (openGLContext, shaderProgram, "normal");
     sourceColour  = createAttribute (openGLContext, shaderProgram, "sourceColour");
     texureCoordIn = createAttribute (openGLContext, shaderProgram, "texureCoordIn");
     }
     
     void enable (OpenGLContext& openGLContext)
     {
     if (position != nullptr)
     {
     openGLContext.extensions.glVertexAttribPointer (position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*)0);
     openGLContext.extensions.glEnableVertexAttribArray (position->attributeID);
     }
     
     if (normal != nullptr)
     {
     openGLContext.extensions.glVertexAttribPointer (normal->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 3));
     openGLContext.extensions.glEnableVertexAttribArray (normal->attributeID);
     }
     
     if (sourceColour != nullptr)
     {
     openGLContext.extensions.glVertexAttribPointer (sourceColour->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 6));
     openGLContext.extensions.glEnableVertexAttribArray (sourceColour->attributeID);
     }
     
     if (texureCoordIn != nullptr)
     {
     openGLContext.extensions.glVertexAttribPointer (texureCoordIn->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), (GLvoid*) (sizeof (float) * 10));
     openGLContext.extensions.glEnableVertexAttribArray (texureCoordIn->attributeID);
     }
     }
     
     void disable (OpenGLContext& openGLContext)
     {
     if (position != nullptr)       openGLContext.extensions.glDisableVertexAttribArray (position->attributeID);
     if (normal != nullptr)         openGLContext.extensions.glDisableVertexAttribArray (normal->attributeID);
     if (sourceColour != nullptr)   openGLContext.extensions.glDisableVertexAttribArray (sourceColour->attributeID);
     if (texureCoordIn != nullptr)  openGLContext.extensions.glDisableVertexAttribArray (texureCoordIn->attributeID);
     }
     
     ScopedPointer<OpenGLShaderProgram::Attribute> position, normal, sourceColour, texureCoordIn;
     
     private:
     static OpenGLShaderProgram::Attribute* createAttribute (OpenGLContext& openGLContext,
     OpenGLShaderProgram& shader,
     const char* attributeName)
     {
     if (openGLContext.extensions.glGetAttribLocation (shader.getProgramID(), attributeName) < 0)
     return nullptr;
     
     return new OpenGLShaderProgram::Attribute (shader, attributeName);
     }
     };*/
    
    //==============================================================================
    // This class just manages the uniform values that the demo shaders use.
    struct Uniforms
    {
        Uniforms (OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram)
        {
            projectionMatrix = createUniform (openGLContext, shaderProgram, "projectionMatrix");
            viewMatrix       = createUniform (openGLContext, shaderProgram, "viewMatrix");
            
            //resolution          = createUniform (openGLContext, shaderProgram, "resolution");
            //audioSampleData     = createUniform (openGLContext, shaderProgram, "audioSampleData");
            
        }
        
        ScopedPointer<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;
        //ScopedPointer<OpenGLShaderProgram::Uniform> /*resolution,*/ audioSampleData;
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
    
    ScopedPointer<OpenGLShaderProgram> shader;
    //ScopedPointer<Attributes> attributes;   // The private structs handle all attributes
    ScopedPointer<Uniforms> uniforms;       // The private structs handle all uniforms
    
    const char* vertexShader;
    const char* fragmentShader;
    
    
    // GUI Interaction
    Draggable3DOrientation draggableOrientation;
    
    // Audio Structures
    RingBuffer<GLfloat> * audioBuffer;
    FFT forwardFFT;
    GLfloat * fftData;
    // This is so that we can initialize fowardFFT in the constructor with the order
    enum
    {
        fftOrder = 10,
        fftSize  = 1 << fftOrder // set 10th bit to one
    };
    
    // If we wanted to optionally have an interchangeable shader system,
    // this would be fairly easy to add. Chack JUCE Demo -> OpenGLDemo.cpp for
    // an implementation example of this. For now, we'll just allow these
    // shader files to be static instead of interchangeable and dynamic.
    // String newVertexShader, newFragmentShader;
    
    // Overlay GUI
    Label statusLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spectrum)
};


#endif /* Spectrum_h */
