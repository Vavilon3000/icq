#pragma once

#include "../../types/message.h"

class QPainter;

namespace Logic
{
 class RecentItemDelegate;
}

namespace Ui
{
    class MessageAlertWidget : public QWidget
    {
        Q_OBJECT
            Q_SIGNALS:

        void clicked(const QString&, const QString&, qint64);
        void closed(const QString&, const QString&, qint64);

    public:
        MessageAlertWidget(const Data::DlgState& state, Logic::RecentItemDelegate* delegate, QWidget* parent);

        QString id() const;
        QString mailId() const;
        qint64 mentionId() const;

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void mousePressEvent(QMouseEvent *) override;
        void mouseReleaseEvent(QMouseEvent *) override;

    private Q_SLOTS:
        void avatarChanged(const QString&);

    private:
        Data::DlgState State_;
        Logic::RecentItemDelegate* Delegate_;
        QStyleOptionViewItem Options_;
    };
}