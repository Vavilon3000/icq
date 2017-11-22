#pragma once

namespace Ui
{
    class MainWindow;

    namespace TitleBar
    {
        const int icon_height = 36;
        const int icon_width = 40;
        const int balloon_size = 20;
    }

    class UnreadWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();

    public:
        UnreadWidget(QWidget* _parent, bool _drawBadgeBorder = true, int32_t _badgeFontSize = 14);
        void setHoverVisible(bool _hoverVisible);
        QPixmap renderToPixmap(unsigned _unreadsCount, bool _hoveredState, bool _pressedState);

    protected:
        void paintEvent(QPaintEvent *) override;
        void mousePressEvent(QMouseEvent *) override;
        void mouseReleaseEvent(QMouseEvent *) override;
        void enterEvent(QEvent *) override;
        void leaveEvent(QEvent *) override;

    protected Q_SLOTS:
        void setUnreads(unsigned);

    protected:
        bool pressed_;
        bool hovered_;
        bool hoverable_;
        unsigned unreads_;
        QString pathIcon_;
        QString pathIconHovered_;
        QString pathIconPressed_;
        bool drawBadgeBorder_;
        int32_t fontSize_;
    };


    class UnreadMsgWidget: public UnreadWidget
    {
        Q_OBJECT

    public:
        UnreadMsgWidget(QWidget* parent);

    private Q_SLOTS:
        void updateIcon();
        void loggedIn();
        void openNextUnread();
    };



    class UnreadMailWidget : public UnreadWidget
    {
        Q_OBJECT

    public:
        UnreadMailWidget(QWidget* parent);

    private:
        QString Email_;

    private Q_SLOTS:
        void mailStatus(QString, unsigned, bool);
        void openMailBox();
    };
}
