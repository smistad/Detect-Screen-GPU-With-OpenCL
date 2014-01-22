#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <iostream>
#include <CL/cl.hpp>
#include <GL/glut.h>

#if defined(__APPLE__) || defined(__MACOSX)
   #include <OpenCL/cl_gl.h>
   #include <OpenGL/OpenGL.h>
#else
#if _WIN32
#else
   #include <GL/glx.h>
   #include <CL/cl_gl.h>
#endif 
#endif


int main(int argc, char ** argv) {
    // Get platforms
    VECTOR_CLASS<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    std::cout << platforms.size() << " OpenCL platforms found." << std::endl;

    for(int j = 0; j < platforms.size(); j++) {
        std::cout << std::endl << "=========================================================================" << std::endl;
        std::cout << "Platform " << j << " with name " << platforms[j].getInfo<CL_PLATFORM_VENDOR>() << std::endl;

        // Get devices for platform
        VECTOR_CLASS<cl::Device> devices;
        platforms[j].getDevices(CL_DEVICE_TYPE_ALL, &devices);
        std::cout << devices.size() << " devices found." << std::endl;
        if(devices.size() == 0) {
            std::cout << "Aborting. No devices found." << std::endl;
            continue;
        }

        // Create properties for CL-GL context
        #if defined(__APPLE__) || defined(__MACOSX)
        // Apple (untested)
        // TODO: create GL context for Apple
        cl_context_properties cps[] = {
           CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
           (cl_context_properties)CGLGetShareGroup(CGLGetCurrentContext()),
           0};

        #else
        #ifdef _WIN32
        // Windows
        // TODO: create GL context for Windows
          cl_context_properties cps[] = {
              CL_GL_CONTEXT_KHR,
              (cl_context_properties)wglGetCurrentContext(),
              CL_WGL_HDC_KHR,
              (cl_context_properties)wglGetCurrentDC(),
              CL_CONTEXT_PLATFORM,
              (cl_context_properties)(platforms[j])(),
              0
          };
        #else
        // Linux
        // Create a GL context using glX
        int  sngBuf[] = {    
                    GLX_RGBA,
                    GLX_RED_SIZE, 1,
                    GLX_GREEN_SIZE, 1,
                    GLX_BLUE_SIZE, 1,
                    GLX_DEPTH_SIZE, 12,
                    None };
         
        // TODO: should probably free this stuff
        Display * display = XOpenDisplay(0);
        XVisualInfo* vi = glXChooseVisual(display, DefaultScreen(display), sngBuf);
        GLXContext gl2Context = glXCreateContext(display, vi, 0, GL_TRUE);
         
        if(gl2Context == NULL) {
            std::cout << "Could not create a GL 2.1 context, please check your graphics drivers" << std::endl;
            continue;
        }

        cl_context_properties cps[] = {
              CL_GL_CONTEXT_KHR,
              (cl_context_properties)gl2Context,
              CL_GLX_DISPLAY_KHR,
              (cl_context_properties)display,
              CL_CONTEXT_PLATFORM,
              (cl_context_properties)(platforms[j])(),
              0
        };
        std::cout << "Current glX context is: " << cps[1] << std::endl;
        #endif
        #endif

        // Query which device is associated with GL context
        cl_device_id cl_gl_device_ids[32];
        size_t returnSize = 0;
        clGetGLContextInfoKHR_fn glGetGLContextInfo_func = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
        glGetGLContextInfo_func(
                cps, 
                CL_DEVICES_FOR_GL_CONTEXT_KHR, 
                32*sizeof(cl_device_id), 
                &cl_gl_device_ids, 
                &returnSize
        );

        if(returnSize == 0) {
            std::cout << "No valid GPU for GL context found" << std::endl;
            continue;
        }

        std::cout << "There are " << returnSize / sizeof(cl_device_id) << " devices that can be associated with the GL context" << std::endl;

        // This code assumes that there is only one GPU that is connected to the screen
        cl_device_id interopDeviceID;
        bool found = false;
        for(int i = 0; i < returnSize/sizeof(cl_device_id); i++) {
            cl::Device device(cl_gl_device_ids[i]);
            if(device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) {
                interopDeviceID = device();
                found = true;
                break;
            }
        }

        if(!found) {
            std::cout << "Could not find a GPU that was associated with the GL context" << std::endl;
            exit(0);
        }

        cl::Device interopDevice(interopDeviceID);
        std::cout << "----------------------------------------------------------------------------------" << std::endl;
        std::cout << "Device with name " << interopDevice.getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "This device is associated with the GL context. And thus most likely connected to the screen." << std::endl;
        std::cout << "It has: " << interopDevice.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024*1024) << " MB of memory" << std::endl;
        std::cout << "----------------------------------------------------------------------------------" << std::endl;

        // Get all other devices
        if(devices.size() > 1) {
            for(int i = 0; i < devices.size(); i++) {
                cl_device_id currentDeviceID = devices[i]();
                if(currentDeviceID != interopDeviceID) {
                    cl::Device device(currentDeviceID);
                    std::cout << "----------------------------------------------------------------------------------" << std::endl;
                    std::cout << "Device with name " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
                    std::cout << "This device is either NOT GPU or NOT associated with the GL context. Thus most likely NOT connected to screen." << std::endl;
                    std::cout << "It has: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024*1024) << " MB of memory" << std::endl;
                    std::cout << "----------------------------------------------------------------------------------" << std::endl;
                }
            }
        }

        // Cleanup
        #if defined(__APPLE__) || defined(__MACOSX)
        #ifdef _WIN32
        #else
        // Linux
        glXDestroyContext(display, gl2Context);
        #endif
        #endif

    }

    return 0;
}
