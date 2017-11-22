#include "stdafx.h"
#include "MessageItem.h"

#include "MessageItemLayout.h"
#include "MessagesModel.h"
#include "MessagesScrollArea.h"
#include "MessageStatusWidget.h"
#include "MessageStyle.h"
#include "ContentWidgets/FileSharingWidget.h"
#include "../contact_list/ContactListModel.h"

#include "../../app_config.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../cache/themes/themes.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/ContextMenu.h"
#include "../../my_info.h"
#include "../../gui_settings.h"
#include "../../theme_settings.h"
#include "../../utils/InterConnector.h"
#include "../../utils/PainterPath.h"
#include "../../utils/Text.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"


namespace Ui
{

    namespace
    {
        int32_t getMessageTopPadding();
        int32_t getMessageBottomPadding();

        QMap<QString, QVariant> makeData(const QString& command);
    }

    MessageData::MessageData()
        : AvatarVisible_(false)
        , SenderVisible_(false)
        , IndentBefore_(false)
        , deliveredToServer_(false)
        , Chat_(false)
        , Id_(-1)
        , AvatarSize_(-1)
        , Time_(0)
    {
        Outgoing_.Outgoing_ = false;
        Outgoing_.Set_ = false;
    }

    bool MessageData::isOutgoing() const
    {
        assert(Outgoing_.Set_);

        return Outgoing_.Outgoing_;
    }

