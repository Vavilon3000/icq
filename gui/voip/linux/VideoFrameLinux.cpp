#include "stdafx.h"
#include "VideoFrameLinux.h"
#include "../CommonUI.h"

extern "C" void glfwSetRoot(void *root);

static std::mutex g_wnd_mutex;

class GLFWScope
{
public:
    GLFWScope()
    {
        if (!glfwInit())
        {
            printf("error: glfw init failed\n");
            assert(0);
        }
    }
    ~GLFWScope() { glfwTerminate(); }
} g_GLFWScope;

platform_linux::GraphicsPanelLinux::GraphicsPanelLinux(
    QWidget* _parent,
    std::vector<Ui::BaseVideoPanel*>&/* panels*/, bool /*primaryVideo*/)
    : platform_specific::GraphicsPanel(_parent)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, 1);
    std::lock_guard<std::mutex> lock(g_wnd_mutex);
    glfwSetRoot((void*)QWidget::winId());
    _videoWindow = glfwCreateWindow(600, 400, "Video", NULL, NULL);
}

platform_linux::GraphicsPanelLinux::~GraphicsPanelLinux()
{
    glfwDestroyWindow(_videoWindow);
}

WId platform_linux::GraphicsPanelLinux::frameId() const
{
    return (WId)_videoWindow;
}

void platform_linux::GraphicsPanelLinux::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    QRect r = rect();
    glfwSetWindowPos(_videoWindow, r.x(), r.y());
    glfwSetWindowSize(_videoWindow, r.width(), r.height());
}

