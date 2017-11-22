#pragma once

#include "../../types/message.h"

class QPainter;

namespace Logic
{
    class RecentItemDelegate;
}

namespace Ui
{
    enum class AlertType
    {
        alertTypeMessage = 0,
        alertTypeEmail = 1,
        alertTypeMentionMe = 2
    };

    class TextEmojiWidget;

    class RecentMessagesAlert : public QWidget
    {
        Q_OBJECT
        Q_SIGNALS:

        void messageClicked(const QString& _aimId, const QString& _mailId, const qint64 _mentionId, const AlertType _alertType);
        void changed();

    public:
        RecentMessagesAlert(Logic::RecentItemDelegate* delegate, const AlertType _alertType);
        ~RecentMessagesAlert();

        void addAlert(const Data::DlgState& state);
        void markShowed();
        bool updateMailStatusAlert(const Data::DlgState& state);

    protected:
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void showEvent(QShowEvent *) override;
        void hideEvent(QHideEvent *) override;

    private:
        void init();
        bool isMailAlert() const;
        bool isMessageAlert() const;
        bool isMentionAlert() const;

    private Q_SLOTS:
        void closeAlert();
        void statsCloseAlert();
        void viewAll();
        void startAnimation();
        void messageAlertClicked(const QString&, const QString&, qint64);
        void messageAlertClosed(const QString&, const QString&, qint64);

    private:
        Logic::RecentItemDelegate* Delegate_;
        QVBoxLayout* Layout_;
        unsigned AlertsCount_;
        QPushButton* CloseButton_;
        QWidget* ViewAllWidget_;
        QTimer* Timer_;
        QPropertyAnimation* Animation_;
        int Height_;
        TextEmojiWidget* emailLabel_;
        unsigned MaxAlertCount_;
        bool CursorIn_;
        bool ViewAllWidgetVisible_;
        AlertType alertType_;
    };
}

Q_DECLARE_METATYPE(Ui::AlertType);