    MessageItemsAvatars::MessageItemsAvatars()
    {
        QObject::connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, [this](const QString& _aimId)
        {
            if (MessageItemsAvatars::instance().data_.find(_aimId) != MessageItemsAvatars::instance().data_.end())
            {
                bool isDefault = false;
                auto &info = MessageItemsAvatars::instance().data_[_aimId];
                info.avatar_ = *Logic::GetAvatarStorage()->GetRounded(_aimId, info.friendlyName_, info.size_, QString(), isDefault, false, false).get();
                if (info.callback_)
                {
                    info.callback_();
                }
            }
        });
    }

    MessageItemsAvatars &MessageItemsAvatars::instance()
    {
        static MessageItemsAvatars instance_;
        return instance_;
    }

    MessageItemsAvatars::~MessageItemsAvatars()
    {
    }

    QPixmap &MessageItemsAvatars::get(const QString& _aimId, const QString& _friendlyName, int _size, const std::function<void()>& _callback)
    {
        if (MessageItemsAvatars::instance().data_.find(_aimId) == MessageItemsAvatars::instance().data_.end())
        {
            bool isDefault = false;
            MessageItemsAvatars::Info info;
            info.avatar_ = *Logic::GetAvatarStorage()->GetRounded(_aimId, _friendlyName, _size, QString(), isDefault, false, false).get();
            info.aimId_ = _aimId;
            info.friendlyName_ = _friendlyName;
            info.size_ = _size;
            info.callback_ = _callback;
            MessageItemsAvatars::instance().data_[_aimId] = info;
        }
        return MessageItemsAvatars::instance().data_[_aimId].avatar_;
    }

    void MessageItemsAvatars::reset(const QString& _aimId)
    {
        if (MessageItemsAvatars::instance().data_.find(_aimId) != MessageItemsAvatars::instance().data_.end())
        {
            MessageItemsAvatars::instance().data_.erase(_aimId);
        }
    }

    MessageItem::MessageItem()
        : MessageItemBase(nullptr)
        , MessageBody_(nullptr)
        , Sender_(nullptr)
        , ContentWidget_(nullptr)
        , Direction_(SelectDirection::NONE)
        , Data_(std::make_shared<MessageData>())
        , ClickedOnAvatar_(false)
        , TimeWidget_(nullptr)
        , Layout_(nullptr)
        , startSelectY_(0)
        , isSelection_(false)
        , isNotAuth_(false)
        , bubbleHovered_(false)
        , timestampHoverEnabled_(true)
    {
    }

    MessageItem::MessageItem(QWidget* _parent)
        : MessageItemBase(_parent)
        , MessageBody_(nullptr)
        , Sender_(nullptr)
        , ContentWidget_(nullptr)
        , Direction_(SelectDirection::NONE)
        , Data_(std::make_shared<MessageData>())
        , ClickedOnAvatar_(false)
        , TimeWidget_(new MessageTimeWidget(this))
        , Layout_(new MessageItemLayout(this))
        , startSelectY_(0)
        , isSelection_(false)
        , isNotAuth_(false)
        , bubbleHovered_(false)
        , timestampHoverEnabled_(true)
    {
        setAttribute(Qt::WA_AcceptTouchEvents);

        setMouseTracking(true);

        Utils::grabTouchWidget(this);
        setFocusPolicy(Qt::NoFocus);

        assert(Layout_);
        setLayout(Layout_);

        auto scrollArea = qobject_cast<MessagesScrollArea*>(parent());
        assert(scrollArea);

        connect(scrollArea, &MessagesScrollArea::recreateAvatarRect, this, &MessageItem::onRecreateAvatarRect);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showMessageHiddenControls, this, &MessageItem::showHiddenControls, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMessageHiddenControls, this, &MessageItem::hideHiddenControls, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setTimestampHoverActionsEnabled, this, &MessageItem::setTimestampHoverEnabled);

        TimeWidget_->hideAnimated();
    }

	MessageItem::~MessageItem()
    {
        if (Data_)
            MessageItemsAvatars::reset(Data_->Sender_);
	}

    void MessageItem::setContact(const QString& _aimId)
    {
        HistoryControlPageItem::setContact(_aimId);
        if (TimeWidget_)
        {
            TimeWidget_->setContact(_aimId);
        }
    }

    void MessageItem::setSender(const QString& _sender)
    {
        Data_->Sender_ = _sender;
    }

	QString MessageItem::formatRecentsText() const
	{
		if (ContentWidget_)
		{
			return ContentWidget_->toRecentsString();
		}

        if (!Data_->Mentions_.empty())
            return Utils::convertMentions(Data_->Text_, Data_->Mentions_);

        return Data_->Text_;
	}

	void MessageItem::setId(qint64 _id, const QString& _aimId)
	{
		Data_->Id_ = _id;
		Data_->AimId_ = _aimId;
	}

    void MessageItem::loadAvatar(const int _size)
    {
        assert(_size > 0);

        if (Data_->AvatarSize_ != _size)
        {
            MessageItemsAvatars::reset(Data_->Sender_);
        }

        Data_->AvatarSize_ = _size;

        setAvatarVisible(true);

        update();
    }

    QSize MessageItem::sizeHint() const
    {
        return QSize(0, evaluateTopContentMargin() + evaluateDesiredContentHeight());
    }

    void MessageItem::onVisibilityChanged(const bool _isVisible)
    {
        if (ContentWidget_)
        {
            ContentWidget_->onVisibilityChanged(_isVisible);
        }
    }

    void MessageItem::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {
        if (ContentWidget_)
        {
            ContentWidget_->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
        }
    }

    Data::Quote MessageItem::getQuote(bool force) const
    {
        Data::Quote quote;
        auto currentSelection = selection(true);
        if (currentSelection.isEmpty() && !force)
            return quote;

        if (force)
        {
            if (MessageBody_)
            {
                quote.text_ = MessageBody_->getPlainText();
                quote.type_ = Data::Quote::Type::text;
            }
            else if (ContentWidget_)
            {
                quote.text_ = ContentWidget_->toString();
                quote.type_ = Data::Quote::Type::file_sharing;
            }
        }
        else
        {
            quote.text_ = std::move(currentSelection);
        }

        if (Data_)
        {
            quote.senderId_ = Data_->isOutgoing() ? MyInfo()->aimId() : Data_->Sender_;
            quote.chatId_ = Data_->AimId_;
            quote.time_ = Data_->Time_;
            quote.msgId_ = Data_->Id_;
            QString senderFriendly = Data_->isOutgoing() ? MyInfo()->friendlyName()
                : (Data_->SenderFriendly_.isEmpty() ? Logic::getContactListModel()->getDisplayName(Data_->AimId_) : Data_->SenderFriendly_);
            quote.senderFriendly_ = std::move(senderFriendly);
            quote.mentions_ = Data_->Mentions_;
        }
        return quote;
    }

    void MessageItem::setHasAvatar(const bool _value)
    {
        setAvatarVisible(_value);
    }

    void MessageItem::setAvatarVisible(const bool _visible)
	{
        const auto visibilityChanged = (Data_->AvatarVisible_ != _visible);

        Data_->AvatarVisible_ = _visible;
        Data_->SenderVisible_ = (!Data_->Sender_.isEmpty() && Data_->AvatarVisible_);

        if (visibilityChanged)
        {
            if (Data_->AvatarVisible_ && Data_->AvatarSize_ <= 0)
            {
                loadAvatar(Utils::scale_bitmap(MessageStyle::getAvatarSize()));
            }

            update();
        }
	}

    void MessageItem::leaveEvent(QEvent* _e)
    {
        ClickedOnAvatar_ = false;
        bubbleHovered_ = false;

        if (timestampHoverEnabled_)
            hideHiddenControls();

        MessageItemBase::leaveEvent(_e);
    }

    void MessageItem::mouseMoveEvent(QMouseEvent* _e)
    {
        const auto pos = _e->pos();
        if (Data_->AvatarSize_ > 0 && isOverAvatar(pos))
        {
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }

        const auto prevHovered = bubbleHovered_;
        bubbleHovered_ = Bubble_.contains(pos);
        if (bubbleHovered_)
        {
            if (!prevHovered)
            {
                QTimer::singleShot(MessageTimestamp::hoverTimestampShowDelay,
                    Qt::CoarseTimer,
                    this,
                    [this]()
                    {
                        if (bubbleHovered_)
                            showHiddenControls();
                    }
                );
            }
        }
        else
        {
            if (timestampHoverEnabled_)
                hideHiddenControls();
        }


        QWidget::mouseMoveEvent(_e);
    }

    void MessageItem::mousePressEvent(QMouseEvent* _e)
    {
        const auto isLeftButtonFlagSet = ((_e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (_e->button() == Qt::LeftButton)
        );

        if (isLeftButton && isOverAvatar(_e->pos()) && Data_->AvatarSize_ > 0)
        {
            ClickedOnAvatar_ = true;
        }

        QWidget::mousePressEvent(_e);
    }

    void MessageItem::trackContextMenu(const QPoint& _pos)
    {
        auto contextMenu = new ContextMenu(this);

        contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/reply_100"))), QT_TRANSLATE_NOOP("context_menu", "Reply"), makeData(qsl("quote")));
        contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/link_100"))), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData(qsl("copy_link")));
        contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/attach_100"))), QT_TRANSLATE_NOOP("context_menu", "Copy to clipboard"), makeData(qsl("copy_file")));
        contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/forward_100"))), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward")));
        contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/download_100"))), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData(qsl("save_as")));

        if (ContentWidget_->hasOpenInBrowserMenu())
            contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/browser_100"))), QT_TRANSLATE_NOOP("context_menu", "Open in browser"), makeData(qsl("open_in_browser")));

        contextMenu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/close_100"))), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData(qsl("delete")));

        connect(contextMenu, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);
        connect(contextMenu, &ContextMenu::triggered, contextMenu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(contextMenu, &ContextMenu::aboutToHide, contextMenu, &ContextMenu::deleteLater, Qt::QueuedConnection);

        const auto isOutgoing = (Data_->Outgoing_.Set_ && Data_->Outgoing_.Outgoing_);
        if (isOutgoing || Logic::getContactListModel()->isYouAdmin(Data_->AimId_))
        {
            contextMenu->addActionWithIcon(QIcon(
                Utils::parse_image_name(qsl(":/context_menu/closeall_100"))),
                QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
                makeData(qsl("delete_all")));
        }
        contextMenu->popup(_pos);
    }

    void MessageItem::trackMenu(const QPoint& _pos)
    {
        auto menu = new ContextMenu(this);
        menu->addActionWithIcon(qsl(":/context_menu/reply_100"), QT_TRANSLATE_NOOP("context_menu", "Reply"), makeData(qsl("quote")));
        menu->addActionWithIcon(qsl(":/context_menu/copy_100"), QT_TRANSLATE_NOOP("context_menu", "Copy text"), makeData(qsl("copy")));
        menu->addActionWithIcon(qsl(":/context_menu/forward_100"), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward")));

        menu->addActionWithIcon(qsl(":/context_menu/close_100"), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData(qsl("delete")));

        connect(menu, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

        const auto isOutgoing = (Data_->Outgoing_.Set_ && Data_->Outgoing_.Outgoing_);
        if (isOutgoing || Logic::getContactListModel()->isYouAdmin(Data_->AimId_))
        {
            menu->addActionWithIcon(
                qsl(":/context_menu/closeall_100"),
                QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
                makeData(qsl("delete_all")));
        }

        if (GetAppConfig().IsContextMenuFeaturesUnlocked())
        {
            menu->addActionWithIcon(qsl(":/context_menu/copy_100"), qsl("Copy Message ID"), makeData(qsl("dev:copy_message_id")));
        }

        menu->popup(_pos);
    }

    void MessageItem::mouseReleaseEvent(QMouseEvent* _e)
    {
        const auto isRightButtonFlagSet = ((_e->buttons() & Qt::RightButton) != 0);
        const auto isRightButton = (
            isRightButtonFlagSet ||
            (_e->button() == Qt::RightButton)
        );

        if (isRightButton)
        {
            if (isOverAvatar(_e->pos()))
            {
                emit avatarMenuRequest(Data_->Sender_);
                return;
            }
            QPoint globalPos = mapToGlobal(_e->pos());
            if (MessageBody_)
            {
                trackMenu(globalPos);
            }
            else if (ContentWidget_)
            {
                if (ContentWidget_->hasContextMenu(globalPos))
                    trackContextMenu(globalPos);
                else
                    trackMenu(globalPos);
            }
        }

        const auto isLeftButtonFlagSet = ((_e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (_e->button() == Qt::LeftButton)
        );

        if (isLeftButton &&
            ClickedOnAvatar_ &&
            isOverAvatar(_e->pos()))
        {
            avatarClicked();
            _e->accept();
            return;
        }

        QWidget::mouseReleaseEvent(_e);
    }

    void MessageItem::paintEvent(QPaintEvent*)
    {
        assert(parent());

        QPainter p(this);

		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::TextAntialiasing);

        drawAvatar(p);

        drawMessageBubble(p);
        const auto lastStatus = getLastStatus();
        if (lastStatus != LastStatus::None)
        {
            drawLastStatusIcon(p,
                lastStatus,
                Data_->Sender_,
                Data_->SenderFriendly_,
                isOutgoing() ? 0 : MessageStyle::getTimeMaxWidth());
        }
    }

    void MessageItem::updateMessageBodyColor()
    {
        if (!MessageBody_)
        {
            return;
        }

        auto textColor = MessageStyle::getTextColor();

        QPalette palette;
        palette.setColor(QPalette::Text, textColor);
        MessageBody_->setPalette(palette);
    }

    void MessageItem::resizeEvent(QResizeEvent* _e)
    {
        HistoryControlPageItem::resizeEvent(_e);
    }

    void MessageItem::hideEvent(QHideEvent *)
    {
        if (!TimeWidget_)
            return;

        if (get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true))
        {
            TimeWidget_->hideAnimated();

            hideMessageStatus();
        }
    }

    void MessageItem::createMessageBody()
    {
        if (MessageBody_)
        {
            return;
        }

        QPalette palette;
        MessageBody_ = new TextEditEx(
            this,
            MessageStyle::getTextFont(),
            palette,
            false,
            false
        );
        updateMessageBodyColor();

        const QString styleSheet = qsl("background: transparent; selection-background-color: %1;")
            .arg(Utils::rgbaStringFromColor(Utils::getSelectionColor()));

        MessageBody_->document()->setDefaultStyleSheet(MessageStyle::getMessageStyle());
        MessageBody_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        MessageBody_->setFrameStyle(QFrame::NoFrame);
        MessageBody_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->horizontalScrollBar()->setEnabled(false);
        MessageBody_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->setOpenLinks(false);
        MessageBody_->setOpenExternalLinks(true);
        MessageBody_->setWordWrapMode(QTextOption::WordWrap);
        Utils::ApplyStyle(MessageBody_, styleSheet);
        MessageBody_->setFocusPolicy(Qt::NoFocus);
        MessageBody_->document()->setDocumentMargin(0);
        MessageBody_->setContextMenuPolicy(Qt::NoContextMenu);
        MessageBody_->setReadOnly(true);
        MessageBody_->setUndoRedoEnabled(false);

        connect(MessageBody_, &QTextBrowser::selectionChanged, [this]() { emit selectionChanged(); });
    }

    void MessageItem::updateSenderControlColor()
    {
        QColor color = theme() ? theme()->contact_name_.text_color_ : MessageStyle::getSenderColor();
        Sender_->setColor(color);
    }

    void MessageItem::createSenderControl()
    {
        if (Sender_)
        {
            return;
        }

        QColor color;
        Sender_ = new TextEmojiWidget(
            this,
            Fonts::appFont(Ui::MessageStyle::getSenderFont().pixelSize()),
            color
        );
        updateSenderControlColor();
    }

    void MessageItem::drawAvatar(QPainter& _p)
    {
        if (!Data_ || Data_->Sender_.isEmpty() || Data_->AvatarSize_ <= 0)
        {
            return;
        }
        auto avatar = MessageItemsAvatars::get(Data_->Sender_, Data_->SenderFriendly_, Data_->AvatarSize_, [this](){ parentWidget()->update(); });
        if (avatar.isNull() || !Data_->AvatarVisible_)
        {
            return;
        }

        const auto rect = getAvatarRect();
        if (rect.isValid())
        {
            _p.drawPixmap(rect, avatar);
        }
    }

    void MessageItem::drawMessageBubble(QPainter& _p)
    {
        if (ContentWidget_ && !ContentWidget_->isBlockElement())
        {
            return;
        }

        if (Bubble_.isEmpty())
        {
            return;
        }

        assert(!Bubble_.isEmpty());

        _p.fillPath(Bubble_, MessageStyle::getBodyBrush(Data_->isOutgoing(), isSelected(), theme()->get_id()));

		QColor qoute_color = QuoteAnimation_.quoteColor();
		if (qoute_color.isValid())
		{
			_p.fillPath(Bubble_, (QBrush(qoute_color)));
		}
    }

    QRect MessageItem::evaluateAvatarRect() const
    {
        QRect result(
            MessageStyle::getLeftMargin(isOutgoing()),
            evaluateTopContentMargin(),
            MessageStyle::getAvatarSize(),
            MessageStyle::getAvatarSize()
        );

        return result;
    }

    QRect MessageItem::evaluateBubbleGeometry(const QRect &_contentGeometry) const
    {
        assert(!_contentGeometry.isEmpty());

        auto bubbleGeometry(_contentGeometry);

        bubbleGeometry.setWidth(
            bubbleGeometry.width());

        return bubbleGeometry;
    }

    QRect MessageItem::evaluateContentHorGeometry(const int32_t contentWidth) const
    {
        assert(contentWidth > 0);

        auto width = contentWidth;

        const QRect result(
            evaluateLeftContentMargin(),
            -1,
            width,
            0);

        return result;
    }

    QRect MessageItem::evaluateContentVertGeometry() const
    {
        const auto contentHeight = evaluateDesiredContentHeight();

        const QRect result(
            -1,
            evaluateTopContentMargin(),
            0,
            contentHeight
        );

        return result;
    }

    int32_t MessageItem::evaluateContentWidth(const int32_t _widgetWidth) const
    {
        assert(_widgetWidth > 0);

        auto contentWidth = _widgetWidth;

        contentWidth -= evaluateLeftContentMargin();
        contentWidth -= evaluateRightContentMargin();

        return contentWidth;
    }

    int32_t MessageItem::evaluateDesiredContentHeight() const
    {
        auto height = MessageStyle::getMinBubbleHeight();

        if (MessageBody_)
        {
            assert(!ContentWidget_);

            const auto textHeight = MessageBody_->getTextSize().height();

            if (textHeight > 0)
            {
                if (platform::is_apple())
                {
                    height = std::max(getMessageTopPadding() + textHeight + getMessageBottomPadding(), MessageStyle::getMinBubbleHeight());
                }
                else
                {
                    height = std::max<int32_t>(
                        height,
                        textHeight
                    );

                    height += getMessageTopPadding();

                    height = std::max<int32_t>(Utils::applyMultilineTextFix(textHeight, height), MessageStyle::getMinBubbleHeight());
                }
            }
        }

        if (ContentWidget_)
        {
            assert(!MessageBody_);

            height = std::max(height, ContentWidget_->height());
        }

        return height;
    }

    int32_t MessageItem::evaluateLeftContentMargin() const
    {
        const auto isOutgoing = this->isOutgoing();
        auto leftContentMargin = MessageStyle::getLeftMargin(isOutgoing);
        if (isOutgoing)
            leftContentMargin += MessageStyle::getTimeMaxWidth();

        if (isAvatarVisible())
        {
            leftContentMargin += MessageStyle::getAvatarSize();
            leftContentMargin += MessageStyle::getAvatarRightMargin();
        }

        return leftContentMargin;
    }

    int32_t MessageItem::evaluateRightContentMargin() const
    {
        const auto isOutgoing = this->isOutgoing();
        auto rightContentMargin = MessageStyle::getRightMargin(isOutgoing);
        if (!isOutgoing)
            rightContentMargin += MessageStyle::getTimeMaxWidth();
        return rightContentMargin;
    }

    int32_t MessageItem::evaluateTopContentMargin() const
    {
        auto topContentMargin = MessageStyle::getTopMargin(hasTopMargin());

        if (Data_->SenderVisible_)
        {
            topContentMargin += MessageStyle::getSenderHeight();
            topContentMargin += MessageStyle::getSenderBottomMargin();
        }

        return topContentMargin;
    }

    QRect MessageItem::getAvatarRect() const
    {
        assert(isAvatarVisible());

        if (!AvatarRect_.isValid())
        {
            AvatarRect_ = evaluateAvatarRect();
            assert(AvatarRect_.isValid());
        }

        return AvatarRect_;
    }

    bool MessageItem::isAvatarVisible() const
    {
        return !isOutgoing();
    }

    bool MessageItem::isBlockItem() const
    {
        assert((bool)MessageBody_ ^ (bool)ContentWidget_);

        return (MessageBody_ || ContentWidget_->isBlockElement());
    }

    bool MessageItem::isOutgoing() const
    {
        if (!Data_)
            return false;

        return Data_->isOutgoing();
    }

    bool MessageItem::isFileSharing() const
    {
        if (!ContentWidget_)
        {
            return false;
        }

        return qobject_cast<HistoryControl::FileSharingWidget*>(ContentWidget_);
    }

    bool MessageItem::isOverAvatar(const QPoint& _p) const
    {
        return (
            isAvatarVisible() &&
            getAvatarRect().contains(_p)
        );
    }

    void MessageItem::manualUpdateGeometry(const int32_t _widgetWidth)
    {
        assert(_widgetWidth > 0);

        const auto contentWidth = evaluateContentWidth(_widgetWidth);

        const auto enoughSpace = (contentWidth > 0);
        if (!enoughSpace)
        {
            return;
        }

        setUpdatesEnabled(false);

        const auto contentHorGeometry = evaluateContentHorGeometry(contentWidth);

        updateMessageBodyHorGeometry(contentHorGeometry);

        updateContentWidgetHorGeometry(contentHorGeometry);

        const auto contentVertGeometry = evaluateContentVertGeometry();
        QRect contentGeometry(
            contentHorGeometry.left(),
            contentVertGeometry.top(),
            contentHorGeometry.width(),
            contentVertGeometry.height());

        assert(!contentGeometry.isEmpty());

        updateMessageBodyFullGeometry(contentGeometry);
        updateSenderGeometry();

        if (ContentWidget_)
        {
            const auto previewTopLeft = ContentWidget_->getLastPreviewGeometry().topLeft();
            if (!previewTopLeft.isNull())
            {
                const auto x = ContentWidget_->mapToParent(previewTopLeft);
                contentGeometry.setLeft(x.x());
            }
        }

        if (!ContentWidget_)
            updateBubbleGeometry(evaluateBubbleGeometry(contentGeometry));
        else
            updateBubbleGeometry(contentGeometry);

        updateTimeGeometry(contentGeometry);
        if (isAvatarVisible())
            AvatarRect_ = evaluateAvatarRect();

        setUpdatesEnabled(true);
    }

    QString MessageItem::contentClass() const
    {
        if (MessageBody_)
        {
            static const auto N_OF_LEFTMOST_CHARS = 5;

            const auto &text = Data_->Text_;

            const auto hellip = ql1s(text.length() > N_OF_LEFTMOST_CHARS ? "..." : "");
            return ql1s("text(") % text.leftRef(N_OF_LEFTMOST_CHARS) % hellip % ql1c(')');
        }

        return ContentWidget_->metaObject()->className();
    }

    void MessageItem::updateBubbleGeometry(const QRect &_bubbleGeometry)
    {
        assert(!_bubbleGeometry.isEmpty());

        Bubble_ = Utils::renderMessageBubble(_bubbleGeometry, MessageStyle::getBorderRadius(), Data_->isOutgoing());
        assert(!Bubble_.isEmpty());
    }

    void MessageItem::updateContentWidgetHorGeometry(const QRect &_contenHorGeometry)
    {
        assert(_contenHorGeometry.width() > 0);

        if (!ContentWidget_)
        {
            return;
        }

        auto left = _contenHorGeometry.left();
        if (ContentWidget_->isBlockElement())
            left += MessageStyle::getBubbleHorPadding();

        ContentWidget_->move(
            left,
            evaluateTopContentMargin()
        );

        if (ContentWidget_->isBlockElement())
        {
            auto blockWidth = _contenHorGeometry.width();
            blockWidth -= (MessageStyle::getBubbleHorPadding() * 2);

            ContentWidget_->setFixedWidth(blockWidth);

            return;
        }

        auto width = _contenHorGeometry.width();

        const auto maxWidgetWidth = ContentWidget_->maxWidgetWidth();

        assert(width > 0);

        if (maxWidgetWidth == -1)
        {
            ContentWidget_->setFixedWidth(width);
            return;
        }

        if (width > maxWidgetWidth)
            width = maxWidgetWidth;

        ContentWidget_->setFixedWidth(width);
    }

    void MessageItem::updateMessageBodyHorGeometry(const QRect &_bubbleHorGeometry)
    {
        assert(_bubbleHorGeometry.width() > 0);

        if (!MessageBody_)
        {
            return;
        }

        const auto bubblePadding = MessageStyle::getBubbleHorPadding();

        auto messageBodyWidth = _bubbleHorGeometry.width();
        messageBodyWidth -= bubblePadding;
        messageBodyWidth -= bubblePadding;

        const auto widthChanged = (messageBodyWidth != MessageBody_->getTextSize().width());
        if (!widthChanged)
        {
            return;
        }

        MessageBody_->setFixedWidth(messageBodyWidth);
        MessageBody_->document()->setTextWidth(messageBodyWidth);
    }

    void MessageItem::updateMessageBodyFullGeometry(const QRect &bubbleRect)
    {
        if (!MessageBody_)
        {
            return;
        }

        const auto textSize = MessageBody_->getTextSize();
        const QRect messageBodyGeometry(
            bubbleRect.left() + MessageStyle::getBubbleHorPadding(),
            bubbleRect.top() + getMessageTopPadding(),
            textSize.width(),
            textSize.height()
        );
        assert(!messageBodyGeometry.isEmpty());

        MessageBody_->setGeometry(messageBodyGeometry);
    }

    void MessageItem::updateSenderGeometry()
    {
        if (!Sender_)
        {
            return;
        }

        if (!Data_->SenderVisible_)
        {
            Sender_->setVisible(false);
            return;
        }

        Sender_->move(
            MessageStyle::getLeftMargin(isOutgoing()),
            MessageStyle::getTopMargin(hasTopMargin())
        );

        Sender_->setVisible(true);
    }

    void MessageItem::updateTimeGeometry(const QRect &_contentGeometry)
    {
        const auto timeWidgetSize = TimeWidget_->sizeHint();

        QPoint posHint;

        assert(posHint.x() >= 0);
        assert(posHint.y() >= 0);

        auto timeX = 0;

        if (posHint.x() > 0) // dead code
        {
            timeX = posHint.x();
            timeX += evaluateLeftContentMargin();
            timeX += MessageStyle::getTimeMarginX();
        }
        else
        {
            if (isOutgoing())
                timeX = _contentGeometry.left() - timeWidgetSize.width() - MessageStyle::getTimeMarginX();
            else
                timeX = _contentGeometry.right() + MessageStyle::getTimeMarginX();
        }

        auto timeY = 0;

        if (posHint.y() > 0) // dead code
        {
            timeY = posHint.y();
            timeY += evaluateTopContentMargin();
        }
        else
        {
            timeY = _contentGeometry.bottom();
        }

        timeY -= MessageStyle::getTimeMarginY();
        timeY -= timeWidgetSize.height();

        const QRect timeGeometry(
            timeX,
            timeY,
            MessageStyle::getTimeMaxWidth(),
            timeWidgetSize.height()
        );

        TimeWidget_->setGeometry(timeGeometry);
    }

	void MessageItem::selectByPos(const QPoint& _pos, bool _doNotSelectImage)
	{
        if (!isSelection_)
        {
			isSelection_ = true;
			startSelectY_ = _pos.y();
        }

		if (MessageBody_)
		{
			MessageBody_->selectByPos(_pos);
            return;
		}

        assert(ContentWidget_);
        if (!ContentWidget_)
        {
            return;
        }

        QRect widgetRect;
        bool selected = false;
        if (ContentWidget_->hasTextBubble())
        {
            selected = ContentWidget_->selectByPos(_pos);
            widgetRect = QRect(mapToGlobal(ContentWidget_->rect().topLeft()), mapToGlobal(ContentWidget_->rect().bottomRight()));
        }
        else
        {
            widgetRect = QRect(mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
        }

        const auto isCursorOverWidget = (
            (widgetRect.top() <= _pos.y()) &&
            (widgetRect.bottom() >= _pos.y())
        );
		if (isCursorOverWidget && !selected && !_doNotSelectImage)
		{
			if (!isSelected())
			{
				select();
			}

            return;
		}

        const auto distanceToWidgetTop = std::abs(_pos.y() - widgetRect.top());
        const auto distanceToWidgetBottom = std::abs(_pos.y() - widgetRect.bottom());
        const auto isCursorCloserToTop = (distanceToWidgetTop < distanceToWidgetBottom);

        if (Direction_ == SelectDirection::NONE)
        {
            Direction_ = (isCursorCloserToTop ? SelectDirection::DOWN : SelectDirection::UP);
        }

        if (selected)
            return;

        const auto isDirectionDown = (Direction_ == SelectDirection::DOWN);
        const auto isDirectionUp = (Direction_ == SelectDirection::UP);

		if (isSelected())
		{
            const auto needToClear = (
                (isCursorCloserToTop && isDirectionDown) ||
                (!isCursorCloserToTop && isDirectionUp)
            );

			if (needToClear && (startSelectY_ < widgetRect.top() || startSelectY_ > widgetRect.bottom()))
            {
				clearSelection();
				isSelection_ = true;
            }

            return;
		}

        const auto needToSelect = (
            (isCursorCloserToTop && isDirectionUp) ||
            (!isCursorCloserToTop && isDirectionDown)
        );

		if (needToSelect)
        {
            select();
        }
	}

	void MessageItem::select()
	{
		HistoryControlPageItem::select();

		if (ContentWidget_)
        {
			ContentWidget_->select(true);
        }
	}

	void MessageItem::clearSelection()
	{
		isSelection_ = false;
        HistoryControlPageItem::clearSelection();

		if (MessageBody_)
        {
			MessageBody_->clearSelection();
        }

		if (ContentWidget_)
		{
			Direction_ = SelectDirection::NONE;

            if (ContentWidget_->hasTextBubble())
            {
                ContentWidget_->clearSelection();
            }

			if (isMessageBubbleVisible())
			{
			}
			else
			{
				ContentWidget_->select(false);
			}
		}
	}

	QString MessageItem::selection(bool _textonly/* = false*/) const
	{
        QString selectedText;
        if (ContentWidget_)
            selectedText = ContentWidget_->selectedText();

        if (!MessageBody_ && !isSelected() && selectedText.isEmpty())
        {
            return QString();
        }

		if (_textonly && MessageBody_ && !MessageBody_->isAllSelected())
        {
			return MessageBody_->selection();
        }

		QString displayName;

		if (isOutgoing())
        {
			displayName = MyInfo()->friendlyName();
        }
		else
        {
			displayName = (
                Data_->Chat_ ?
                    Sender_->text() :
                    Logic::getContactListModel()->getDisplayName(Data_->AimId_)
            );
        }

        QString format = (
            _textonly ?
                QString() :
                qsl("%1 (%2):\n").arg(
                    displayName,
                    QDateTime::fromTime_t(Data_->Time_).toString(qsl("dd.MM.yyyy hh:mm"))
                )
        );

        if (MessageBody_)
		{
            const auto bodySelection = MessageBody_->selection();
            if (!bodySelection.isEmpty())
            {
                format += bodySelection;
                return format;
            }
            return QString();
		}

        assert(isSelected() || !selectedText.isEmpty());

		if (!Data_->StickerText_.isEmpty())
        {
			format += Data_->StickerText_;
        }
		else
        {
            format += ContentWidget_->toString();
        }

        if (!_textonly && !format.isEmpty())
        {
            format += ql1c('\n');
        }

        return format;
	}

    bool MessageItem::isSelected() const
    {
        return HistoryControlPageItem::isSelected();
    }

    bool MessageItem::isTextSelected() const
    {
        return MessageBody_
            && !MessageBody_->selection().isEmpty();
    }

    void MessageItem::avatarClicked()
    {
        Utils::openDialogOrProfile(Data_->Sender_, Data_->AimId_);
    }

    void MessageItem::forwardRoutine()
    {
        if (isFileSharing())
        {
            auto p = qobject_cast<HistoryControl::FileSharingWidget*>(ContentWidget_);
            if (p)
                p->shareRoutine();
        }
        else
        {
            emit forward({ getQuote(true) });
        }
    }

    void MessageItem::setNotAuth(const bool _isNotAuth)
    {
        isNotAuth_ = _isNotAuth;
    }

    bool MessageItem::isNotAuth()
    {
        return isNotAuth_;
    }

	void MessageItem::setQuoteSelection()
	{
		QuoteAnimation_.startQuoteAnimation();
		if (ContentWidget_)
			ContentWidget_->startQuoteAnimation();
	}

    void MessageItem::setMentions(Data::MentionMap _mentions)
    {
        Data_->Mentions_ = std::move(_mentions);
    }

	void MessageItem::menu(QAction* _action)
	{
		const auto params = _action->data().toMap();
		const auto command = params[qsl("command")].toString();

        if (command == ql1s("dev:copy_message_id"))
        {
            const auto idStr = QString::number(getId());

            QApplication::clipboard()->setText(idStr);
        }

        QString displayName;

        if (isOutgoing())
        {
            displayName = MyInfo()->friendlyName();
        }
        else
        {
            displayName = (
                Data_->Chat_ ?
                    Sender_->text() :
                    Logic::getContactListModel()->getDisplayName(Data_->AimId_)
            );
        }

        const bool isCopyCommand = command == ql1s("copy");
        auto format = (
            isCopyCommand ?
                QString() :
                qsl("%1 (%2):\n").arg(
                    displayName,
                    QDateTime::fromTime_t(Data_->Time_).toString(qsl("dd.MM.yyyy hh:mm"))
                )
        );

        if (MessageBody_)
        {
            format += MessageBody_->getPlainText();
        }
        else
        {
            if (!Data_->StickerText_.isEmpty())
            {
                format += Data_->StickerText_;
            }
            else
            {
                format += ContentWidget_->toString();
            }
        }

		if (isCopyCommand)
		{
			emit copy(format);
		}
		else if (command == ql1s("quote"))
		{
            emit quote({ getQuote(true) });
		}
        else if (command == ql1s("forward"))
        {
            forwardRoutine();
        }
        else if (command == ql1s("copy_link"))
        {
            emit copy(ContentWidget_->toLink());
        }
        else if (command == ql1s("open_in_browser"))
        {
            QDesktopServices::openUrl(ContentWidget_->toLink());
        }
        else if (command == ql1s("copy_file"))
        {
            ContentWidget_->copyFile();
        }
        else if (command == ql1s("save_as"))
        {
            ContentWidget_->saveAs();
        }
        else if ((command == ql1s("delete_all")) || (command == ql1s("delete")))
        {
            const auto isPendingMessage = (Data_->Id_ <= -1);
            if (isPendingMessage)
            {
                return;
            }

            QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                QT_TRANSLATE_NOOP("popup_window", "YES"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "Delete message"),
                nullptr
            );

            if (confirm && Data_)
            {
                const auto is_shared = (command == ql1s("delete_all"));
                GetDispatcher()->deleteMessages({ Data_->Id_ }, Data_->AimId_, is_shared);
            }
        }
	}

    void MessageItem::onRecreateAvatarRect()
    {
        AvatarRect_ = QRect();
    }

    void MessageItem::showHiddenControls()
    {
        if (TimeWidget_ && isVisible() && get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true))
            TimeWidget_->showAnimated();

        showMessageStatus();
    }

    void MessageItem::hideHiddenControls()
    {
        const bool canHide = TimeWidget_
            && !bubbleHovered_
            && isVisible()
            && get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true);

        if (canHide)
            TimeWidget_->hideAnimated();

        hideMessageStatus();
    }

    void MessageItem::setTimestampHoverEnabled(const bool _enabled)
    {
        timestampHoverEnabled_ = _enabled;
    }

    bool MessageItem::isMessageBubbleVisible() const
	{
		if (!ContentWidget_)
		{
			return true;
		}

		return ContentWidget_->isBlockElement();
	}

	void MessageItem::setMessage(const QString& _message)
	{
        assert(!ContentWidget_);

		Data_->Text_ = _message;

        if (!parent())
        {
            return;
        }

        setUpdatesEnabled(false);

        createMessageBody();
        MessageBody_->setMentions(Data_->Mentions_);

        {
            const QSignalBlocker blocker(MessageBody_->verticalScrollBar());
            MessageBody_->document()->clear();

            const bool showLinks = (!isNotAuth() || isOutgoing());
            Logic::Text4Edit(_message, *MessageBody_, Logic::Text2DocHtmlMode::Escape, showLinks, true);
        }

        MessageBody_->setVisible(true);

        Layout_->setDirty();

        setUpdatesEnabled(true);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateGeometry();
        update();
	}

    void MessageItem::setLastStatus(LastStatus _lastStatus)
    {
        if (getLastStatus() != _lastStatus)
        {
            HistoryControlPageItem::setLastStatus(_lastStatus);
            Layout_->setDirty();
            updateGeometry();
            update();
        }
    }

    void MessageItem::setDeliveredToServer(const bool _delivered)
    {
        setOutgoing(Data_->isOutgoing(), true, Data_->Chat_);
        setTime(Data_->Time_);
    }

	void MessageItem::setOutgoing(const bool _isOutgoing, const bool _isDeliveredToServer, const bool _isMChat, const bool _isInit)
	{
        Data_->Outgoing_.Outgoing_ = _isOutgoing;
        Data_->Outgoing_.Set_ = true;
        Data_->Chat_ = _isMChat;

        if (_isOutgoing && (Data_->deliveredToServer_ != _isDeliveredToServer || _isInit))
            Data_->deliveredToServer_ = _isDeliveredToServer;
	}

    void MessageItem::setMchatSenderAimId(const QString& _senderAimId)
    {
        MessageSenderAimId_ = _senderAimId;
    }

	void MessageItem::setMchatSender(const QString& _sender)
	{
        createSenderControl();

		Sender_->setText(_sender);
        Data_->SenderFriendly_ = _sender;

        auto senderName = _sender;
        Utils::removeLineBreaks(InOut senderName);

		Data_->SenderVisible_ = (!senderName.isEmpty() && Data_->AvatarVisible_);

        Sender_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

        updateGeometry();
	}

	void MessageItem::setTime(const int32_t _time)
	{
		Data_->Time_ = _time;

        TimeWidget_->setTime(_time);

        updateTimeGeometry(geometry());

        TimeWidget_->showIfNeeded();
	}

	void MessageItem::setTopMargin(const bool _value)
	{
        const auto topMarginUpdated = (hasTopMargin() != _value);
        if (topMarginUpdated)
        {
            AvatarRect_ = QRect();
        }

        Data_->IndentBefore_ = _value;

        HistoryControlPageItem::setTopMargin(_value);
	}

    themes::themePtr MessageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(Data_.get() ? Data_->AimId_ : getAimid());
    }

	void MessageItem::setContentWidget(HistoryControl::MessageContentWidget* _widget)
	{
		assert(_widget);
		assert(!ContentWidget_);
        assert(!MessageBody_);

		ContentWidget_ = _widget;

        connect(ContentWidget_, &HistoryControl::MessageContentWidget::stateChanged, this, &MessageItem::updateData, Qt::UniqueConnection);

        if (!parent())
        {
            return;
        }

        auto success = QObject::connect(
            ContentWidget_,
            &HistoryControl::MessageContentWidget::forcedLayoutUpdatedSignal,
            this,
            [this]
            {
                Layout_->setDirty();
            },
            Qt::DirectConnection
        );
        assert(success);

        ContentWidget_->setParent(this);
        ContentWidget_->show();

        TimeWidget_->raise();

		Utils::grabTouchWidget(ContentWidget_);

        updateGeometry();
        update();
	}

	void MessageItem::setStickerText(const QString& _text)
	{
		Data_->StickerText_ = _text;
	}

	void MessageItem::setDate(const QDate& _date)
	{
		Data_->Date_ = _date;
	}

	bool MessageItem::selected()
	{
		if (!MessageBody_)
		{
			return false;
		}

		return !MessageBody_->textCursor().selectedText().isEmpty();
	}

	QDate MessageItem::date() const
	{
		return Data_->Date_;
	}

	qint64 MessageItem::getId() const
	{
		return Data_->Id_;
	}

    bool MessageItem::isRemovable() const
	{
		return (
            ContentWidget_ ?
                ContentWidget_->canUnload() :
                true
        );
	}

    bool MessageItem::isUpdateable() const
    {
        return (
            ContentWidget_ ?
                ContentWidget_->canReplace() :
                true
        );
    }

	QString MessageItem::toLogString() const
	{
        const auto contentWidgetLogStr = ContentWidget_ ? ContentWidget_->toLogString() : QString();
		if (MessageBody_)
		{
			const auto text = MessageBody_->getPlainText().replace(ql1c('\n'), ql1s("\\n"));
            return contentWidgetLogStr % ql1s("	text=<") % text % ql1c('>');
		}
        return contentWidgetLogStr;
	}

    bool MessageItem::updateData()
    {
        updateMessageBodyColor();

        updateSenderControlColor();

        update();

        return true;
    }

	void MessageItem::updateWith(MessageItem& _messageItem)
	{
        auto &data = _messageItem.Data_;
        assert(data);
        assert(Data_ != data);

		Data_ = std::move(data);
        setAimid(Data_->AimId_);

        if (_messageItem.MessageBody_)
        {
            assert(!Data_->Text_.isEmpty());
            assert(!_messageItem.ContentWidget_);

            if (ContentWidget_)
            {
                ContentWidget_->deleteLater();
                ContentWidget_ = nullptr;
            }

            setMessage(Data_->Text_);
        }

        if (_messageItem.ContentWidget_)
        {
            assert(Data_->Text_.isEmpty());
            assert(!_messageItem.MessageBody_);

            if (MessageBody_)
            {
                MessageBody_->deleteLater();
                MessageBody_ = nullptr;
            }

            if (ContentWidget_ && ContentWidget_->canReplace())
            {
                ContentWidget_->deleteLater();
                ContentWidget_ = nullptr;

                setContentWidget(_messageItem.ContentWidget_);
                _messageItem.ContentWidget_ = nullptr;
            }
        }

		setTime(Data_->Time_);
		setOutgoing(Data_->isOutgoing(), Data_->deliveredToServer_, Data_->Chat_);
		setTopMargin(Data_->IndentBefore_);

		if (Data_->AvatarVisible_)
        {
            loadAvatar(Utils::scale_bitmap(MessageStyle::getAvatarSize()));
        }
		else
        {
			setAvatarVisible(false);
        }

        setMchatSenderAimId(Data_->Sender_);
        setMchatSender(Data_->SenderFriendly_);
	}

	std::shared_ptr<MessageData> MessageItem::getData() const
	{
		return Data_;
	}

    namespace
    {
        int32_t getMessageTopPadding()
        {
            return Utils::scale_value(4);
        }

        int32_t getMessageBottomPadding()
        {
            return Utils::scale_value(8);
        }

        QMap<QString, QVariant> makeData(const QString& _command)
        {
            QMap<QString, QVariant> result;
            result[qsl("command")] = _command;
            return result;
        }
    }
}


