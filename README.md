# 3DAudioVisualizers - C\++, JUCE, OpenGL

A 3D audio visualizer suite that reads in audio files or pulls audio from microphone input and shows visualizations rendered with OpenGL. This suite includes 3 visualizers:
1. 2D oscilloscope
2. 3D oscilloscope pipe thing
3. 3D audio spectrum.
_The 3D visualizers can be rotated realtime, by dragging on the visualizer with the mouse._

A basic demonstration can be seen on [YouTube].
The video demonstrates the project in its final state before I turned it in for my Computer Graphics class at Baylor. What is seen in the video was created in a single week (which was super hard lol even though it’s not the prettiest).

Since the original demonstration above, I’ve fixed a lot of bugs, cleaned up some comments, and redesigned the **RingBuffer**.

## Some Things I Learned
- **Ring Buffers** are important to Realtime, Lock Free programming (especially in audio development). The realtime audio thread can quickly feed audio into the ring buffer repeatedly without any real performance hit. Other “not as realtime” threads, such as the graphics thread can pull from that ring buffer, always grabbing the most recent data. These lower priority threads can allow for some latency in their callbacks. For example, if you drop a few visual frames in your graphics thread, no one will really notice. But, if you drop some samples in your audio thread, the audio will sound bad.
- **OpenGL and shaders** are a pain to setup, but they are super powerful and modular once you know how to use them. (To other first time OpenGLers: don’t get discouraged, you’ll get it eventually. Also this website is insanely helpful: [LearnOpenGL.com]). I’m no expert on OpenGL at all, but was able to make this.
- **Audio Waveform visualizations** actually exclude a good amount of data. They tend to average the data because there is so much of it. For example, the most standard audio sampling rate is 44,100 samples per second. If you were to attempt to visualize all 44,100 samples in a single second of audio with one sample per pixel on your screen, it wouldn’t even fit on the screen. My Mac has a horizontal resolution of 2,880 pixels. That would only allow you to visualize 1/15th of all the audio stored in a single second! Various strategies are used to average this data and produce a good looking waveform. This [blog post] was very helpful. For my oscilloscope visualizations I used an interpolation technique instead of averaging samples together. I may write a post on this technique later.
- **OpenGL and the GPU** can handle a lot, including calculating and generating 3D points in realtime. In the 3D oscilloscope visualizer I used the geometry shader to calculate many points on the fly on the GPU. The upside to this is less CPU processing is used to calculate points, allowing the audio thread utilize the CPU without worry. The downside is that it takes a bit of extra time to load this visualizer because it must instantiate a lot of variables on the GPU for processing. In this situation I probably would’ve just calculated the points on the CPU since it wouldn’t really have been that intensive to calculate.
- **Dry-erase boards** are freaking awesome. To get a lot of the audio visualization algorithms mapped out I avidly used the dry erase boards. I looked like a mad scientist: [add funny image here . . .]