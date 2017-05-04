//
//  Oscilloscope2D.h
//  3DAudioVisualizers
//
//  Created by Tim Arterbury on 4/29/17.
//
//

#ifndef Oscilloscope2D_h
#define Oscilloscope2D_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "RingBuffer.h"

/** This Oscilloscope2D uses a Shader Based Implementation.
 */

class Oscilloscope2D :    public Component,
                        public OpenGLRenderer
{
    
public:
    
    Oscilloscope2D (RingBuffer<GLfloat> * audioBuffer)
    {
        this->audioBuffer = audioBuffer;
        
        // Attach and start OpenGL
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        // THIS DOES NOT START THE VISUALIZER, It must be explicitly started
        // with the start() function.
        
        
        // Setup GUI Overlay Label: Status of Shaders and crap, compiler errors, etc.
        addAndMakeVisible (statusLabel);
        statusLabel.setJustificationType (Justification::topLeft);
        statusLabel.setFont (Font (14.0f));
    }
    
    ~Oscilloscope2D()
    {
        // Turn of OpenGL
        openGLContext.setContinuousRepainting (false);
        openGLContext.detach();
        
        // Detach AudioBuffer
        audioBuffer = nullptr;
    }
    
    //==========================================================================
    // Oscilloscope2D Control Functions
    
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
        // Setup Shaders
        createShaders();
        
        // Setup Buffer Objects
        openGLContext.extensions.glGenBuffers (1, &VBO); // Vertex Buffer Object
        openGLContext.extensions.glGenBuffers (1, &EBO); // Element Buffer Object
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
        
        // Setup the Uniforms for use in the Shader
        
        if (uniforms->resolution != nullptr)
            uniforms->resolution->set ((GLfloat) renderingScale * getWidth(), (GLfloat) renderingScale * getHeight());
            
        if (uniforms->audioSampleData != nullptr)
            uniforms->audioSampleData->set (audioBuffer->readSamples (256, 1), 256);    // RingBuffer Channel 0 still doesn't work lolz what the crap
        
        
        
        // Define Vertices for a Square
        GLfloat vertices[] = {
            1.0f,   1.0f,  0.0f,  // Top Right
            1.0f,  -1.0f,  0.0f,  // Bottom Right
            -1.0f, -1.0f,  0.0f,  // Bottom Left
            -1.0f,  1.0f,  0.0f   // Top Left
        };
        // Define Which Vertex Indexes Make the Square
        GLuint indices[] = {  // Note that we start from 0!
            0, 1, 3,   // First Triangle
            1, 2, 3    // Second Triangle
        };
        
        // Vertex Array Object stuff for later
        //openGLContext.extensions.glGenVertexArrays(1, &VAO);
        //openGLContext.extensions.glBindVertexArray(VAO);
        
        // VBO (Vertex Buffer Object) - Bind and Write to Buffer
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, VBO);
        openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
                                                                    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
                                                                    // Don't we want GL_DYNAMIC_DRAW since this
                                                                    // vertex data will be changing alot??
                                                                    // test this
        
        // EBO (Element Buffer Object) - Bind and Write to Buffer
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, EBO);
        openGLContext.extensions.glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
                                                                    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
                                                                    // Don't we want GL_DYNAMIC_DRAW since this
                                                                    // vertex data will be changing alot??
                                                                    // test this
        
        // Setup Vertex Attributes
        openGLContext.extensions.glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        openGLContext.extensions.glEnableVertexAttribArray (0);
    
        // Draw Vertices
        //glDrawArrays (GL_TRIANGLES, 0, 6); // For just VBO's (Vertex Buffer Objects)
        glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // For EBO's (Element Buffer Objects) (Indices)
        
    
        
        // Reset the element buffers so child Components draw correctly
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
        //openGLContext.extensions.glBindVertexArray(0);
    }
    
    
    //==========================================================================
    // JUCE Callbacks
    
    void paint (Graphics& g) override
    {
    }
    
    void resized () override
    {
        // Resize Status Label text
        // This is overdone, make this like 1 line later
        Rectangle<int> area (getLocalBounds().reduced (4));
        Rectangle<int> top (area.removeFromTop (75));
        statusLabel.setBounds (top);
    }
    
