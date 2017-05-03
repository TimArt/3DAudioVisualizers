//  Copyright © 2016 Fatih Gazimzyanov. Contacts: virgil7g@gmail.com
//  Copyright © 2013 Tyler Tesch.       Contacts: https://www.youtube.com/user/Advenio4821

//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//          http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and

#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <map>
#include "FFTAudioStream.h"

std::string getFileContents(const char *filename) {
    std::ifstream in (filename, std::ios::in | std::ios::binary);
    if (in) {
        std::string contents;
        in.seekg (0, std::ios::end);
        contents.resize((unsigned int) in.tellg());
        in.seekg (0, std::ios::beg);
        in.read (&contents[0], contents.size());
        in.close();
        return (contents);
    }
    return "";
}

#define WIDTH 1280
#define HEIGHT 720
#define SHADER_FILE_WAVE "D:/Code/Projects/sound-visualizer/Shader_wave.frag"
#define SHADER_FILE_SPECTRUM "D:/Code/Projects/sound-visualizer/Shader_spectrum.frag"
#define SHADER_FILE_EQUALIZER "D:/Code/Projects/sound-visualizer/Shader_equalizer.frag"

enum VISUALIZATION_MODE {
    WAVE, SPECTRUM, EQUALIZER
};

static const int WAVE_DATA_SIZE = 256;
static const int EQUALIZER_COLUMNS = 32;
static const int EQUALIZER_INERTIA = 500;
static const int COLUMNS_MARGIN = 4;

float waveData[WAVE_DATA_SIZE];
float spectrumData[WAVE_DATA_SIZE];
float previousSpectrumData[WAVE_DATA_SIZE];
int columnsInertia[EQUALIZER_COLUMNS];
std::map<int, std::string> shaderPathMap;
std::map<int, std::string> songsPathMap;
int currentShader = 0;
int currentSong = 0;

sf::SoundBuffer soundBuffer;
FFTAudioStream fftAudioStream;
GLuint vertexShader;
GLuint fragmentShader;
GLuint shaderProgram;
GLint posAttrib;
GLint timeLoc;
GLint sampleLoc;
GLint waveLoc;
GLint resLoc;
GLuint vao;
GLuint vbo;
GLint lowFilterLoc;
GLint highFilterLoc;

float shaderVertices[] = {
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f
};


// Vertex shader
const GLchar *vertexSource =
        "#version 150 core\n"
                "in vec2 position;"
                "void main() {"
                "   gl_Position = vec4(position, 0.0, 1.0);"
                "}";

struct ToGLStr {
    const char *p;

    ToGLStr(const std::string &s) : p(s.c_str()) {}

    operator const char **() { return &p; }
};


void equalizerModesInitialization();

void loadAudioFile(std::string filePath);

void deleteShader();

void loadShader(std::string shaderFilePath);

void openGLInitialization();

void openGLCleanup();

void waveDataVisualization(const std::vector<complex> dataVector);

void spectrumVisualisation(const std::vector<complex> dataVector);

void equalizerVisualization(const std::vector<complex> dataVector);

void getFileNames(int argumentCount, char *argumentArray[]);

/**
 *  Main function
 *
 *  @Controls
 *  P - play/pause;
 *  M - switch visualization mode;
 *  R - reload the shader;
 *  T - change track;
 *  Q - increase bottom filter threshold;
 *  Z - decrease bottom filter threshold;
 *  W - increase upper filter threshold;
 *  X - decrease upper filter threshold;
 *  Escape - exit.
 * */
