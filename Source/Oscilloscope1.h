//
//  Oscilloscope1.h
//  3DAudioVisualisers
//
//  Created by Tim Arterbury on 4/26/17.
//
//

#ifndef Oscilloscope1_h
#define Oscilloscope1_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "RingBuffer.h"
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <glut.h>
#endif

class Oscilloscope1 :   public Component,
                        public OpenGLRenderer
{
public:
    
    Oscilloscope1 (RingBuffer<float> * audioBuffer)
    {
        this->audioBuffer = audioBuffer;
    
        // FIFO Initialization
        for (int i = 0; i < oscilloscopeRes; ++i)
        {
            oscillosopeHeightFifo[i] = 0.0;
        }
        oscopeFifoPos = 0;
        
        // Set default 3D orientation
        draggableOrientation.reset(Vector3D<float>(0.0, 1.0, 0.0));

        
        // Attach and start OpenGL
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        //openGLContext.setContinuousRepainting (true);
    }
    
    ~Oscilloscope1()
    {
        // Turn of OpenGL
        openGLContext.setContinuousRepainting (false);
        openGLContext.detach();
        
        audioBuffer = nullptr;
    }
    
    //==============================================================================
    // OpenGL Callbacks
    
    /** Called before rendering OpenGL, after an OpenGLContext has been associated
     with this OpenGLRenderer (this component is a OpenGLRenderer).
     Implement this method to set up any GL objects that you need for rendering.
    */
    void newOpenGLContextCreated() override
    {
        OpenGLHelpers::clear (Colours::black);
        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0f);                   // Set background depth to farthest
        glDepthFunc(GL_LEQUAL);
        glShadeModel(GL_SMOOTH);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        
        // Attempted Hip Lighting
        /*
         GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
         GLfloat mat_shininess[] = { 50.0 };
         GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
         
         glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
         glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
         glLightfv(GL_LIGHT0, GL_POSITION, light_position);
         
         
         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);
         */
    }
    
    /** Called when done rendering OpenGL, as an OpenGLContext object is closing.
     Implement this method to free any GL objects that you created during rendering.
    */
    void openGLContextClosing() override
    {
    }
    
    /** The OpenGL rendering callback.
     */
    void renderOpenGL() override
    {
        // Clear the depth buffer to allow overdraw
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Projection Matrix
        const int w = getWidth();
        const int h = getHeight();
        GLfloat aspect = (GLfloat)w / (GLfloat)h;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, aspect, 0.1f, 100.0f);
        
        // Draw Axes In Corner FOR DEBUGGING
        const float axesLength = 0.4;
        glPushMatrix();
        // Move the axes to the screen corner
        glTranslatef(-2.5, -2.0, 0.0);
        // Draw our axes
        glBegin(GL_LINES);
        // draw line for x axis
        glColor3f(1.0, 0.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(axesLength, 0.0, 0.0);
        // Draw line for y axis
        glColor3f(0.0, 1.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, axesLength, 0.0);
        // Draw line for Z axis
        glColor3f(0.0, 0.0, 1.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, 0.0, axesLength);
        glEnd();
        // Load the previous matrix
        glPopMatrix();
        
        
        // ModelView Matrix
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslated(0.0, 0.0, -7.0);
        Matrix3D<float> rotationMatrix = draggableOrientation.getRotationMatrix();
        glMultMatrixf(rotationMatrix.mat);
        
        const int numSamplesToRead = 50;
        float * samples = audioBuffer->readSamples (numSamplesToRead, 1);
        Range<float> maxLevel = FloatVectorOperations::findMinAndMax (samples, numSamplesToRead);
        delete [] samples;
        
        const float lineHeight = jmap (std::abs(maxLevel.getStart()), 0.0f, 1.0f, 0.0f, 3.0f);
        
        // Shift the All Oscope Data right
        memmove(oscillosopeHeightFifo + 1, oscillosopeHeightFifo, (oscilloscopeRes - 1) * sizeof (float));
        oscillosopeHeightFifo[0] = lineHeight;
        
        const float distanceBetweenCubes = 0.08;
        const float heightZoom = 100.0;
        const float cubeWidth = 0.03;
        
        glPushMatrix();
        glTranslated(-(distanceBetweenCubes * (float)oscilloscopeRes) / 2.0, 0.0, 0.0);
        glColor3f(0.4, 5.0, 8.0);
        for (int i = 0; i < oscilloscopeRes; ++i)
        {
            
            glPushMatrix();
            glTranslated((float) i * distanceBetweenCubes, 0.0, 0.0);
            glScalef(1.0, oscillosopeHeightFifo[i] * heightZoom, 1.0);
            glutSolidCube(cubeWidth);
            
            glPopMatrix();
            
            // CUBIC INTERPOLATION HACK
            
//            glPushMatrix();
//            glRotatef(-90.0, 1.0, 1.0, 1.0);
//            glutSolidCube(0.1);
//            glPopMatrix();
            
            glPushMatrix();
            glTranslated((float) i * distanceBetweenCubes, 0.0, 0.0);
            glScalef(1.0, 1.0, oscillosopeHeightFifo[i] * heightZoom);
            glutSolidCube(cubeWidth);
            
 
            glPopMatrix();
            
        }
        glPopMatrix();
    }
    
    
    void resized () override
    {
        draggableOrientation.setViewport (getLocalBounds());
    }
    
    void mouseDown (const MouseEvent& e) override
    {
        draggableOrientation.mouseDown (e.getPosition());
    }
    
    void mouseDrag (const MouseEvent& e) override
    {
        draggableOrientation.mouseDrag (e.getPosition());
    }
    
    
    void start()
    {
        openGLContext.setContinuousRepainting (true);
    }
    
    void stop()
    {
        openGLContext.setContinuousRepainting (false);
    }

    
    
    
private:
    
    const static int oscilloscopeRes = 100;
    float oscillosopeHeightFifo [oscilloscopeRes];
    int oscopeFifoPos;
    
    // Input Audio Buffer
    RingBuffer<float> * audioBuffer;

    Draggable3DOrientation draggableOrientation;
    OpenGLContext openGLContext;
    //ScopedPointer<OpenGLShaderProgram> shader; // For shaders if we do that crap
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscilloscope1)
};

#endif /* Oscilloscope1_h */
