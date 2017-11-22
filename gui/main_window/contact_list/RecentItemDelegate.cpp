#include "stdafx.h"
#include "RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "SearchModelDLG.h"

#include "ContactList.h"
#include "../history_control/MessagesModel.h"
#include "../history_control/LastStatusAnimation.h"

#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "RecentsItemRenderer.h"

#include "../../gui_settings.h"

namespace Logic
{
    RecentItemDelegate::RecentItemDelegate(QObject* parent)
        : AbstractItemDelegateWithRegim(parent)
        , StateBlocked_(false)
    {
    }

    void RecentItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const Data::DlgState& _dlg, bool _dragOverlay, bool _renderAsInCL) const
    {
        auto curViewParams = viewParams_;
        if (viewParams_.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH && _renderAsInCL)
            curViewParams.regim_ = ::Logic::MembersWidgetRegim::CONTACT_LIST;

        const auto hasMouseOver = (platform::is_apple() ? Logic::getRecentsModel()->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver) : true);
        const auto isSelected = (_option.state & QStyle::State_Selected) && !StateBlocked_;
        const auto isHovered = (_option.state & QStyle::State_MouseOver) && !StateBlocked_ && !isSelected && hasMouseOver;

        _painter->save();
        _painter->setRenderHint(QPainter::Antialiasing);
        _painter->setRenderHint(QPainter::TextAntialiasing);
        _painter->setRenderHint(QPainter::SmoothPixmapTransform);
        _painter->translate(_option.rect.topLeft());

        if (_dlg.AimId_.startsWith(ql1c('~')) && _dlg.AimId_.endsWith(ql1c('~')))
        {
            const bool isFavorites = _dlg.AimId_ == ql1s("~favorites~");
            if (isFavorites || _dlg.AimId_ == ql1s("~recents~"))
                ContactList::RenderServiceItem(*_painter, _dlg.GetText(), isFavorites, isFavorites, curViewParams);
            else if (_dlg.AimId_ == ql1s("~unknowns~"))
                ContactList::RenderUnknownsHeader(*_painter, QT_TRANSLATE_NOOP("contact_list", "New contacts"), Logic::getUnknownsModel()->totalUnreads(), curViewParams);
            else if (_dlg.AimId_ == ql1s("~more people~"))
                ContactList::RenderServiceButton(*_painter, _dlg.GetText(), isHovered, curViewParams, _option.rect.height());
            else if (_dlg.AimId_ == ql1s("~messages~") || _dlg.AimId_ == ql1s("~people~"))
                ContactList::RenderServiceItem(*_painter, _dlg.GetText(), false, false, curViewParams);
            else if (_dlg.AimId_ == ql1s("~more chats~"))
            {
                static QPixmap p(Utils::parse_image_name(qsl(":/resources/i_search_more_100.png")));
                Utils::check_pixel_ratio(p);

                ContactList::RecentItemVisualData visData(
                    _dlg.AimId_, p, QString(), QString(),
                    isHovered, false, _dlg.GetText(), false,
                    QDateTime(), 0, false, QString(),
                    false, false, QPixmap(), false,
                    QString(), false, -1, false);

                ContactList::RenderRecentsItem(*_painter, visData, curViewParams, _option.rect);

                if (!isHovered)
                {
                    static QPen linePen(CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_LIGHT));
                    linePen.setWidth(0);
                    _painter->setRenderHint(QPainter::Antialiasing, false);
                    _painter->setPen(linePen);
                    _painter->drawLine(ContactList::GetRecentsParams(curViewParams.regim_).getContactNameX(), 0, ItemWidth(curViewParams), 0);
                }
            }
            else
                assert(false);

            _painter->restore();
            return;
        }

        const auto isMultichat = Logic::getContactListModel()->isChat(_dlg.AimId_);
        auto state = isMultichat ? QString() : Logic::getContactListModel()->getState(_dlg.AimId_);

        if ((_option.state & QStyle::State_Selected) && (state == ql1s("online") || state == ql1s("mobile")))
            state += ql1s("_active");

        bool isDefaultAvatar = false;
        QString aimId = _dlg.AimId_;
        const bool isMail = _dlg.AimId_ == ql1s("mail");
        if (isMail)
        {
            auto i1 = _dlg.Friendly_.indexOf(ql1c('<'));
            auto i2 = _dlg.Friendly_.indexOf(ql1c('>'));
            if (i1 != -1 && i2 != -1)
                aimId = _dlg.Friendly_.mid(i1 + 1, _dlg.Friendly_.length() - i1 - (_dlg.Friendly_.length() - i2 + 1));
        }

        auto displayName = (isMail || _dlg.isFromGlobalSearch_) ? _dlg.Friendly_ : Logic::getContactListModel()->getDisplayName(_dlg.AimId_);

        auto avatar = GetAvatarStorage()->GetRounded(
            aimId,
            displayName,
            Utils::scale_bitmap(ContactList::GetRecentsParams(curViewParams.regim_).getAvatarSize()),
            state,
            isDefaultAvatar,
            false,
            ContactList::GetRecentsParams(curViewParams.regim_).isCL()
        );

        auto message = _dlg.GetText();

        bool isTyping = false;

        if (curViewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH && curViewParams.regim_ != ::Logic::MembersWidgetRegim::CONTACT_LIST)
            for (auto iter_typing = typings_.rbegin(); iter_typing != typings_.rend(); ++iter_typing)
            {
                if (iter_typing->aimId_ == _dlg.AimId_)
                {
                    isTyping = true;

                    message.clear();

                    if (isMultichat)
                        message += iter_typing->getChatterName() % ql1c(' ');

                    message += QT_TRANSLATE_NOOP("contact_list", "typing...");

                    break;
                }
            }

        const auto isOfficial = _dlg.Official_ || Logic::getContactListModel()->isOfficial(_dlg.AimId_);
        auto isDrawLastRead = false;

        QPixmap lastReadAvatar;

        bool isOutgoing = _dlg.Outgoing_;
        bool isLastRead = (_dlg.LastMsgId_ >= 0 && _dlg.TheirsLastRead_ > 0 && _dlg.LastMsgId_ <= _dlg.TheirsLastRead_);

        const QDateTime dateTime = QDateTime::fromTime_t(_dlg.Time_);

        if (isOutgoing && !isMultichat)
        {
            if (isLastRead && !Logic::GetMessagesModel()->hasPending(_dlg.AimId_))
            {
                lastReadAvatar = *GetAvatarStorage()->GetRounded(
                    _dlg.AimId_,
                    displayName,
                    Utils::scale_bitmap(ContactList::GetRecentsParams(curViewParams.regim_).getLastReadAvatarSize()),
                    QString(),
                    isDefaultAvatar,
                    false,
                    ContactList::GetRecentsParams(curViewParams.regim_).isCL()
                );
                isDrawLastRead = true;
            }
            else
            {
                if (_dlg.IsLastMessageDelivered)
                {
                    pendingsDialogs.remove(_dlg.AimId_);
                }
                else
                {
                    constexpr qint64 timeout = 5 * 1000 * 60;
                    const auto dateTimeInMsecs = dateTime.toUTC().toMSecsSinceEpoch();
                    const auto currentDateTimeInMsecs = QDateTime::currentMSecsSinceEpoch();
                    if ((currentDateTimeInMsecs - dateTimeInMsecs > timeout)
                        || pendingsDialogs.contains(_dlg.AimId_)
                        || (currentDateTimeInMsecs - qApp->property("startupTime").toLongLong() < timeout))
                    {
                        pendingsDialogs.insert(_dlg.AimId_);

                        lastReadAvatar = Ui::LastStatusAnimation::getPendingPixmap(isSelected);
                        isDrawLastRead = true;
                    }
                }
            }
        }

        if (isMail && _dlg.MailId_.isEmpty())
            displayName = message;

        QString sender = _dlg.senderNick_;
        if (_dlg.Chat_)
        {
            auto dn = Logic::getContactListModel()->getDisplayName(_dlg.senderAimId_);
            if (!dn.isEmpty() && dn != _dlg.senderAimId_)
                sender = dn;
        }

        const int unreadCount = viewParams_.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH ? _dlg.UnreadCount_ : 0;

        ContactList::RecentItemVisualData visData(
            _dlg.AimId_,
            *avatar,
            state,
            message,
            isHovered,
            isSelected,
            displayName.isEmpty() ? _dlg.AimId_ : displayName,
            true /* hasLastSeen */,
            dateTime,
            unreadCount,
            Logic::getContactListModel()->isMuted(_dlg.AimId_),
            sender,
            isOfficial,
            isDrawLastRead,
            lastReadAvatar,
            isTyping,
            _dlg.SearchTerm_,
            _dlg.HasLastMsgId(),
            _dlg.SearchedMsgId_,
            _dlg.isFromGlobalSearch_,
            _dlg.unreadMentionsCount_ > 0);

        visData.IsMailStatus_ = isMail && _dlg.MailId_.isEmpty();

        ContactList::RenderRecentsItem(*_painter, visData, curViewParams, _option.rect);
        if (_dragOverlay)
            ContactList::RenderRecentsDragOverlay(*_painter, curViewParams);

        _painter->restore();
    }

    void RecentItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        const Data::DlgState dlg = _index.data(Qt::DisplayRole).value<Data::DlgState>();

        if (dlg.AimId_ == ql1s("~recents~") && !Logic::getRecentsModel()->getFavoritesCount())
            return;

        paint(_painter, _option, dlg, _index == DragIndex_, shouldRenderCompact(_index));
    }

    bool RecentItemDelegate::shouldRenderCompact(const QModelIndex & _index) const
    {
        if (!_index.isValid())
            return false;

        const auto allMsgNdx = Logic::getSearchModelDLG()->getMessagesHeaderIndex();

        return
            viewParams_.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH
            && (_index.row() < allMsgNdx || allMsgNdx == -1)
            && !Logic::getSearchModelDLG()->isSearchInDialog();
    }

    QSize RecentItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& _i) const
    {
        auto& recentParams = ContactList::GetRecentsParams(viewParams_.regim_);

        auto width = recentParams.itemWidth();

        if (viewParams_.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            if (Logic::getSearchModelDLG()->isServiceItem(_i.row()))
            {
                if (Logic::getSearchModelDLG()->isServiceButton(_i))
                    return QSize(width, recentParams.serviceButtonHeight());
                else
                    return QSize(width, recentParams.serviceItemHeight());
            }
            else if (shouldRenderCompact(_i))
            {
                if (Logic::getSearchModelDLG()->isResultFromGlobalSearch(_i))
                    return QSize(width, ContactList::GetContactListParams().globalItemHeight());
                else
                    return QSize(width, ContactList::GetContactListParams().itemHeight());
            }
        }
        else
        {
            if (Logic::getRecentsModel()->isServiceItem(_i))
            {
                if (Logic::getRecentsModel()->isUnknownsButton(_i))
                    return QSize(width, recentParams.unknownsItemHeight());
                else
                {
                    if (Logic::getRecentsModel()->isRecentsHeader(_i) && !Logic::getRecentsModel()->getFavoritesCount())
                        return QSize();
                    else
                        return QSize(width, recentParams.serviceItemHeight());
                }

            }
        }
        return QSize(width, recentParams.itemHeight());
    }

    QSize RecentItemDelegate::sizeHintForAlert() const
    {
        return QSize(ContactList::GetRecentsParams(viewParams_.regim_).itemWidth(), ContactList::GetRecentsParams(viewParams_.regim_).itemHeight());
    }

    void RecentItemDelegate::blockState(bool value)
    {
        StateBlocked_ = value;
    }

    void RecentItemDelegate::addTyping(const TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter == typings_.end())
        {
            typings_.push_back(_typing);
        }
    }

    void RecentItemDelegate::removeTyping(const TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter != typings_.end())
        {
            typings_.erase(iter);
        }
    }

    void RecentItemDelegate::setDragIndex(const QModelIndex& index)
    {
        DragIndex_ = index;
    }

    void RecentItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }

    bool RecentItemDelegate::getPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }

    void RecentItemDelegate::setFixedWidth(int _newWidth)
    {
        viewParams_.fixedWidth_ = _newWidth;
    }

    void RecentItemDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

    RecentItemDelegate::ItemKey::ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber)
        : IsSelected(isSelected)
        , IsHovered(isHovered)
        , UnreadDigitsNumber(unreadDigitsNumber)
    {
        assert(unreadDigitsNumber >= 0);
        assert(unreadDigitsNumber <= 2);
    }

    bool RecentItemDelegate::ItemKey::operator < (const ItemKey &_key) const
    {
        if (IsSelected != _key.IsSelected)
        {
            return (IsSelected < _key.IsSelected);
        }

        if (IsHovered != _key.IsHovered)
        {
            return (IsHovered < _key.IsHovered);
        }

        if (UnreadDigitsNumber != _key.UnreadDigitsNumber)
        {
            return (UnreadDigitsNumber < _key.UnreadDigitsNumber);
        }

        return false;
    }
}
