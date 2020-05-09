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
#define ITERATION_LENGTH_MAX 3.0f
#define ITERATION_LENGTH_MIN 0.001f
#define ITERATION_LENGTH_STRENGTH 3.0f


#include <iostream>
#include <cmath>

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
#include "camera.h"


// function declarations
GLFWwindow* initialiseOpenGL();
void framebufferSizeCallback(GLFWwindow*, int, int);
void mouseCallback(GLFWwindow*, double, double);
void mouseButtonCallback(GLFWwindow*, int, int, int);
void scrollCallback(GLFWwindow*, double, double);
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

// variables used in the main loop
float last_frame_time = 0.0f;
float delta_time = 0.0f;
float current_swap_time = 0.0f;
float iteration_length = 0.1f;
bool stopping = false;
bool run = true;

// fps counter variables
float fps_sum = 0.0f;
const int fps_steps = 5;
int fps_steps_counter = 0;

// variables used in callbacks
bool mouse_first_check = true;
bool mouse_hidden = true;
float mouse_last_x = scr_width / 2.0f;
float mouse_last_y = scr_width / 2.0f;

// screenshot variables
bool taking_screenshot = false;
Screen* screen_ptr;

// camera pointer
Camera* camera_ptr;

int main(int argc, const char * argv[]) {
    GLFWwindow* window = initialiseOpenGL();
    
    Screen screen(scr_width, scr_height, "src/shaders/screen/screen.vs", "src/shaders/screen/screen.fs", "src/shaders/automata/automata.vs", "src/shaders/automata/automata.fs");
    screen_ptr = &screen;
    
    KernelGL kernel("src/kernels/kernel_automata.ocl", "iterate");
    kernel.createImagesGL("textures/die4.png", "processTexture");
    
    Camera camera(scr_width, scr_height, kernel.width, kernel.height);
    camera_ptr = &camera;
    
    while(!glfwWindowShouldClose(window)) {
        float current_time = glfwGetTime();
        delta_time = current_time - last_frame_time;
        last_frame_time = current_time;
        current_swap_time += delta_time;
        
        processInput(window);
        
        camera.transferData(screen.automata_shader, "pos_x", "pos_y", "width_inv", "height_inv");
        
        kernel.transferData(screen.automata_shader, "automata");
        screen.draw();
        
        glFinish();
        
        if(run && current_swap_time > iteration_length) {
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
    
    screen_ptr->resize(width, height);
    camera_ptr->resize(width, height);
}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    
    if(glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        iteration_length *= exp(-delta_time * ITERATION_LENGTH_STRENGTH);
        if(iteration_length < ITERATION_LENGTH_MIN) iteration_length = ITERATION_LENGTH_MIN;
        std::cout << "Changed iteration length to: " << iteration_length << " s" << std::endl;
    } else if(glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        iteration_length *= exp(delta_time * ITERATION_LENGTH_STRENGTH);
        if(iteration_length > ITERATION_LENGTH_MAX) iteration_length = ITERATION_LENGTH_MAX;
        std::cout << "Changed iteration length to: " << iteration_length << " s" << std::endl;
    }
    
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        if(!taking_screenshot) screen_ptr->takeScreenshot();
        taking_screenshot = true;
    } else if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
        taking_screenshot = false;
    }
    
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if(!stopping) run = !run;
        stopping = true;
    } else if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        stopping = false;
    }
}

void mouseCallback(GLFWwindow* window, double pos_x, double pos_y) {
    if(mouse_first_check) {
        mouse_last_x = pos_x;
        mouse_last_y = pos_y;
        mouse_first_check = false;
    }
    
    float offset_x = pos_x - mouse_last_x;
    float offset_y = mouse_last_y - pos_y;
    
    mouse_last_x = pos_x;
    mouse_last_y = pos_y;
    
    if(mouse_hidden) camera_ptr->move(offset_x, -offset_y);
}

void scrollCallback(GLFWwindow* window, double offset_x, double offset_y) {
    camera_ptr->zoom(offset_y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if(mouse_hidden) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        mouse_hidden = !mouse_hidden;
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
    
    // set the callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    
    // tell GLFW to capture the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if(glewInit() != GLEW_OK) {
        std::cerr << "ERROR: OpenGL: Failed to initialize GLEW" << std::endl;
        glGetError();
        exit(-1);
    }
    
    return window;
}
