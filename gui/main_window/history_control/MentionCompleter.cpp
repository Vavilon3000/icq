#include "stdafx.h"
#include "MentionCompleter.h"

#include "../../controls/TransparentScrollBar.h"
#include "../contact_list/MentionModel.h"
#include "../contact_list/MentionItemDelegate.h"
#include "../../utils/utils.h"

namespace
{
    const auto maxItemsForMaxHeight = 7;
}

namespace Ui
{
    MentionCompleter::MentionCompleter(QWidget* _parent)
        : QWidget(_parent)
        , model_(Logic::GetMentionModel())
        , delegate_(new Logic::MentionItemDelegate(this))
    {
        Utils::ApplyStyle(
            this,
            ql1s("border-top-color:")
            % CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER).name()
            % ql1s("; border-top-width: 1dip; border-top-style: solid; background-color: #ffffff;")
        );

        QWidget* wdg = new QWidget();
        auto rootLayout = Utils::emptyVLayout(this);
        rootLayout->addWidget(wdg);

        auto wLayout = Utils::emptyVLayout(wdg);
        view_ = CreateFocusableViewAndSetTrScrollBar(wdg);
        view_->setSelectByMouseHover(true);
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        view_->setFrameShape(QFrame::NoFrame);
        view_->setSpacing(0);
        view_->setModelColumn(0);
        view_->setUniformItemSizes(false);
        view_->setBatchSize(50);
        view_->setStyleSheet(qsl("background: transparent;"));
        view_->setCursor(Qt::PointingHandCursor);
        view_->setMouseTracking(true);
        view_->setAcceptDrops(false);
        view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        view_->setAutoScroll(false);
        view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        wLayout->addWidget(view_);

        view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        Utils::grabTouchWidget(view_->viewport(), true);

        view_->setModel(model_);
        view_->setItemDelegate(delegate_);

        connect(view_, &FocusableListView::clicked, this, &MentionCompleter::itemClicked, Qt::QueuedConnection);
        connect(model_, &Logic::MentionModel::results, this, &MentionCompleter::onResults, Qt::QueuedConnection);
    }

    void MentionCompleter::setDialogAimId(const QString & _aimid)
    {
        model_->setDialogAimId(_aimid);
    }

    bool MentionCompleter::insertCurrent()
    {
        const auto selectedIndex = view_->selectionModel()->currentIndex();
        if (selectedIndex.isValid())
        {
            emit itemClicked(selectedIndex);
            return true;
        }
        return false;
    }

    void MentionCompleter::hideEvent(QHideEvent * _event)
    {
        emit hidden();
    }

    void MentionCompleter::keyPressEvent(QKeyEvent * _event)
    {
        _event->ignore();
        if (_event->key() == Qt::Key_Up || _event->key() == Qt::Key_Down)
        {
            const auto pressedUp = _event->key() == Qt::Key_Up;
            keyUpOrDownPressed(pressedUp);
            _event->accept();
        }
        else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
        {
            if (insertCurrent())
                _event->accept();
        }
    }

    int MentionCompleter::calcHeight() const
    {
        const auto count = itemCount();

        auto newHeight = 0;
        QStyleOptionViewItem unused;
        for (auto i = 0; i < std::min(count, maxItemsForMaxHeight); ++i)
            newHeight += delegate_->sizeHint(unused, model_->index(i)).height();

        if (count > maxItemsForMaxHeight)
            newHeight += delegate_->itemHeight() / 2; // show user there are more items below

        return newHeight;
    }

    void MentionCompleter::setSearchPattern(const QString & _pattern)
    {
        model_->setSearchPattern(_pattern);
    }

    QString MentionCompleter::getSearchPattern() const
    {
        return model_->getSearchPattern();
    }

    void MentionCompleter::beforeShow()
    {
        model_->beforeShow();
    }

    void MentionCompleter::onResults()
    {
        if (itemCount())
            view_->selectionModel()->setCurrentIndex(model_->index(0), QItemSelectionModel::ClearAndSelect);
        else
            view_->selectionModel()->clear();

        view_->scrollToTop();
        recalcHeight();
        emit results(itemCount());
    }

    void MentionCompleter::itemClicked(const QModelIndex& _current)
    {
        const auto model = qobject_cast<const Logic::MentionModel*>(_current.model());
        if (model && !model_->isServiceItem(_current))
        {
            auto item = _current.data().value<Logic::MentionModelItem>();
            emit contactSelected(item.aimId_, item.friendlyName_);
            hide();
        }
    }

    void MentionCompleter::keyUpOrDownPressed(const bool _isUpPressed)
    {
        auto inc = _isUpPressed ? -1 : 1;

        QModelIndex i = view_->selectionModel()->currentIndex();

        i = model_->index(i.row() + inc);

        while (model_->isServiceItem(i))
        {
            i = model_->index(i.row() + inc);
            if (!i.isValid())
                return;
        }

        {
            QSignalBlocker sb(view_->selectionModel());
            view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        }
        model_->emitChanged(model_->index(i.row() - inc), i);
        view_->scrollTo(i);
    }

    void MentionCompleter::recalcHeight()
    {
        const auto curBot = y() + height();

        setFixedHeight(calcHeight());
        move(0, curBot - height());
    }

    bool MentionCompleter::completerVisible() const
    {
        return isVisible() && height() > 0;
    }

    int MentionCompleter::itemCount() const
    {
        return model_->rowCount();
    }

    bool MentionCompleter::hasSelectedItem() const
    {
        return view_->selectionModel()->currentIndex().isValid();
    }
}

