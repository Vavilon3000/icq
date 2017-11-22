#include "stdafx.h"
#include "HistoryControl.h"

#include "HistoryControlPage.h"
#include "MessagesModel.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/log/log.h"
#include "../contact_list/RecentsModel.h"
#include "../../gui_settings.h"

namespace
{
    const std::chrono::milliseconds update_timer_timeout = std::chrono::seconds(60);
    const auto maxPagesInDialogHistory = 1;
}

namespace Ui
{
	HistoryControl::HistoryControl(QWidget* parent)
		: QWidget(parent)
		, timer_(new QTimer(this))
	{
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        vertical_layout_ = Utils::emptyVLayout(this);
        stacked_widget_ = new QStackedWidget(this);
        stacked_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        page_ = new QWidget();
        page_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        vertical_layout_2_ = Utils::emptyVLayout(page_);
        stacked_widget_->addWidget(page_);
        vertical_layout_->addWidget(stacked_widget_);
        Testing::setAccessibleName(page_, qsl("AS page_"));
        Testing::setAccessibleName(stacked_widget_, qsl("AS stacked_widget_"));

        connect(timer_, &QTimer::timeout, this, &HistoryControl::updatePages, Qt::QueuedConnection);
        timer_->setInterval(update_timer_timeout.count());
        timer_->setTimerType(Qt::CoarseTimer);
        timer_->setSingleShot(false);
        timer_->start();

        connect(Logic::getContactListModel(), &Logic::ContactListModel::leave_dialog, this, &HistoryControl::leaveDialog, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &HistoryControl::closeDialog, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &HistoryControl::onContactSelected, Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &HistoryControl::closeDialog, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &HistoryControl::mainPageChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addPageToDialogHistory, this, &HistoryControl::addPageToDialogHistory);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::switchToPrevDialog, this, &HistoryControl::switchToPrevDialogPage);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearDialogHistory, this, &HistoryControl::clearDialogHistory);
	}

	HistoryControl::~HistoryControl()
	{
	}

    void HistoryControl::cancelSelection()
    {
        for (const auto &page : Utils::as_const(pages_))
        {
            assert(page);
            page->cancelSelection();
        }
    }

    HistoryControlPage* HistoryControl::getHistoryPage(const QString& aimId) const
    {
        const auto iter = pages_.find(aimId);
        if (iter == pages_.end())
        {
            return nullptr;
        }

        return *iter;
    }

    void HistoryControl::notifyApplicationWindowActive(const bool isActive)
    {
        if (!isActive)
        {
            for (const auto &page : Utils::as_const(pages_))
            {
                page->suspendVisisbleItems();
            }
        }
        else
        {
            auto page = getCurrentPage();
            if (page)
                page->resumeVisibleItems();
        }

        auto page = getCurrentPage();
        if (page)
            page->notifyApplicationWindowActive(isActive);
    }

    void HistoryControl::scrollHistoryToBottom(const QString& _contact) const
    {
        auto page = getHistoryPage(_contact);
        if (page)
            page->scrollToBottom();
    }

    void HistoryControl::mouseReleaseEvent(QMouseEvent *e)
    {
        QWidget::mouseReleaseEvent(e);

        if (getCurrentPage())
            emit clicked();
    }

	void HistoryControl::updatePages()
	{
        const auto currentTime = QTime::currentTime();
        constexpr int time = 300;
		for (auto iter = times_.begin(); iter != times_.end(); )
		{
			if (iter.value().secsTo(currentTime) >= time && iter.key() != current_)
			{
				closeDialog(iter.key());
 				iter = times_.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

    void HistoryControl::contactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId)
    {
        assert(!_aimId.isEmpty());

        const bool contactChanged = (_aimId != current_);

        const Data::DlgState data = Logic::getRecentsModel()->getDlgState(_aimId);
        const qint64 lastReadMsg = data.UnreadCount_ > 0 ? data.YoursLastRead_ : -1;
        auto scrollMode = Logic::scroll_mode_type::none;
        if (_messageId == -1)
        {
            if (Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, true))
            {
                _messageId = lastReadMsg;
                if (_messageId != -1)
                    scrollMode = Logic::scroll_mode_type::unread;
            }
        }
        else
        {
            scrollMode = Logic::scroll_mode_type::search;
        }

        auto oldPage = getCurrentPage();
        if (oldPage)
        {
            oldPage->setQuoteId(_quoteId);

            if (_messageId == -1)
            {
                if (oldPage->aimId() == _aimId)
                {
                    Utils::InterConnector::instance().setFocusOnInput();
                    return;
                }

                oldPage->updateState(true);
            }

            if (contactChanged)
                oldPage->pageLeave();
        }

        auto page = getHistoryPage(_aimId);
        const auto createNewPage = (page == nullptr);
        if (createNewPage)
        {
            auto newPage = new HistoryControlPage(this, _aimId);
            newPage->setQuoteId(_quoteId);

            Testing::setAccessibleName(newPage, qsl("AS HistoryControlPage") % _aimId);

            page = *(pages_.insert(_aimId, newPage));
            connect(page, &HistoryControlPage::quote, this, &HistoryControl::quote, Qt::QueuedConnection);
            //connect(page, &HistoryControlPage::forward, this, &HistoryControl::forward, Qt::QueuedConnection);

            stacked_widget_->addWidget(page);

            Logic::getContactListModel()->setCurrentCallbackHappened(newPage);
        }

        Logic::MessageKey neededMessageKey;

        if (_messageId != -1)
        {
            auto key = Logic::GetMessagesModel()->getKey(_aimId, _messageId);
            if (!key.isEmpty() && page->containsWidgetWithKey(key))
            {
                neededMessageKey = std::move(key);
            }
            else
            {
                Logic::GetMessagesModel()->messagesDeletedUpTo(_aimId, -1);
                Logic::GetMessagesModel()->setRecvLastMsg(_aimId, false);
            }
        }

        if (neededMessageKey.isEmpty())
            emit Utils::InterConnector::instance().historyControlReady(_aimId, _messageId, data, -1, createNewPage, scrollMode);

        const auto prevButtonVisible = !dialogHistory_.empty();
        for (const auto& p : Utils::as_const(pages_))
        {
            const auto isBackgroundPage = (p != page);
            if (isBackgroundPage)
                p->suspendVisisbleItems();

            page->setPrevChatButtonVisible(prevButtonVisible);
        }

        stacked_widget_->setCurrentWidget(page);
        page->updateState(false);
        page->open();

        if (contactChanged)
            page->pageOpen();

        if (!current_.isEmpty())
            times_[current_] = QTime::currentTime();
        current_ = _aimId;

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        GetDispatcher()->post_message_to_core(qsl("dialogs/add"), collection.get());

        if (!neededMessageKey.isEmpty())
        {
            if (scrollMode == Logic::scroll_mode_type::unread)
                Logic::GetMessagesModel()->updateNew(_aimId, _messageId);
            page->setQuoteId(-1); // drop QuoteId to avoid double starting of animation
            page->scrollTo(neededMessageKey, scrollMode);
        }
        else if (_quoteId > 0)
        {
            Logic::GetMessagesModel()->emitQuote(_quoteId);
        }
    }

	void HistoryControl::contactSelectedToLastMessage(QString _aimId, qint64 _messageId)
	{
		//const auto bMoveHistory = Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false);
		//if (bMoveHistory && _messageId > 0)
		//{
  //          HistoryControlPage* page = getCurrentPage();
  //          if (page && page->aimId() == _aimId)
  //          {
  //              page->showNewMessageForce();
  //              emit contactSelected(_aimId, _messageId, -1);
  //          }
		//}
	}

    void HistoryControl::leaveDialog(const QString& _aimId)
    {
        auto iter_page = pages_.constFind(_aimId);
        if (iter_page == pages_.cend())
        {
            return;
        }

        if (current_ == _aimId)
        {
            switchToEmpty();
        }
    }

    void HistoryControl::switchToEmpty()
    {
        current_.clear();
        stacked_widget_->setCurrentIndex(0);
        Ui::MainPage::instance()->hideInput();
    }

    void HistoryControl::addPageToDialogHistory(const QString & _aimId)
    {
        if (dialogHistory_.empty() || dialogHistory_.back() != _aimId)
        {
            dialogHistory_.push_back(_aimId);
            emit Utils::InterConnector::instance().pageAddedToDialogHistory();
        }

        while (dialogHistory_.size() > maxPagesInDialogHistory)
            dialogHistory_.pop_front();
    }

    void HistoryControl::clearDialogHistory()
    {
        dialogHistory_.clear();
        emit Utils::InterConnector::instance().noPagesInDialogHistory();
    }

    void HistoryControl::switchToPrevDialogPage()
    {
        if (!dialogHistory_.empty())
        {
            Logic::getContactListModel()->setCurrent(dialogHistory_.back(), -1, true);
            dialogHistory_.pop_back();
        }

        if (dialogHistory_.empty())
        {
            emit Utils::InterConnector::instance().noPagesInDialogHistory();
        }
    }

	void HistoryControl::closeDialog(const QString& _aimId)
	{
		auto iter_page = pages_.find(_aimId);
		if (iter_page == pages_.end())
		{
			return;
		}

		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", _aimId);
		GetDispatcher()->post_message_to_core(qsl("dialogs/remove"), collection.get());
		Logic::GetMessagesModel()->removeDialog(_aimId);

		stacked_widget_->removeWidget(iter_page.value());

        if (current_ == _aimId)
        {
            switchToEmpty();
        }

        iter_page.value()->pageLeave();

		delete iter_page.value();
		pages_.erase(iter_page);

        emit Utils::InterConnector::instance().dialogClosed(_aimId);
	}

    void HistoryControl::mainPageChanged()
    {
        auto page = getCurrentPage();
        if (!page)
            return;

        if (Utils::InterConnector::instance().getMainWindow()->isMainPage())
        {
            page->pageOpen();
        }
        else
        {
            page->pageLeave();
        }
    }

    void HistoryControl::onContactSelected(const QString & _aimId)
    {
        auto page = getCurrentPage();
        if (page && _aimId.isEmpty())
        {
            switchToEmpty();
            page->pageLeave();
        }
    }

    HistoryControlPage* HistoryControl::getCurrentPage() const
    {
        assert(stacked_widget_);

        return qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
    }

    void HistoryControl::inputTyped()
    {
        auto page = getCurrentPage();
        assert(page);
        if (!page)
            return;

        page->inputTyped();
    }
}
