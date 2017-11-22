#include "stdafx.h"

#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../controls/TextEditEx.h"

#include "Common.h"
#include "ContactList.h"
#include "../../gui_settings.h"

namespace ContactList
{
    VisualDataBase::VisualDataBase(
        const QString& _aimId,
        const QPixmap& _avatar,
        const QString& _state,
        const QString& _status,
        const bool _isHovered,
        const bool _isSelected,
        const QString& _contactName,
        const bool _hasLastSeen,
        const QDateTime& _lastSeen,
        const bool _isWithCheckBox,
        const bool _isChatMember,
        const bool _isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const QString& _role,
        const int _unreadsCounter,
        const QString& _term,
        const bool _muted,
        const bool _notInCL)
        : AimId_(_aimId)
        , Avatar_(_avatar)
        , State_(_state)
        , ContactName_(_contactName)
        , IsHovered_(_isHovered)
        , IsSelected_(_isSelected)
        , HasLastSeen_(_hasLastSeen)
        , LastSeen_(_lastSeen)
        , isCheckedBox_(_isWithCheckBox)
        , isChatMember_(_isChatMember)
        , isOfficial_(_isOfficial)
        , drawLastRead_(_drawLastRead)
        , lastReadAvatar_(_lastReadAvatar)
        , role_(_role)
        , unreadsCounter_(_unreadsCounter)
        , searchTerm_(_term)
        , isMuted_(_muted)
        , notInCL_(_notInCL)
        , Status_(_status)
    {
        assert(!AimId_.isEmpty());
        assert(!ContactName_.isEmpty());
    }

    const QString& VisualDataBase::GetStatus() const
    {
        return Status_;
    }

    bool VisualDataBase::HasStatus() const
    {
        return !Status_.isEmpty();
    }

    void VisualDataBase::SetStatus(const QString& _status)
    {
        Status_ = _status;
    }

    QString FormatTime(const QDateTime& _time)
    {
        if (!_time.isValid())
        {
            return QString();
        }

        const auto current = QDateTime::currentDateTime();

        const auto days = _time.daysTo(current);

        if (days == 0)
        {
            const auto minutes = _time.secsTo(current) / 60;
            if (minutes < 1)
            {
                return QT_TRANSLATE_NOOP("contact_list", "now");
            }

            if (minutes < 10)
            {
                return QString::number(minutes) + QT_TRANSLATE_NOOP("contact_list", " min");
            }

            return _time.time().toString(Qt::SystemLocaleShortDate);
        }

        if (days == 1)
        {
            return QT_TRANSLATE_NOOP("contact_list", "yesterday");
        }

        const auto date = _time.date();
        return Utils::GetTranslator()->formatDate(date, date.year() == current.date().year());
    }

    TextEditExUptr CreateTextBrowser(const QString& _name, const QString& _stylesheet, const int _textHeight)
    {
        assert(!_name.isEmpty());
        assert(!_stylesheet.isEmpty());

        TextEditExUptr ctrl(new Ui::TextEditEx(nullptr, QFont(), QColor(), false, false));

        ctrl->setObjectName(_name);
        ctrl->setStyleSheet(_stylesheet);
        if (_textHeight)
            ctrl->setFixedHeight(_textHeight);

        ctrl->setFrameStyle(QFrame::NoFrame);
        ctrl->setContentsMargins(0, 0, 0, 0);

        ctrl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ctrl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        return ctrl;
    }


    int ItemWidth(bool _fromAlert, bool _isWithCheckBox, bool _isPictureOnlyView)
    {
        if (_isPictureOnlyView)
        {
            auto params = ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
            params.setIsCL(false);
            return params.itemHorPadding() + params.getAvatarSize() + params.itemHorPadding();
        }

        if (_fromAlert)
            return ContactList::GetRecentsParams(Logic::MembersWidgetRegim::FROM_ALERT).itemWidth();

        // TODO : use L::checkboxWidth
        return (_isWithCheckBox ? Utils::scale_value(30) : 0) + std::min(Utils::scale_value(400), ItemLength(true, 1. / 3, 0));
    }

