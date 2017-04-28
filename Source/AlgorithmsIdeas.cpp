/* void renderOpenGL() override
 {
     // Clear the depth buffer to allow overdraw
     glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     
     // Projection Matrix
     const int w = getParentWidth();
     const int h = getParentHeight();
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
     */
    // Draw Grid
    /*
     glPushMatrix();
     glTranslatef (-5.0, -2.0, -10.0);
     glColor3f (1.0, 1.0, 1.0);
     glBegin (GL_LINES);
     for (int i = 0; i <= 10; ++i)
     {
     glVertex3f ((float)i, 0.0f, 0.0f);
     glVertex3f ((float)i, 0.0f, 10.0f);
     glVertex3f (0.0f, 0.0f, (float)i);
     glVertex3f (10.0f, 0.0f, (float)i);
     }
     glEnd();
     glPopMatrix();
     */

    // Draw Lame Cube
    //        glColor3f (1.0f, 1.0f, 1.0f);
    //        glutWireCube(1.2);
    //        glColor3f (0.1f, 0.4f, 1.0f);
    //        glutSolidCube(1.0);

    // Grab Samples from Ring Buffer and determine the amplitude to paint
    /* const int numSamplesToRead = 1400; // 50 samples per pixel suggestion http://www.supermegaultragroovy.com/2009/10/06/drawing-waveforms/
     
     // RING BUFFER PROBLEM
     // PROBLEM DOESN'T WORK FOR CHANNEL 0, EDIT RING BUFFER
     float * samples = ringBuffer->readSamples (numSamplesToRead, 1);
     Range<float> maxLevel = FloatVectorOperations::findMinAndMax (samples, numSamplesToRead);
     const float cubeSize = jmap (std::abs(maxLevel.getStart()), 0.0f, 1.0f, 0.4f, 4.0f);
     delete [] samples;*/



    //glColor3f (1.0f, 1.0f, 1.0f);
    //glutWireCube(cubeSize);
    //glColor3f (0.1f, 0.4f, 1.0f);
    //glutSolidCube(cubeSize);

    // Draw Oscilloscope Visualizer
    // X Axis: Time
    // Y Axis & Z Axis: Amplitude

    /* MODIFIED OSCOPE ALGORITHM
     const int numSamplesToRead = 50;
     float * samples = ringBuffer->readSamples (numSamplesToRead, 1);
     Range<float> maxLevel = FloatVectorOperations::findMinAndMax (samples, numSamplesToRead);
     delete [] samples;
     bool maxStart;
     if (std::abs(maxLevel.getStart()) > std::abs(maxLevel.getEnd()))
     {
     maxStart = true;
     }
     else
     {
     maxStart = false;
     }
     
     float lineHeight;
     if (maxStart)
     {
     lineHeight = jmap (maxLevel.getStart(), -1.0f, 1.0f, -2.0f, 2.0f);
     }
     else
     {
     lineHeight = jmap (maxLevel.getEnd(), -1.0f, 1.0f, -2.0f, 2.0f);
     }
     
     // Shift the All Oscope Data right
     memmove(oscillosopeHeightFifo + 1, oscillosopeHeightFifo, (oscilloscopeRes - 1) * sizeof (float));
     oscillosopeHeightFifo[0] = lineHeight;
     
     glPushMatrix();
     glTranslated(-(0.08 * (float)oscilloscopeRes) / 2.0, 0.0, 0.0);
     glBegin(GL_LINES);
     glColor3f(1.0, 0.0, 0.0);
     for (int i = 0; i < oscilloscopeRes; ++i)
     {
     glVertex3f ((float) i * 0.08, 0.0, 0.0); // X Position
     glVertex3f((float) i * 0.08, oscillosopeHeightFifo[i], 0.0); // Y Position
     //glVertex3f ((float) i * 0.08, 0.0, 0.0); // X Position
     //glVertex3f((float) i * 0.08, 0.0, oscillosopeHeightFifo[i]); // Z Position
     
     }
     glEnd();
     glPopMatrix();
     
     
     */

    /*
     const int numSamplesToRead = 50;
     float * samples = ringBuffer->readSamples (numSamplesToRead, 1);
     Range<float> maxLevel = FloatVectorOperations::findMinAndMax (samples, numSamplesToRead);
     delete [] samples;
     
     const float lineHeight = jmap (std::abs(maxLevel.getStart()), 0.0f, 1.0f, 0.0f, 3.0f);
     
     // Shift the All Oscope Data right
     memmove(oscillosopeHeightFifo + 1, oscillosopeHeightFifo, (oscilloscopeRes - 1) * sizeof (float));
     oscillosopeHeightFifo[0] = lineHeight;
     
     glPushMatrix();
     glTranslated(-(0.08 * (float)oscilloscopeRes) / 2.0, 0.0, 0.0);
     //glBegin(GL_LINES);
     glColor3f(0.4, 5.0, 8.0);
     for (int i = 0; i < oscilloscopeRes; ++i)
     {
     
     glPushMatrix();
     glTranslated((float) i * 0.08, 0.0, 0.0);
     glScalef(1.0, oscillosopeHeightFifo[i] * 16.0, 1.0);
     glutSolidCube(0.1);
     */
    // CUBIC INTERPOLATION HACK
    /*
     glPushMatrix();
     glRotatef(-90.0, 1.0, 1.0, 1.0);
     glutSolidCube(0.1);
     glPopMatrix();
     */

    //glPopMatrix();

    //glVertex3f ((float) i * 0.08, 0.0, 0.0); // X Position
    //glVertex3f((float) i * 0.08, oscillosopeHeightFifo[i], 0.0); // Y Position
    //glVertex3f ((float) i * 0.08, 0.0, 0.0); // X Position
    //glVertex3f((float) i * 0.08, -oscillosopeHeightFifo[i], 0.0); // Y Position
    //glVertex3f ((float) i * 0.08, 0.0, 0.0); // X Position
    //glVertex3f((float) i * 0.08, 0.0, oscillosopeHeightFifo[i]); // Z Position

    //}
    //glEnd();
    //glPopMatrix();
    // lines across x axis
    /* glColor3f(1.0, 0.0, 0.0);
     glVertex3f(0.0, 0.0, 0.0);
     glVertex3f(cubeSize, 0.0, 0.0);
     // Draw line for y axis
     glColor3f(0.0, 1.0, 0.0);
     glVertex3f(0.0, 0.0, 0.0);
     glVertex3f(0.0, cubeSize, 0.0);
     // Draw line for Z axis
     glColor3f(0.0, 0.0, 1.0);
     glVertex3f(0.0, 0.0, 0.0);
     glVertex3f(0.0, 0.0, cubeSize);
     glEnd();*/