private:
    
    //==========================================================================
    // OpenGL Functions
    
    
    /** Loads the OpenGL Shaders and sets up the whole ShaderProgram
    */
    void createShaders()
    {
        vertexShader =
        "attribute vec3 position;\n"
        /*"attribute vec4 sourceColour;\n"
        "attribute vec2 texureCoordIn;\n"*/
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position, 1.0);\n"
        "}\n";
        
        fragmentShader =
        "uniform vec2  resolution;\n"
        "uniform float audioSampleData[256];\n"
        "\n"
        "void getAmplitudeForXPos (in float xPos, out float audioAmplitude)\n"
        "{\n"
        // Buffer size - 1
        "   float perfectSamplePosition = 255.0 * xPos / resolution.x;\n"
        "   int leftSampleIndex = int (floor (perfectSamplePosition));\n"
        "   int rightSampleIndex = int (ceil (perfectSamplePosition));\n"
        "   audioAmplitude = mix (audioSampleData[leftSampleIndex], audioSampleData[rightSampleIndex], fract (perfectSamplePosition));\n"
        "}\n"
        "\n"
        "#define THICKNESS 0.02\n"
        "void main()\n"
        "{\n"
        "    float y = gl_FragCoord.y / resolution.y;\n"
        "    float amplitude = 0.0;\n"
        "    getAmplitudeForXPos (gl_FragCoord.x, amplitude);\n"
        "\n"
        // Centers & Reduces Wave Amplitude
        "    amplitude = 0.5 - amplitude / 3.0;\n"
        "    float r = abs (THICKNESS / (amplitude-y));\n"
        "\n"
        "gl_FragColor = vec4 (r - abs (r * 0.2), r - abs (r * 0.2), r - abs (r * 0.2), 1.0);\n"
        "}\n";
        
        ScopedPointer<OpenGLShaderProgram> newShader (new OpenGLShaderProgram (openGLContext));
        String statusText;
        
        if (newShader->addVertexShader (OpenGLHelpers::translateVertexShaderToV3 (vertexShader))
            && newShader->addFragmentShader (OpenGLHelpers::translateFragmentShaderToV3 (fragmentShader))
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
            //projectionMatrix = createUniform (openGLContext, shaderProgram, "projectionMatrix");
            //viewMatrix       = createUniform (openGLContext, shaderProgram, "viewMatrix");
            
            resolution          = createUniform (openGLContext, shaderProgram, "resolution");
            audioSampleData     = createUniform (openGLContext, shaderProgram, "audioSampleData");
            
        }
        
        //ScopedPointer<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;
        ScopedPointer<OpenGLShaderProgram::Uniform> resolution, audioSampleData;
        
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
    
    
    // OpenGL Variables
    OpenGLContext openGLContext;
    GLuint VBO, VAO, EBO;
    
    ScopedPointer<OpenGLShaderProgram> shader;
    //ScopedPointer<Attributes> attributes;   // The private structs handle all attributes
    ScopedPointer<Uniforms> uniforms;       // The private structs handle all uniforms
    
    const char* vertexShader;
    const char* fragmentShader;

    
    // Audio Buffer
    RingBuffer<GLfloat> * audioBuffer;
    
    // If we wanted to optionally have an interchangeable shader system,
    // this would be fairly easy to add. Chack JUCE Demo -> OpenGLDemo.cpp for
    // an implementation example of this. For now, we'll just allow these
    // shader files to be static instead of interchangeable and dynamic.
    // String newVertexShader, newFragmentShader;
    
    // Overlay GUI
    Label statusLabel;
    
};


#endif /* Oscilloscope2D_h */
