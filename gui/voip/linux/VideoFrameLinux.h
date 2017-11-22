#ifndef __VIDEO_FRAME_LINUX_H__
#define __VIDEO_FRAME_LINUX_H__

#include <QWidget>
#include "../VideoFrame.h"
#include <GLFW/glfw3.h>
#include <mutex>

namespace Ui
{
    class BaseVideoPanel;
}

namespace platform_linux
{

class GraphicsPanelLinux : public platform_specific::GraphicsPanel
{
    Q_OBJECT
public:
    GraphicsPanelLinux(QWidget* _parent, std::vector<Ui::BaseVideoPanel*>& _panels, bool primaryVideo);
    virtual ~GraphicsPanelLinux();

    WId frameId() const;
    void resizeEvent(QResizeEvent * event) override;
private:
    GLFWwindow *_videoWindow;
};

}

#endif//__VIDEO_FRAME_WIN32_H__
