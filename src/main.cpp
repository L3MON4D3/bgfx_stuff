#include "Perlin.hpp"
#include "Util.hpp"
#include "ModelBuilder.hpp"

#include "bgfx/bgfx.h"
#include "bgfx/defines.h"
#include "bgfx/platform.h"
#include "bx/math.h"

#include <iostream>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>


bgfx::VertexLayout worldWp::PosColorVertex::layout;

worldWp::PosColorVertex cubeVertices[] = {
	{-1.0f, -1.0f,  0.0f, 0xff000000 },
	{-1.0f,  1.0f,  0.0f, 0xff0000ff },
	{ 1.0f,  1.0f,  0.0f, 0xff00ff00 },
	{ 1.0f, -1.0f,  0.0f, 0xff00ffff },
};

const uint16_t cubeTriList[] = {
    0, 2, 1,
    0, 3, 2
};

/**
 * Create new GLFW-Window with dims width x height. GLFW needs to be initialized.
 * @param init Pass empty bgfx::Init.
 */
GLFWwindow* create_window(int width, int height) {
    //Magic code from entry_glfw.cpp from bgfx.
    GLFWwindow *window =
        glfwCreateWindow(width, height, "worldWp", nullptr, nullptr);

    bgfx::Init init;

    init.platformData.ndt = glfwGetX11Display();
    init.platformData.nwh = (void*) glfwGetX11Window(window);
    
    glfwGetWindowSize(window, &width, &height);
    init.resolution.height = height;
    init.resolution.width = width;
    init.resolution.reset = BGFX_RESET_VSYNC;

    bgfx::init(init);

    return window;
}

int main(int argc, char** argv){
    //using so lines dont get too long.
    using namespace bgfx;

    RandomGenerator::Perlin pln(345, .1, .1, 10);
    worldWp::ModelBuilder builder(10, 10, pln);
    //Call renderFrame before init (in create_window) to render on this thread.
    glfwInit();
    glfwSetErrorCallback(worldWp::util::glfw_errorCallback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //init should not go out of scope until program finishes.
    int width = 1000, height = 1000;
    renderFrame();
    GLFWwindow *window = create_window(width, height);

    worldWp::PosColorVertex::init();

    const ViewId clearView = 0;
    setViewClear(clearView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    setViewRect(clearView, 0, 0, BackbufferRatio::Equal);

    VertexBufferHandle vbh = builder.getVBufferHandle();
    //VertexBufferHandle vbh = createVertexBuffer(
    //    makeRef(cubeVertices, sizeof(cubeVertices)),
    //    worldWp::PosColorVertex::layout);

    IndexBufferHandle ibh = builder.getIBufferHandle();
    //IndexBufferHandle ibh = createIndexBuffer(
    //    makeRef(cubeTriList, sizeof(cubeTriList)));


    ShaderHandle vsh = worldWp::util::load_shader("build/src/vs_simple.bin");
    ShaderHandle fsh = worldWp::util::load_shader("build/src/fs_simple.bin");
    ProgramHandle program = createProgram(vsh, fsh, true);

    touch(clearView);

    float pos {-15.0f};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int oldWidth = width, oldHeight = height;
        glfwGetWindowSize(window, &width, &height);
        if (width != oldWidth || height != oldHeight) {
            reset(width, height, BGFX_RESET_VSYNC);
            setViewRect(clearView, 0, 0, BackbufferRatio::Equal);
        }

        bx::Vec3 at  {0.0f, 0.0f,   0.0f};
        bx::Vec3 eye {0.0f, 0.0f, -25.0f};

        float view[16];
        bx::mtxLookAt(view, eye, at);

        float proj[16];
        bx::mtxProj(proj,
            90.0f,
            ((float)width)/height,
            0.1f,
            100.0f,
            bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(clearView, view, proj);
        bgfx::setViewRect(clearView, 0, 0, width, height);

        touch(clearView);

        float mtx[16];
        bx::mtxRotateY(mtx, 0.0f);
        //bx::mtxRotateXY(mtx, pos*.1, -pos*.1);
        pos+=0.01;
        //mtx[12] = pos+=0.01;
        //mtx[13] = pos+=0.01;
        //mtx[14] = 0.0f;

        bgfx::setTransform(mtx);

        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_DEFAULT);

        bgfx::submit(clearView, program);

        frame();
    }

    destroy(vbh);
    destroy(ibh);
    shutdown();
    glfwTerminate();
    return 0;
}
