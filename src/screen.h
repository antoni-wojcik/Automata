//
//  screen.h
//  Automata
//
//  Created by Antoni Wójcik on 06/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef screen_h
#define screen_h

// include the OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Screen {
private:
    unsigned int VBO, VAO, EBO;
    
    void createScreen() {
        float vertices[12] = {
            1.0f,  1.0f, 0.0f,  // top right
            1.0f, -1.0f, 0.0f,  // bottom right
            -1.0f, -1.0f, 0.0f,  // bottom left
            -1.0f,  1.0f, 0.0f   // top left
        };
        unsigned int indices[6] = {  // note that we start from 0!
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
        };
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

public:
    Shader shader;
    
    Screen(const char* screen_vertex_path, const char* screen_fragment_path) : shader(screen_vertex_path, screen_fragment_path) {
        createScreen();
    }
    
    ~Screen() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    
    void clear() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    void draw() {
        shader.use();
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glBindVertexArray(0);
    }
    
    void takeScreenshot(short width, short height, const std::string& name = "screenshot", bool show_image = false) {
        std::cout << "Taking screenshot: " << name << ".tga" << std::endl;
        short TGA_header[] = {0, 2, 0, 0, 0, 0, width, height, 24};
        char* pixel_data = new char[3 * width * height]; //there are 3 colors (RGB) for each pixel
        std::ofstream file("screenshots/" + name + ".tga", std::ios::out | std::ios::binary);
        if(!pixel_data || !file) {
            std::cerr << "ERROR: COULD NOT TAKE THE SCREENSHOT" << std::endl;
            exit(-1);
        }
        
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixel_data);
        
        file.write((char*)TGA_header, 9 * sizeof(short));
        file.write(pixel_data, 3 * width * height);
        file.close();
        delete [] pixel_data;
        
        if(show_image) {
            std::cout << "Opening the screenshot" << std::endl;
            std::system(("open " + name + ".tga").c_str());
        }
    }
};

#endif /* screen_h */
