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
    unsigned int scr_width, scr_height;
    short width, height;
    
    unsigned int screen_texture;
    GLuint screen_texture_loc;
    
    Shader screen_shader;
    
    unsigned int VBO, VAO, EBO, FBO;
    
    
    void createFramebuffer() {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        
        glGenTextures(1, &screen_texture);
        glBindTexture(GL_TEXTURE_2D, screen_texture);
        
        screen_shader.use();
        screen_texture_loc = glGetUniformLocation(screen_shader.ID, "screen_texture");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_texture, 0);
        
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "ERROR: OpenGL: Failed to create framebuffer" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
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
    
    inline void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, width, height);
    }
    
    inline void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, scr_width, scr_height);
    }

public:
    Shader automata_shader;
    
    Screen(unsigned int scr_width_u, unsigned int scr_height_u, const char* screen_vertex_path, const char* screen_fragment_path, const char* automata_vertex_path, const char* automata_fragment_path) : screen_shader(screen_vertex_path, screen_fragment_path), automata_shader(automata_vertex_path, automata_fragment_path) {
        scr_width = scr_width_u;
        scr_height = scr_height_u;
        width = (scr_width_u+1)/2;
        height = (scr_height_u+1)/2;
        
        createScreen();
        createFramebuffer();
    }
    
    ~Screen() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteFramebuffers(1, &FBO);
    }
    
    void draw() {
        bind();
        automata_shader.use();
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        unbind();
        screen_shader.use();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screen_texture);
        glUniform1i(screen_texture_loc, 0);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glBindVertexArray(0);
    }
    
    void resize(unsigned int scr_width_u, unsigned int scr_height_u) {
        scr_width = scr_width_u;
        scr_height = scr_height_u;
        width = (scr_width_u+1)/2;
        height = (scr_height_u+1)/2;
        
        glDeleteFramebuffers(1, &FBO);
        glDeleteTextures(1, &screen_texture);
        
        createFramebuffer();
    }
    
    void takeScreenshot(const std::string& name = "screenshot", bool show_image = false) {
        std::cout << "Taking screenshot: " << name << ".tga " << ", dimensions: " << width << ", " << height << std::endl;
        short TGA_header[] = {0, 2, 0, 0, 0, 0, width, height, 24};
        char* pixel_data = new char[3 * width * height]; //there are 3 colors (RGB) for each pixel
        std::ofstream file("screenshots/" + name + ".tga", std::ios::out | std::ios::binary);
        if(!pixel_data || !file) {
            std::cerr << "ERROR: COULD NOT TAKE THE SCREENSHOT" << std::endl;
            exit(-1);
        }
        
        bind();
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixel_data);
        unbind();
        glFinish();
        
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