// }

/** TEST VISUALIZER **/
/** TEMPORARY TIMER CALLBACK TO CONTINUOUSLY USE JUCE PAINT
 This is to demo the RingBuffer and make sure it is working.
 */

//Constructor()
//:audioImage (Image::RGB, 512, 512, true) // For the temporary 2D visualiser
//{
    
//}
// TEMPORARY NonGL 2D testing visualizer
//Image audioImage;

//void timerCallback() override
//{
//    drawNextLineOfTestOscilloscope();
//    repaint();
//}

/*
void drawNextLineOfTestOscilloscope()
{
    const int rightHandEdge = audioImage.getWidth() - 1;
    const int imageHeight = audioImage.getHeight();
    const int imageMidPoint = imageHeight / 2.0;
    
    // first, shuffle our image leftwards by 1 pixel..
    audioImage.moveImageSection (0, 0, 1, 0, rightHandEdge, imageHeight);
    
    // Grab Samples from Ring Buffer and determine the amplitude to paint
    const int numSamplesToRead = 50; // 50 samples per pixel suggestion http://www.supermegaultragroovy.com/2009/10/06/drawing-waveforms/
    
    // RING BUFFER PROBLEM
    // PROBLEM DOESN'T WORK FOR CHANNEL 0, EDIT RING BUFFER
    float * samples = ringBuffer->readSamples (numSamplesToRead, 1);
    Range<float> maxLevel = FloatVectorOperations::findMinAndMax (samples, numSamplesToRead);
    const float heightToPaint = jmap (std::abs(maxLevel.getStart()), 0.0f, 1.0f, 0.0f, (float) imageHeight / 2.0f);
    delete [] samples;
    
    // Set pixels for visualizer image
    for (int y = 1; y < imageHeight; ++y)
    {
        if (y >= imageMidPoint - heightToPaint && y <= imageMidPoint + heightToPaint)
            audioImage.setPixelAt (rightHandEdge, y, Colours::lightblue);
        else
            audioImage.setPixelAt (rightHandEdge, y, Colours::black);
    }
}
*/
/*
void paint (Graphics& g) override
{
    // You can add your component specific drawing code here!
    // This will draw over the top of the openGL background.
    
    g.fillAll (Colours::grey);
    
    // Draw TEMPORARY visualiser image
    g.setOpacity (0.5f);
    g.drawImage (audioImage, getLocalBounds().toFloat());
    
}*/
