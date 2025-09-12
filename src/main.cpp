#define GL_SILENCE_DEPRECATION
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

// Use OpenGL 3.3+ core functions directly
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <map>
#include <iomanip>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/stb_truetype.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace sf;

// Mandelbrot set parameters
struct MandelbrotParams {
    double zoom = 1.0;
    double offsetX = 0.3;
    double offsetY = 1.0;
    int maxIterations = 100;
    int colorMode = 0;
    int colorModeBg = 0;
    bool adaptiveIterations = true;
    
    // Mouse interaction state
    bool isDragging = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;

    vector<Vector3f> colors = {
        Vector3f(1.0f, 1.0f, 1.0f),
        Vector3f(1.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        Vector3f(0.0f, 0.0f, 1.0f),
        Vector3f(1.0f, 1.0f, 0.0f),
        Vector3f(1.0f, 0.0f, 1.0f),
        Vector3f(0.0f, 1.0f, 1.0f),
    };
    vector<Vector3f> colorsBg = {
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.5f, 0.0f, 0.0f),
        Vector3f(0.0f, 0.5f, 0.0f),
        Vector3f(0.0f, 0.0f, 0.5f),
        Vector3f(0.5f, 0.5f, 0.0f),
        Vector3f(0.5f, 0.0f, 0.5f),
        Vector3f(0.0f, 0.5f, 0.5f),
    };
    
    void reset() {
        zoom = 2.0;
        offsetX = 0.0;
        offsetY = 0.0;
        maxIterations = 100;
        colorMode = 0;
        adaptiveIterations = true;
    }
};

enum ArgType {
    ARG_NO_DEPTH,
    ARG_AA,
    ARG_VSYNC,
    ARG_USE_DOUBLE,
    ARG_MAX_ITERS,
    ARG_UNKNOWN
};

ArgType getArgType(const char* arg) {
    if (strcmp(arg, "--no-depth") == 0) return ARG_NO_DEPTH;
    if (strcmp(arg, "--aa") == 0)       return ARG_AA;
    if (strcmp(arg, "--vsync") == 0)    return ARG_VSYNC;
    if (strcmp(arg, "--use-double") == 0) return ARG_USE_DOUBLE;
    if (strcmp(arg, "--max-iters") == 0) return ARG_MAX_ITERS;
    return ARG_UNKNOWN;
}

// Function to check OpenGL errors
void checkGLError(const string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cerr << "OpenGL error after " << operation << ": " << error;
        switch(error) {
            case GL_INVALID_ENUM: cerr << " (GL_INVALID_ENUM)"; break;
            case GL_INVALID_VALUE: cerr << " (GL_INVALID_VALUE)"; break;
            case GL_INVALID_OPERATION: cerr << " (GL_INVALID_OPERATION)"; break;
            case GL_OUT_OF_MEMORY: cerr << " (GL_OUT_OF_MEMORY)"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: cerr << " (GL_INVALID_FRAMEBUFFER_OPERATION)"; break;
            default: break;
        }
        cerr << endl;
    }
}

// Function to read shader file
string readShaderFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Failed to open shader file: " << filepath << endl;
        return "";
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to compile shader
GLuint compileShader(const string& source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        cerr << "Shader compilation failed: " << infoLog << endl;
    }
    
    return shader;
}

