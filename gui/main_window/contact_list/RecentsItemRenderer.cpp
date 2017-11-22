#include "stdafx.h"

#include "RecentsItemRenderer.h"
#include "Common.h"
#include "RecentsModel.h"
#include "ContactList.h"
#include "../../gui_settings.h"
#include "../../controls/CommonStyle.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"
#include "../../utils/Text2DocConverter.h"
#include "../../controls/TextEditEx.h"
#include "../../utils/utils.h"
#include "../../my_info.h"

namespace ContactList
{
    RecentItemVisualData::RecentItemVisualData(
        const QString &_aimId,
        const QPixmap &_avatar,
        const QString &_state,
        const QString &_status,
        const bool _isHovered,
        const bool _isSelected,
        const QString &_contactName,
        const bool _hasLastSeen,
        const QDateTime &_lastSeen,
        const int _unreadsCounter,
        const bool _muted,
        const QString &_senderNick,
        const bool _isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const bool _isTyping,
        const QString _term,
        const bool _hasLastMsg,
        qint64 _msgId,
        const bool _notInCL,
        const bool _hasUnreadMentions)
        : VisualDataBase(_aimId, _avatar, _state, _status, _isHovered, _isSelected, _contactName, _hasLastSeen, _lastSeen, false /*_isWithCheckBox*/
            , false /* _isChatMember */, _isOfficial, _drawLastRead, _lastReadAvatar, QString() /* role */, _unreadsCounter, _term, _muted, _notInCL)
        , IsTyping_(_isTyping)
        , senderNick_(_senderNick)
        , HasLastMsg_(_hasLastMsg)
        , msgId_(_msgId)
        , IsMailStatus_(false)
        , hasUnreadMentions_(_hasUnreadMentions)
    {
        assert(_unreadsCounter >= 0);
    }

