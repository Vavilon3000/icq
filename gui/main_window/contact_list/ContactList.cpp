#include "stdafx.h"

#include "Common.h"
#include "ContactList.h"
#include "ContactItem.h"
#include "ContactListModel.h"
#include "ContactListItemDelegate.h"
#include "ContactListItemRenderer.h"
#include "ContactListWidget.h"
#include "ContactListUtils.h"
#include "RecentsItemRenderer.h"
#include "RecentsModel.h"
#include "RecentItemDelegate.h"
#include "UnknownsModel.h"
#include "UnknownItemDelegate.h"
#include "SearchModelDLG.h"
#include "../MainWindow.h"
#include "../ContactDialog.h"
#include "../contact_list/ChatMembersModel.h"
#include "../livechats/LiveChatsModel.h"
#include "../livechats/LiveChatItemDelegate.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../settings/SettingsTab.h"
#include "../../types/contact.h"
#include "../../types/typing.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/CustomButton.h"
#include "../../controls/HorScrollableView.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../fonts.h"
#include "../history_control/MessagesModel.h"

namespace
{
    const int autoscroll_offset_recents = 68;
    const int autoscroll_offset_cl = 44;
    const int autoscroll_speed_pixels = 10;
    const int autoscroll_timeout = 50;

    const int LEFT_OFFSET = 16;
    const int LIVECHATS_BACK_WIDTH = 52;
    const int BACK_HEIGHT = 12;
    const int RECENTS_HEIGHT = 68;
}

namespace Ui
{
    RCLEventFilter::RCLEventFilter(ContactList* _cl)
    : QObject(_cl)
    , cl_(_cl)
    {

    }

