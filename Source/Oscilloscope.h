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
#include "RingBuffer.h"
#include <fstream>

/** This Oscilloscope uses a Shader Based Implementation.
 */

class Oscilloscope :    public Component,
                        public OpenGLRenderer
{
    
public:
    
    Oscilloscope (RingBuffer<GLfloat> * audioBuffer)
    {
        // Set's the version to 3.2
        openGLContext.setOpenGLVersionRequired (OpenGLContext::OpenGLVersion::openGL3_2);
        
        this->audioBuffer = audioBuffer;
        
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
        // Setup Shaders
        createShaders();
        
        // Setup Buffer Objects
        openGLContext.extensions.glGenBuffers (1, &VBO); // Vertex Buffer Object
        //openGLContext.extensions.glGenBuffers (1, &EBO); // Element Buffer Object
    }
    
    /** Called when done rendering OpenGL, as an OpenGLContext object is closing.
     Implement this method to free any GL objects that you created during rendering.
     */
    void openGLContextClosing() override
    {
        waveShader->release();
        waveShader = nullptr;
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
        waveShader->use();
        
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
        
        if (uniforms->resolution != nullptr)
            uniforms->resolution->set ((GLfloat) 100.0, (GLfloat) 100.0);
            //uniforms->resolution->set ((GLfloat) renderingScale * getWidth(), (GLfloat) renderingScale * getHeight());
            
        if (uniforms->audioSampleData != nullptr)
            uniforms->audioSampleData->set (audioBuffer->readSamples (256, 1), 256); // RingBuffer Channel 0 still doesn't work lolz what the crap
        
        // Define Screen (Square)
        /*GLfloat vertices[] = {
            -0.5f,  0.5f, // Top-left
             0.5f,  0.5f, // Top-right
             0.5f, -0.5f, // Bottom-right
            -0.5f, -0.5f  // Bottom-left
        };*/
        
        // Define Origin or Object 0.0
        GLfloat vertices[] = { 0.0f, 0.0f, 0.0f };
        
        
        // Vertex Array Object stuff for later
        openGLContext.extensions.glGenVertexArrays(1, &VAO);
        openGLContext.extensions.glBindVertexArray(VAO);
        
        // VBO (Vertex Buffer Object) - Bind and Write to Buffer
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, VBO);
        openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                                                                    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
                                                                    // Don't we want GL_DYNAMIC_DRAW since this
                                                                    // vertex data will be changing alot??
                                                                    // test this
        
        
        // Setup Vertex Attributes (Sets up vertices to work as only x, y values)
        openGLContext.extensions.glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        openGLContext.extensions.glEnableVertexAttribArray (0);
    
        // Draw Vertices
        glDrawArrays (GL_POINTS, 0, 1); // For just VBO's (Vertex Buffer Objects)
        //glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // For EBO's (Element Buffer Objects) (Indices)
        
    
        
        // Reset the element buffers so child Components draw correctly
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
        openGLContext.extensions.glBindVertexArray(0);
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
        "layout (location = 0) in vec2 position;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position, 0.0f, 1.0f);\n"
        "}\n";
        
        
        //        "void buildHouse (vec4 position)\n"
        //        "{\n"
        //        "    gl_Position = position + vec4(-0.2f, -0.2f, 0.0f, 0.0f);\n"
        //        "    gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        //        "    EmitVertex();\n"
        //        "    gl_Position = position + vec4( 0.2f, -0.2f, 0.0f, 0.0f);\n"
        //        "    gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        //        "    EmitVertex();\n"
        //        "    gl_Position = position + vec4(-0.2f,  0.2f, 0.0f, 0.0f);\n"
        //        "    gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        //        "    EmitVertex();\n"
        //        "    gl_Position = position + vec4( 0.2f,  0.2f, 0.0f, 0.0f);\n"
        //        "    gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        //        "    EmitVertex();\n"
        //        "    gl_Position = position + vec4( 0.0f,  0.4f, 0.0f, 0.0f);\n"
        //        "    gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        //        "    EmitVertex();\n"
        //        "    EndPrimitive();\n"
        //        "}\n"
        
//        geometryShader =
//        "#version 330 core\n"
//        "layout (points) in;\n"
//        //"layout (triangle_strip, max_vertices = 5) out;\n"
//        "layout (points, max_vertices = 1) out;\n"
//        "uniform mat4 projectionMatrix;\n"
//        "uniform mat4 viewMatrix;\n"
//        "uniform vec2 resolution;\n"
//        "uniform float audioSampleData[256];\n"
//        
//        "void getAmplitudeForXPos (in float xPos, out float audioAmplitude)\n"
//        "{\n"
//                                    // Buffer size - 1
//        "   float perfectSamplePosition = 255.0f * xPos / resolution.x;\n"
//        "   int leftSampleIndex = int (floor (perfectSamplePosition));\n"
//        "   int rightSampleIndex = int (ceil (perfectSamplePosition));\n"
//        "   audioAmplitude = mix (audioSampleData[leftSampleIndex], audioSampleData[rightSampleIndex], fract (perfectSamplePosition));\n"
//        "}\n"
//        "\n"
//        "void main() {\n"
//        "\n"
//        "    float sampleWidth = 1.0f / resolution.x;\n"
//        "    for (int i = 0; i < resolution.x; ++i)\n"
//        "    {\n"
//        "        float amplitude = 0.0f;\n"
//        "        getAmplitudeForXPos (sampleWidth * float(i), amplitude);\n"
//        "        gl_Position = vec4 (gl_in[0].gl_Position.x + sampleWidth * float(i), 1.0f / amplitude, 0.0f, 1.0f);\n"
//        //"        gl_Position = projectionMatrix * viewMatrix * vec4 (gl_in[0].gl_Position.x + sampleWidth * float(i), 1.0f / amplitude, 0.0f, 0.0f);\n"
//        "        EmitVertex();\n"
//        "        EndPrimitive();\n"
//        "    }\n"
//        //"    buildHouse (gl_in[0].gl_Position);\n"
//        "}\n";
        
        // Oscilloscope Triangle Rendering Geometry Shader
        // THIS MOSTLY WORKS BUT INEFFIEIECNTLY REPEATS VERTICES
        // When fixing makes sure to pay attention to
        // layout (triangle_strip, max_vertices = 598) out;
        // max_vertices needs to be based and calculated from other numbers
        waveGeometryShader =
        "#version 330 core\n"
        
        // User Defined Variables
        "#define WAVE_RENDERING_WIDTH 4.0f\n"
        "#define WAVE_RENDERING_HEIGHT 3.0f\n"
        "#define WAVE_WIDTH_RESOLUTION 50\n"
        "#define WAVE_RADIUS 0.1f\n"
        "#define WAVE_GIRTH_RESOLUTION 5\n"
        "#define PI 3.1415926538f\n"
        
        // Calculated Data based of User Defined Variables (this might be more inefficient than passing variables to functions)
        "#define WIDTH_POSITIONAL_OFFSET (WAVE_RENDERING_WIDTH / (float (WAVE_WIDTH_RESOLUTION) - 1.0f))\n"
        "#define GIRTH_ANGLE_OFFSET (2.0f * PI / float (WAVE_GIRTH_RESOLUTION))\n"
        
        // Input / Output
        "layout (points) in;\n"
        // PROBLEM MY ALGORITHM IS REPEATING VERTICES
        // !! max_vertices MUST be (WAVE_GIRTH_RESOLUTION * WAVE_WIDTH_RESOLUTION * 2) + 2 * (WAVE_WIDTH_RESOLUTION - 1) cuz of repeated vertices!!
        "layout (triangle_strip, max_vertices = 598) out;\n"
        
        // Uniforms
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "uniform float audioSampleData[256];\n"
        
        /** Gets the amplitude for a given x position of a wave slice.
        */
        "void getAmplitudeForXPos (in float xPos, out float audioAmplitude)\n"
        "{\n"
        //                                Buffer size - 1
        "    float perfectSamplePosition = 255.0f * xPos / WAVE_RENDERING_WIDTH;\n"
        "    int leftSampleIndex = int (floor (perfectSamplePosition));\n"
        "    int rightSampleIndex = int (ceil (perfectSamplePosition));\n"
            // Output the result
        "    audioAmplitude = mix (audioSampleData[leftSampleIndex], audioSampleData[rightSampleIndex], fract (perfectSamplePosition));\n"
        "}\n"
        
        /** Calculates the origin point for a given slice division in the wave,
            from which more points will be extrapolated.
        */
        "void getSliceOrigin (in int sliceIndex, out vec2 sliceOrigin)\n"
        "{\n"
            // Calculate x and y positions
        "    float xPos = float (sliceIndex) * WIDTH_POSITIONAL_OFFSET;\n"
        "    float amplitude = 0.0f;\n"
        "    getAmplitudeForXPos (xPos, amplitude);\n"
        "    float yPos = WAVE_RENDERING_HEIGHT * (amplitude + 1.0f) / 2.0f;\n"
            // Shift coordinates to center in the rendering box
        "    xPos = xPos - (WAVE_RENDERING_WIDTH / 2.0f);\n"
        "    yPos = yPos - (WAVE_RENDERING_HEIGHT / 2.0f);\n"
            // Output the result
        "    sliceOrigin = vec2 (xPos, yPos);\n"
        "}\n"
        
        
        /** Calculates an extrapolated point from the given origin of a slice
            based on the specific index of the girth division.
         */
        "void extrude (in vec2 sliceOrigin, in int girthIndex, out vec2 extrapolationZY)\n"
        "{\n"
        "    float zPos = WAVE_RADIUS * cos (float (girthIndex) * GIRTH_ANGLE_OFFSET);\n"
        "    float yPos = WAVE_RADIUS * sin (float (girthIndex) * GIRTH_ANGLE_OFFSET);\n"
        "    extrapolationZY = vec2 (zPos, yPos + sliceOrigin.y);\n"
        "}\n"
        
        "void main() {\n"
        "\n"
            // Calculate 1st slice origin
        "    vec2 sliceOrigin1;\n"
        "    getSliceOrigin (0, sliceOrigin1);\n"
        
        // For every two Slice Origin's calculate their extrapolations and render
        // their triangle strips
        "    for (int i = 1; i < WAVE_WIDTH_RESOLUTION; ++i)\n"
        "    {\n"
                // Calculate 2nd slice origin
        "        vec2 sliceOrigin2;\n"
        "        getSliceOrigin (i, sliceOrigin2);\n"
        
                // Calculate the first extrapolation for each of the two slice origins
                // The naming convention is extrap< 1st or 2nd origin >_< 1st or 2nd extrapolation >
        "        vec2 extrap1_1;\n"
        "        extrude (sliceOrigin1, 0, extrap1_1);\n"
        "        vec2 extrap2_1;\n"
        "        extrude (sliceOrigin2, 0, extrap2_1);\n"
        
                // Emit the first two vertices
        "        gl_Position = gl_in[0].gl_Position + vec4 (sliceOrigin1.x, extrap1_1[1], extrap1_1[0], 0.0f);\n"
        "        gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "        EmitVertex();\n"
        //"        EndPrimitive();\n"
        
        "        gl_Position = gl_in[0].gl_Position + vec4 (sliceOrigin2.x, extrap2_1[1], extrap2_1[0], 0.0f);\n"
        "        gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "        EmitVertex();\n"
        //"        EndPrimitive();\n"

        
                // For every 2 girth width positions, on every 2 slice origins,
                // connect them together with a traingle strip
        "        for (int j = 1; j < WAVE_GIRTH_RESOLUTION; ++j)\n"
        "        {\n"
                    // Calculate the second extrapolations for each of the two slice origins
        "            vec2 extrap1_2;\n"
        "            extrude (sliceOrigin1, j, extrap1_2);\n"
        "            vec2 extrap2_2;\n"
        "            extrude (sliceOrigin2, j, extrap2_2);\n"
        
                    // Emit the 2 new vertices
        "            gl_Position = gl_in[0].gl_Position + vec4 (sliceOrigin1.x, extrap1_2[1], extrap1_2[0], 0.0f);\n"
        "            gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "            EmitVertex();\n"
        //"        EndPrimitive();\n"
        
        "            gl_Position = gl_in[0].gl_Position + vec4 (sliceOrigin2.x, extrap2_2[1], extrap2_2[0], 0.0f);\n"
        "            gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "            EmitVertex();\n"
       // "        EndPrimitive();\n"

                    // Update the first extrapolcations for the two slice origins
        //"            extrap1_1 = extrap1_2;\n"
        //"            extrap2_1 = extrap2_2;\n"
        
        "            if (j == WAVE_GIRTH_RESOLUTION - 1)\n"
        "            {\n"
        "                gl_Position = gl_in[0].gl_Position + vec4 (sliceOrigin1.x, extrap1_1[1], extrap1_1[0], 0.0f);\n"
        "                gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "                EmitVertex();\n"
        
        "                gl_Position = gl_in[0].gl_Position + vec4 (sliceOrigin2.x, extrap2_1[1], extrap2_1[0], 0.0f);\n"
        "                gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "                EmitVertex();\n"
        "            }\n"
        "        }\n"
        
                // End the triangle strip since it has wrapped around the two
                // slices and created a cylinder
        "        EndPrimitive();\n"
        
                // Update the first slice origin
        "        sliceOrigin1 = sliceOrigin2;\n"
        "    }\n"
        "}\n";

        
        
        // Oscilloscope Point rendering Geometry Shader
       /* waveGeometryShader =
        "#version 330 core\n"
        "#define WAVE_RENDERING_WIDTH 4.0f\n"
        "#define WAVE_RENDERING_HEIGHT 5.0f\n"
        "#define WAVE_WIDTH_RESOLUTION 40\n"
        "#define WAVE_RADIUS 0.1f\n"
        "#define WAVE_GIRTH_RESOLUTION 20\n"
        "#define PI 3.1415926538\n"
        "layout (points) in;\n"
                        // max_vertices MUST be WAVE_GIRTH_RESOLUTION * WAVE_WIDTH_RESOLUTION
        "layout (points, max_vertices = 800) out;\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "uniform float audioSampleData[256];\n"
        
        "void getAmplitudeForXPos (in float xPos, out float audioAmplitude)\n"
        "{\n"
                                    // Buffer size - 1
        "   float perfectSamplePosition = 255.0f * xPos / WAVE_RENDERING_WIDTH;\n"
        "   int leftSampleIndex = int (floor (perfectSamplePosition));\n"
        "   int rightSampleIndex = int (ceil (perfectSamplePosition));\n"
        "   audioAmplitude = mix (audioSampleData[leftSampleIndex], audioSampleData[rightSampleIndex], fract (perfectSamplePosition));\n"
        "}\n"
        
        "void extrude (in vec2 flatPos)\n"
        "{\n"
        "    float angleOffset = 2.0f * PI / float(WAVE_GIRTH_RESOLUTION);\n"
        "    for (int i = 0; i < WAVE_GIRTH_RESOLUTION; ++i)\n"
        "    {\n"
        "        float zPos = WAVE_RADIUS * cos (float(i) * angleOffset);\n"
        "        float yPos = WAVE_RADIUS * sin (float(i) * angleOffset);\n"
        "        gl_Position = gl_in[0].gl_Position + vec4 (flatPos.x, flatPos.y + yPos, zPos, 0.0f);\n"
        "        gl_Position = projectionMatrix * viewMatrix * gl_Position;\n"
        "        EmitVertex();\n"
        "        EndPrimitive();\n"
        "    }\n"
        "}\n"
        
        "void main() {\n"
        "\n"
        "\n"
        "    float widthOffset = WAVE_RENDERING_WIDTH / (float(WAVE_WIDTH_RESOLUTION) - 1.0f);\n"
        "    for (int i = 0; i < WAVE_WIDTH_RESOLUTION; ++i)\n"
        "    {\n"
            // Calculate x and y positions
        "        float xPos = float (i) * widthOffset;\n"
        "        float amplitude = 0.0f;\n"
        "        getAmplitudeForXPos (xPos, amplitude);\n"
        "        float yPos = WAVE_RENDERING_HEIGHT * (amplitude + 1.0f) / 2.0f;\n"
            // Shift coordinates to center in the rendering box
        "        xPos = xPos - (WAVE_RENDERING_WIDTH / 2.0f);\n"
        "        yPos = yPos - (WAVE_RENDERING_HEIGHT / 2.0f);\n"
            // Extrude Points
        "        extrude (vec2 (xPos, yPos));\n"
        "    }\n"
        "\n"
        "}\n";
        
        */
        
        
       /*
        waveGeometryShader =
        "uniform vec2 resolution;\n"
        "uniform float audioSampleData[256];\n"
        "uniform mat4 projectionMatrix;\n"      // Use proj and view to run the final vertices through the view rotation and projection below
        "uniform mat4 viewMatrix;\n"            // gl_Position = projectionMatrix * viewMatrix * vec4(position, 0.0f, 1.0f);
        "\n"
        "void getAmplitudeForXPos (in float xPos, out float audioAmplitude)\n"
        "{\n"
                                // Buffer size - 1
        "   float perfectSamplePosition = 255.0f * xPos / resolution.x;\n"
        "   int leftSampleIndex = int (floor (perfectSamplePosition));\n"
        "   int rightSampleIndex = int (ceil (perfectSamplePosition));\n"
        "   audioAmplitude = mix (audioSampleData[leftSampleIndex], audioSampleData[rightSampleIndex], fract (perfectSamplePosition));\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "    float y = gl_FragCoord.y / resolution.y;\n"
        "\n"
        "    float amplitude = 0.0f;\n"
        "    getAmplitudeForXPos (gl_FragCoord.x, amplitude);\n"
        "\n"
        // Centers & Reduces Wave Amplitude
        "    amplitude = 0.5f - amplitude / 3.0f;\n"
        //"    vec4 colour = vec4(0.95, 0.57, 0.03, 0.7);\n"
        //"    gl_FragColor = colour;\n"
        "float r = abs (THICKNESS / (amplitude-y));\n"
        "\n"
        //"    vec4 pos = vec4 (x, r, 0.0, 1.0);\n"
        //"    vec4 transformedPos = projectionMatrix * viewMatrix * pos;\n"
        "\n"
        "gl_FragColor = vec4 (r - abs (r * 0.2f), r - abs (r * 0.2f), r - abs (r * 0.2f), 1.0f);\n"
        "}\n";

*/
        
        /*
        // Attempted Phong Fragment Shaders
        fragmentShader =
        "#version 330 core\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec3 LightPos;\n"   // Extra in variable, since we need the light position in view space we calculate this in the vertex shader
        
        "out vec4 color;\n"
        
        //"uniform vec3 lightColor;\n"
        //"uniform vec3 objectColor;\n"
        
        "void main()\n"
        "{\n"
            // Define Light Color & Wave Color
            "vec3 lightColor = vec3 (1.0f, 1.0f, 1.0f);\n"
            "vec3 waveColor = vec3 (0.0f, 1.0f, 0.0f);\n"
        
            // Ambient
            "float ambientStrength = 0.1f;\n"
            "vec3 ambient = ambientStrength * lightColor;\n"
            
            // Diffuse
            "vec3 norm = normalize (Normal);\n"
            "vec3 lightDir = normalize (LightPos - FragPos);\n"
            "float diff = max (dot (norm, lightDir), 0.0);\n"
            "vec3 diffuse = diff * lightColor;\n"
            
            // Specular
            "float specularStrength = 0.5f;\n"
            "vec3 viewDir = normalize (-FragPos);\n" // The viewer is at (0,0,0) so viewDir is (0,0,0) - Position => -Position
            "vec3 reflectDir = reflect (-lightDir, norm);\n"
            "float spec = pow (max(dot(viewDir, reflectDir), 0.0), 32);\n"
            "vec3 specular = specularStrength * spec * lightColor;\n"
            
            "vec3 result = (ambient + diffuse + specular) * waveColor;\n"
            "color = vec4(result, 1.0f);\n"
        "}\n";
        */
        
        
        // Base Shader, Paints the object green
        fragmentShader =
        "#version 330 core\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "    color = vec4 (0.0f, 1.0f, 0.0f, 1.0f);\n"
        "}\n";
        
        lightFragmentShader =
        "#version 330 core\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
            "color = vec4(1.0f);\n"
        "}\n";
        
       
//        // Base Shader, Paints the object green
//        fragmentShader =
//        "#version 330 core\n"
//        "out vec4 color;\n"
//        "void main()\n"
//        "{\n"
//        "    color = vec4 (0.0f, 1.0f, 0.0f, 1.0f);\n"
//        "}\n";
        
        
        /*
        //"varying vec4 destinationColour;\n"
        //"varying vec2 textureCoordOut;\n"
        "uniform vec2  resolution;\n"
        "uniform float audioSampleData[256];\n"
        //"uniform mat4 projectionMatrix;\n"
        //"uniform mat4 viewMatrix;\n"
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
        "#define THICKNESS 0.008\n"
        "void main()\n"
        "{\n"
        "    float y = gl_FragCoord.y / resolution.y;\n"
        "\n"
        "    float amplitude = 0.0;\n"
        "    getAmplitudeForXPos (gl_FragCoord.x, amplitude);\n"
        "\n"
        // Centers & Reduces Wave Amplitude
        "    amplitude = 0.5 - amplitude / 3.0;\n"
        //"    vec4 colour = vec4(0.95, 0.57, 0.03, 0.7);\n"
        //"    gl_FragColor = colour;\n"
        "float r = abs (THICKNESS / (amplitude-y));\n"
        "\n"
        //"    vec4 pos = vec4 (x, r, 0.0, 1.0);\n"
       //"    vec4 transformedPos = projectionMatrix * viewMatrix * pos;\n"
        "\n"
        "gl_FragColor = vec4 (r - abs (r * 0.2), r - abs (r * 0.2), r - abs (r * 0.2), 1.0);\n"
        "}\n";
        */
        ScopedPointer<OpenGLShaderProgram> newShader (new OpenGLShaderProgram (openGLContext));
        String statusText;
        
        if (newShader->addVertexShader ((vertexShader))
            && newShader->addShader (waveGeometryShader, GL_GEOMETRY_SHADER)
            && newShader->addFragmentShader ((fragmentShader))
            && newShader->link())
        {
            //attributes = nullptr;
            uniforms = nullptr;
            
            waveShader = newShader;
            waveShader->use();
            
            //attributes = new Attributes (openGLContext, *shader);
            uniforms   = new Uniforms (openGLContext, *waveShader);
            
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
            
            resolution          = createUniform (openGLContext, shaderProgram, "resolution");
            audioSampleData     = createUniform (openGLContext, shaderProgram, "audioSampleData");
            
        }
        
        ScopedPointer<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;
        ScopedPointer<OpenGLShaderProgram::Uniform> resolution, audioSampleData;
        ScopedPointer<OpenGLShaderProgram::Uniform> lightPosition;
        
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
    GLuint VBO, VAO;/*, EBO;*/
    
    ScopedPointer<OpenGLShaderProgram> waveShader;
    //ScopedPointer<Attributes> attributes;   // The private structs handle all attributes
    ScopedPointer<Uniforms> uniforms;       // The private structs handle all uniforms
    
    const char* vertexShader;
    const char* fragmentShader;
    const char* lightFragmentShader;
    const char* waveGeometryShader;
    
    // GUI Interaction
    Draggable3DOrientation draggableOrientation;
    
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


#endif /* Oscilloscope_h */
