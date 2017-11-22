#pragma once

namespace Ui
{
    class qt_gui_settings;

    class SemitransparentWindowAnimated : public QWidget
    {
        Q_OBJECT

    public:
        SemitransparentWindowAnimated(QWidget* _parent, int _duration);

        Q_PROPERTY(int step READ getStep WRITE setStep)

        void setStep(int _val) { Step_ = _val; update(); }
        int getStep() const { return Step_; }

        void Show();
        void Hide();
        void forceHide();
        bool isSemiWindowVisible() const;

        bool isMainWindow() const;
        void updateSize();

    private Q_SLOTS:
        void finished();

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual void mousePressEvent(QMouseEvent *e) override;
        virtual void hideEvent(QHideEvent * event) override;

        void decSemiwindowsCount();
        void incSemiwindowsCount();
        int  getSemiwindowsCount() const;
        bool isSemiWindowsTouchSwallowed() const;
        void setSemiwindowsTouchSwallowed(bool _val);

    private:
        QPropertyAnimation* Animation_;
        int Step_;
        bool main_;
        bool isMainWindow_;
    };
}
