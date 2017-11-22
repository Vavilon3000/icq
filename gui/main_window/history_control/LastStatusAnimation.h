#pragma once


namespace Ui
{
    class HistoryControlPageItem;
    enum class LastStatus;

    class LastStatusAnimation : public QObject
    {
        Q_OBJECT

    public:
        explicit LastStatusAnimation(HistoryControlPageItem* _parent);
        ~LastStatusAnimation();

        void setLastStatus(LastStatus _lastStatus);

        void hideStatus();
        void showStatus();

        void drawLastStatus(
            QPainter& _p,
            const int _x,
            const int _y,
            const int _dx,
            const int _dy) const;

        static QPixmap getPendingPixmap(const bool _selected);

    private:

        void startPendingTimer();
        void stopPendingTimer();
        bool isPendingTimerActive() const;

    private:

        HistoryControlPageItem* widget_;

        bool isActive_;
        bool isPlay_;
        bool show_;

        QTimer* pendingTimer_;

        LastStatus  lastStatus_;
    };
}