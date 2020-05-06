//
//  kernel.h
//  Automata
//
//  Created by Antoni Wójcik on 06/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef kernel_h
#define kernel_h

// include the standard libraries
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>

// include the OpenCL library (C++ binding)
#define __CL_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "cl2.hpp"
#include "opencl_error.h"

// include the OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// include the STB library to read texture files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shader.h"

class KernelGL {
private:
    class ImageGLObj {
    private:
        cl_GLuint texture_ID;
        GLenum texture_format;
        
        cl::ImageGL image_GL;
        
        void generateGLTexture(const unsigned char* texture_data = NULL) {
            glGenTextures(1, &texture_ID);
            
            glBindTexture(GL_TEXTURE_2D, texture_ID);
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //GL_CLAMP_TO_EDGE or GL_REPEAT
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //USE NEAREST TO SPEED UP
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            
            glTexImage2D(GL_TEXTURE_2D, 0, texture_format, width, height, 0, texture_format, GL_UNSIGNED_BYTE, texture_data);
            
            glFinish();
        }
        
        void generateGLTextureFromFile(const char* texture_path) {
            int channels_num;
            unsigned char* texture_data = stbi_load(texture_path, &width, &height, &channels_num, 4);
            
            if(texture_data) {
                if(channels_num == 1) texture_format = GL_RED;
                else if(channels_num == 3) texture_format = GL_RGB;
                else texture_format = GL_RGBA;
                
                generateGLTexture(texture_data);

                stbi_image_free(texture_data);
            } else {
                std::cerr << "ERROR: STBI: Texture failed to load at path: " << texture_path << std::endl;

                stbi_image_free(texture_data);
                
                exit(-1);
            }
        }
        
    public:
    int width, height;
        
        ImageGLObj() {}
        
        ImageGLObj(const char* texture_path, cl::Context context) {
            generateGLTextureFromFile(texture_path);
            image_GL = cl::ImageGL(context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texture_ID);
        }
        
        ImageGLObj(ImageGLObj image, cl::Context context) {
            width = image.width;
            height = image.height;
            texture_format = image.texture_format;
            generateGLTexture();
            image_GL = cl::ImageGL(context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texture_ID);
        }
        
        void setKernelArg(cl::Kernel& kernel, int kernel_pos) {
            kernel.setArg(kernel_pos, image_GL);
        }
        
        void transferImageToShader(Shader& shader, const char* shader_identifier) {
            shader.use();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_ID);
            
            glUniform1i(glGetUniformLocation(shader.ID, shader_identifier), 0);
        }
    };
    
    cl::Device device;
    cl::Context context;
    cl::Program program;
    cl::Kernel kernel;
    
    int width, height;
    
    ImageGLObj image[2];
    
    int current_image_in, current_image_out;
    
    
    void processError(cl::Error e) {
        std::cerr << "ERROR: OpenCL: OTHER: " << e.what() << ": " << e.err() << std::endl;
        if(e.err() == CL_BUILD_PROGRAM_FAILURE) {
            std::cerr << "ERROR: OpenCL: CANNOT BUILD PROGRAM: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
        } else {
            std::cerr << oclErrorString(e.err()) << "\nUSE:\nhttps://streamhpc.com/blog/2013-04-28/opencl-error-codes\nTO VERIFY ERROR TYPE" << std::endl;
        }
        
        exit(-1);
    }
    
    std::string loadSource(const char* kernel_path) {
        std::string kernel_code;
        std::ifstream kernel_file;
        kernel_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        
        try {
            kernel_file.open(kernel_path);
            std::ostringstream kernel_stream;
            kernel_stream << kernel_file.rdbuf();
            kernel_file.close();
            kernel_code = kernel_stream.str();
        } catch(std::ifstream::failure e) {
            std::cerr << "ERROR: OpenCL KERNEL: CANNOT READ KERNEL CODE" << std::endl;
            exit(-1); //stop executing the program with the error code -1;
        }
        return kernel_code;
    }
    
    void buildProgram(const char* kernel_path) {
        // build the program given the source
        std::vector<cl::Platform> platforms;
        std::vector<cl::Device> devices;
        
        // find platform
        
        cl::Platform::get(&platforms);
        if(platforms.size() == 0) {
            std::cerr << "ERROR: OpenCL: NO PLATFORMS FOUND" << std::endl;;
        }
        
        // find device
        
        platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if(devices.size() == 0) {
            std::cerr << "ERROR: OpenCL: NO DEVICES FOUND" << std::endl;
        }
        
        device = devices[1]; //choose the graphics card
        std::cout << "SUCCESS: OpenCL: USING A DEVICE: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        
        // create shared context between OpenCL and OpenGL - therefore no communication via host needed!
        
        CGLContextObj CGLGetCurrentContext(void);
        CGLShareGroupObj CGLGetShareGroup(CGLContextObj);
        CGLContextObj kCGLContext = CGLGetCurrentContext();
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);

        cl_context_properties properties[] = {
          CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
          (cl_context_properties) kCGLShareGroup,
          0
        };
        
        context = cl::Context(device, properties);
        
        // upload program source
        
        std::string kernel_code = loadSource(kernel_path);
        cl::Program::Sources sources;
        sources.push_back({kernel_code.c_str(), kernel_code.length()});
        
        // build the program
        
        program = cl::Program(context, sources);
        program.build({device});
    }
    
    void createKernel(const char* kernel_name) {
        // create the kernel given the name
        
        kernel = cl::Kernel(program, kernel_name);
    }
    
    void setKernelArgs() {
        try {
            image[current_image_in].setKernelArg(kernel, 0);
            image[current_image_out].setKernelArg(kernel, 1);
        } catch(cl::Error e) {
            processError(e);
        }
    }
    
    void swapImages() {
        int temp = current_image_out;
        current_image_in = current_image_out;
        current_image_out = temp;
    }
    
public:
    KernelGL(const char* kernel_path, const char* kernel_name) {
        try {
            buildProgram(kernel_path);
            createKernel(kernel_name);
        } catch(cl::Error e) {
            processError(e);
        }
    }
    
    void createImagesGL(const char* texture_path) {
        
        // create two images and swap them with each iteration
        
        image[0] = ImageGLObj(texture_path, context);
        image[1] = ImageGLObj(image[0], context);
        
        width = image[0].width;
        height = image[0].height;
        
        current_image_out = 0;
        current_image_in = 1;
    }
    
    void transferData(Shader& shader, const char* shader_identifier) {
        image[current_image_out].transferImageToShader(shader, shader_identifier);
    }
    
    void iterate() {
        swapImages();
        setKernelArgs();
        
        try {
            // create a command queue and enqueue the kernel to process the current input image
            
            cl::CommandQueue queue(context, device);
            queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(size_t(width), size_t(height)), cl::NullRange);
            queue.finish();
        } catch(cl::Error e) {
            processError(e);
        }
    }
};

#endif /* kernel_h */