    static bool showLastMessage()
    {
        return Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);
    }

    void RenderRecentsItem(QPainter &_painter, const RecentItemVisualData &_item, const ViewParams& _viewParams, const QRect& _itemRect)
    {
        auto contactListInRecents = GetRecentsParams(_viewParams.regim_);
        _painter.save();

        _painter.setBrush(Qt::NoBrush);
        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(_painter, _item.IsHovered_, _item.IsSelected_, _viewParams, _itemRect.height());

        const auto ratio = Utils::scale_bitmap(1);
        const auto avatarX = _viewParams.pictOnly_ ? (_itemRect.width() - _item.Avatar_.width() / ratio) / 2 : contactListInRecents.getAvatarX();
        const auto avatarY = (_itemRect.height() - _item.Avatar_.height() / ratio) / 2;
        RenderAvatar(_painter, avatarX, avatarY, _item.Avatar_, contactListInRecents.getAvatarSize());

        int rightMargin = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_) - contactListInRecents.itemHorPadding();

        if (_viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT && _viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            if (_viewParams.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN)
            {
                if (!_viewParams.pictOnly_)
                    rightMargin = RenderRemove(_painter, _item.IsSelected_, contactListInRecents, _viewParams);

                if (!_item.unreadsCounter_)
                {
                    if (!_viewParams.pictOnly_)
                    {
                        rightMargin = RenderAddContact(
                            _painter,
                            rightMargin - Utils::scale_value(16),
                            _item.IsSelected_,
                            contactListInRecents)
                            - contactListInRecents.itemHorPadding();
                    }
                }
                else
                {
                    rightMargin = RenderNotifications(
                        _painter,
                        _item.unreadsCounter_,
                        false /* muted */,
                        _viewParams,
                        contactListInRecents,
                        false /* isUnknownHeader */,
                        _item.IsSelected_,
                        _item.IsHovered_)
                        - contactListInRecents.itemContentPadding();
                }
            }
            else
            {
                bool addPadding = false;
                if (_item.notInCL_ && !_viewParams.pictOnly_)
                {
                    rightMargin = RenderAddContact(_painter, rightMargin, _item.IsSelected_, contactListInRecents);
                    addPadding = true;
                }
                else if (!_item.drawLastRead_)
                {
                    rightMargin = RenderNotifications(
                        _painter,
                        _item.unreadsCounter_,
                        _item.isMuted_,
                        _viewParams,
                        contactListInRecents,
                        false /* isUnknownHeader */,
                        _item.IsSelected_,
                        _item.IsHovered_);
                    addPadding = true;
                }

                if (_item.hasUnreadMentions_ && !_viewParams.pictOnly_)
                {
                    rightMargin = RenderMentionSign(
                        _painter,
                        contactListInRecents,
                        rightMargin - (addPadding? Utils::scale_value(8) : 0),
                        _item.IsSelected_);
                }

                rightMargin -= contactListInRecents.itemContentPadding();
            }
        }

        if (_viewParams.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
            rightMargin = RenderDate(_painter, _item.LastSeen_, _item, contactListInRecents, _viewParams);

        if (_viewParams.pictOnly_)
        {
            _painter.restore();
            return;
        }

        if (_item.IsMailStatus_)
        {
            RenderContactName(_painter, _item, contactListInRecents.nameYForMailStatus(), rightMargin, _viewParams, contactListInRecents);
            _painter.restore();
            return;
        }

        const auto contactNameY = _item.notInCL_ && !_item.GetStatus().isEmpty() ? Utils::scale_value(3) : contactListInRecents.getContactNameY();
        RenderContactName(_painter, _item, contactNameY, rightMargin, _viewParams, contactListInRecents);

        if (_viewParams.regim_ == ::Logic::MembersWidgetRegim::CONTACT_LIST && !_item.notInCL_)
        {
            _painter.restore();
            return;
        }

        const int lastReadWidth =
            contactListInRecents.getLastReadAvatarSize() + contactListInRecents.getlastReadRightMargin() + contactListInRecents.getlastReadLeftMargin();

        rightMargin -= (_item.drawLastRead_ ? lastReadWidth : 0);

        const int messageWidth = RenderContactMessage(_painter, _item, rightMargin, _viewParams, contactListInRecents);

        if (_item.drawLastRead_ && !_item.IsTyping_ && _viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH && showLastMessage())
        {
            RenderLastReadAvatar(_painter, _item.lastReadAvatar_, contactListInRecents.messageX() + messageWidth + contactListInRecents.getlastReadLeftMargin(), contactListInRecents);
        }

        _painter.restore();
    }

    void RenderRecentsDragOverlay(QPainter &_painter, const ViewParams& _viewParams)
    {
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);
        auto recentParams = GetRecentsParams(_viewParams.regim_);

        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        QColor overlayColor(ql1s("#ffffff"));
        overlayColor.setAlphaF(0.9);
        _painter.fillRect(0, 0, width, recentParams.itemHeight(), QBrush(overlayColor));
        _painter.setBrush(QBrush(Qt::transparent));
        QPen pen (CommonStyle::getColor(CommonStyle::Color::GREEN_FILL), recentParams.dragOverlayBorderWidth(), Qt::DashLine, Qt::RoundCap);
        _painter.setPen(pen);
        _painter.drawRoundedRect(
            recentParams.dragOverlayPadding(),
            recentParams.dragOverlayVerPadding(),
            width - recentParams.itemHorPadding(),
            recentParams.itemHeight() - recentParams.dragOverlayVerPadding(),
            recentParams.dragOverlayBorderRadius(),
            recentParams.dragOverlayBorderRadius()
            );

        QPixmap p(Utils::parse_image_name(qsl(":/resources/file_sharing/upload_cl_100.png")));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = width / 2 - p.width() / 2. / ratio;
        int y = (recentParams.itemHeight() / 2) - (p.height() / 2. / ratio);
        _painter.drawPixmap(x, y, p);

        _painter.restore();
    }

    void RenderServiceItem(QPainter &_painter, const QString& _text, bool _renderState, bool _isFavorites, const ViewParams& _viewParams)
    {
        _painter.save();

        _painter.setRenderHint(QPainter::Antialiasing);

        static auto pen(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        _painter.setPen(pen);

        static auto font(Fonts::appFontScaled(12, platform::is_windows_vista_or_late() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal));
        _painter.setFont(font);

        auto recentParams = GetRecentsParams(_viewParams.regim_);
        const auto ratio = Utils::scale_bitmap(1);
        const auto height = recentParams.serviceItemHeight();

        if (!_viewParams.pictOnly_)
            Utils::drawText(_painter, QPointF(recentParams.itemHorPadding(), height / 2), Qt::AlignVCenter, _text);
        else
        {
            _painter.save();
            QPen line_pen;
            line_pen.setColor(CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER));
            _painter.setPen(line_pen);
            auto p = QPixmap(Utils::parse_image_name(_isFavorites ? qsl(":/resources/icon_favorites_100.png") : qsl(":/resources/icon_recents_100.png")));
            Utils::check_pixel_ratio(p);
            int y = height / 2;
            int xp = ItemWidth(_viewParams) / 2 - (p.width() / 2. / ratio);
            int yp = height / 2 - (p.height() / 2. / ratio);
            _painter.drawLine(0, y, xp - recentParams.serviceItemIconPadding(), y);
            _painter.drawLine(xp + p.width() / ratio + recentParams.serviceItemIconPadding(), y, ItemWidth(_viewParams), y);
            _painter.drawPixmap(xp, yp, p);
            _painter.restore();
        }

        if (_renderState && !_viewParams.pictOnly_)
        {
            QImage p(Utils::parse_image_name(qsl(":/controls/arrow_a_100")));

            if (!Logic::getRecentsModel()->isFavoritesVisible())
                p = std::move(p).mirrored();
            Utils::check_pixel_ratio(p);

            QFontMetrics m(font);
            int x = recentParams.itemHorPadding() + m.width(_text) + recentParams.favoritesStatusPadding();
            int y = height / 2 - (p.height() / 2. / ratio);
            _painter.drawImage(x, y, p);
        }

        _painter.restore();
    }

    void RenderServiceButton(QPainter &_painter, const QString& _text, const bool _isHovered, const ViewParams& _viewParams, const int _itemHeight)
    {
        _painter.save();

        const auto recentParams = GetRecentsParams(_viewParams.regim_);
        const auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        if (_isHovered)
        {
            static QBrush hoverBrush(CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_LIGHT));
            _painter.fillRect(0, 0, width, _itemHeight, hoverBrush);
        }

        static const auto font = Fonts::appFontScaled(14, Fonts::FontWeight::Medium);
        _painter.setFont(font);

        static const auto pen(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
        _painter.setPen(pen);

        Utils::drawText(_painter, QPointF(width - recentParams.itemHorPadding(), _itemHeight / 2), Qt::AlignVCenter | Qt::AlignRight, _text);

        _painter.restore();
    }

    void RenderUnknownsHeader(QPainter &_painter, const QString& _title, const int _count, const ViewParams& _viewParams)
    {
        auto recentParams = GetRecentsParams(_viewParams.regim_);
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        if (!_viewParams.pictOnly_)
        {
            QPen pen;
            pen.setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
            _painter.setPen(pen);

            QFont f;
            if (_count)
            {
                f = Fonts::appFontScaled(16, Fonts::FontWeight::Medium);
            }
            else
            {
                f = Fonts::appFontScaled(16);
            }
            _painter.setFont(f);
            QFontMetrics metrics(f);
            Utils::drawText(_painter, QPointF(recentParams.itemHorPadding(), recentParams.unknownsItemHeight() / 2.), Qt::AlignVCenter, _title);
        }
        else
        {
            QPixmap pict(Utils::parse_image_name(qsl(":/resources/unknown_100.png")));
            Utils::check_pixel_ratio(pict);
            _painter.drawPixmap(
                recentParams.getAvatarX() + (recentParams.getAvatarSize() / 2) - pict.width() / (2 * Utils::scale_bitmap(1)),
                recentParams.getAvatarY(), pict);
        }

        _painter.restore();

        if (_count)
        {
            _painter.save();

            _painter.setBrush(Qt::NoBrush);
            _painter.setPen(Qt::NoPen);
            _painter.setRenderHint(QPainter::Antialiasing);

            RenderNotifications(_painter, _count, false, _viewParams, recentParams, true /* isUnknownHeader */, false, false);
            _painter.restore();
        }
    }
}