// Function to create shader program
GLuint createShaderProgram(const string& vertexPath, const string& fragmentPath, bool useDouble = false) {
    string vertexSource = readShaderFile(vertexPath);
    string fragmentSource = readShaderFile(fragmentPath);
    
    // Add double precision define if requested
    if (useDouble) {
        // Insert the define after the version directive
        size_t versionEnd = fragmentSource.find('\n');
        if (versionEnd != string::npos) {
            fragmentSource.insert(versionEnd + 1, "#define USE_DOUBLE_PRECISION\n");
        }
    }
    
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check linking status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        cerr << "Shader program linking failed: " << infoLog << endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

// Character structure for text rendering
struct Character {
    GLuint textureID;  // ID handle of the glyph texture
    int sizeX, sizeY;  // Size of glyph
    int bearingX, bearingY;  // Offset from baseline to left/top of glyph
    int advance;       // Horizontal offset to advance to next glyph
};

// Text renderer class
class TextRenderer {
public:
    map<char, Character> characters;
    GLuint VAO, VBO;
    GLuint shaderProgram;
    
    TextRenderer() {}
    
    bool initialize(const string& fontPath, int fontSize) {
        // Load font file
        ifstream file(fontPath, ios::binary);
        if (!file.is_open()) {
            cerr << "Failed to open font file: " << fontPath << endl;
            return false;
        }
        
        // Get file size and read data
        file.seekg(0, ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, ios::beg);
        
        vector<unsigned char> fontData(fileSize);
        file.read(reinterpret_cast<char*>(fontData.data()), fileSize);
        file.close();
        
        // Initialize stb_truetype
        stbtt_fontinfo font;
        if (!stbtt_InitFont(&font, fontData.data(), 0)) {
            cerr << "Failed to initialize font" << endl;
            return false;
        }
        
        // Calculate scale for desired font size
        float scale = stbtt_ScaleForPixelHeight(&font, fontSize);
        
        // Generate characters for ASCII 32-127
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        for (unsigned char c = 32; c < 128; c++) {
            int width, height, xoff, yoff;
            unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xoff, &yoff);
            
            // Generate texture
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
            
            // Set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            // Get character metrics
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
            
            // Store character
            Character character = {
                texture,
                width, height,
                xoff, yoff,
                (int)(advance * scale)
            };
            characters[c] = character;
            
            // Free bitmap data
            stbtt_FreeBitmap(bitmap, nullptr);
        }
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // Create shader program for text
        shaderProgram = createShaderProgram("res/shaders/text_vertex.glsl", "res/shaders/text_fragment.glsl");
        if (shaderProgram == 0) {
            cerr << "Failed to create text shader program!" << endl;
            return false;
        }
        
        // Configure VAO/VBO for texture quads
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        return true;
    }
    
    void renderText(const string& text, float x, float y, float scale, const sf::Vector3f& color, int windowWidth, int windowHeight) {
        // Create orthographic projection matrix
        float left = 0.0f;
        float right = static_cast<float>(windowWidth);
        float bottom = static_cast<float>(windowHeight);
        float top = 0.0f;
        float near_plane = -1.0f;
        float far_plane = 1.0f;
        
        // Orthographic projection matrix
        float projection[16] = {
            2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left),
            0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom),
            0.0f, 0.0f, -2.0f / (far_plane - near_plane), -(far_plane + near_plane) / (far_plane - near_plane),
            0.0f, 0.0f, 0.0f, 1.0f
        };
        
        // Enable blending for text rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Use text shader
        glUseProgram(shaderProgram);
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        GLint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
        GLint texLoc = glGetUniformLocation(shaderProgram, "text");
        
        glUniformMatrix4fv(projLoc, 1, GL_TRUE, projection); // GL_TRUE for row-major
        glUniform3f(colorLoc, color.x, color.y, color.z);
        glActiveTexture(GL_TEXTURE0);
        if (texLoc != -1) {
            glUniform1i(texLoc, 0); // Bind texture unit 0
        }
        glBindVertexArray(VAO);
        
        // Iterate through all characters
        for (char c : text) {
            Character ch = characters[c];
            
            float xpos = x + ch.bearingX * scale;
            float ypos = y - (ch.sizeY - ch.bearingY) * scale;
            
            float w = ch.sizeX * scale;
            float h = ch.sizeY * scale;
            
            // Update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 1.0f },            
                { xpos,     ypos,       0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 0.0f },
                
                { xpos,     ypos + h,   0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 0.0f },
                { xpos + w, ypos + h,   1.0f, 1.0f }           
            };
            
            // Render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            
            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Advance cursors for next glyph
            x += ch.advance * scale;
        }
        
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }
    
    ~TextRenderer() {
        // Cleanup
        for (auto& pair : characters) {
            glDeleteTextures(1, &pair.second.textureID);
        }
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (shaderProgram) glDeleteProgram(shaderProgram);
    }
};

