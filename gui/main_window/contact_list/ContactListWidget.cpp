#include "stdafx.h"
#include "ContactListWidget.h"
#include "ContactListModel.h"
#include "ContactListItemDelegate.h"
#include "ContactListItemRenderer.h"
#include "SearchModelDLG.h"
#include "RecentsModel.h"
#include "RecentsItemRenderer.h"
#include "RecentItemDelegate.h"
#include "SearchMembersModel.h"
#include "ChatMembersModel.h"
#include "ContactListUtils.h"
#include "SearchWidget.h"

#include "../../controls/LabelEx.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/InterConnector.h"

namespace
{
    QItemDelegate* delegateFromRegin(QObject* parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _membersModel)
    {
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            auto d = new Logic::RecentItemDelegate(parent);
            d->setRegim(Logic::MembersWidgetRegim::HISTORY_SEARCH);
            return d;
        }

        return new Logic::ContactListItemDelegate(parent, _regim, _membersModel);
    }

    QAbstractItemModel* modelForRegim(Logic::MembersWidgetRegim _regim, Logic::AbstractSearchModel* _searchModel, Logic::ChatMembersModel* _membersModel, bool _is_seacrh_in_dialog)
    {
        if (Logic::is_members_regim(_regim))
            return _membersModel;

        if (_is_seacrh_in_dialog)
            return Logic::getSearchModelDLG();

        if (_regim != Logic::MembersWidgetRegim::SELECT_MEMBERS
            && _regim != Logic::MembersWidgetRegim::SHARE_LINK
            && _regim != Logic::MembersWidgetRegim::SHARE_TEXT
            && _regim != Logic::MembersWidgetRegim::VIDEO_CONFERENCE)
            return Logic::getContactListModel();

        return _searchModel;
    }

    Logic::AbstractSearchModel* searchModelForRegim(QObject* parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _membersModel)
    {
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
            return Logic::getSearchModelDLG();

        if (!Logic::is_members_regim(_regim))
            return Logic::getCustomSearchModelDLG(false, _regim != Logic::MembersWidgetRegim::SHARE_LINK && _regim != Logic::MembersWidgetRegim::SHARE_TEXT && _regim != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);

        auto m = Logic::getSearchMemberModel(parent);
        m->setChatMembersModel(_membersModel);
        return m;
    }
}

namespace Ui
{
    SearchInAllChatsButton::SearchInAllChatsButton(QWidget* _parent)
        : QWidget(_parent)
        , hover_(false)
        , select_(false)
    {
        setFixedHeight(::ContactList::GetContactListParams().searchInAllChatsHeight());
    }

