
/** This is an unused class that shows the basic framework of a Visualizer
    we will be implementing.
 
    This could've been used as a class to inherit from but, it would reduce
    the flexibility of using some of the objects and the code might be a bit
    more confusing. Instead, take this file, copy it, and replace all the
    "OpenGLVisualizerSkeleton" instances with the name of your visualizer class.
*/

#ifndef OpenGLVisualizerSkeleton_h
#define OpenGLVisualizerSkeleton_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "RingBuffer.h"
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <glut.h>
#endif

class OpenGLVisualizerSkeleton :    public Component,
                                    public OpenGLRenderer
{
public:
    
    OpenGLVisualizerSkeleton (RingBuffer<float> * audioBuffer)
    {
        this->audioBuffer = audioBuffer;
        
        // Set default 3D orientation
        draggableOrientation.reset(Vector3D<float>(0.0, 1.0, 0.0));
        
        // Attach OpenGL Context
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
    }
    
    ~OpenGLVisualizerSkeleton()
    {
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
        
        // Do Lighting Crap Here if you're feeling that
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
    // OpenGL Components
    OpenGLContext openGLContext;
    Draggable3DOrientation draggableOrientation; // Used for global rotation matrix
    
    // Input Audio Buffer
    RingBuffer<float> * audioBuffer;
    
};

#endif /* OpenGLVisualizerSkeleton_h */