int main(int argc, char* argv[]) {
    // Initialize Mandelbrot parameters
    MandelbrotParams params;
    // Create context settings with depth buffer
    ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 4;  // OpenGL 4.1 required for double precision
    settings.minorVersion = 1;
    settings.antiAliasingLevel = 4;
    settings.attributeFlags = ContextSettings::Core;  // Request core profile

    bool useDouble = false; bool useVsync = true;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            switch (getArgType(argv[i])) {
                case ARG_NO_DEPTH:
                    settings.depthBits = 0;
                    settings.stencilBits = 0;
                    break;
                case ARG_AA: {
                    if (i + 1 >= argc) {
                        cerr << "Missing value for --aa" << endl;
                        return -1;
                    }
                    int value = stoi(argv[++i]); // consume number
                    if (value < 0 || value > 16) {
                        cerr << "Anti-aliasing level must be between 0 and 16" << endl;
                        return -1;
                    }
                    settings.antiAliasingLevel = value;
                    break;
                }
                case ARG_VSYNC: {
                    if (i + 1 >= argc) {
                        cerr << "Missing value for --vsync (true/false)" << endl;
                        return -1;
                    }
                    string val = argv[++i];
                    if (val == "true") {
                        useVsync = true;
                    } else if (val == "false") {
                        useVsync = false;
                    } else {
                        cerr << "Invalid value for --vsync (must be true/false)" << endl;
                        return -1;
                    }
                    break;
                }
                case ARG_MAX_ITERS: {
                    if (i + 1 >= argc) {
                        cerr << "Missing value for --max-iters" << endl;
                        return -1;
                    }
                    int value = stoi(argv[++i]); // consume number
                    if (value < 0 || value > 1000) {
                        cerr << "Max iterations must be between 0 and 1000" << endl;
                        return -1;
                    }
                    params.maxIterations = value;
                    break;
                }
                case ARG_USE_DOUBLE: useDouble = true; break;
                case ARG_UNKNOWN: cerr << "Unknown argument: " << argv[i] << endl; return -1;
            }
        }
    }
    
    // create the window with OpenGL context settings
    Window window(VideoMode({1200, 800}), "Mandelbrot Set Explorer - C++", State::Windowed, settings);
    window.setVerticalSyncEnabled(useVsync);

    // activate the window
    if (!window.setActive(true)) {
        cerr << "Warning: Failed to activate OpenGL context" << endl;
    }

    // Initialize OpenGL states
    glDisable(GL_DEPTH_TEST);  // We don't need depth testing for 2D rendering
    glDisable(GL_CULL_FACE);   // Disable face culling
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background
    checkGLError("OpenGL initialization");

    // Print OpenGL version info
    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    
#ifdef __APPLE__
    cout << "Note: On macOS, you may see 'FALLBACK' warnings - these are expected and don't affect functionality." << endl;