    bool RCLEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Gesture)
        {
            QGestureEvent* guesture  = static_cast<QGestureEvent*>(_event);
            if (QGesture *tapandhold = guesture->gesture(Qt::TapAndHoldGesture))
            {
                if (tapandhold->hasHotSpot() && tapandhold->state() == Qt::GestureFinished)
                {
                    cl_->triggerTapAndHold(true);
                    guesture->accept(Qt::TapAndHoldGesture);
                }
            }
        }
        if (_event->type() == QEvent::DragEnter || _event->type() == QEvent::DragMove)
        {
            Utils::InterConnector::instance().getMainWindow()->closeGallery();
            Utils::InterConnector::instance().getMainWindow()->activate();
            QDropEvent* de = static_cast<QDropEvent*>(_event);
            if (de->mimeData() && de->mimeData()->hasUrls())
            {
                de->acceptProposedAction();
                cl_->dragPositionUpdate(de->pos());
            }
            else
            {
                de->setDropAction(Qt::IgnoreAction);
            }
            return true;
        }
        if (_event->type() == QEvent::DragLeave)
        {
            cl_->dragPositionUpdate(QPoint());
            return true;
        }
        if (_event->type() == QEvent::Drop)
        {
            QDropEvent* e = static_cast<QDropEvent*>(_event);
            const QMimeData* mimeData = e->mimeData();
            QList<QUrl> urlList;
            if (mimeData->hasUrls())
            {
                urlList = mimeData->urls();
            }

            cl_->dropFiles(e->pos(), urlList);
            e->acceptProposedAction();
            cl_->dragPositionUpdate(QPoint());
        }

        if (_event->type() == QEvent::MouseButtonDblClick)
        {
            _event->ignore();
            return true;
        }

        if (_event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(_event);
            if (e->button() == Qt::LeftButton)
            {
                cl_->triggerTapAndHold(false);
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    ContactList::ContactList(QWidget* _parent)
    : QWidget(_parent)
    , liveChatsDelegate_(new Logic::LiveChatItemDelegate(this))
    , listEventFilter_(new RCLEventFilter(this))
    , noRecentsYet_(nullptr)
    , popupMenu_(nullptr)
    , recentsDelegate_(new Logic::RecentItemDelegate(this))
    , unknownsDelegate_(new Logic::UnknownItemDelegate(this))
    , contactListWidget_(new ContactListWidget(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr))
    , scrolledView_(nullptr)
    , scrollMultipler_(1)
    , currentTab_(RECENTS)
    , prevTab_(RECENTS)
    , noRecentsYetShown_(false)
    , tapAndHold_(false)
    , pictureOnlyView_(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setStyleSheet(Utils::LoadStyle(qsl(":/qss/contact_list")));
        auto mainLayout = Utils::emptyVLayout(this);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        stackedWidget_ = new QStackedWidget(this);

        recentsPage_ = new QWidget();
        recentsLayout_ = Utils::emptyVLayout(recentsPage_);
        recentsView_ = CreateFocusableViewAndSetTrScrollBar(recentsPage_);
        recentsView_->setFrameShape(QFrame::NoFrame);
        recentsView_->setLineWidth(0);
        recentsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recentsView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        recentsView_->setAutoScroll(false);
        recentsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        recentsView_->setUniformItemSizes(false);
        recentsView_->setBatchSize(50);
        recentsView_->setStyleSheet(qsl("background: transparent;"));
        recentsView_->setCursor(Qt::PointingHandCursor);
        recentsView_->setMouseTracking(true);
        recentsView_->setAcceptDrops(true);
        recentsView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        connect(recentsView_->verticalScrollBar(), &QScrollBar::actionTriggered, this, &ContactList::recentsScrollActionTriggered, Qt::DirectConnection);

        recentsLayout_->addWidget(recentsView_);
        stackedWidget_->addWidget(recentsPage_);

        Testing::setAccessibleName(recentsView_, qsl("AS recentsView_"));
        Testing::setAccessibleName(recentsPage_, qsl("AS recentsPage_"));
        {
            liveChatsPage_ = new QWidget();
            auto livechatsLayout = Utils::emptyVLayout(liveChatsPage_);

            auto back = new CustomButton(liveChatsPage_, qsl(":/controls/arrow_left_100"));
            back->setFixedSize(QSize(Utils::scale_value(LIVECHATS_BACK_WIDTH), Utils::scale_value(BACK_HEIGHT)));
            back->setStyleSheet(qsl("background: transparent; border-style: none;"));
            back->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            back->setAlign(Qt::AlignLeft);
            auto topWidget = new QWidget(liveChatsPage_);
            auto hLayout = Utils::emptyHLayout(topWidget);
            hLayout->setContentsMargins(0, 0, Utils::scale_value(LEFT_OFFSET + LIVECHATS_BACK_WIDTH), 0);
            hLayout->addWidget(back);
            auto topicLabel = new LabelEx(liveChatsPage_);
            Utils::grabTouchWidget(topicLabel);
            topicLabel->setAlignment(Qt::AlignCenter);
            topicLabel->setFixedHeight(Utils::scale_value(48));
            topicLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            topicLabel->setText(QT_TRANSLATE_NOOP("groupchats","Live chats"));
            topicLabel->setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Medium));
            topicLabel->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
            hLayout->addWidget(topicLabel);
            livechatsLayout->addWidget(topWidget);

            back->setCursor(QCursor(Qt::PointingHandCursor));
            connect(back, &QAbstractButton::clicked, this, &ContactList::recentsClicked);

            liveChatsView_ = CreateFocusableViewAndSetTrScrollBar(liveChatsPage_);
            liveChatsView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            liveChatsView_->setFrameShape(QFrame::NoFrame);
            liveChatsView_->setFrameShadow(QFrame::Plain);
            liveChatsView_->setFocusPolicy(Qt::NoFocus);
            liveChatsView_->setLineWidth(0);
            liveChatsView_->setBatchSize(50);
            liveChatsView_->setStyleSheet(qsl("background: transparent;"));
            liveChatsView_->setCursor(Qt::PointingHandCursor);
            liveChatsView_->setMouseTracking(true);
            liveChatsView_->setAcceptDrops(true);
            liveChatsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            liveChatsView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
            liveChatsView_->setAutoScroll(false);

            livechatsLayout->addWidget(liveChatsView_);
            stackedWidget_->addWidget(liveChatsPage_);
        }

        stackedWidget_->addWidget(contactListWidget_);
        mainLayout->addWidget(stackedWidget_);

        stackedWidget_->setCurrentIndex(0);

        recentsView_->setAttribute(Qt::WA_MacShowFocusRect, false);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showNoRecentsYet, this, static_cast<void(ContactList::*)()>(&ContactList::showNoRecentsYet));
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideNoRecentsYet, this, &ContactList::hideNoRecentsYet);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::myProfileBack, this, &ContactList::myProfileBack);

        Utils::grabTouchWidget(recentsView_->viewport(), true);

        contactListWidget_->installEventFilterToView(listEventFilter_);
        recentsView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        recentsView_->viewport()->installEventFilter(listEventFilter_);

        connect(QScroller::scroller(recentsView_->viewport()), &QScroller::stateChanged, this, &ContactList::touchScrollStateChangedRecents, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::select, this, Utils::QOverload<const QString&, qint64, qint64, Logic::UpdateChatSelection>::of(&ContactList::select), Qt::QueuedConnection);

        connect(contactListWidget_->getView(), &Ui::FocusableListView::clicked, this, &ContactList::statsCLItemPressed, Qt::QueuedConnection);

        connect(this, &ContactList::groupClicked, Logic::getContactListModel(), &Logic::ContactListModel::groupClicked, Qt::QueuedConnection);

        Logic::getUnknownsModel(); // just initialization
        recentsView_->setModel(Logic::getRecentsModel());
        recentsView_->setItemDelegate(recentsDelegate_);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::pressed, this, &ContactList::itemPressed, Qt::QueuedConnection);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::clicked, this, &ContactList::itemClicked, Qt::QueuedConnection);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::clicked, this, &ContactList::statsRecentItemPressed, Qt::QueuedConnection);

        recentsView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

        liveChatsView_->setModel(Logic::GetLiveChatsModel());
        liveChatsView_->setItemDelegate(liveChatsDelegate_);
        connect(liveChatsView_, &Ui::FocusableListView::pressed, this, &ContactList::liveChatsItemPressed, Qt::QueuedConnection);
        liveChatsView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

        Utils::grabTouchWidget(liveChatsView_->viewport(), true);
        connect(QScroller::scroller(liveChatsView_->viewport()), &QScroller::stateChanged, this, &ContactList::touchScrollStateChangedLC, Qt::QueuedConnection);

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::orderChanged, this, &ContactList::recentOrderChanged, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, &ContactList::recentOrderChanged, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::orderChanged, this, &ContactList::recentOrderChanged, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &ContactList::recentOrderChanged, Qt::QueuedConnection);

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, [=]()
        {
            if (recentsView_ && recentsView_->model() == Logic::getRecentsModel())
                emit Logic::getRecentsModel()->refresh();
        });
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, [=]()
        {
            if (recentsView_ && recentsView_->model() == Logic::getUnknownsModel())
                emit Logic::getUnknownsModel()->refresh();
        });

        // Prepare settings
        {
            settingsTab_ = new SettingsTab(this);
            stackedWidget_->insertWidget(SETTINGS, settingsTab_);
        }

        connect(Logic::getContactListModel(), &Logic::ContactListModel::needSwitchToRecents, this, &ContactList::switchToRecents);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::selectContact, this, [this](const QString& contact) {
            select(contact);
        }, Qt::DirectConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::selectContact, this, [this](const QString& contact) {
            select(contact);
        }, Qt::DirectConnection);

        if (contactListWidget_->getRegim() == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            connect(get_gui_settings(), &Ui::qt_gui_settings::received, this, &ContactList::guiSettingsChanged, Qt::QueuedConnection);
            guiSettingsChanged();
        }

        scrollTimer_ = new QTimer(this);
        scrollTimer_->setInterval(autoscroll_timeout);
        scrollTimer_->setSingleShot(false);
        connect(scrollTimer_, &QTimer::timeout, this, &ContactList::autoScroll, Qt::QueuedConnection);

        connect(GetDispatcher(), &core_dispatcher::typingStatus, this, &ContactList::typingStatus);

        connect(GetDispatcher(), &core_dispatcher::messagesReceived, this, &ContactList::messagesReceived);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, [this]()
        {
            if (recentsView_->model() != Logic::getUnknownsModel())
            {
                recentsView_->setModel(Logic::getUnknownsModel());
                recentsView_->setItemDelegate(unknownsDelegate_);
                recentsView_->update();
                if (platform::is_apple())
                    emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);
            }
        });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, [this]()
        {
            if (recentsView_->model() != Logic::getRecentsModel())
            {
                Logic::getUnknownsModel()->markAllRead();
                recentsView_->setModel(Logic::getRecentsModel());
                recentsView_->setItemDelegate(recentsDelegate_);
                recentsView_->update();
                if (platform::is_apple())
                    emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);
            }
        });

        connect(contactListWidget_, &Ui::ContactListWidget::itemSelected, this, &ContactList::itemSelected, Qt::QueuedConnection);
        connect(contactListWidget_, &Ui::ContactListWidget::changeSelected, this, &ContactList::changeSelected, Qt::QueuedConnection);
    }

    ContactList::~ContactList()
    {

    }

    void ContactList::setSearchMode(bool _search)
    {
        if (isSearchMode() == _search)
            return;

        if (_search)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search);

        if (_search)
        {
            stackedWidget_->setCurrentIndex(SEARCH);
            contactListWidget_->getSearchModel()->setFocus();
        }
        else
        {
            stackedWidget_->setCurrentIndex(currentTab_);
        }
    }

    bool ContactList::isSearchMode() const
    {
        return stackedWidget_->currentIndex() == SEARCH;
    }

    void ContactList::itemClicked(const QModelIndex& _current)
    {
        if (qobject_cast<const Logic::ContactListModel*>(_current.model()))
        {
            Data::Contact* cont = _current.data().value<Data::Contact*>();
            if (cont->GetType() == Data::GROUP)
            {
                emit groupClicked(cont->GroupId_);
                return;
            }
        }

        const auto unkModel = qobject_cast<const Logic::UnknownsModel *>(_current.model());
        const auto changeSelection =
            !(QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
            && !(unkModel && unkModel->isServiceItem(_current));

        if (changeSelection)
        {
            emit Utils::InterConnector::instance().clearDialogHistory();
            contactListWidget_->selectionChanged(_current);
        }
    }

    void ContactList::itemPressed(const QModelIndex& _current)
    {
        if (qobject_cast<const Logic::RecentsModel*>(_current.model()) && Logic::getRecentsModel()->isServiceItem(_current))
        {
            if ((QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton) && Logic::getRecentsModel()->isUnknownsButton(_current))
            {
                emit Utils::InterConnector::instance().unknownsGoSeeThem();
            }
            else if ((QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton) && Logic::getRecentsModel()->isFavoritesGroupButton(_current))
            {
                Logic::getRecentsModel()->toggleFavoritesVisible();
            }
            return;
        }

        if (qobject_cast<const Logic::UnknownsModel *>(_current.model()) && (QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton))
        {
            const auto rect = recentsView_->visualRect(_current);
            const auto pos1 = recentsView_->mapFromGlobal(QCursor::pos());
            if (rect.contains(pos1))
            {
                QPoint pos(pos1.x(), pos1.y() - rect.y());
                if (Logic::getUnknownsModel()->isServiceItem(_current))
                {
                    const auto &buttonRect = ::ContactList::DeleteAllFrame();
                    const auto fullRect = rect.united(buttonRect);
                    if (QRect(buttonRect.x(), fullRect.y(), buttonRect.width(), fullRect.height()).contains(pos))
                    {
                        if (Utils::GetConfirmationWithTwoButtons(
                            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                            QT_TRANSLATE_NOOP("popup_window", "YES"),
                            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete all unknown contacts?"),
                            QT_TRANSLATE_NOOP("popup_window", "Close all"),
                            nullptr))
                        {
                            emit Utils::InterConnector::instance().unknownsDeleteThemAll();
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unknowns_closeall);
                        }
                        return;
                    }
                }
                else
                {
                    const auto aimId = Logic::aimIdFromIndex(_current, contactListWidget_->getRegim());
                    if (!aimId.isEmpty())
                    {
                        if (unknownsDelegate_->isInAddContactFrame(pos) && Logic::getUnknownsModel()->unreads(_current.row()) == 0)
                        {
                            Logic::getContactListModel()->addContactToCL(aimId);
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unknowns_add_user);
                            return;
                        }
                        else if (unknownsDelegate_->isInRemoveContactFrame(pos))
                        {
                            Logic::getContactListModel()->removeContactFromCL(aimId);
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unknowns_close);

                            return;
                        }
                    }
                }
            }
        }

        if (QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
        {
            triggerTapAndHold(false);

            if (qobject_cast<const Logic::RecentsModel *>(_current.model())
                || qobject_cast<const Logic::UnknownsModel *>(_current.model()))
            {
                showRecentsPopup_menu(_current);
            }
            else if (qobject_cast<const Logic::SearchModelDLG *>(_current.model()))
            {
                const auto dlg = _current.data().value<Data::DlgState>();
                if (dlg.IsContact_)
                {
                    auto cont = Logic::getContactListModel()->getContactItem(dlg.AimId_);
                    if (cont)
                        showContactsPopupMenu(cont->get_aimid(), cont->is_chat());
                }
            }
            else
            {
                auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();
                if (cont)
                    showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }
        }
    }

    void ContactList::liveChatsItemPressed(const QModelIndex& _current)
    {
        {
            QSignalBlocker sb(liveChatsView_->selectionModel());
            liveChatsView_->selectionModel()->setCurrentIndex(_current, QItemSelectionModel::ClearAndSelect);
        }
        Logic::GetLiveChatsModel()->select(_current);
    }

    void ContactList::statsRecentItemPressed(const QModelIndex& /*_current*/)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_recents);
    }

    void ContactList::statsCLItemPressed(const QModelIndex& _current)
    {
        if (isSearchMode())
        {
            if (qobject_cast<Logic::SearchModelDLG*>(contactListWidget_->getSearchModel()))
            {
                auto dlg = _current.data().value<Data::DlgState>();
                if (!dlg.IsContact_)
                {
                    if (contactListWidget_->getSearchInDialog())
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_dialog_openmessage);
                    else
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_openmessage);
                    return;
                }
            }
        }
        else
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_cl);
        }
    }

	void ContactList::changeTab(CurrentTab _currTab, bool silent)
	{
        if (getPictureOnlyView() && (_currTab != RECENTS && _currTab != SETTINGS))
            return;

        if (_currTab != RECENTS)
        {
            emit Utils::InterConnector::instance().unknownsGoBack();
        }

		if (currentTab_ != _currTab)
		{
            if (currentTab_ == SETTINGS)
            {
                settingsTab_->cleanSelection();
                Utils::InterConnector::instance().restoreSidebar();
            }
            else if (currentTab_ == LIVE_CHATS)
            {
                emit Utils::InterConnector::instance().popPagesToRoot();
            }

            prevTab_ = currentTab_;
            currentTab_ = _currTab;
            updateTabState();
        }

        else if (currentTab_ != LIVE_CHATS)
        {
            if (recentsView_->model() == Logic::getRecentsModel())
                Logic::getRecentsModel()->sendLastRead();
            else
                Logic::getUnknownsModel()->sendLastRead();
        }

        if (!silent)
            emit tabChanged(currentTab_);
    }

    void ContactList::triggerTapAndHold(bool _value)
    {
        tapAndHold_ = _value;
        contactListWidget_->triggerTapAndHold(_value);
    }

    bool ContactList::tapAndHoldModifier() const
    {
        return tapAndHold_;
    }

    void ContactList::dragPositionUpdate(const QPoint& _pos, bool fromScroll)
    {
        int autoscroll_offset = Utils::scale_value(autoscroll_offset_cl);
        auto valid = true;
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = contactListWidget_->getView()->indexAt(_pos);

            if (index.isValid())
            {
                const auto dlg = Logic::getSearchModelDLG()->data(index, Qt::DisplayRole).value<Data::DlgState>();
                const auto role = Logic::getContactListModel()->getYourRole(dlg.AimId_);
                if (role == ql1s("notamember") || role == ql1s("readonly"))
                    valid = false;
            }

            if (valid)
            {
                contactListWidget_->setDragIndexForDelegate(index);
                if (index.isValid())
                    emit Logic::getSearchModelDLG()->dataChanged(index, index);
            }
            else
            {
                contactListWidget_->setDragIndexForDelegate(QModelIndex());
            }

            scrolledView_ = contactListWidget_->getView();
        }
        else if (currentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = recentsView_->indexAt(_pos);

            if (index.isValid())
            {
                Data::DlgState dlg;
                if (recentsView_->model() == Logic::getRecentsModel())
                    dlg = Logic::getRecentsModel()->data(index, Qt::DisplayRole).value<Data::DlgState>();
                else
                    dlg = Logic::getUnknownsModel()->data(index, Qt::DisplayRole).value<Data::DlgState>();

                const auto role = Logic::getContactListModel()->getYourRole(dlg.AimId_);
                if (role == ql1s("notamember") || role == ql1s("readonly"))
                    valid = false;
            }

            if (valid)
            {
                if (recentsView_->itemDelegate() == recentsDelegate_)
                    recentsDelegate_->setDragIndex(index);
                else
                    unknownsDelegate_->setDragIndex(index);
                if (index.isValid())
                {
                    if (recentsView_->model() == Logic::getRecentsModel())
                        emit Logic::getRecentsModel()->dataChanged(index, index);
                    else
                        emit Logic::getUnknownsModel()->dataChanged(index, index);
                }
            }
            else
            {
                recentsDelegate_->setDragIndex(QModelIndex());
            }

            scrolledView_ = recentsView_;
            autoscroll_offset = Utils::scale_value(autoscroll_offset_recents);
        }


        auto rTop = scrolledView_->rect();
        rTop.setBottomLeft(QPoint(rTop.x(), autoscroll_offset));

        auto rBottom = scrolledView_->rect();
        rBottom.setTopLeft(QPoint(rBottom.x(), rBottom.height() - autoscroll_offset));

        if (!_pos.isNull() && (rTop.contains(_pos) || rBottom.contains(_pos)))
        {
            scrollMultipler_ =  (rTop.contains(_pos)) ? 1 : -1;
            scrollTimer_->start();
        }
        else
        {
            scrollTimer_->stop();
        }

        if (!fromScroll)
            lastDragPos_ = _pos;

        scrolledView_->update();
    }

    void ContactList::dropFiles(const QPoint& _pos, const QList<QUrl> _files)
    {
        auto send = [](const QList<QUrl> files, const QString& aimId)
        {
            for (const QUrl& url : files)
            {
                if (url.isLocalFile())
                {
                    QFileInfo info(url.toLocalFile());
                    bool canDrop = !(info.isBundle() || info.isDir());
                    if (info.size() == 0)
                        canDrop = false;

                    if (canDrop)
                    {
                        Ui::GetDispatcher()->uploadSharedFile(aimId, url.toLocalFile());
                        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_recents);
                        auto cd = Utils::InterConnector::instance().getContactDialog();
                        if (cd)
                            cd->onSendMessage(aimId);
                    }
                }
                else if (url.isValid())
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", aimId);
                    const auto text = url.toString().toUtf8();
                    collection.set_value_as_string("message", text.data(), text.size());
                    Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());
                    auto cd = Utils::InterConnector::instance().getContactDialog();
                    if (cd)
                        cd->onSendMessage(aimId);
                }
            }
        };
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = contactListWidget_->getView()->indexAt(_pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::getSearchModelDLG()->data(index, Qt::DisplayRole));
                if (data)
                {
                    const auto role = Logic::getContactListModel()->getYourRole(data->AimId_);
                    if (role != ql1s("notamember") && role != ql1s("readonly"))
                    {
                        if (data->AimId_ != Logic::getContactListModel()->selectedContact())
                            emit Logic::getContactListModel()->select(data->AimId_, -1);

                        send(_files, data->AimId_);
                    }
                }
                emit Logic::getSearchModelDLG()->dataChanged(index, index);
            }
            contactListWidget_->setDragIndexForDelegate(QModelIndex());
        }
        else if (currentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = recentsView_->indexAt(_pos);
                bool isRecents = true;
                Data::DlgState data;
                if (recentsView_->model() == Logic::getRecentsModel())
                {
                    data = qvariant_cast<Data::DlgState>(Logic::getRecentsModel()->data(index, Qt::DisplayRole));
                }
                else
                {
                    data = qvariant_cast<Data::DlgState>(Logic::getUnknownsModel()->data(index, Qt::DisplayRole));
                    isRecents = false;
                }
                if (!data.AimId_.isEmpty())
                {
                    const auto role = Logic::getContactListModel()->getYourRole(data.AimId_);
                    if (role != ql1s("notamember") && role != ql1s("readonly"))
                    {
                        if (data.AimId_ != Logic::getContactListModel()->selectedContact())
                            emit Logic::getContactListModel()->select(data.AimId_, -1);

                        send(_files, data.AimId_);
                        if (isRecents)
                            emit Logic::getRecentsModel()->dataChanged(index, index);
                        else
                            emit Logic::getUnknownsModel()->dataChanged(index, index);
                    }
                }
            }
            recentsDelegate_->setDragIndex(QModelIndex());
            unknownsDelegate_->setDragIndex(QModelIndex());
        }
    }

    void ContactList::recentsClicked()
    {
        if (currentTab_ == RECENTS)
            emit Utils::InterConnector::instance().activateNextUnread();
        else
            switchToRecents();
    }

    void ContactList::switchToRecents()
    {
        changeTab(RECENTS);
    }

    void ContactList::settingsClicked()
    {
        changeTab(SETTINGS);
    }

    void ContactList::updateTabState()
    {
        stackedWidget_->setCurrentIndex(currentTab_);

        emit Utils::InterConnector::instance().makeSearchWidgetVisible(currentTab_ != SETTINGS && currentTab_ != LIVE_CHATS);

        recentOrderChanged();
    }

    void ContactList::guiSettingsChanged()
    {
        currentTab_ = 0;
        updateTabState();
    }

	void ContactList::onSendMessage(const QString& /*_contact*/)
	{
        switchToRecents();
	}

	void ContactList::recentOrderChanged()
	{
        if (currentTab_ == RECENTS)
        {
            QSignalBlocker sb(recentsView_->selectionModel());

            if (recentsView_->model() == Logic::getRecentsModel())
                recentsView_->selectionModel()->setCurrentIndex(Logic::getRecentsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            else
                recentsView_->selectionModel()->setCurrentIndex(Logic::getUnknownsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        }
    }

    void ContactList::touchScrollStateChangedRecents(QScroller::State _state)
    {
        recentsView_->blockSignals(_state != QScroller::Inactive);
        recentsView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        if (recentsView_->model() == Logic::getRecentsModel())
        {
            recentsView_->selectionModel()->setCurrentIndex(Logic::getRecentsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            recentsDelegate_->blockState(_state != QScroller::Inactive);
        }
        else
        {
            recentsView_->selectionModel()->setCurrentIndex(Logic::getUnknownsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            unknownsDelegate_->blockState(_state != QScroller::Inactive);
        }
    }

    void ContactList::touchScrollStateChangedLC(QScroller::State _state)
    {
        liveChatsView_->blockSignals(_state != QScroller::Inactive);
        liveChatsView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        liveChatsView_->selectionModel()->setCurrentIndex(Logic::getContactListModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        liveChatsDelegate_->blockState(_state != QScroller::Inactive);
    }

    void ContactList::changeSelected(const QString& _aimId)
    {
        auto stuff = [](auto view, auto index)
        {
            if (index.isValid())
            {
                if (view->selectionModel())
                    view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
                view->scrollTo(index);
            }
            else
            {
                if (view->selectionModel())
                    view->selectionModel()->clearSelection();
            }
            view->update();
            if (view->selectionModel())
                view->selectionModel()->blockSignals(false);
        };

        QModelIndex index;
        recentsView_->selectionModel()->blockSignals(true);
        index = Logic::getRecentsModel()->contactIndex(_aimId);
        if (!index.isValid())
            index = Logic::getUnknownsModel()->contactIndex(_aimId);

        stuff(recentsView_, index);

        contactListWidget_->getView()->selectionModel()->blockSignals(true);
        index = Logic::getContactListModel()->contactIndex(_aimId);
        stuff(contactListWidget_->getView(), index);
    }

    void ContactList::select(const QString& _aimId)
    {
        select(_aimId, -1, -1, Logic::UpdateChatSelection::No);
    }

    void ContactList::select(const QString& _aimId, qint64 _message_id, qint64 _quote_id, Logic::UpdateChatSelection _mode)
    {
        contactListWidget_->select(_aimId, _message_id, _quote_id, _mode);
        if (currentTab_ == SETTINGS)
            recentsClicked();
    }

    void ContactList::showContactsPopupMenu(const QString& aimId, bool _is_chat)
    {
        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactList::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        if (!_is_chat)
        {
#ifndef STRIP_VOIP
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/call_100"))), QT_TRANSLATE_NOOP("context_menu","Call"), Logic::makeData(qsl("contacts/call"), aimId));
#endif //STRIP_VOIP
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/profile_100"))), QT_TRANSLATE_NOOP("context_menu", "Profile"), Logic::makeData(qsl("contacts/Profile"), aimId));
        }

        popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/ignore_100"))), QT_TRANSLATE_NOOP("context_menu", "Ignore"), Logic::makeData(qsl("contacts/ignore"), aimId));
        if (!_is_chat)
        {
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/spam_100"))), QT_TRANSLATE_NOOP("context_menu", "Report spam"), Logic::makeData(qsl("contacts/spam"), aimId));
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/delete_100"))), QT_TRANSLATE_NOOP("context_menu", "Delete"), Logic::makeData(qsl("contacts/remove"), aimId));
        }
        else
        {
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/context_menu/delete_100")), QT_TRANSLATE_NOOP("context_menu", "Leave and delete"), Logic::makeData(qsl("contacts/remove"), aimId));
        }

        popupMenu_->popup(QCursor::pos());
    }

    void ContactList::showRecentsPopup_menu(const QModelIndex& _current)
    {
        if (recentsView_->model() == Logic::getUnknownsModel())
        {
            return;
        }

        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            Testing::setAccessibleName(popupMenu_, qsl("popup_menu_"));
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactList::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
        QString aimId = dlg.AimId_;

        if (dlg.UnreadCount_ != 0)
        {
            auto icon = QIcon(Utils::parse_image_name(qsl(":/context_menu/read_100")));
            popupMenu_->addActionWithIcon(icon, QT_TRANSLATE_NOOP("context_menu", "Mark as read"), Logic::makeData(qsl("recents/mark_read"), aimId));
        }

        if (Logic::getRecentsModel()->isFavorite(aimId))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/favorite_off_100"))), QT_TRANSLATE_NOOP("context_menu", "Remove from favorites"), Logic::makeData(qsl("recents/unfavorite"), aimId));
        else
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/favorite_100"))), QT_TRANSLATE_NOOP("context_menu", "Add to favorites"), Logic::makeData(qsl("recents/favorite"), aimId));

        if (Logic::getContactListModel()->isMuted(dlg.AimId_))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/unmute_100"))), QT_TRANSLATE_NOOP("context_menu", "Turn on notifications"), Logic::makeData(qsl("recents/unmute"), aimId));
        else
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/mute_100"))), QT_TRANSLATE_NOOP("context_menu", "Turn off notifications"), Logic::makeData(qsl("recents/mute"), aimId));

        auto ignore_icon = QIcon(Utils::parse_image_name(qsl(":/context_menu/ignore_100")));
        popupMenu_->addActionWithIcon(ignore_icon, QT_TRANSLATE_NOOP("context_menu", "Ignore"), Logic::makeData(qsl("recents/ignore"), aimId));

        if (!Logic::getRecentsModel()->isFavorite(aimId))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/close_100"))), QT_TRANSLATE_NOOP("context_menu", "Close"), Logic::makeData(qsl("recents/close"), aimId));

        if (Logic::getRecentsModel()->totalUnreads() != 0)
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/readall_100"))), QT_TRANSLATE_NOOP("context_menu", "Read all"), Logic::makeData(qsl("recents/read_all")));

        popupMenu_->popup(QCursor::pos());
    }

    void ContactList::showPopupMenu(QAction* _action)
    {
        Logic::showContactListPopup(_action);
    }

    void ContactList::autoScroll()
    {
        if (scrolledView_)
        {
            scrolledView_->verticalScrollBar()->setValue(scrolledView_->verticalScrollBar()->value() - (Utils::scale_value(autoscroll_speed_pixels) * scrollMultipler_));
            dragPositionUpdate(lastDragPos_, true);
        }
    }


    void ContactList::showNoRecentsYet(QWidget *_parent, QWidget *_list, QLayout *_layout, std::function<void()> _action)
    {
        if (noRecentsYetShown_)
            return;
        if (!noRecentsYet_)
        {
            noRecentsYetShown_ = true;
            _list->hide();
            noRecentsYet_ = new QWidget(_parent);
            noRecentsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(noRecentsYet_);
            {
                auto mainLayout = Utils::emptyVLayout(noRecentsYet_);
                mainLayout->setAlignment(Qt::AlignCenter);
                {
                    auto noRecentsWidget = new QWidget(noRecentsYet_);
                    auto noRecentsLayout = new QVBoxLayout(noRecentsWidget);
                    noRecentsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noRecentsLayout->setAlignment(Qt::AlignCenter);
                    noRecentsLayout->setContentsMargins(0, 0, 0, 0);
                    noRecentsLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noRecentsPlaceholder = new QWidget(noRecentsWidget);
                        noRecentsPlaceholder->setObjectName(qsl("noRecents"));
                        noRecentsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noRecentsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        noRecentsLayout->addWidget(noRecentsPlaceholder);
                    }
                    {
                        auto noRecentsLabel = new LabelEx(noRecentsWidget);
                        noRecentsLabel->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY));
                        noRecentsLabel->setFont(Fonts::appFontScaled(15));
                        noRecentsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noRecentsLabel->setAlignment(Qt::AlignCenter);
                        noRecentsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "You have no opened chats yet"));
                        noRecentsLayout->addWidget(noRecentsLabel);
                    }
                    mainLayout->addWidget(noRecentsWidget);
                }
            }
        }
    }

    void ContactList::hideNoRecentsYet()
    {
        if (noRecentsYet_)
        {
            noRecentsYet_->hide();
            noRecentsYet_->deleteLater();
            noRecentsYet_ = nullptr;
            recentsView_->show();
            noRecentsYetShown_ = false;
        }
    }

    void ContactList::showNoRecentsYet()
    {
        if (Logic::getRecentsModel()->rowCount() != 0 || Logic::getUnknownsModel()->itemsCount() != 0)
            return;

        showNoRecentsYet(recentsPage_, recentsView_, recentsLayout_, [this]()
                         {
                             emit Utils::InterConnector::instance().contacts();
                             Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_write_msg);
                         });
    }

    void ContactList::typingStatus(Logic::TypingFires _typing, bool _isTyping)
    {
        if (recentsView_->model() == Logic::getRecentsModel() && Logic::getRecentsModel()->contactIndex(_typing.aimId_).isValid())
        {
            if (_isTyping)
                recentsDelegate_->addTyping(_typing);
            else
                recentsDelegate_->removeTyping(_typing);

            auto modeIndex = Logic::getRecentsModel()->contactIndex(_typing.aimId_);
            Logic::getRecentsModel()->dataChanged(modeIndex, modeIndex);
        }
    }

    void ContactList::messagesReceived(const QString& _aimId, const QVector<QString>& _chatters)
    {
        auto contactItem = Logic::getContactListModel()->getContactItem(_aimId);
        if (!contactItem)
            return;

        if (recentsView_->model() == Logic::getRecentsModel())
        {
            if (contactItem->is_chat())
                for (const auto& chatter: _chatters)
                    recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, chatter, QString()));
            else
                recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, QString(), QString()));

            auto modelIndex = Logic::getRecentsModel()->contactIndex(_aimId);
            Logic::getRecentsModel()->dataChanged(modelIndex, modelIndex);
        }
        else
        {
            auto modelIndex = Logic::getUnknownsModel()->contactIndex(_aimId);
            Logic::getUnknownsModel()->dataChanged(modelIndex, modelIndex);
        }
    }

    void ContactList::selectSettingsVoipTab()
    {
        settingsTab_->settingsVoiceVideoClicked();
    }
    bool ContactList::getPictureOnlyView() const
    {
        return pictureOnlyView_;
    }

    void ContactList::setPictureOnlyView(bool _isPictureOnly)
    {
        if (pictureOnlyView_ == _isPictureOnly)
            return;

        pictureOnlyView_ = _isPictureOnly;
        recentsDelegate_->setPictOnlyView(pictureOnlyView_);
        unknownsDelegate_->setPictOnlyView(pictureOnlyView_);
        recentsView_->setFlow(QListView::TopToBottom);
        settingsTab_->setCompactMode(_isPictureOnly);

        Logic::getUnknownsModel()->setDeleteAllVisible(!pictureOnlyView_);

        if (pictureOnlyView_)
            setSearchMode(false);

        recentOrderChanged();
    }

    void ContactList::setItemWidth(int _newWidth)
    {
        recentsDelegate_->setFixedWidth(_newWidth);
        unknownsDelegate_->setFixedWidth(_newWidth);
        contactListWidget_->setWidthForDelegate(_newWidth);
    }

    void ContactList::openThemeSettings()
    {
        prevTab_ = currentTab_;
        currentTab_ = SETTINGS;
        updateTabState();
        settingsTab_->settingsThemesClicked();

        if (recentsView_->model() == Logic::getRecentsModel())
            Logic::getRecentsModel()->sendLastRead();
        else
            Logic::getUnknownsModel()->sendLastRead();

        emit tabChanged(currentTab_);
    }

    QString ContactList::getSelectedAimid() const
    {
        return contactListWidget_->getSelectedAimid();
    }

    void ContactList::setSearchInDialog(bool _isSearchInDialog)
    {
        contactListWidget_->setSearchInDialog(_isSearchInDialog);
    }

    bool ContactList::getSearchInDialog() const
    {
        return contactListWidget_->getSearchInDialog();
    }

    void ContactList::dialogClosed(const QString& _aimid)
    {
        auto searhModel = qobject_cast<Logic::SearchModelDLG*>(contactListWidget_->getSearchModel());
        auto chatAimid = searhModel ? searhModel->getDialogAimid() : QString();

        if (chatAimid == _aimid)
        {
            emit contactListWidget_->searchEnd();
        }
    }

    void ContactList::myProfileBack()
    {
        changeTab((CurrentTab)prevTab_);
    }

    void ContactList::recentsScrollActionTriggered(int value)
    {
        recentsView_->verticalScrollBar()->setSingleStep(Utils::scale_value(RECENTS_HEIGHT));
    }
}
