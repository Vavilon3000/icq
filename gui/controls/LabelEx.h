#pragma once

namespace Ui
{
    class LabelEx : public QLabel
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        LabelEx(QWidget* _parent);
        void setColor(const QColor &_color);

    protected:
        void mouseReleaseEvent(QMouseEvent*) override;
    };
}