int main(int argumentCount, char *argumentArray[]) {

    getFileNames(argumentCount, argumentArray);
    loadAudioFile(songsPathMap[currentSong]);
    equalizerModesInitialization();

    // Create window
    sf::Window window(sf::VideoMode(WIDTH, HEIGHT), songsPathMap[currentSong], sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    openGLInitialization();
    loadShader(shaderPathMap[currentShader]);

    while (window.isOpen()) {
        sf::Event windowEvent;
        while (window.pollEvent(windowEvent)) {
            switch (windowEvent.type) {
                case sf::Event::Closed:
                    fftAudioStream.stop();
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch (windowEvent.key.code) {
                        case sf::Keyboard::P:
                            // Play Sound
                            fftAudioStream.getStatus() != sf::SoundSource::Status::Playing
                            ? fftAudioStream.play()
                            : fftAudioStream.pause();
                            break;
                        case sf::Keyboard::M:
                            // Mode switching
                            currentShader == shaderPathMap.size() - 1
                            ? (currentShader = 0)
                            : (currentShader++);
                            loadShader(shaderPathMap[currentShader]);
                            break;
                        case sf::Keyboard::R:
                            // Reload the shader
                            // hopefully this is safe
                            deleteShader();
                            loadShader(shaderPathMap[currentShader]);
                            break;
                        case sf::Keyboard::T:
                            currentSong == songsPathMap.size() - 1
                            ? (currentSong = 0)
                            : (currentSong++);
                            loadAudioFile(songsPathMap[currentSong]);
                            window.setTitle(songsPathMap[currentSong]);
                            fftAudioStream.play();
                            break;
                        case sf::Keyboard::Q:
                            fftAudioStream.setLowFilterValue(fftAudioStream.getLowFilterValue() + 5);
                            break;
                        case sf::Keyboard::Z:
                            fftAudioStream.setLowFilterValue(fftAudioStream.getLowFilterValue() - 5);
                            break;
                        case sf::Keyboard::W:
                            fftAudioStream.setHighFilterValue(fftAudioStream.getHighFilterValue() + 5);
                            break;
                        case sf::Keyboard::X:
                            fftAudioStream.setHighFilterValue(fftAudioStream.getHighFilterValue() - 5);
                            break;
                        case sf::Keyboard::Escape:
                            window.close();
                            break;
                    }
                    break;
            }
        }

        GLfloat time = (GLfloat) clock() / (GLfloat) CLOCKS_PER_SEC;
        glUniform1f(timeLoc, time);

        switch (currentShader) {
            case WAVE:
                waveDataVisualization(fftAudioStream.getCurrentSampleWaveVector());
                glUniform1fv(waveLoc, WAVE_DATA_SIZE, waveData);
                break;
            case SPECTRUM:
                spectrumVisualisation(fftAudioStream.getCurrentSampleCleanSpectrumVector());
                glUniform1fv(waveLoc, WAVE_DATA_SIZE, spectrumData);
                glUniform1f(lowFilterLoc, fftAudioStream.getLowFilterValue());
                glUniform1f(highFilterLoc, fftAudioStream.getHighFilterValue());
                break;
            case EQUALIZER:
                equalizerVisualization(fftAudioStream.getCurrentSampleSpectrumVector());
                glUniform1fv(waveLoc, WAVE_DATA_SIZE, spectrumData);
                break;
            default:
                break;
        }

        glUniform1fv(sampleLoc, WAVE_DATA_SIZE, spectrumData);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        window.display();
    }
    openGLCleanup();
}

/**
 *  Getting file from main() arguments.
 *
 *  You can just drag&drop music files on *.exe file or use "Open with..." dialogue.
 * */
void getFileNames(int argumentCount, char *argumentArray[]) {
    if (argumentCount < 2) {
        songsPathMap[0] = "You didn't provide a music file\n";
    } else {
        for (int argument = 0; argument < argumentCount - 1; ++argument) {
            songsPathMap[argument] = argumentArray[argument + 1];
            std::cout << argumentArray[argument + 1] << std::endl;
        }
    }
}

/**
 *  Wave data visualization function.
 *
 *  Simply shows sample wave visualization.
 *
 *  @param dataVector should contain wave data.
 * */
void waveDataVisualization(const std::vector<complex> dataVector) {
    if (dataVector.data() != NULL) {
        int picker = FFTAudioStream::SAMPLES_TO_STREAM / WAVE_DATA_SIZE;
        for (int i = 0; i < WAVE_DATA_SIZE; i++) {
            waveData[i] = (float) dataVector[i * picker].re();
            waveData[i] *= 0.0001;
        }
    }
}

/**
 *  "Equalizer" visualization function.
 *
 *  Shows dynamic spectrum waves with inertial animation.
 *  Pick data from big sample vector and convert to screen array.
 *  Also you can control number of columns and columns margin with EQUALIZER_COLUMNS and COLUMNS_MARGIN.
 *  You can change inertia with EQUALIZER_INERTIA.
 *
 *  @param dataVector should contain spectrum data.
 * */
void equalizerVisualization(const std::vector<complex> dataVector) {
    if (dataVector.data() != NULL) {
        int picker = FFTAudioStream::SAMPLES_TO_STREAM / 4 / WAVE_DATA_SIZE;
        float sum = 0;
        int columnWidth = WAVE_DATA_SIZE / EQUALIZER_COLUMNS;
        for (int columnNumber = 0; columnNumber < EQUALIZER_COLUMNS; columnNumber++) {
            for (int i = 0; i < columnWidth; i++) {
                int currentIndex = columnWidth * columnNumber + i;
                sum += dataVector[currentIndex * picker].re();
            }
            sum *= 0.00000015;
            for (int i = 0; i < columnWidth; i++) {
                int currentIndex = columnWidth * columnNumber + i;
                if (i < columnWidth - COLUMNS_MARGIN) {
                    if (sum > previousSpectrumData[currentIndex]) {
                        columnsInertia[columnNumber] = EQUALIZER_INERTIA;
                        spectrumData[currentIndex] = sum;
                    } else {
                        spectrumData[currentIndex] = previousSpectrumData[currentIndex] -
                                                     previousSpectrumData[currentIndex] *
                                                     ((float) EQUALIZER_INERTIA + 1 -
                                                      columnsInertia[columnNumber]) / ((float) EQUALIZER_INERTIA);
                    }
                } else {
                    spectrumData[currentIndex] = 0.0;
                }
            }
            sum = 0;
        }
    }

    for (int i = 0; i < EQUALIZER_COLUMNS; ++i) {
        if (columnsInertia[i] != 0) {
            columnsInertia[i]--;
        }
    }

    for (int i = 0; i < WAVE_DATA_SIZE; ++i) {
        previousSpectrumData[i] = spectrumData[i];
    }
}

/**
 *  Spectrum visualization function.
 *
 *  Pick data from big sample vector and convert to screen array.
 *
 *  @param dataVector should contain spectrum data.
 * */
void spectrumVisualisation(const std::vector<complex> dataVector) {
    if (dataVector.data() != NULL) {
        int picker = FFTAudioStream::SAMPLES_TO_STREAM / 4 / WAVE_DATA_SIZE;
        for (int i = 0; i < WAVE_DATA_SIZE; i++) {
            spectrumData[i] = (float) dataVector[i * picker].re();
            spectrumData[i] *= 0.000001;
        }
    }
}

/**
 *  Final openGL memory cleanup.
 * */
void openGLCleanup() {
    // Clean up
    deleteShader();
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

/**
 *  Setup base vertex shader.
 * */
void openGLInitialization() {
    glewExperimental = GL_TRUE;
    glewInit();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shaderVertices), shaderVertices, GL_STATIC_DRAW);
}

