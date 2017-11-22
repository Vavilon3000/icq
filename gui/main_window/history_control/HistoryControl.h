#pragma once

#include "../../types/message.h"

namespace core
{
    enum class typing_status;
}

namespace Ui
{
    class history_control;
    class HistoryControlPage;

    class HistoryControl : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void quote(const QVector<Data::Quote>&);
        void forward(const QVector<Data::Quote>&);
        void clicked();

    public Q_SLOTS:
        void contactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId);
        void contactSelectedToLastMessage(QString _aimId, qint64 _messageId);
        void switchToEmpty();

        void addPageToDialogHistory(const QString& _aimId);
        void clearDialogHistory();
        void switchToPrevDialogPage();

    public:
        HistoryControl(QWidget* parent);
        ~HistoryControl();
        void cancelSelection();
        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        void notifyApplicationWindowActive(const bool isActive);
        void scrollHistoryToBottom(const QString& _contact) const;
        void inputTyped();

    protected:
        void mouseReleaseEvent(QMouseEvent *) override;

    private Q_SLOTS:
        void updatePages();
        void closeDialog(const QString& _aimId);
        void leaveDialog(const QString& _aimId);
        void mainPageChanged();
        void onContactSelected(const QString& _aimId);

    private:
        HistoryControlPage* getCurrentPage() const;

        QMap<QString, HistoryControlPage*> pages_;
        QMap<QString, QTime> times_;
        QString current_;
        QTimer* timer_;
        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QStackedWidget *stacked_widget_;
        QWidget* page_;

        std::deque<QString> dialogHistory_;
    };
}