    int ItemWidth(const ViewParams& _viewParams)
    {
        return ItemWidth(_viewParams.regim_ == ::Logic::MembersWidgetRegim::FROM_ALERT,
            IsSelectMembers(_viewParams.regim_), _viewParams.pictOnly_);
    }

    int CorrectItemWidth(int _itemWidth, int _fixedWidth)
    {
        return _fixedWidth == -1 ? _itemWidth : _fixedWidth;
    }

    int ItemLength(bool _isWidth, double _koeff, int _addWidth)
    {
        return ItemLength(_isWidth, _koeff, _addWidth, Utils::InterConnector::instance().getMainWindow());
    }

    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent)
    {
        assert(!!parent && "Common.cpp (ItemLength)");
        auto mainRect = Utils::GetWindowRect(parent);
        if (mainRect.width() && mainRect.height())
        {
            auto mainLength = _isWidth ? mainRect.width() : mainRect.height();
            return _addWidth + mainLength * _koeff;
        }
        assert("Couldn't get rect: Common.cpp (ItemLength)");
        return 0;
    }

    bool IsPictureOnlyView()
    {
        auto mainRect = Utils::GetMainRect();

        // TODO : use Utils::MinWidthForMainWindow
        return mainRect.width() <= Utils::scale_value(800);
    }

    ContactListParams& GetContactListParams()
    {
        static ContactListParams params(true);
        return params;
    }

    ContactListParams& GetRecentsParams(int _regim)
    {
        if (_regim == Logic::MembersWidgetRegim::FROM_ALERT || _regim == Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            static ContactListParams params(false);
            return params;
        }
        else if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            return GetContactListParams();
        }
        else
        {
            const auto show_last_message = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);
            static ContactListParams params(!show_last_message);
            params.setIsCL(!show_last_message);
            return params;
        }
    }

    void RenderAvatar(QPainter &_painter, const int _x, const int _y, const QPixmap& _avatar, const int _avatarSize)
    {
        if (!_avatar.isNull())
            _painter.drawPixmap(_x, _y, _avatarSize, _avatarSize, _avatar);
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const ViewParams& _viewParams, const int _height)
    {
        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        QRect rect(0, 0, width, _height);

        RenderMouseState(_painter, _isHovered, _isSelected, rect);
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const QRect& _rect)
    {
        if (!_isHovered && !_isSelected)
            return;

        QBrush brush;
        if (_isHovered)
        {
            static QBrush hoverBrush(CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_LIGHT));
            brush = hoverBrush;
        }
        if (_isSelected)
        {
            static QBrush selectedBrush(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL));
            brush = selectedBrush;
        }

        _painter.fillRect(_rect, brush);
    }

    int RenderDate(QPainter &_painter, const QDateTime &_ts, const VisualDataBase &_item, const ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        const auto regim = _viewParams.regim_;
        const auto isWithCheckBox = IsSelectMembers(regim);
        auto timeXRight =
            CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_)
            - _contactList.itemHorPadding();

        if (!_ts.isValid())
        {
            return timeXRight;
        }

        const auto timeStr = FormatTime(_ts);
        if (timeStr.isEmpty())
        {
            return timeXRight;
        }

        static QFontMetrics m(_contactList.timeFont());
        const auto leftBearing = m.leftBearing(timeStr[0]);
        const auto rightBearing = m.rightBearing(timeStr[timeStr.length() - 1]);
        const auto timeWidth = (m.tightBoundingRect(timeStr).width() + leftBearing + rightBearing);
        const auto timeX = timeXRight - timeWidth;

        if ((!isWithCheckBox && !Logic::is_members_regim(regim) && !Logic::is_admin_members_regim(regim))
            || (Logic::is_members_regim(regim) && !_item.IsHovered_)
            || (!Logic::is_admin_members_regim(regim) && !_item.IsHovered_))
        {
            _painter.save();
            _painter.setFont(_contactList.timeFont());
            _painter.setPen(_contactList.timeFontColor(_item.IsSelected_));
            _painter.drawText(timeX, _contactList.timeY(), timeStr);
            _painter.restore();
        }

        return timeX;
    }

    int RenderContactNameNative(
        QPainter &_painter,
        const int _y,
        const int _maxWidth,
        const ContactList::ContactListParams& _contactList,
        const Fonts::FontWeight _weight,
        const QColor _color,
        const QString& _name,
        const QString& _term)
    {
        QFont font = _contactList.getContactNameFont(_weight);

        QFontMetrics m(font);

        const auto elidedString = m.elidedText(_name, Qt::ElideRight, _maxWidth);

        _painter.save();

        auto index = elidedString.indexOf(_term, 0, Qt::CaseInsensitive);

        QString left = elidedString.mid(0, index);
        QString mid;
        QString right;

        if (index != -1)
        {
            mid = elidedString.mid(index, _term.length());
            right= elidedString.mid(left.size() + mid.size());
        }

        auto termPen = QPen(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
        auto termFont = font;
        termFont.setWeight(QFont::Weight::Medium);
        QFontMetrics termM(termFont);

        _painter.setFont(font);
        _painter.setPen(_color);

        auto contactNameX = _contactList.getContactNameX();
        auto xOffset = 0;
        _painter.drawText(QPoint(contactNameX, _y), left);

        _painter.setPen(termPen);
        xOffset += m.width(left);
        _painter.setFont(termFont);
        _painter.drawText(QPoint(contactNameX + xOffset, _y), mid);

        _painter.setPen(_color);
        _painter.setFont(font);
        xOffset += termM.width(mid);
        _painter.drawText(QPoint(contactNameX + xOffset, _y), right);

        _painter.restore();

        return m.width(elidedString);
    }

    void RenderContactName(
        QPainter &_painter,
        const ContactList::VisualDataBase &_visData,
        const int _y,
        const int _rightMargin,
        ContactList::ViewParams _viewParams,
        ContactList::ContactListParams& _contactList)
    {
        assert(_y > 0);
        assert(_rightMargin > 0);
        assert(!_visData.ContactName_.isEmpty());

        _contactList.setLeftMargin(_viewParams.leftMargin_);

        QColor color;

        const auto hasUnreads = (_viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT && (_visData.unreadsCounter_ > 0));
        const auto weight = (hasUnreads ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);

        QString name;

        int height = 0;

        if (_contactList.isCL())
        {
            auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_visData.isChatMember_ || _visData.isCheckedBox_);
            color = _contactList.getNameFontColor(_visData.IsSelected_, isMemberSelected);
            name = qsl("name");
            height = _contactList.contactNameHeight();
        }
        else
        {
            color = _contactList.getNameFontColor(_visData.IsSelected_, false);
            name = hasUnreads ? qsl("nameUnread") : qsl("name");
            height = _contactList.contactNameHeight();
        }

        QPixmap mutedBadge;
        if (_visData.isMuted_)
        {
            if (_visData.IsSelected_)
                mutedBadge = QPixmap(Utils::parse_image_name(qsl(":/resources/mute_100_active.png")));
            else
                mutedBadge = QPixmap(Utils::parse_image_name(qsl(":/resources/mute_100.png")));

            Utils::check_pixel_ratio(mutedBadge);
        }

        QPixmap officialMark;

        if (_visData.isOfficial_)
        {
            officialMark = QPixmap(Utils::parse_image_name(qsl(":/resources/official_badge_blue_100.png")));
            Utils::check_pixel_ratio(officialMark);
        }

        int maxWidth = _rightMargin - _contactList.getContactNameX();

        if (_contactList.isCL())
        {
            maxWidth -= _contactList.itemContentPadding();
        }

        if (!mutedBadge.isNull())
        {
            maxWidth -= mutedBadge.width();
        }

        if (!officialMark.isNull())
        {
            maxWidth -= officialMark.width();
        }

        int textWidth = 0;

        auto searchTerm = _visData.IsSelected_ ? QString() : _visData.searchTerm_;

        if (platform::is_apple())
        {
            textWidth = RenderContactNameNative(_painter, _y + Utils::scale_value(16), maxWidth, _contactList, weight, color, _visData.ContactName_, searchTerm);
        }
        else
        {
            const auto styleSheetQss = _contactList.getContactNameStylesheet(color.name(), weight);
            static auto textControl = CreateTextBrowser(name, styleSheetQss, height);

            textControl->setStyleSheet(styleSheetQss);
            textControl->setFixedWidth(maxWidth);

            QFontMetrics m(textControl->font());
            const auto elidedString = m.elidedText(_visData.ContactName_, Qt::ElideRight, maxWidth);

            auto &doc = *textControl->document();
            doc.clear();

            highlightMatchedTerm(elidedString, searchTerm, *textControl, maxWidth, Emoji::EmojiSizePx::Auto, _contactList.contactNameFontSize());

            Logic::FormatDocument(doc, _contactList.contactNameHeight());

            textControl->render(&_painter, QPoint(_contactList.getContactNameX(), _y));

            textWidth = doc.idealWidth();
        }

        if (_visData.isMuted_)
        {
            const auto ratio = Utils::scale_bitmap(1);
            const auto pX = _contactList.getContactNameX() + textWidth
                + (_visData.isOfficial_ ? officialMark.width() / ratio + _contactList.official_hor_padding() : 0)
                + _contactList.mute_hor_padding();
            const auto pY = _y + (_contactList.contactNameHeight() - (mutedBadge.height() / ratio)) / 2;
            _painter.drawPixmap(pX, pY, mutedBadge);
        }

        if (_visData.isOfficial_)
        {
            double ratio = Utils::scale_bitmap(1);
            const auto pX = _contactList.getContactNameX() + textWidth + _contactList.official_hor_padding();
            const auto pY = _y + (_contactList.contactNameHeight() - (officialMark.height() / ratio)) / 2;
            _painter.drawPixmap(pX, pY, officialMark);
        }
    }

    void highlightMatchedTerm(const QString& _text,
                              const QString& _term,
                              Ui::TextEditEx& _edit,
                              int maxWidth,
                              const Emoji::EmojiSizePx _emojiSize,
                              int fontSize)
    {
        struct CutTextResult {
            QString left;
            QString mid;
            QString right;
            int maxWidth;
        };

        static QString lastTerm;
        static QHash<QString, CutTextResult> cutResults;

        QTextCursor cursor = _edit.textCursor();

        QFontMetrics m(_edit.font());
        const auto elidedString = m.elidedText(_text, Qt::ElideRight, maxWidth);

        CutTextResult current;

        if (!_term.isEmpty() && elidedString.contains(_term, Qt::CaseInsensitive))
        {
            if (_term != lastTerm)
            {
                lastTerm = _term;
                cutResults.clear();
            }

            bool needUpdate = false;
            if (!cutResults.count(elidedString) || cutResults[elidedString].maxWidth != maxWidth)
                needUpdate = true;

            if (needUpdate)
            {
                Logic::CutText(elidedString, _term, maxWidth, m, m, cursor, Logic::Text2DocHtmlMode::Pass, false, nullptr, current.left, current.right, current.mid);
                current.maxWidth = maxWidth;
                cutResults.insert(elidedString, current);
            }
            else
            {
                current = cutResults[elidedString];
            }
        }
        else
        {
            current.left = elidedString;
        }

        const auto charFormat = cursor.charFormat();
        auto boldCharFormat = charFormat;

        boldCharFormat.setFont(Fonts::appFont(fontSize, Fonts::FontWeight::Medium));
        boldCharFormat.setForeground(QBrush(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT)));

        Logic::Text4Edit(current.left, _edit, cursor, Logic::Text2DocHtmlMode::Pass, false, _emojiSize);

        cursor.setCharFormat(boldCharFormat);
        Logic::Text4Edit(current.mid, _edit, cursor, Logic::Text2DocHtmlMode::Pass, false, _emojiSize);

        cursor.setCharFormat(charFormat);
        Logic::Text4Edit(current.right, _edit, cursor, Logic::Text2DocHtmlMode::Pass, false, _emojiSize);
    }

    int RenderAddContact(QPainter &_painter, const int _rightMargin, const bool _isSelected, ContactListParams& _recentParams)
    {
        auto img = QPixmap(Utils::parse_image_name(_isSelected ? qsl(":/resources/i_plus_100_active.png") : qsl(":/resources/i_plus_100.png")));
        Utils::check_pixel_ratio(img);

        double ratio = Utils::scale_bitmap(1);
        _recentParams.addContactFrame().setX(_rightMargin - (img.width() / ratio));
        _recentParams.addContactFrame().setY((_recentParams.itemHeight() / 2) - (img.height() / ratio / 2.));
        _recentParams.addContactFrame().setWidth(img.width() / ratio);
        _recentParams.addContactFrame().setHeight(img.height() / ratio);
        _painter.save();
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);
        _painter.drawPixmap(_recentParams.addContactFrame().x(),
            _recentParams.addContactFrame().y(),
            img);
        _painter.restore();

        return _recentParams.addContactFrame().x();
    }

    void RenderCheckbox(QPainter &_painter, const VisualDataBase &_visData, const ContactListParams& _contactList)
    {
        auto img = QPixmap(_visData.isChatMember_ ?
            Utils::parse_image_name(qsl(":/controls/checkbox_100_disabled"))
            : (_visData.isCheckedBox_
                ? Utils::parse_image_name(qsl(":/controls/checkbox_100_active"))
                : Utils::parse_image_name(qsl(":/controls/checkbox_100"))));

        Utils::check_pixel_ratio(img);
        double ratio = Utils::scale_bitmap(1);
        _painter.drawPixmap(_contactList.itemHorPadding(), _contactList.getItemMiddleY() - (img.height() / 2. / ratio), img);
    }

    int RenderRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        auto remove_img = QPixmap(Utils::parse_image_name(_isSelected ? qsl(":/controls/close_b_100") : qsl(":/controls/close_a_100")));
        Utils::check_pixel_ratio(remove_img);

        double ratio = Utils::scale_bitmap(1);
        _contactList.removeContactFrame().setX(
            CorrectItemWidth(ItemWidth(false, false, false), _viewParams.fixedWidth_)
            - _contactList.itemHorPadding() - (remove_img.width() / ratio) + Utils::scale_value(8));
        _contactList.removeContactFrame().setY(
            (_contactList.itemHeight() / 2) - (remove_img.height() / ratio / 2.));
        _contactList.removeContactFrame().setWidth(remove_img.width());
        _contactList.removeContactFrame().setHeight(remove_img.height());

        _painter.save();
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

        _painter.drawPixmap(_contactList.removeContactFrame().x(),
            _contactList.removeContactFrame().y(),
            remove_img);

        _painter.restore();

        return _contactList.removeContactFrame().x();

    }

    int GetXOfRemoveImg(int _width)
    {
        auto _contactList = GetContactListParams();
        return
            CorrectItemWidth(ItemWidth(false, false), _width)
            - _contactList.itemHorPadding()
            - _contactList.removeSize().width()
            + Utils::scale_value(8);
    }

    bool IsSelectMembers(int regim)
    {
        return (regim == Logic::MembersWidgetRegim::SELECT_MEMBERS) || (regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE);
    }
}
