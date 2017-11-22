#pragma once

namespace Themes
{
    enum class PixmapResourceId;
}

namespace Ui
{

    class HistoryControlPageItem;

    class MessageTimeWidget : public QWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void showAnimated();
        void hideAnimated();
        void showIfNeeded();

    public:
        explicit MessageTimeWidget(HistoryControlPageItem *messageItem);

        void setTime(const int32_t timestamp);
        virtual QSize sizeHint() const override;
        void setContact(const QString& _aimId) { aimId_ = _aimId; }
        QColor getTimeColor() const;

    protected:
        virtual void paintEvent(QPaintEvent *) override;

    private:
        bool visible_;
        QString aimId_;
        QString TimeText_;
        QSize TimeTextSize_;
    };

}