namespace ContactList
{
    Emoji::EmojiSizePx GetEmojiSize()
    {
        static std::unordered_map<int32_t, Emoji::EmojiSizePx> info;

        if (info.empty())
        {
            info.emplace(100, Emoji::EmojiSizePx::_16);
            info.emplace(125, Emoji::EmojiSizePx::_22);
            info.emplace(150, Emoji::EmojiSizePx::_27);
            info.emplace(200, Emoji::EmojiSizePx::_32);
        }

        return info[(int32_t)(Utils::getScaleCoefficient() * 100)];
    }


    int RenderContactMessage(QPainter &_painter, const RecentItemVisualData &_visData, const int _rightMargin, const ViewParams& _viewParams, ContactListParams& _recentParams)
    {
        if (!_visData.HasStatus())
        {
            return 0;
        }

        static auto plainTextControl = CreateTextBrowser(qsl("message"), _recentParams.getMessageStylesheet(_recentParams.messageFontSize(), false, false), _recentParams.messageHeight());
        static auto unreadsTextControl = CreateTextBrowser(qsl("messageUnreads"), _recentParams.getMessageStylesheet(_recentParams.messageFontSize(), true, false), _recentParams.messageHeight());
        static auto selectedTextControl = CreateTextBrowser(qsl("message"), _recentParams.getMessageStylesheet(_recentParams.messageFontSize(), false, true), _recentParams.messageHeight());

        static auto gSearchTextControl = CreateTextBrowser(qsl("message"), _recentParams.getMessageStylesheet(_recentParams.globalContactMessageFontSize(), false, false), _recentParams.messageHeight());
        static auto gSearchTextControlSel = CreateTextBrowser(qsl("message"), _recentParams.getMessageStylesheet(_recentParams.globalContactMessageFontSize(), false, true), _recentParams.messageHeight());

        const auto hasUnreads = (_viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT
            && _viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH
            && (_visData.unreadsCounter_ > 0));

        auto &globalTextCtrl = _visData.IsSelected_ ? gSearchTextControlSel : gSearchTextControl;
        auto &unreadsTextCtrl = hasUnreads ? unreadsTextControl : plainTextControl;
        auto &msgTextCtrl = _visData.IsSelected_ ? selectedTextControl : unreadsTextCtrl;
        auto &textControl = _visData.notInCL_ ? globalTextCtrl : msgTextCtrl;

        const auto maxWidth = (_rightMargin - _recentParams.messageX());
        textControl->setFixedWidth(maxWidth);
        textControl->setWordWrapMode(QTextOption::WrapMode::NoWrap);

        auto &doc = *textControl->document();
        doc.clear();

        if (_viewParams.regim_ != Logic::MembersWidgetRegim::FROM_ALERT && _viewParams.regim_ != Logic::MembersWidgetRegim::HISTORY_SEARCH && !(_visData.notInCL_ || showLastMessage()))
            return -1;

        const auto &senderNick = _visData.senderNick_;
        const auto isChat = _visData.AimId_.endsWith(ql1s("@chat.agent"));
        const auto sameChatName = isChat && _visData.ContactName_ == senderNick;
        const auto outgoingInChat = isChat && senderNick == Ui::MyInfo()->friendlyName();

        auto messageTextMaxWidth = maxWidth;

        if (!senderNick.isEmpty() && !_visData.IsTyping_ && !sameChatName && !outgoingInChat)
        {
            static const auto fix = ql1s(": ");

            const QString fixedNick = senderNick.simplified() % fix;

            auto cursor = textControl->textCursor();
            const auto charFormat = cursor.charFormat();

            cursor.setCharFormat(charFormat);

            Logic::Text4Edit(fixedNick, *textControl, cursor, Logic::Text2DocHtmlMode::Pass, false, GetEmojiSize());

            cursor.setCharFormat(charFormat);

            messageTextMaxWidth -= doc.idealWidth();
        }

        const auto text = _visData.GetStatus().simplified();
        const auto fontSize = _visData.notInCL_ ? _recentParams.globalContactMessageFontSize() : _recentParams.messageFontSize();
        auto searchTerm = _visData.IsSelected_ ? QString() : _visData.searchTerm_;
        highlightMatchedTerm(text, searchTerm, *textControl, messageTextMaxWidth, GetEmojiSize(), fontSize);

        Logic::FormatDocument(doc, _recentParams.messageHeight());

        const auto messageY = _visData.notInCL_ ? Utils::scale_value(20) : _recentParams.messageY();
        textControl->render(&_painter, QPoint(_recentParams.messageX(), messageY));

        return doc.idealWidth();
    }

