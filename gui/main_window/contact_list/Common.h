#pragma once

#include "../../namespaces.h"
#include "../../controls/CommonStyle.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../cache/emoji/Emoji.h"

FONTS_NS_BEGIN

enum class FontFamily;
enum class FontWeight;

FONTS_NS_END

namespace Ui
{
    class TextEditEx;
}

namespace ContactList
{
    typedef std::unique_ptr<Ui::TextEditEx> TextEditExUptr;

    struct VisualDataBase
    {
        VisualDataBase(
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
            const bool _official,
            const bool _drawLastRead,
            const QPixmap& _lastReadAvatar,
            const QString& role,
            const int _UnreadsCounter,
            const QString& _term,
            const bool _muted,
            const bool _notInCL
        );

        const QString AimId_;

        const QPixmap &Avatar_;

        const QString State_;

        const QString &ContactName_;

        const bool IsHovered_;

        const bool IsSelected_;

        const bool HasLastSeen_;

        const QDateTime LastSeen_;

        bool IsOnline() const { return HasLastSeen_ && !LastSeen_.isValid(); }

        bool HasLastSeen() const { return HasLastSeen_; }

        const bool isCheckedBox_;

        bool isChatMember_;

        bool isOfficial_;

        const bool drawLastRead_;

        const QPixmap& lastReadAvatar_;

        const QString& GetStatus() const;

        bool HasStatus() const;

        void SetStatus(const QString& _status);

        QString role_;

        const int unreadsCounter_;

        const QString searchTerm_;

        const bool isMuted_;
        const bool notInCL_;

    private:
        QString Status_;
    };

    QString FormatTime(const QDateTime &_time);

    TextEditExUptr CreateTextBrowser(const QString& _name, const QString& _stylesheet, const int _textHeight);

    int ItemWidth(bool _fromAlert, bool _isWithCheckBox, bool _isPictureOnlyView = false);

    struct ViewParams;
    int ItemWidth(const ViewParams& _viewParams);

    int CorrectItemWidth(int _itemWidth, int _fixedWidth);

