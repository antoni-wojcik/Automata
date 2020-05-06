//
//  main.cpp
//  Automata
//
//  Created by Antoni Wójcik on 05/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#define RETINA
#define SCR_WIDTH 800
#define SCR_HEIGHT 800


#include <iostream>

// include the OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.h>

//include the OpenCL library (C++ binding)
#define __CL_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "cl2.hpp"

#include "shader.h"
#include "kernel.h"
#include "screen.h"


// function declarations
GLFWwindow* initialiseOpenGL();
void framebufferSizeCallback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);
void countFPS(float);

#ifdef RETINA
// dimensions of the viewport (they have to be multiplied by 2 at the retina displays)
unsigned int scr_width = SCR_WIDTH*2;
unsigned int scr_height = SCR_HEIGHT*2;
#else
unsigned int scr_width = SCR_WIDTH;
unsigned int scr_height = SCR_HEIGHT;
#endif


void macWindowFix(GLFWwindow* window);
bool mac_fixed = false;

// variables used in the main loop
float last_frame_time = 0.0f;
float delta_time = 0.0f;
float current_swap_time = 0.0f;
const float swap_length = 0.05f;

// fps counter variables
float fps_sum = 0.0f;
const int fps_steps = 5;
int fps_steps_counter = 0;

// screenshot variables
bool taking_screenshot = false;
Screen* screen_ptr;

int main(int argc, const char * argv[]) {
    GLFWwindow* window = initialiseOpenGL();
    
    Screen screen("src/shaders/screen.vs", "src/shaders/screen.fs");
    screen_ptr = &screen;
    
    KernelGL kernel("src/kernels/kernel_automata.ocl", "iterate");
    kernel.createImagesGL("textures/test2.png");
    
    while(!glfwWindowShouldClose(window)) {
        float current_time = glfwGetTime();
        delta_time = current_time - last_frame_time;
        last_frame_time = current_time;
        current_swap_time += delta_time;
        
        processInput(window);
        
        
        kernel.transferData(screen.shader, "automata");
        screen.draw();
        
        if(current_swap_time > swap_length) {
            kernel.iterate();
            current_swap_time = 0.0f;
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    scr_width = width;
    scr_height = height;
}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        if(!taking_screenshot) screen_ptr->takeScreenshot(scr_width, scr_height);
        taking_screenshot = true;
    } else if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
        taking_screenshot = false;
    }
}

void countFPS(float delta_time) {
    
    // count fps
    if(fps_steps_counter == fps_steps) {
        fps_steps_counter = 0;
        fps_sum = 0;
    }
    fps_sum += delta_time;
    fps_steps_counter++;
}

GLFWwindow* initialiseOpenGL() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    
    GLFWwindow* window;
    
    #ifdef RETINA
    window = glfwCreateWindow(scr_width/2, scr_height/2, "Clouds", NULL, NULL);
    #else
    window = glfwCreateWindow(scr_width, scr_height, "Clouds", NULL, NULL);
    #endif
    if(window == NULL) {
        std::cout << "ERROR: OpenGL: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    
    glfwMakeContextCurrent(window);
    
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    if(glewInit() != GLEW_OK) {
        std::cerr << "ERROR: OpenGL: Failed to initialize GLEW" << std::endl;
        glGetError();
        exit(-1);
    }
    
    return window;
}
