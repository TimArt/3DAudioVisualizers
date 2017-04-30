//
//  Oscilloscope.h
//  3DAudioVisualizers
//
//  Created by Tim Arterbury on 4/29/17.
//
//

#ifndef Oscilloscope_h
#define Oscilloscope_h

#include "../JuceLibraryCode/JuceHeader.h"

/** This Oscilloscope uses a Shader Based Implementation.
 */

class Oscilloscope :    public Component,
                        public OpenGLRenderer
{
    
public:
    
    Oscilloscope()
    {
        // Set default 3D orientation
        draggableOrientation.reset(Vector3D<float>(0.0, 1.0, 0.0));
        
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
    
    ~Oscilloscope()
    {
        // Turn of OpenGL
        openGLContext.setContinuousRepainting (false);
        openGLContext.detach();
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
        // Vertex Array Object
        /*GLuint vao;
        glGenVertexArrays (1, &vao);
        glBindVertexArray(vao);
        
        // Vertex Buffer Object
        GLuint vbo;
        glGenBuffers(1, &vbo);
        
        // Defines a square using two triangles
        GLfloat vertices[] = {
            // X and Y values
            
            // Triangle 1
            -1.0f,  1.0f,
            1.0f, 1.0f,
            1.0f, -1.0f,
            
            // Triangle 2
            -1.0f,1.0f,
            -1.0f,-1.0f,
            1.0f,-1.0f
        };
        
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                                    // Don't we want GL_DYNAMIC_DRAW since this
                                    // vertex data will be changing alot??
                                    // test this
        
        */
        // Setup Shaders
        createShaders();
        
        // Specify the layout of the vertex data
        //GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
        //glEnableVertexAttribArray(posAttrib);
        //glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        
        
    }
    
    /** Called when done rendering OpenGL, as an OpenGLContext object is closing.
     Implement this method to free any GL objects that you created during rendering.
     */
    void openGLContextClosing() override
    {
        shader = nullptr;
        attributes = nullptr;
        uniforms = nullptr;
    }
    
    
    /** The OpenGL rendering callback.
     */
    void renderOpenGL() override
    {
        jassert (OpenGLHelpers::isContextActive());
        
        // Setup Viewport and Defailt Background
        const float desktopScale = (float) openGLContext.getRenderingScale();
        OpenGLHelpers::clear (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glViewport (0, 0, roundToInt (desktopScale * getWidth()), roundToInt (desktopScale * getHeight()));
        
        
        // Setup Shader
        shader->use();
        
        // Setup the Uniforms for use in the Shader
        if (uniforms->projectionMatrix != nullptr)
            uniforms->projectionMatrix->setMatrix4 (getProjectionMatrix().mat, 1, false);
        
        if (uniforms->viewMatrix != nullptr)
            uniforms->viewMatrix->setMatrix4 (getViewMatrix().mat, 1, false);
        
        // Draw Square (draws a square made up of two triangles)
        GLfloat vertices[] = {
            // X, Y, Z values
            
            // Triangle 1
            -1.0f,  1.0f,  0.0f,
             1.0f,  1.0f,  0.0f,
             1.0f, -1.0f,  0.0f,
            
            // Triangle 2
            -1.0f,  1.0f,  0.0f,
            -1.0f, -1.0f,  0.0f,
             1.0f, -1.0f,  0.0f
        };
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        GLuint vertexBuffer;//, indexBuffer;
        openGLContext.extensions.glGenBuffers (1, &vertexBuffer);
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
        openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
                                                                // Don't we want GL_DYNAMIC_DRAW since this
                                                                // vertex data will be changing alot??
                                                                // test this
        
        /*openGLContext.extensions.glGenBuffers (1, &indexBuffer);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        openGLContext.extensions.glBufferData (GL_ELEMENT_ARRAY_BUFFER, numIndices * (int) sizeof (juce::uint32),
                                               shape.mesh.indices.getRawDataPointer(), GL_STATIC_DRAW);
        */

        
        /*openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer);*/
        attributes->enable (openGLContext);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        attributes->disable (openGLContext);
        
        
        // Reset the element buffers so child Components draw correctly
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
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
    // OpenGL Functions
    
    Matrix3D<float> getProjectionMatrix() const
    {
        float w = 1.0f / (0.5f + 0.1f);
        float h = w * getLocalBounds().toFloat().getAspectRatio (false);
        return Matrix3D<float>::fromFrustum (-w, w, -h, h, 4.0f, 30.0f);
    }
    
    Matrix3D<float> getViewMatrix() const
    {
        Matrix3D<float> viewMatrix (Vector3D<float> (0.0f, 0.0f, -10.0f));
       // Matrix3D<float> rotationMatrix
        //= viewMatrix.rotated (Vector3D<float> (-0.3f, 5.0f * std::sin (getFrameCounter() * 0.01f), 0.0f));
        Matrix3D<float> rotationMatrix = draggableOrientation.getRotationMatrix();
        
        return rotationMatrix * viewMatrix;
    }
    
    /** Loads the OpenGL Shaders and sets up the whole ShaderProgram
    */
    void createShaders()
    {
        vertexShader =
        "attribute vec4 position;\n"
        "attribute vec4 sourceColour;\n"
        "attribute vec2 texureCoordIn;\n"
        "\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "\n"
        "varying vec4 destinationColour;\n"
        "varying vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    destinationColour = sourceColour;\n"
        "    textureCoordOut = texureCoordIn;\n"
        "    gl_Position = projectionMatrix * viewMatrix * position;\n"
        "}\n";
        
        fragmentShader =
        "varying vec4 destinationColour;\n"
        "varying vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 colour = vec4(0.95, 0.57, 0.03, 0.7);\n"
        "    gl_FragColor = colour;\n"
        "}\n";
        
        ScopedPointer<OpenGLShaderProgram> newShader (new OpenGLShaderProgram (openGLContext));
        String statusText;
        
        if (newShader->addVertexShader (OpenGLHelpers::translateVertexShaderToV3 (vertexShader))
            && newShader->addFragmentShader (OpenGLHelpers::translateFragmentShaderToV3 (fragmentShader))
            && newShader->link())
        {
            attributes = nullptr;
            uniforms = nullptr;
            
            shader = newShader;
            shader->use();
            
            attributes = new Attributes (openGLContext, *shader);
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
    struct Vertex
    {
        float position[3];
        float normal[3];
        float colour[4];
        float texCoord[2];
    };
    
    //==============================================================================
    // This class just manages the attributes that the shaders use.
    struct Attributes
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
                openGLContext.extensions.glVertexAttribPointer (position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), 0);
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
    };
    
    //==============================================================================
    // This class just manages the uniform values that the demo shaders use.
    struct Uniforms
    {
        Uniforms (OpenGLContext& openGLContext, OpenGLShaderProgram& shaderProgram)
        {
            projectionMatrix = createUniform (openGLContext, shaderProgram, "projectionMatrix");
            viewMatrix       = createUniform (openGLContext, shaderProgram, "viewMatrix");
        }
        
        ScopedPointer<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;
        
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
    
    ScopedPointer<OpenGLShaderProgram> shader;
    ScopedPointer<Attributes> attributes;   // The private structs handle all attributes
    ScopedPointer<Uniforms> uniforms;       // The private structs handle all uniforms
    
    const char* vertexShader;
    const char* fragmentShader;
    
    // GUI Interaction
    Draggable3DOrientation draggableOrientation;
    
    // If we wanted to optionally have an interchangeable shader system,
    // this would be fairly easy to add. Chack JUCE Demo -> OpenGLDemo.cpp for
    // an implementation example of this. For now, we'll just allow these
    // shader files to be static instead of interchangeable and dynamic.
    // String newVertexShader, newFragmentShader;
    
    
    
    // Overlay GUI
    Label statusLabel;
    
};


#endif /* Oscilloscope_h */
