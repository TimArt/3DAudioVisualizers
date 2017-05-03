#include "fmod.hpp"
#include "fmod_errors.h"
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#define GLEW_STATIC
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cerrno>

std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  return "";
}

#define WIDTH 1280
#define HEIGHT 720
#define SHADER_FILE "Shader.frag"
#define SONG_FILE "Moonlight.mp3"

//Vertex shader
// Makes all the XY values of the vertices have a Z value of 0.0.
const GLchar* vertexSource =
    "#version 150 core\n"
    "in vec2 position;"
    "void main() {"
    "   gl_Position = vec4(position, 0.0, 1.0);"
    "}";

struct ToGLStr {
  const char *p;
  ToGLStr(const std::string& s) : p(s.c_str()) {}
  operator const char**() { return &p; }
};

int main() {
	//initiate FMOD 
	FMOD::System *system;
	FMOD::Sound *sound;
	FMOD::Channel *channel;
	FMOD_RESULT result;

	FMOD::System_Create(&system);
	channel = 0;
	system->init(32, FMOD_INIT_NORMAL,0);

	//Create window
	sf::Window window(sf::VideoMode(WIDTH,HEIGHT), "Visualizer", sf::Style::Close);
	window.setVerticalSyncEnabled(true);

	glewExperimental = GL_TRUE;
	glewInit();

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);

    // Defines a square using two triangles
	float vertices[] = {
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

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
	std::string shader = get_file_contents(SHADER_FILE);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, ToGLStr(shader), NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "gl_FragColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

	// Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLint timeLoc = glGetUniformLocation(shaderProgram, "iGlobalTime");

	GLint sampleLoc = glGetUniformLocation(shaderProgram, "Spectrum");
	GLint waveLoc = glGetUniformLocation(shaderProgram, "Wavedata");

	GLint resLoc = glGetUniformLocation(shaderProgram, "iResolution");
	
	glUniform3f(resLoc, WIDTH, HEIGHT, WIDTH*HEIGHT);

	while(window.isOpen())
	{
		sf::Event windowEvent;
		while(window.pollEvent(windowEvent))
		{
			switch(windowEvent.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				switch(windowEvent.key.code) {
				case sf::Keyboard::P:
					system->createSound(SONG_FILE, FMOD_HARDWARE, 0, &sound);
					sound->setMode(FMOD_LOOP_OFF);
					system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
					break;
				case sf::Keyboard::R: //Reload the shader
					//hopefully this is safe
					glDeleteProgram(shaderProgram);
					glDeleteShader(fragmentShader);
					glDeleteShader(vertexShader);

					vertexShader = glCreateShader(GL_VERTEX_SHADER);
					glShaderSource(vertexShader, 1, &vertexSource, NULL);
					glCompileShader(vertexShader);

					// Create and compile the fragment shader
					shader = get_file_contents(SHADER_FILE);

					fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSource(fragmentShader, 1, ToGLStr(shader), NULL);
					glCompileShader(fragmentShader);

					// Link the vertex and fragment shader into a shader program
					shaderProgram = glCreateProgram();
					glAttachShader(shaderProgram, vertexShader);
					glAttachShader(shaderProgram, fragmentShader);
					glBindFragDataLocation(shaderProgram, 0, "gl_FragColor");
					glLinkProgram(shaderProgram);
					glUseProgram(shaderProgram);

					// Specify the layout of the vertex data
					posAttrib = glGetAttribLocation(shaderProgram, "position");
					glEnableVertexAttribArray(posAttrib);
					glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

					timeLoc = glGetUniformLocation(shaderProgram, "iGlobalTime");

					sampleLoc = glGetUniformLocation(shaderProgram, "Spectrum");
					waveLoc = glGetUniformLocation(shaderProgram, "Wavedata");

					resLoc = glGetUniformLocation(shaderProgram, "iResolution");
	
					glUniform3f(resLoc, WIDTH, HEIGHT, WIDTH*HEIGHT);
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
				break;
			}
		}
		
		float spectrum[256];
		float wavedata[512];
		system->getWaveData(wavedata, 256, 0);
		system->getSpectrum(spectrum, 256, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);

		GLfloat time = (GLfloat)clock()/ (GLfloat)CLOCKS_PER_SEC;
		glUniform1f(timeLoc, time);
		glUniform1fv(sampleLoc, 256, spectrum);
		glUniform1fv(waveLoc, 256, wavedata);
		glClearColor(0.0f,0.0f,0.0f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		window.display();
	}
	
	//Clean up
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(1, &vbo);

	glDeleteVertexArrays(1, &vao);
}