    void SearchInAllChatsButton::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        static QPen linePen(CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER));
        linePen.setWidth(0);
        painter.setPen(linePen);
        painter.drawLine(0, 0, width(), 0);

        ::ContactList::ViewParams viewParams;
        viewParams.fixedWidth_ = width();
        ::ContactList::RenderServiceContact(painter, hover_, select_, QT_TRANSLATE_NOOP("contact_list", "SEARCH IN ALL CHATS"), Data::ContactType::SEARCH_IN_ALL_CHATS, 0, viewParams);
    }

    void SearchInAllChatsButton::enterEvent(QEvent* _e)
    {
        hover_ = true;
        update();
        return QWidget::enterEvent(_e);
    }

    void SearchInAllChatsButton::leaveEvent(QEvent* _e)
    {
        hover_ = false;
        update();
        return QWidget::leaveEvent(_e);
    }

    void SearchInAllChatsButton::mousePressEvent(QMouseEvent* _e)
    {
        select_ = true;
        update();
        return QWidget::mousePressEvent(_e);
    }

    void SearchInAllChatsButton::mouseReleaseEvent(QMouseEvent* _e)
    {
        select_ = false;
        update();
        emit clicked();
        return QWidget::mouseReleaseEvent(_e);
    }


    SearchInChatLabel::SearchInChatLabel(QWidget* _parent, Logic::AbstractSearchModel* _searchModel)
        : QWidget(_parent)
        , model_(_searchModel)
    {
    }

    void SearchInChatLabel::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        ::ContactList::ViewParams viewParams;

        const auto searchModel = qobject_cast<Logic::SearchModelDLG*>(model_);
        const auto chatAimid = searchModel ? searchModel->getDialogAimid() : QString();
        const QString text = QT_TRANSLATE_NOOP("contact_list", "Search in ") % QChar(0x00AB) % Logic::getContactListModel()->getDisplayName(chatAimid).toUpper() % QChar(0x00BB);
        ::ContactList::RenderServiceItem(painter, text, false /* renderState */, false /* drawLine */, viewParams);
    }

    EmptyIgnoreListLabel::EmptyIgnoreListLabel(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    void EmptyIgnoreListLabel::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        ::ContactList::ViewParams viewParams;
        ::ContactList::RenderServiceContact(
            painter,
            false /* Hover_ */,
            false /* Select_ */,
            QT_TRANSLATE_NOOP("sidebar", "You have no ignored contacts"),
            Data::ContactType::EMPTY_IGNORE_LIST,
            0,
            viewParams
        );
}

    GlobalSearchHeader::GlobalSearchHeader(QWidget* _parent)
        : QWidget(_parent)
        , backButton_(Utils::parse_image_name(qsl(":/controls/arrow_left_100")))
        , backWidth_(Utils::scale_value(72))
        , backHovered_(false)
    {
        Utils::check_pixel_ratio(backButton_);
        setMouseTracking(true);
    }

    void GlobalSearchHeader::setCaption(const QString& _caption)
    {
        caption_ = _caption;
        update();
    }

    void GlobalSearchHeader::paintEvent(QPaintEvent *)
    {
        QPainter painter(this);
        static const auto ratio = Utils::scale_bitmap(1);
        static const auto bx = ::ContactList::GetContactListParams().itemHorPadding();
        static const auto by = (height() - backButton_.height() / ratio) / 2;
        painter.drawPixmap(bx, by, backButton_);

        static const auto textColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        static const auto textFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
        painter.setPen(textColor);
        painter.setFont(textFont);
        QFontMetrics metrics(textFont);
        Utils::drawText(painter, rect().center(), Qt::AlignVCenter | Qt::AlignHCenter, caption_);
    }

    void GlobalSearchHeader::showEvent(QShowEvent*)
    {
        updateBackHoverState(false);
    }

    void GlobalSearchHeader::hideEvent(QHideEvent*)
    {
        updateBackHoverState(false);
    }

    void GlobalSearchHeader::mouseMoveEvent(QMouseEvent* _event)
    {
        updateBackHoverState(_event->pos().x() <= backWidth_);
    }

    void GlobalSearchHeader::leaveEvent(QEvent *)
    {
        updateBackHoverState(false);
    }

    void GlobalSearchHeader::mouseReleaseEvent(QMouseEvent *)
    {
        if (backHovered_)
            emit Utils::InterConnector::instance().globalHeaderBackClicked();
    }

    void GlobalSearchHeader::updateBackHoverState(const bool _hovered)
    {
        if (backHovered_ == _hovered)
            return;

        backHovered_ = _hovered;
        setCursor(backHovered_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }

    ContactListWidget::ContactListWidget(QWidget* _parent, const Logic::MembersWidgetRegim& _regim, Logic::ChatMembersModel* _chatMembersModel, Logic::AbstractSearchModel* _searchModel /*= nullptr*/)
        : QWidget(_parent)
        , clDelegate_(nullptr)
        , noContactsYet_(nullptr)
        , noSearchResults_(nullptr)
        , searchSpinner_(nullptr)
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , searchModel_(_searchModel)
        , popupMenu_(nullptr)
        , searchSpinnerShown_(false)
        , isSearchInDialog_(false)
        , noSearchResultsShown_(false)
        , noContactsYetShown_(false)
        , initial_(false)
        , tapAndHold_(false)
        , scrollPosition_(0)
    {
        initSearchModel(_searchModel);

        layout_ = Utils::emptyVLayout(this);
        searchInChatLabel_ = new SearchInChatLabel(this, searchModel_);
        searchInChatLabel_->setContentsMargins(0, 0, 0, 0);
        searchInChatLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        searchInChatLabel_->setFixedHeight(::ContactList::GetContactListParams().serviceItemHeight());
        layout_->addWidget(searchInChatLabel_);
        searchInChatLabel_->setVisible(false);

        globalSearchHeader_ = new GlobalSearchHeader();
        globalSearchHeader_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        globalSearchHeader_->setFixedHeight(::ContactList::GetContactListParams().globalSearchHeaderHeight());
        layout_->addWidget(globalSearchHeader_);
        globalSearchHeader_->setVisible(false);

        view_ = CreateFocusableViewAndSetTrScrollBar(this);
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        view_->setFrameShape(QFrame::NoFrame);
        view_->setSpacing(0);
        view_->setModelColumn(0);
        view_->setUniformItemSizes(false);
        view_->setBatchSize(50);
        view_->setStyleSheet(qsl("background: transparent;"));
        view_->setCursor(Qt::PointingHandCursor);
        view_->setMouseTracking(true);
        view_->setAcceptDrops(true);
        view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        view_->setAutoScroll(false);
        view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        layout_->addWidget(view_);

        emptyIgnoreListLabel_ = new EmptyIgnoreListLabel(this);
        emptyIgnoreListLabel_->setContentsMargins(0, 0, 0, 0);
        emptyIgnoreListLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emptyIgnoreListLabel_->setFixedHeight(::ContactList::GetContactListParams().itemHeight());
        layout_->addWidget(emptyIgnoreListLabel_);
        emptyIgnoreListLabel_->setVisible(false);

        searchInAllButton_ = new SearchInAllChatsButton(this);
        Testing::setAccessibleName(searchInAllButton_, qsl("SearchInAllChatsButton"));
        searchInAllButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        layout_->addWidget(searchInAllButton_);
        searchInAllButton_->setVisible(false);

        switchToInitial(true);

        view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        Utils::grabTouchWidget(view_->viewport(), true);

        connect(searchInAllButton_, &SearchInAllChatsButton::clicked, this, &ContactListWidget::onDisableSearchInDialogButton, Qt::QueuedConnection);
        connect(view_, &Ui::FocusableListView::activated, this, &ContactListWidget::searchClicked, Qt::QueuedConnection);
        connect(view_, &Ui::FocusableListView::clicked, this, static_cast<void(ContactListWidget::*)(const QModelIndex&)>(&ContactListWidget::itemClicked), Qt::QueuedConnection);
        connect(view_, &Ui::FocusableListView::pressed, this, &ContactListWidget::itemPressed, Qt::QueuedConnection);
        connect(view_->verticalScrollBar(), &QScrollBar::valueChanged, Logic::getContactListModel(), &Logic::ContactListModel::scrolled, Qt::QueuedConnection);
        connect(view_->verticalScrollBar(), &QAbstractSlider::rangeChanged, this, &ContactListWidget::scrollRangeChanged, Qt::QueuedConnection);
        connect(QScroller::scroller(view_->viewport()), &QScroller::stateChanged, this, &ContactListWidget::touchScrollStateChanged, Qt::QueuedConnection);

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showNoSearchResults, this, &ContactListWidget::showNoSearchResults);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideNoSearchResults, this, &ContactListWidget::hideNoSearchResults);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showSearchSpinner, this, &ContactListWidget::showSearchSpinner);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideSearchSpinner, this, &ContactListWidget::hideSearchSpinner);
        }

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::globalSearchHeaderNeeded, this, &ContactListWidget::setGlobalSearchHeader);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideGlobalSearchHeader,   this, &ContactListWidget::hideGlobalSearchHeader);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::globalHeaderBackClicked,  this, &ContactListWidget::onGlobalHeaderBackClicked, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showNoContactsYet, this, &ContactListWidget::showNoContactsYet);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideNoContactsYet, this, &ContactListWidget::hideNoContactsYet);
    }

    ContactListWidget::~ContactListWidget()
    {

    }

    void ContactListWidget::connectSearchWidget(SearchWidget* _widget)
    {
        connect(_widget, &SearchWidget::enterPressed,   this, &ContactListWidget::searchResult, Qt::QueuedConnection);
        connect(_widget, &SearchWidget::upPressed,      this, &ContactListWidget::searchUpPressed, Qt::QueuedConnection);
        connect(_widget, &SearchWidget::downPressed,    this, &ContactListWidget::searchDownPressed, Qt::QueuedConnection);
        connect(_widget, &SearchWidget::search, searchModel_, &Logic::AbstractSearchModel::searchPatternChanged, Qt::QueuedConnection);
        connect(_widget, &SearchWidget::search,         this, &ContactListWidget::searchPatternChanged, Qt::QueuedConnection);

        connect(this,    &ContactListWidget::searchEnd, _widget, &SearchWidget::searchCompleted, Qt::QueuedConnection);
    }

    void ContactListWidget::installEventFilterToView(QObject* _filter)
    {
        view_->viewport()->installEventFilter(_filter);
    }

    void ContactListWidget::setIndexWidget(int index, QWidget* widget)
    {
        view_->setIndexWidget(searchModel_->index(index, 0), widget);
    }

    void ContactListWidget::setClDelegate(Logic::AbstractItemDelegateWithRegim* _delegate)
    {
        clDelegate_ = _delegate;
        view_->setItemDelegate(_delegate);
    }

    void ContactListWidget::setWidthForDelegate(int _width)
    {
        clDelegate_->setFixedWidth(_width);
    }

    void ContactListWidget::setDragIndexForDelegate(const QModelIndex& _index)
    {
        clDelegate_->setDragIndex(_index);
    }

    void ContactListWidget::setEmptyIgnoreLabelVisible(bool _isVisible)
    {
        emptyIgnoreListLabel_->setVisible(_isVisible);
        view_->setVisible(!_isVisible);
    }

    void ContactListWidget::setSearchInDialog(bool _isSearchInDialog, bool _switchModel)
    {
        if (isSearchInDialog_ == _isSearchInDialog)
            return;

        isSearchInDialog_ = _isSearchInDialog;
        searchInAllButton_->setVisible(_isSearchInDialog);
        searchInChatLabel_->setVisible(_isSearchInDialog);

        if (_switchModel && regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            view_->setModel(modelForRegim(regim_, searchModel_, chatMembersModel_, isSearchInDialog_));
    }

    bool ContactListWidget::getSearchInDialog() const
    {
        return isSearchInDialog_;
    }

    bool ContactListWidget::isSearchMode() const
    {
        return !initial_;
    }

    QString ContactListWidget::getSelectedAimid() const
    {
        const QModelIndexList indexes = view_->selectionModel()->selectedIndexes();
        if (indexes.isEmpty())
            return QString();
        return aimIdFromIndex(indexes.first(), regim_);
    }

    Logic::MembersWidgetRegim ContactListWidget::getRegim() const
    {
        return regim_;
    }

    Logic::AbstractSearchModel* ContactListWidget::getSearchModel() const
    {
        return searchModel_;
    }

    FocusableListView* ContactListWidget::getView() const
    {
        return view_;
    }

    void ContactListWidget::triggerTapAndHold(bool _value)
    {
        tapAndHold_ = _value;
    }

    bool ContactListWidget::tapAndHoldModifier() const
    {
        return tapAndHold_;
    }

    void ContactListWidget::searchResult()
    {
        itemClicked(view_->selectionModel()->currentIndex());
    }

    void ContactListWidget::searchUpPressed()
    {
        searchUpOrDownPressed(true);
    }

    void ContactListWidget::searchDownPressed()
    {
        searchUpOrDownPressed(false);
    }

    void ContactListWidget::selectionChanged(const QModelIndex & _current)
    {
        QString aimid = aimIdFromIndex(_current, regim_);

        if (aimid.isEmpty())
            return;

        qint64 mess_id = -1;
        const auto searchModel = qobject_cast<const Logic::SearchModelDLG*>(_current.model());
        if (searchModel && !searchModel->isServiceItem(_current.row()))
        {
            auto dlg = _current.data().value<Data::DlgState>();
            if (!dlg.IsContact_)
                mess_id = dlg.SearchedMsgId_;
        }

        select(aimid, mess_id, mess_id, Logic::UpdateChatSelection::No);

        if (mess_id == -1)
            emit itemClicked(aimid);

        const auto index = view_->selectionModel()->currentIndex();
        if (index.isValid())
        {
            view_->selectionModel()->clear();
            searchModel_->dataChanged(index, index);
        }
    }

    void ContactListWidget::select(const QString& _aimId)
    {
        select(_aimId, -1, -1, Logic::UpdateChatSelection::No);
    }

    void ContactListWidget::select(const QString& _aimId, const qint64 _message_id, const qint64 _quote_id, Logic::UpdateChatSelection _mode)
    {
        const auto isSelectMembers = isSelectMembersRegim();

        if (!isSelectMembers && regim_ != Logic::MembersWidgetRegim::IGNORE_LIST)
        {
            Logic::getContactListModel()->setCurrent(_aimId, _message_id, regim_ == Logic::CONTACT_LIST_POPUP);
        }

        if (_message_id == -1 || _mode == Logic::UpdateChatSelection::Yes)
        {
            emit changeSelected(_aimId);
        }

        emit itemSelected(_aimId, _message_id, _quote_id);

        if (!isSelectMembers && isSearchMode() && _message_id == -1 && _quote_id == -1)
        {
            emit searchEnd();

            if (!Logic::getContactListModel()->isNotAuth(_aimId))
            {
                emit Utils::InterConnector::instance().unknownsGoBack();
            }
        }
    }

    void ContactListWidget::itemClicked(const QModelIndex& _current)
    {
        const auto sDlgModel = qobject_cast<const Logic::SearchModelDLG*>(_current.model());
        const auto changeSelection =
            !(QApplication::mouseButtons() & Qt::RightButton)
            && !(sDlgModel && sDlgModel->isServiceItem(_current.row()));

        if (sDlgModel)
        {
            if (changeSelection)
            {
                const auto dlg = _current.data().value<Data::DlgState>();
                if (dlg.isFromGlobalSearch_)
                {
                    Data::ChatInfo info;
                    if (sDlgModel->getChatInfo(_current, info))
                    {
                        emit Utils::InterConnector::instance().liveChatSelected();
                        emit Utils::InterConnector::instance().showLiveChat(std::make_shared<Data::ChatInfo>(info));
                    }
                    else
                    {
                        Logic::getContactListModel()->setCurrent(dlg.AimId_, -1, false);
                        emit itemSelected(dlg.AimId_, -1, -1);
                    }
                }
                else
                {
                    emit Utils::InterConnector::instance().clearDialogHistory();
                    selectionChanged(_current);
                }
            }
            else
            {
                auto model = const_cast<Logic::SearchModelDLG*>(sDlgModel);
                const auto tmpVal = view_->verticalScrollBar()->value();
                if (model->serviceItemClick(_current))
                {
                    scrollPosition_ = tmpVal;
                    view_->scrollToTop();
                }
            }
        }
        else if (changeSelection)
        {
            emit Utils::InterConnector::instance().clearDialogHistory();
            selectionChanged(_current);
        }
    }

    void ContactListWidget::itemPressed(const QModelIndex& _current)
    {
        const auto sdlgModel = qobject_cast<const Logic::SearchModelDLG*>(_current.model());
        if (sdlgModel && sdlgModel->getSearchInHistory() && (QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton))
        {
            const auto rect = view_->visualRect(_current);
            const auto cursorPos = view_->mapFromGlobal(QCursor::pos());
            const auto aimId = aimIdFromIndex(_current, regim_);

            if (rect.contains(cursorPos) && !sdlgModel->isServiceItem(_current.row()) && !aimId.isEmpty())
            {
                if (rect.width() - cursorPos.x() < Utils::scale_value(52))
                {
                    Data::ChatInfo info;
                    if (sdlgModel->getChatInfo(_current, info) && !info.Stamp_.isEmpty())
                    {
                        Logic::getContactListModel()->joinLiveChat(info.Stamp_, true);

                        QMetaObject::Connection* const connection = new QMetaObject::Connection;
                        *connection = connect(Logic::getContactListModel(), &Logic::ContactListModel::liveChatJoined, [info, connection](QString _aimId)
                        {
                            if (_aimId == info.AimId_)
                            {
                                emit Utils::InterConnector::instance().clearDialogHistory();
                                Logic::getContactListModel()->setCurrent(info.AimId_, -1, true);
                            }

                            disconnect(*connection);
                            delete connection;
                        });
                    }
                    else
                    {
                        Logic::getContactListModel()->addContactToCL(aimId);
                        select(aimId, -1, -1, Logic::UpdateChatSelection::No);
                    }
                    return;
                }
            }
        }

        if (QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
        {
            triggerTapAndHold(false);

            if (qobject_cast<const Logic::SearchModelDLG *>(_current.model()))
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
                const auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();
                if (cont)
                    showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }
        }
    }

    void ContactListWidget::onSearchResults()
    {
        const auto pattern = searchModel_->getCurrentPattern();
        if (isSelectMembersRegim() || pattern == currentPattern_)
            return;

        currentPattern_ = pattern;
        view_->clearSelection();

        if (searchModel_->rowCount())
        {
            QModelIndex i = searchModel_->index(0);
            {
                QSignalBlocker sb(view_->selectionModel());
                view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
            }
            searchModel_->dataChanged(i, i);
            view_->scrollTo(i);
        }
    }

    void ContactListWidget::searchResults(const QModelIndex & _current, const QModelIndex &)
    {
        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
            return;
        if (!_current.isValid())
        {
            emit searchEnd();
            return;
        }

        if (qobject_cast<const Logic::SearchModelDLG*>(_current.model()))
        {
            selectionChanged(_current);
            emit searchEnd();

            auto aimid = Logic::aimIdFromIndex(_current, regim_);
            if (!aimid.isEmpty())
            {
                qint64 mess_id = -1;
                const auto searchModel = qobject_cast<const Logic::SearchModelDLG*>(_current.model());
                if (searchModel)
                {
                    auto dlg = _current.data().value<Data::DlgState>();
                    if (!dlg.IsContact_)
                        mess_id = dlg.SearchedMsgId_;
                }
                emit itemSelected(aimid, mess_id, mess_id);
            }
            return;
        }

        Data::Contact* cont = _current.data().value<Data::Contact*>();
        if (!cont)
        {
            view_->clearSelection();
            view_->selectionModel()->clearCurrentIndex();
            return;
        }

        if (cont->GetType() != Data::GROUP)
            select(cont->AimId_);

        view_->clearSelection();
        view_->selectionModel()->clearCurrentIndex();

        emit searchEnd();
    }

    void ContactListWidget::searchClicked(const QModelIndex& _current)
    {
        searchResults(_current, QModelIndex());
    }

    void ContactListWidget::showPopupMenu(QAction* _action)
    {
        Logic::showContactListPopup(_action);
    }

    void ContactListWidget::showContactsPopupMenu(const QString& aimId, bool _is_chat)
    {
        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactListWidget::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        if (Logic::is_members_regim(regim_) || Logic::is_select_members_regim(regim_))
            return;

        if (!_is_chat)
        {
#ifndef STRIP_VOIP
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/call_100"))), QT_TRANSLATE_NOOP("context_menu", "Call"), Logic::makeData(qsl("contacts/call"), aimId));
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
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/delete_100"))), QT_TRANSLATE_NOOP("context_menu", "Leave and delete"), Logic::makeData(qsl("contacts/remove"), aimId));
        }

        popupMenu_->popup(QCursor::pos());
    }

    void ContactListWidget::searchPatternChanged(const QString& p)
    {
        switchToInitial(p.isEmpty());
    }

    void ContactListWidget::onDisableSearchInDialogButton()
    {
        emit Utils::InterConnector::instance().disableSearchInDialog();
        emit Utils::InterConnector::instance().repeatSearch();

        setSearchInDialog(false, false);
    }

    void ContactListWidget::touchScrollStateChanged(QScroller::State _state)
    {
        view_->blockSignals(_state != QScroller::Inactive);
        view_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        if (!isSearchMode())
            view_->selectionModel()->setCurrentIndex(Logic::getContactListModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        clDelegate_->blockState(_state != QScroller::Inactive);
    }

    void ContactListWidget::showNoContactsYet()
    {
        if (noContactsYetShown_)
            return;
        if (!noContactsYet_)
        {
            noContactsYetShown_ = true;
            view_->setMaximumHeight(Utils::scale_value(50));
            noContactsYet_ = new QWidget(this);
            noContactsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            layout_->addWidget(noContactsYet_);
            {
                auto mainLayout = Utils::emptyVLayout(noContactsYet_);
                mainLayout->setAlignment(Qt::AlignCenter);
                {
                    auto noContactsWidget = new QWidget(noContactsYet_);
                    auto noContactsLayout = new QVBoxLayout(noContactsWidget);
                    noContactsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noContactsLayout->setAlignment(Qt::AlignCenter);
                    noContactsLayout->setContentsMargins(0, 0, 0, 0);
                    noContactsLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noContactsPlaceholder = new QWidget(noContactsWidget);
                        noContactsPlaceholder->setObjectName(qsl("noContacts"));
                        noContactsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noContactsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        noContactsLayout->addWidget(noContactsPlaceholder);
                    }
                    {
                        auto noContactLabel = new LabelEx(noContactsWidget);
                        noContactLabel->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY));
                        noContactLabel->setFont(Fonts::appFontScaled(15));
                        noContactLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noContactLabel->setAlignment(Qt::AlignCenter);
                        noContactLabel->setWordWrap(true);
                        noContactLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Looks like you have no contacts yet"));
                        noContactsLayout->addWidget(noContactLabel);
                    }
                    mainLayout->addWidget(noContactsWidget);
                }
            }
            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_FindFriend);
        }
    }

    void ContactListWidget::hideNoContactsYet()
    {
        if (noContactsYet_)
        {
            noContactsYet_->setHidden(true);
            layout_->removeWidget(noContactsYet_);
            noContactsYet_->deleteLater();
            noContactsYet_ = nullptr;
            view_->setMaximumHeight(QWIDGETSIZE_MAX);

            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_HideFindFriend);

            noContactsYetShown_ = false;
        }
    }

    void ContactListWidget::showNoSearchResults()
    {
        if (Logic::getSearchModelDLG()->count() == 0)
        {
            view_->hide();

            if (noSearchResultsShown_)
                return;

            noSearchResultsShown_ = true;
            if (!noSearchResults_)
            {
                view_->hide();
                noSearchResults_ = new QWidget(this);
                noSearchResults_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                {
                    auto mainLayout = Utils::emptyVLayout(noSearchResults_);
                    {
                        auto noSearchResultsWidget = new QWidget(noSearchResults_);
                        auto noSearchLayout = new QVBoxLayout(noSearchResultsWidget);
                        noSearchResultsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noSearchLayout->setAlignment(Qt::AlignCenter);
                        noSearchLayout->setContentsMargins(0, Utils::scale_value(60), 0, 0);
                        noSearchLayout->setSpacing(Utils::scale_value(36));
                        {
                            auto noSearchResultsPlaceholder = new QWidget(noSearchResultsWidget);
                            noSearchResultsPlaceholder->setObjectName(qsl("noSearchResults"));
                            noSearchResultsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                            noSearchResultsPlaceholder->setFixedHeight(Utils::scale_value(100));
                            noSearchLayout->addWidget(noSearchResultsPlaceholder);
                        }
                        {
                            auto noSearchResultsLabel = new LabelEx(noSearchResultsWidget);
                            noSearchResultsLabel->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
                            noSearchResultsLabel->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Bold));
                            noSearchResultsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                            noSearchResultsLabel->setAlignment(Qt::AlignCenter);
                            noSearchResultsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "No messages found"));
                            noSearchLayout->addWidget(noSearchResultsLabel);
                        }

                        mainLayout->addWidget(noSearchResultsWidget);
                        mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
                    }
                }

                auto insertNdx = layout_->indexOf(searchInAllButton_) - 1;
                layout_->insertWidget(insertNdx, noSearchResults_);
            }
            else
            {
                view_->hide();
                noSearchResults_->show();
            }
        }
    }

    void ContactListWidget::hideNoSearchResults()
    {
        if (noSearchResultsShown_)
        {
            noSearchResults_->hide();
            view_->show();
            noSearchResultsShown_ = false;
        }
    }

    void ContactListWidget::showSearchSpinner(Logic::SearchModelDLG* _senderModel)
    {
        if (searchModel_ != _senderModel || !isVisible())
            return;

        view_->hide();

        if (searchSpinnerShown_)
            return;

        if (!searchSpinner_)
        {
            searchSpinnerShown_ = true;
            searchSpinner_ = new QWidget(this);
            searchSpinner_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            {
                auto mainLayout = Utils::emptyVLayout(searchSpinner_);
                mainLayout->setAlignment(Qt::AlignCenter);
                {
                    auto searchSpinnerWidget = new QWidget(searchSpinner_);
                    auto searchSpinnerLayout = new QVBoxLayout(searchSpinnerWidget);
                    searchSpinnerWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    searchSpinnerLayout->setAlignment(Qt::AlignCenter);
                    searchSpinnerLayout->setContentsMargins(0, 0, 0, 0);
                    searchSpinnerLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto w = new QLabel(searchSpinnerWidget);
                        auto spinner = new QMovie(qsl(":/resources/gifs/r_spinner_100.gif"), QByteArray(), w);
                        spinner->setScaledSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
                        w->setMovie(spinner);
                        searchSpinnerLayout->addWidget(w);
                        spinner->start();
                    }

                    mainLayout->addWidget(searchSpinnerWidget);
                }
            }

            auto insertNdx = layout_->indexOf(searchInAllButton_) - 1;
            layout_->insertWidget(insertNdx, searchSpinner_);
        }
    }


    void ContactListWidget::hideSearchSpinner()
    {
        if (searchSpinnerShown_)
        {
            searchSpinner_->hide();
            layout_->removeWidget(searchSpinner_);
            searchSpinner_->deleteLater();
            searchSpinner_ = nullptr;
            view_->show();
            searchSpinnerShown_ = false;
        }
    }

    void ContactListWidget::switchToInitial(bool _initial)
    {
        if (initial_ == _initial)
            return;

        initial_ = _initial;
        if (initial_)
        {
            if (clDelegate_ == nullptr)
                setClDelegate(qobject_cast<Logic::AbstractItemDelegateWithRegim*>(delegateFromRegin(this, regim_, chatMembersModel_)));

            view_->setModel(modelForRegim(regim_, searchModel_, chatMembersModel_, isSearchInDialog_));
            return;
        }

        view_->setModel(searchModel_);
    }

    void ContactListWidget::searchUpOrDownPressed(bool _isUpPressed)
    {
        if (!searchModel_)
            return;

        auto inc = _isUpPressed ? -1 : 1;

        QModelIndex i = view_->selectionModel()->currentIndex();

        i = searchModel_->index(i.row() + inc);

        while (searchModel_->isServiceItem(i.row()))
        {
            i = searchModel_->index(i.row() + inc);
            if (!i.isValid())
                return;
        }

        view_->selectionModel()->blockSignals(true);
        view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        view_->selectionModel()->blockSignals(false);
        searchModel_->emitChanged(i.row() - inc, i.row());
        view_->scrollTo(i);
    }

    void ContactListWidget::initSearchModel(Logic::AbstractSearchModel* _searchModel)
    {
        if (!_searchModel)
            searchModel_ = searchModelForRegim(this, regim_, chatMembersModel_);
        else
            searchModel_ = _searchModel;

        searchModel_->searchPatternChanged(QString());

        auto smDlg = qobject_cast<Logic::SearchModelDLG*>(searchModel_);
        if (smDlg)
            connect(this, &ContactListWidget::searchEnd, smDlg, &Logic::SearchModelDLG::searchEnded, Qt::QueuedConnection);

        connect(searchModel_, &Logic::AbstractSearchModel::results, this, &ContactListWidget::onSearchResults);
    }

    bool ContactListWidget::isSelectMembersRegim() const
    {
        return  regim_ == Logic::MembersWidgetRegim::SELECT_MEMBERS ||
                regim_ == Logic::MembersWidgetRegim::MEMBERS_LIST ||
                regim_ == Logic::MembersWidgetRegim::VIDEO_CONFERENCE ||
                regim_ == Logic::MembersWidgetRegim::SHARE_TEXT ||
                regim_ == Logic::MembersWidgetRegim::SHARE_LINK;
    }

    void ContactListWidget::setGlobalSearchHeader(const QString & _caption)
    {
        globalSearchHeader_->setCaption(_caption);
        globalSearchHeader_->show();
    }

    void ContactListWidget::hideGlobalSearchHeader()
    {
        globalSearchHeader_->hide();
    }

    void ContactListWidget::onGlobalHeaderBackClicked()
    {
        hideGlobalSearchHeader();
    }

    void ContactListWidget::scrollRangeChanged(int _min, int _max)
    {
        auto searchModel = qobject_cast<Logic::SearchModelDLG*>(searchModel_);
        if (searchModel && _min != _max && scrollPosition_)
        {
            if (searchModel && searchModel->getCurrentViewMode() == Logic::searchModel::searchView::results_page)
            {
                view_->verticalScrollBar()->setValue(scrollPosition_);
                scrollPosition_ = 0;
            }
        }
    }
}