#endif

    // Create shader program
    GLuint shaderProgram = createShaderProgram("res/shaders/vertex.glsl", "res/shaders/fragment.glsl", useDouble);
    if (shaderProgram == 0) {
        cerr << "Failed to create shader program!" << endl;
        return -1;
    }
    cout << "Shader program created successfully!" << endl;
    
    // Initialize text renderer
    TextRenderer textRenderer;
    if (!textRenderer.initialize("res/OpenSans-Regular.ttf", 24)) {
        cerr << "Failed to initialize text renderer!" << endl;
        return -1;
    }
    cout << "Text renderer initialized successfully!" << endl;
    
    // Fullscreen quad vertices (position only)
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,  // Bottom left
         1.0f, -1.0f, 0.0f,  // Bottom right
         1.0f,  1.0f, 0.0f,  // Top right
        -1.0f,  1.0f, 0.0f   // Top left
    };
    
    // Quad indices
    unsigned int indices[] = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };

    // Generate and bind VAO, VBO, EBO
    GLuint VAO, VBO, EBO;
    
    // Clear any existing OpenGL errors
    while(glGetError() != GL_NO_ERROR);
    
    glGenVertexArrays(1, &VAO);
    checkGLError("glGenVertexArrays");
    
    glGenBuffers(1, &VBO);
    checkGLError("glGenBuffers VBO");
    
    glGenBuffers(1, &EBO);
    checkGLError("glGenBuffers EBO");

    glBindVertexArray(VAO);
    checkGLError("glBindVertexArray");

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    checkGLError("glBindBuffer VBO");
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    checkGLError("glBufferData VBO");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    checkGLError("glBindBuffer EBO");
    
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    checkGLError("glBufferData EBO");

    // Position attribute (location 0) - only position, no color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    checkGLError("glVertexAttribPointer position");
    
    glEnableVertexAttribArray(0);
    checkGLError("glEnableVertexAttribArray position");

    // Unbind to prevent accidental modification
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkGLError("VAO setup complete");
    
    cout << "Rendering setup complete!" << endl;

    // Get uniform locations for Mandelbrot parameters
    GLint resolutionLoc = glGetUniformLocation(shaderProgram, "resolution");
    GLint zoomLoc, offsetLoc;
    
    // Always use single precision uniforms (shader compatibility)
    // But keep double precision on CPU side for better calculations
    zoomLoc = glGetUniformLocation(shaderProgram, "zoom");
    offsetLoc = glGetUniformLocation(shaderProgram, "offset");
    
    GLint maxIterationsLoc = glGetUniformLocation(shaderProgram, "maxIterations");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    GLint colorBgLoc = glGetUniformLocation(shaderProgram, "colorBg");
    GLint adaptiveIterationsLoc = glGetUniformLocation(shaderProgram, "adaptiveIterations");
    
    cout << "Uniform locations - resolution: " << resolutionLoc << ", zoom: " << zoomLoc 
         << ", offset: " << offsetLoc << ", maxIterations: " << maxIterationsLoc 
         << ", colorMode: " << colorLoc << ", adaptive: " << adaptiveIterationsLoc << endl;
    
    if (useDouble) {
        cout << "Using double precision for CPU calculations with high precision shader" << endl;
    }

    Clock clock;
    
    // FPS calculation variables
    int frameCount = 0;
    float fps = 0.0f;
    Time fpsUpdateTime = clock.getElapsedTime();
    
    // Print controls
    cout << "\n=== CONTROLS ===" << endl;
    cout << "Mouse wheel: Zoom in/out" << endl;
    cout << "Left click + drag: Pan view" << endl;
    cout << "R: Reset view" << endl;
    cout << "C: Cycle color modes" << endl;
    cout << "V: Cycle color modes backwards" << endl;
    cout << "B: Cycle background color modes" << endl;
    cout << "N: Cycle background color modes backwards" << endl;
    cout << "+/-: Increase/decrease iterations" << endl;
    cout << "A: Toggle adaptive iterations" << endl;
    cout << "ESC: Exit" << endl;
    
    // run the main loop
    bool running = true;
    while (running) {
        // handle events
        while (const optional event = window.pollEvent()) {
            if (event->is<Event::Closed>()) {
                running = false;
            }
            else if (const auto* resized = event->getIf<Event::Resized>()) {
                glViewport(0, 0, resized->size.x, resized->size.y);
            }
            else if (const auto* keyPressed = event->getIf<Event::KeyPressed>()) {
                switch (keyPressed->code) {
                    case Keyboard::Key::Escape:
                        running = false;
                        break;
                    case Keyboard::Key::R:
                        params.reset();
                        cout << "View reset" << endl;
                        break;
                    case Keyboard::Key::C:
                        params.colorMode = (params.colorMode + 1) % params.colors.size();
                        break;
                    case Keyboard::Key::V:
                        params.colorMode = (params.colorMode - 1 + params.colors.size()) % params.colors.size();
                        break;
                    case Keyboard::Key::B:
                        params.colorModeBg = (params.colorModeBg + 1) % params.colors.size();
                        break;
                    case Keyboard::Key::N:
                        params.colorModeBg = (params.colorModeBg - 1 + params.colors.size()) % params.colors.size();
                        break;
                    case Keyboard::Key::Equal:  // + key
                        params.maxIterations = min(1000, params.maxIterations + 10);
                        break;
                    case Keyboard::Key::Hyphen:  // - key
                        params.maxIterations = max(10, params.maxIterations - 10);
                        break;
                    case Keyboard::Key::A:  // Toggle adaptive iterations
                        params.adaptiveIterations = !params.adaptiveIterations;
                        break;
                    default:
                        break;
                }
            }
            else if (const auto* mouseButtonPressed = event->getIf<Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == Mouse::Button::Left) {
                    params.isDragging = true;
                    params.lastMouseX = static_cast<float>(mouseButtonPressed->position.x);
                    params.lastMouseY = static_cast<float>(mouseButtonPressed->position.y);
                }
            }
            else if (const auto* mouseButtonReleased = event->getIf<Event::MouseButtonReleased>()) {
                if (mouseButtonReleased->button == Mouse::Button::Left) {
                    params.isDragging = false;
                }
            }
            else if (const auto* mouseMoved = event->getIf<Event::MouseMoved>()) {
                if (params.isDragging) {
                    float deltaX = static_cast<float>(mouseMoved->position.x) - params.lastMouseX;
                    float deltaY = static_cast<float>(mouseMoved->position.y) - params.lastMouseY;
                    
                    // Convert screen coordinates to complex plane coordinates
                    Vector2u windowSize = window.getSize();
                    double aspectRatio = static_cast<double>(windowSize.x) / static_cast<double>(windowSize.y);
                    
                    params.offsetX -= (static_cast<double>(deltaX) / static_cast<double>(windowSize.x)) * params.zoom * aspectRatio * 2.0;
                    params.offsetY -= (static_cast<double>(deltaY) / static_cast<double>(windowSize.y)) * params.zoom * 2.0;
                    
                    params.lastMouseX = static_cast<float>(mouseMoved->position.x);
                    params.lastMouseY = static_cast<float>(mouseMoved->position.y);
                }
            }
            else if (const auto* mouseWheelScrolled = event->getIf<Event::MouseWheelScrolled>()) {
                if (mouseWheelScrolled->wheel == Mouse::Wheel::Vertical) {
                    // Smaller zoom steps for more precise control
                    double zoomFactor = mouseWheelScrolled->delta > 0 ? 0.85 : 1.176;
                    
                    // Get mouse position relative to center
                    Vector2i mousePos = Mouse::getPosition(window);
                    Vector2u windowSize = window.getSize();
                    
                    double mouseX = (static_cast<double>(mousePos.x) / static_cast<double>(windowSize.x) - 0.5) * 2.0;
                    double mouseY = -(static_cast<double>(mousePos.y) / static_cast<double>(windowSize.y) - 0.5) * 2.0;
                    
                    double aspectRatio = static_cast<double>(windowSize.x) / static_cast<double>(windowSize.y);
                    mouseX *= aspectRatio;
                    
                    // Convert to complex plane coordinates
                    double complexX = mouseX * params.zoom + params.offsetX;
                    double complexY = mouseY * params.zoom + params.offsetY;
                    
                    // Zoom
                    params.zoom *= zoomFactor;
                    
                    // Adjust offset to zoom towards mouse position
                    params.offsetX = complexX - mouseX * params.zoom;
                    params.offsetY = complexY - mouseY * params.zoom;
                }
            }
        }

        // clear the buffers
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Get current window size for resolution uniform
        Vector2u windowSize = window.getSize();
        
        // Set uniforms for Mandelbrot rendering
        Vector3f color = params.colors[params.colorMode];
        Vector3f colorBg = params.colorsBg[params.colorModeBg];
        glUniform2f(resolutionLoc, static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
        
        // Always use float uniforms but convert from double precision CPU values
        glUniform1f(zoomLoc, static_cast<float>(params.zoom));
        glUniform2f(offsetLoc, static_cast<float>(params.offsetX), static_cast<float>(params.offsetY));
        
        glUniform1i(maxIterationsLoc, params.maxIterations);
        glUniform3f(colorLoc, color.x, color.y, color.z);
        glUniform3f(colorBgLoc, colorBg.x, colorBg.y, colorBg.z);
        glUniform1i(adaptiveIterationsLoc, params.adaptiveIterations ? 1 : 0);

        // Draw fullscreen quad
        glBindVertexArray(VAO);
        checkGLError("bind VAO for drawing");
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // 6 indices for 2 triangles
        checkGLError("draw elements");
        
        glBindVertexArray(0);  // Unbind VAO after drawing

        // Calculate FPS
        frameCount++;
        Time currentTime = clock.getElapsedTime();
        if (currentTime - fpsUpdateTime >= seconds(0.5f)) { // Update FPS every 0.5 seconds
            fps = frameCount / (currentTime - fpsUpdateTime).asSeconds();
            frameCount = 0;
            fpsUpdateTime = currentTime;
        }
        
        // Render FPS text
        stringstream fpsStream;
        fpsStream << fixed << setprecision(0) << "FPS: " << fps;
        textRenderer.renderText(fpsStream.str(), 10.0f, 30.0f, 1.0f, sf::Vector3f(1.0f, 1.0f, 1.0f), windowSize.x, windowSize.y);

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    return 0;
}