    void RenderLastReadAvatar(QPainter &_painter, const QPixmap& _avatar, const int _xOffset, ContactListParams& _recentParams)
    {
        _painter.setOpacity(0.7);
        _painter.drawPixmap(_xOffset, _recentParams.lastReadY(), _recentParams.getLastReadAvatarSize(), _recentParams.getLastReadAvatarSize(), _avatar);
    }

    int RenderNotifications(QPainter &_painter, const int _unreads, bool _muted, const ViewParams& _viewParams, ContactListParams& _recentParams, bool _isUnknownHeader, bool _isActive, bool _isHover)
    {
        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        const auto unreadsSize = Utils::scale_value(20);
        assert(_unreads >= 0);

        if (_unreads <= 0)
        {
            return width;
        }

        static QFontMetrics m(_recentParams.unreadsFont());

        const auto text = Utils::getUnreadsBadgeStr(_unreads);
        const auto unreadsRect = m.tightBoundingRect(text);
        const auto firstChar = text[0];
        const auto lastChar = text[text.size() - 1];
        const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

        auto balloonWidth = unreadsWidth;
        const auto isLongText = (text.length() > 1);
        if (isLongText)
        {
            balloonWidth += (_recentParams.unreadsPadding() * 2);
        }
        else
        {
            balloonWidth = unreadsSize;
        }

        auto unreadsX = width -
            ((_viewParams.regim_ != Logic::MembersWidgetRegim::UNKNOWN) ?
            _recentParams.itemHorPadding()
            : _recentParams.itemHorPadding()) - balloonWidth;
        auto unreadsY = _isUnknownHeader ?
            _recentParams.unknownsUnreadsY(_viewParams.pictOnly_)
            : (_recentParams.itemHeight() - unreadsSize) / 2;

        if (_viewParams.pictOnly_)
        {
            unreadsX = _recentParams.itemHorPadding();
            if (!_isUnknownHeader)
                unreadsY = (_recentParams.itemHeight() - _recentParams.getAvatarSize()) / 2;
        }
        else if (_viewParams.regim_ == Logic::MembersWidgetRegim::UNKNOWN)
        {
            unreadsX -= _recentParams.removeSize().width();
        }

        auto borderColor = CommonStyle::getFrameColor();
        if (_isHover)
        {
            borderColor = CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_LIGHT);
        }
        else if (_isActive)
        {
            borderColor = (_viewParams.pictOnly_) ? CommonStyle::getColor(CommonStyle::Color::GREEN_FILL) : Qt::transparent;
        }