    int ItemLength(bool _isWidth, double _koeff, int _addWidth);
    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent);

    bool IsPictureOnlyView();

    struct ViewParams
    {
        ViewParams(int _regim, int _fixedWidth, int _leftMargin, int _rightMargin)
            : regim_(_regim)
            , fixedWidth_(_fixedWidth)
            , leftMargin_(_leftMargin)
            , rightMargin_(_rightMargin)
            , pictOnly_(false)
        {}

        ViewParams()
            : regim_(-1)
            , fixedWidth_(-1)
            , leftMargin_(0)
            , rightMargin_(0)
            , pictOnly_(false)
        {}

        int regim_;
        int fixedWidth_;
        int leftMargin_;
        int rightMargin_;
        bool pictOnly_;
    };

    class ContactListParams
    {
    public:

        //Common
        int itemHeight() const { return isCl_ ? Utils::scale_value(44) : Utils::scale_value(64); }
        int globalItemHeight() const { return Utils::scale_value(48); }
        int itemWidth() const { return Utils::scale_value(320); }
        int itemHorPadding() const { return  Utils::scale_value(16); }
        int itemContentPadding() const { return Utils::scale_value(16); }
        int getItemMiddleY() const
        {
            return getAvatarSize() / 2 + getAvatarY();
        };
        int serviceItemHeight() const { return  Utils::scale_value(24); }
        int serviceButtonHeight() const { return  Utils::scale_value(44); }
        int serviceItemIconPadding() const { return Utils::scale_value(12); }

        //Contact avatar
        int getAvatarX() const
        {
            if (isCl_ && withCheckBox_)
                return itemHorPadding() + checkboxSize() + Utils::scale_value(12);

            return itemHorPadding();
        }
        int getAvatarY() const
        {
            return (itemHeight() - getAvatarSize()) / 2;
        }
        int getAvatarSize() const
        {
            return (!isCl_) ?
                Utils::scale_value(48) : Utils::scale_value(32);
        }

        //Contact name
        int getContactNameX() const
        {
            return (!isCl_) ?
                (getAvatarX() + getAvatarSize() + itemContentPadding()) :
                (getAvatarX() + getAvatarSize() + Utils::scale_value(12) + leftMargin_);
        }
        int getContactNameY() const
        {
            return (!isCl_) ?
                Utils::scale_value(10) : Utils::scale_value(10);
        }

        int nameYForMailStatus() const
        {
            return Utils::scale_value(20);
        }

        int contactNameFontSize() const
        {
            return platform::is_windows_vista_or_late() ?
                Utils::scale_value(16) : Utils::scale_value(15);
        }

        int contactNameHeight() const
        {
            return Utils::scale_value(22);
        }

        QString getContactNameStylesheet(const QString& _fontColor, const Fonts::FontWeight _fontWeight) const
        {
            assert(_fontWeight > Fonts::FontWeight::Min);
            assert(_fontWeight < Fonts::FontWeight::Max);

            const auto fontQss = Fonts::appFontFullQss(contactNameFontSize(), Fonts::defaultAppFontFamily(), _fontWeight);
            return qsl("%1; color: %2; background-color: transparent;").arg(fontQss, _fontColor);
        };

        QFont getContactNameFont(const Fonts::FontWeight _fontWeight) const
        {
            return Fonts::appFontScaled(contactNameFontSize(), _fontWeight);
        }

        QColor getNameFontColor(bool _isSelected, bool _isMemberChecked) const
        {
            return _isSelected ?
                QColor(ql1s("#ffffff")) : _isMemberChecked ?
                CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT) :
                CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY);
        }

        //Message
        int messageFontSize() const
        {
            return platform::is_windows_vista_or_late() ?
                Utils::scale_value(14) : Utils::scale_value(13);
        }

        int globalContactMessageFontSize() const
        {
            return Utils::scale_value(12);
        }

        int messageHeight() const
        {
            return  Utils::scale_value(24);
        }

        int messageX() const
        {
            return  getContactNameX();
        }

        int messageY() const
        {
            return  Utils::scale_value(30);
        }

        QString getRecentsMessageFontColor(const bool _isUnread) const
        {
            const auto color = _isUnread ?
                CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY).name() :
                CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT).name();

            return color;
        };

        QString getMessageStylesheet(const int _fontSize, const bool _isUnread, const bool _isSelected) const
        {
            const auto fontWeight = Fonts::FontWeight::Normal;
            const auto fontQss = Fonts::appFontFullQss(_fontSize, Fonts::defaultAppFontFamily(), fontWeight);
            const auto fontColor = _isSelected ? qsl("#ffffff") : getRecentsMessageFontColor(_isUnread);
            return qsl("%1; color: %2; background-color: transparent").arg(fontQss, fontColor);
        };

        //Unknown contacts page
        int unknownsUnreadsY(bool _pictureOnly) const
        {
            return _pictureOnly ? Utils::scale_value(4) : Utils::scale_value(14);
        }
        int unknownsItemHeight() const { return  Utils::scale_value(40); }

        //Time
        int timeY() const
        {
            return !isCl_ ?
                (getAvatarY() + Utils::scale_value(24)) : Utils::scale_value(27);
        }

        QFont timeFont() const
        {
            return Fonts::appFontScaled(12);
        }

        QColor timeFontColor(bool _isSelected) const
        {
            return _isSelected ? QColor(ql1s("#ffffff")) : CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY);
        }

        //Additional options
        int checkboxSize() const { return Utils::scale_value(20); }
        QSize removeSize() const { return QSize(Utils::scale_value(28), Utils::scale_value(24)); }
        int role_offset() const { return  Utils::scale_value(24); }
        int role_ver_offset() const { return Utils::scale_value(4); }

        //Groups in Contact list
        int groupY()  const { return Utils::scale_value(17); }
        QFont groupFont() const { return Fonts::appFontScaled(12); }
        QColor groupColor() const { return CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT); }

        //Unreads counter
        QFont unreadsFont() const { return Fonts::appFontScaled(13, Fonts::FontWeight::Medium); }
        int unreadsPadding() const { return  Utils::scale_value(8); }

        //Last seen
        int lastReadY() const { return  Utils::scale_value(38); }
        int getLastReadAvatarSize() const { return Utils::scale_value(12); }
        int getlastReadLeftMargin() const { return Utils::scale_value(4); }
        int getlastReadRightMargin() const { return Utils::scale_value(4); }


        QFont emptyIgnoreListFont() const { return Fonts::appFontScaled(16); }

        //Search in the dialog
        QColor searchInAllChatsColor() const { return CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT); }
        QFont searchInAllChatsFont() const { return Fonts::appFontScaled(14, Fonts::FontWeight::Medium); }
        int searchInAllChatsHeight() const { return   Utils::scale_value(48); }
        int searchInAllChatsY() const { return Utils::scale_value(28); }

        int dragOverlayPadding() const { return Utils::scale_value(8); }
        int dragOverlayBorderWidth() const { return Utils::scale_value(2); }
        int dragOverlayBorderRadius() const { return Utils::scale_value(8); }
        int dragOverlayVerPadding() const { return  Utils::scale_value(1); }

        int official_hor_padding() const { return  Utils::scale_value(2); }
        int mute_hor_padding() const { return  Utils::scale_value(2); }

        int globalSearchHeaderHeight() const { return Utils::scale_value(32); }

        ContactListParams(bool _isCl)
            : isCl_(_isCl)
            , withCheckBox_(false)
            , leftMargin_(0)
        {}

        bool getWithCheckBox() const
        {
            return withCheckBox_;
        }

        void setWithCheckBox(bool _withCheckBox)
        {
            withCheckBox_ = _withCheckBox;
        }

        void setLeftMargin(int _leftMargin)
        {
            leftMargin_ = _leftMargin;
        }

        int getLeftMargin() const
        {
            return leftMargin_;
        }

        bool isCL() const
        {
            return isCl_;
        }

        void setIsCL(bool _isCl)
        {
            isCl_ = _isCl;
        }

        void resetParams()
        {
            leftMargin_ = 0;
            withCheckBox_ = false;
        }

        int favoritesStatusPadding() { return  Utils::scale_value(4); }

        QRect& addContactFrame()
        {
            static QRect addContactFrameRect(0, 0, 0, 0);
            return addContactFrameRect;
        }

        QRect& removeContactFrame()
        {
            static QRect removeContactFrameRect(0, 0, 0, 0);
            return removeContactFrameRect;
        }

        QRect& deleteAllFrame()
        {
            static QRect deleteAllFrameRect(0, 0, 0, 0);
            return deleteAllFrameRect;
        }

    private:
        bool isCl_;
        bool withCheckBox_;
        int leftMargin_;
    };

    ContactListParams& GetContactListParams();

    ContactListParams& GetRecentsParams(int _regim);

    void RenderAvatar(QPainter &_painter, const int _x, const int _y, const QPixmap& _avatar, const int _avatarSize);

    void RenderMouseState(QPainter &_painter, const bool isHovered, const bool _isSelected, const ContactList::ViewParams& _viewParams, const int _height);
    void RenderMouseState(QPainter &_painter, const bool isHovered, const bool _isSelected, const QRect& _rect);

    void RenderContactName(
        QPainter &_painter,
        const ContactList::VisualDataBase &_visData,
        const int _y,
        const int _rightMargin,
        ContactList::ViewParams _viewParams,
        ContactList::ContactListParams& _contactList);

    int RenderContactNameNative(QPainter &_painter,
        const int _y,
        const int _maxWidth,
        const ContactList::ContactListParams& _contactList,
        const Fonts::FontWeight _weight,
        const QColor _color,
        const QString& _name,
        const QString &_term = QString());

    void highlightMatchedTerm(const QString& _text,
        const QString& _term,
        Ui::TextEditEx& _edit,
        int maxWidth,
        const Emoji::EmojiSizePx _emojiSize,
        int fontSize);

    int RenderDate(QPainter &_painter, const QDateTime &_date, const VisualDataBase &_item, const ContactListParams& _contactList, const ViewParams& _viewParams);

    int RenderAddContact(QPainter &_painter, const int _rightMargin, const bool _isSelected, ContactListParams& _recentParams);

    void RenderCheckbox(QPainter &_painter, const VisualDataBase &_visData, const ContactListParams& _contactList);

    int RenderRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams);

    int GetXOfRemoveImg(int width);

    bool IsSelectMembers(int regim);
}

namespace Logic
{
    class AbstractItemDelegateWithRegim : public QItemDelegate
    {
        Q_OBJECT
    public:
        AbstractItemDelegateWithRegim(QObject* parent)
            : QItemDelegate(parent)
        {}
        virtual void setRegim(int _regim) = 0;

        virtual void setFixedWidth(int width) = 0;

        virtual void blockState(bool value) = 0;

        virtual void setDragIndex(const QModelIndex& index) = 0;
    };
}
