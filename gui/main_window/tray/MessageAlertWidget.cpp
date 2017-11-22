#include "stdafx.h"
#include "MessageAlertWidget.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace Ui
{
    MessageAlertWidget::MessageAlertWidget(const Data::DlgState& state, Logic::RecentItemDelegate* delegate, QWidget* parent)
        : QWidget(parent)
        , State_(state)
        , Delegate_(delegate)
    {
        Options_.initFrom(this);
        setFixedSize(Delegate_->sizeHintForAlert());
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &MessageAlertWidget::avatarChanged, Qt::QueuedConnection);
    }

    QString MessageAlertWidget::id() const
    {
        return State_.AimId_;
    }

    QString MessageAlertWidget::mailId() const
    {
        return State_.MailId_;
    }

    qint64 MessageAlertWidget::mentionId() const
    {
        return State_.SearchedMsgId_;
    }

    void MessageAlertWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        Delegate_->paint(&painter, Options_, State_, false);
    }

    void MessageAlertWidget::resizeEvent(QResizeEvent* e)
    {
        Options_.initFrom(this);
        return QWidget::resizeEvent(e);
    }

    void MessageAlertWidget::enterEvent(QEvent* e)
    {
        Options_.state = Options_.state | QStyle::State_MouseOver;
        update();
        return QWidget::enterEvent(e);
    }

    void MessageAlertWidget::leaveEvent(QEvent* e)
    {
        Options_.state = Options_.state & ~QStyle::State_MouseOver;
        update();
        return QWidget::leaveEvent(e);
    }

    void MessageAlertWidget::mousePressEvent(QMouseEvent* e)
    {
        e->accept();
    }

    void MessageAlertWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        e->accept();
        if (e->button() == Qt::RightButton)
            emit closed(State_.AimId_, State_.MailId_, State_.SearchedMsgId_);
        else
            emit clicked(State_.AimId_, State_.MailId_, State_.SearchedMsgId_);
    }

    void MessageAlertWidget::avatarChanged(const QString& aimId)
    {
        if (aimId != State_.AimId_ && State_.Friendly_.indexOf(aimId) == -1)
            return;

        update();
    }
}