        const auto bgColor = _isActive ? QColor(ql1s("#ffffff")) : CommonStyle::getColor(_muted? CommonStyle::Color::GRAY_FILL_DARK : CommonStyle::Color::GREEN_FILL);
        const auto textColor = _isActive ? CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT) : QColor(ql1s("#ffffff"));
        Utils::drawUnreads(&_painter, _recentParams.unreadsFont(), bgColor, textColor, borderColor, _unreads, unreadsSize, unreadsX, unreadsY);

        return unreadsX;
    }

    int RenderMentionSign(QPainter &_painter, ContactListParams& _recentParams, const int _rightMargin, const bool _isActive)
    {
        QPixmap pm;
        if (_isActive)
            pm = Utils::parse_image_name(qsl(":/resources/counter_mention_100_active.png"));
        else
            pm = Utils::parse_image_name(qsl(":/resources/counter_mention_100.png"));

        if (pm.isNull())
            return _rightMargin;

        Utils::check_pixel_ratio(pm);

        const auto ratio = Utils::scale_bitmap(1);
        const auto x = _rightMargin - (pm.width() / ratio);
        const auto y = (_recentParams.itemHeight() - (pm.height() / ratio)) / 2;

        _painter.drawPixmap(x, y, pm);

        return x - _recentParams.itemContentPadding();
    }

    void RenderDeleteAllItem(QPainter &_painter, const QString& _title, bool _isMouseOver, const ViewParams& _viewParams)
    {
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPen pen;
        pen.setColor(_isMouseOver ? CommonStyle::getColor(CommonStyle::Color::TEXT_RED) : CommonStyle::getColor(CommonStyle::Color::TEXT_RED));
        _painter.setPen(pen);
        const auto font_size = Utils::scale_value(16);
        QFont f = Fonts::appFont(font_size);
        _painter.setFont(f);
        QFontMetrics metrics(f);
        auto titleRect = metrics.boundingRect(_title);
        auto recentParams = GetRecentsParams(_viewParams.regim_);

        recentParams.deleteAllFrame().setHeight(recentParams.unknownsItemHeight());
        recentParams.deleteAllFrame().setX(
            CorrectItemWidth(ItemWidth(false, false, false),_viewParams.fixedWidth_)
            - recentParams.itemHorPadding()
            - titleRect.width()
        );
        recentParams.deleteAllFrame().setY(recentParams.unknownsItemHeight() / 2.);
        recentParams.deleteAllFrame().setWidth(ItemWidth(false, false, false) - recentParams.deleteAllFrame().x());
        Utils::drawText(_painter, QPointF(recentParams.deleteAllFrame().x(), recentParams.deleteAllFrame().y()), Qt::AlignVCenter, _title);

        _painter.restore();
    }

    QRect DeleteAllFrame()
    {
        auto recentParams = GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
        return recentParams.deleteAllFrame();
    }
}
