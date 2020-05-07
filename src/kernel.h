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
        
        void generateGLTexture() {
            glGenTextures(1, &texture_ID);
            
            glBindTexture(GL_TEXTURE_2D, texture_ID);
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8I, width, height, 0, GL_RGBA_INTEGER, GL_INT, NULL);
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL); // for float textures
            
            glFinish();
        }
        
        void generateGLTextureFromFile(const char* texture_path) {
            int channels_num;
            GLenum texture_format;
            unsigned char* texture_data = stbi_load(texture_path, &width, &height, &channels_num, 4);
            
            if(texture_data) {
                if(channels_num == 1) texture_format = GL_RED;
                else if(channels_num == 3) texture_format = GL_RGB;
                else texture_format = GL_RGBA;
                
                glGenTextures(1, &texture_ID);
                
                glBindTexture(GL_TEXTURE_2D, texture_ID);
                
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                
                glTexImage2D(GL_TEXTURE_2D, 0, texture_format, width, height, 0, texture_format, GL_UNSIGNED_BYTE, texture_data);
                
                glFinish();

                stbi_image_free(texture_data);
            } else {
                std::cerr << "ERROR: STBI: Texture failed to load at path: " << texture_path << std::endl;

                stbi_image_free(texture_data);
                
                exit(-1);
            }
        }
        
    public:
    int width, height;
    
    cl::ImageGL image_GL;
        
        ImageGLObj() {}
        
        ImageGLObj(const char* texture_path, const char* kernel_name, cl::Device& device, cl::Context& context, cl::Program& program) {
            generateGLTextureFromFile(texture_path);
            cl_GLuint texture_file_ID = texture_ID;
            generateGLTexture();
            
            cl::ImageGL textureImage(context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, texture_file_ID);
            image_GL = cl::ImageGL(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture_ID);
            
            // create the kernel to process the initial texture
            
            cl::Kernel processing_kernel(program, kernel_name);
            
            // set the arguments
            
            processing_kernel.setArg(0, textureImage);
            processing_kernel.setArg(1, image_GL);
            
            // process the initial texture
            
            cl::CommandQueue queue(context, device);
            queue.enqueueNDRangeKernel(processing_kernel, cl::NullRange, cl::NDRange(size_t(width), size_t(height)), cl::NullRange);
            queue.finish();
            
            // free texture memory
                                     
            glDeleteTextures(1, &texture_file_ID);
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
    
    cl::Image2D image_in;
    ImageGLObj image_out;
    
    
    void processError(cl::Error& e) {
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
        kernel.setArg(0, image_in);
        image_out.setKernelArg(kernel, 1);
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
    
    void createImagesGL(const char* texture_path, const char* kernel_name) {
        // create two images and swap them with each iteration
        
        try {
            image_out = ImageGLObj(texture_path, kernel_name, device, context, program);
        
            width = image_out.width;
            height = image_out.height;
            cl::ImageFormat image_format(CL_RGBA, CL_SIGNED_INT8);
            image_in = cl::Image2D(context, CL_MEM_READ_ONLY, image_format, width, height);
        } catch(cl::Error e) {
            processError(e);
        }
    }
    
    void transferData(Shader& shader, const char* shader_identifier) {
        image_out.transferImageToShader(shader, shader_identifier);
    }
    
    void iterate() {
        try {
            // create a command queue and enqueue the kernel to process the current input image
            
            std::vector<cl::Memory> mem_objs;
            
            mem_objs.push_back(image_out.image_GL);
            
            // set the kernel arguments
            
            setKernelArgs();
            
            // make sure the OpenGL has freed the texture before proceeding
            
            cl::CommandQueue queue(context, device);
            queue.enqueueAcquireGLObjects(&mem_objs);
            queue.enqueueCopyImage(image_out.image_GL, image_in, {0, 0, 0}, {0, 0, 0}, {(size_t)width, (size_t)height, 1});
            queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(size_t(width), size_t(height)), cl::NullRange);
            queue.enqueueReleaseGLObjects(&mem_objs);
            queue.finish();
        } catch(cl::Error e) {
            processError(e);
        }
    }
};

#endif /* kernel_h */