/**
 *  Shader file loading.
 *
 *  Loads shader file (you can use absolute paths), sets shader variables location.
 *
 *  @param shaderFilePath is a path to your *.frag file.
 * */
void loadShader(std::string shaderFilePath) {
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    std::string shaderData = getFileContents(shaderFilePath.data());

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, ToGLStr(shaderData), NULL);
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
    glEnableVertexAttribArray((GLuint) posAttrib);
    glVertexAttribPointer((GLuint) posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

    timeLoc = glGetUniformLocation(shaderProgram, "iGlobalTime");
    sampleLoc = glGetUniformLocation(shaderProgram, "spectrum");
    waveLoc = glGetUniformLocation(shaderProgram, "waveData");
    resLoc = glGetUniformLocation(shaderProgram, "iResolution");
    lowFilterLoc = glGetUniformLocation(shaderProgram, "lowFilter");
    highFilterLoc = glGetUniformLocation(shaderProgram, "highFilter");

    glUniform3f(resLoc, WIDTH, HEIGHT, WIDTH * HEIGHT);
}

/**
 *  Audio file loading to soundBuffer.
 *
 *  @warning Use >= 160 KB/sec file bitrate.
 *
 *  @param filePath takes from main() arguments, don't use absolute path in release.
 */
void loadAudioFile(std::string filePath) {
    fftAudioStream.stop();
    // load an audio soundBuffer from a sound file
    soundBuffer.loadFromFile(filePath);
    // initialize and play our custom stream
    fftAudioStream.load(soundBuffer);
}

/**
 *  Shader files initialization.
 *
 *  Add shader file path for each visualization mode.
 *  It will be used in visualization switching.
 */
void equalizerModesInitialization() {
    shaderPathMap[WAVE] = SHADER_FILE_WAVE;
    shaderPathMap[SPECTRUM] = SHADER_FILE_SPECTRUM;
    shaderPathMap[EQUALIZER] = SHADER_FILE_EQUALIZER;
}

void deleteShader() {
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
}
