#include "stdafx.h"

#include "../../../app_config.h"
#include "../../../cache/avatars/AvatarStorage.h"
#include "../../../cache/themes/themes.h"
#include "../../../controls/ContextMenu.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../gui_settings.h"
#include "../../../theme_settings.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/log/log.h"
#include "../../../utils/UrlParser.h"
#include "../../../my_info.h"
#include "../StickerInfo.h"

#include "../../contact_list/ContactList.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/SelectionContactsForGroupChat.h"

#include "../../MainPage.h"

#include "../ActionButtonWidget.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageItemLayout.h"
#include "IItemBlockLayout.h"
#include "IItemBlock.h"
#include "Selection.h"
#include "Style.h"
#include "TextBlock.h"

#include "ComplexMessageItem.h"
#include "../../../contextMenuEvent.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    BlockSelectionType evaluateBlockSelectionType(const QRect &blockGeometry, const QPoint &selectionTop, const QPoint &selectionBottom);

    QMap<QString, QVariant> makeData(const QString& command, const QString &arg = QString());
}

ComplexMessageItem::ComplexMessageItem(
    QWidget *parent,
    const int64_t id,
    const QDate date,
    const QString &chatAimid,
    const QString &senderAimid,
    const QString &senderFriendly,
    const QString &sourceText,
    const Data::MentionMap& _mentions,
    const bool isOutgoing)
    : MessageItemBase(parent)

    , ChatAimid_(chatAimid)
    , Date_(date)
    , FullSelectionType_(BlockSelectionType::Invalid)
    , HoveredBlock_(nullptr)
    , Id_(id)
    , Initialized_(false)
    , IsOutgoing_(isOutgoing)
    , IsDeliveredToServer_(true)
    , Layout_(nullptr)
    , MenuBlock_(nullptr)
    , MouseLeftPressedOverAvatar_(false)
    , MouseRightPressedOverItem_(false)
    , Sender_(nullptr)
    , SenderAimid_(senderAimid)
    , SenderFriendly_(senderFriendly)
    , ShareButton_(nullptr)
    , SourceText_(sourceText)
    , TimeWidget_(nullptr)
    , Time_(-1)
    , bQuoteAnimation_(false)
    , bObserveToSize_(false)
    , timestampHoverEnabled_(true)
    , bubbleHovered_(false)
    , mentions_(_mentions)
{
    assert(Id_ >= -1);
    assert(!SenderAimid_.isEmpty());
    assert(!SenderFriendly_.isEmpty());
    assert(!ChatAimid_.isEmpty());
    assert(Date_.isValid());

    Layout_ = new ComplexMessageItemLayout(this);
    setLayout(Layout_);

    connectSignals();
}

void ComplexMessageItem::clearSelection()
{
    assert(!Blocks_.empty());

    FullSelectionType_ = BlockSelectionType::None;

    for (auto block : Blocks_)
    {
        block->clearSelection();
    }
}

QString ComplexMessageItem::formatRecentsText() const
{
    assert(!Blocks_.empty());

    if (Blocks_.empty())
    {
        return qsl("warning: invalid message block");
    }

    QString textOnly;

    unsigned textBlocks = 0, fileSharingBlocks = 0, linkBlocks = 0, quoteBlocks = 0, otherBlocks = 0;

    for (auto b : Blocks_)
    {
        switch (b->getContentType())
        {
            case IItemBlock::Text:
                if (!b->getTrimmedText().isEmpty())
                {
                    textOnly += b->formatRecentsText();
                    ++textBlocks;
                }
                break;

            case IItemBlock::Link:
                ++linkBlocks;
                textOnly += b->formatRecentsText();
                break;

            case IItemBlock::FileSharing:
                ++fileSharingBlocks;
                break;

            case IItemBlock::Quote:
                ++quoteBlocks;
                break;

            case IItemBlock::Other:
            default:
                ++otherBlocks;
                textOnly += b->formatRecentsText();
                break;
        }
    }

    QString res;
    if (((linkBlocks || quoteBlocks) && !textOnly.isEmpty()) ||
        (fileSharingBlocks && (linkBlocks || otherBlocks || textBlocks || quoteBlocks) && !textOnly.isEmpty()))
    {
        res = textOnly;
    }
    else
    {
        for (const auto& _block : Blocks_)
            res += _block->formatRecentsText();
    }

    if (!mentions_.empty())
        return Utils::convertMentions(res, mentions_);

    return res;
}

qint64 ComplexMessageItem::getId() const
{
    return Id_;
}

QString ComplexMessageItem::getQuoteHeader() const
{
    const auto displayName = getSenderFriendly();
    const auto timestamp = QDateTime::fromTime_t(Time_).toString(qsl("dd.MM.yyyy hh:mm"));
    return qsl("%1 (%2):\n").arg(displayName, timestamp);
}

QString ComplexMessageItem::getSelectedText(const bool isQuote) const
{
    QString text;
    const auto isEmptySelection = (FullSelectionType_ == BlockSelectionType::None);
    if (isEmptySelection && !isSelected())
    {
        return text;
    }
    text.reserve(1024);

    if (isQuote)
    {
        text += getQuoteHeader();
    }

    const auto isFullSelection = (FullSelectionType_ == BlockSelectionType::Full);
    if (isFullSelection)
    {
        text += SourceText_;
        return text;
    }

    const auto selectedText = getBlocksText(Blocks_, true, isQuote);
    if (selectedText.isEmpty())
    {
        return text;
    }

    text += selectedText;

    return text;
}

const QString& ComplexMessageItem::getChatAimid() const
{
    return ChatAimid_;
}

QDate ComplexMessageItem::getDate() const
{
    assert(Date_.isValid());
    return Date_;
}

const QString& ComplexMessageItem::getSenderAimid() const
{
    assert(!SenderAimid_.isEmpty());
    return SenderAimid_;
}

std::shared_ptr<const themes::theme> ComplexMessageItem::getTheme() const
{
    return get_qt_theme_settings()->themeForContact(ChatAimid_);
}

bool ComplexMessageItem::isOutgoing() const
{
    return IsOutgoing_;
}

bool ComplexMessageItem::isSimple() const
{
    return (Blocks_.size() == 1 && Blocks_[0]->isSimple());
}

bool ComplexMessageItem::isUpdateable() const
{
    return true;
}

void ComplexMessageItem::onBlockSizeChanged()
{
    Layout_->onBlockSizeChanged();

    updateShareButtonGeometry();

    updateGeometry();

    update();
}

void ComplexMessageItem::onHoveredBlockChanged(IItemBlock *newHoveredBlock)
{
    if (newHoveredBlock &&
        !newHoveredBlock->isSharingEnabled())
    {
        if (newHoveredBlock->containSharingBlock())
            newHoveredBlock = HoveredBlock_;
        else
            newHoveredBlock = nullptr;
    }
    const auto hoveredBlockChanged = (newHoveredBlock != HoveredBlock_);
    if (!hoveredBlockChanged)
        return;

    HoveredBlock_ = newHoveredBlock;

    updateShareButtonGeometry();
}

void ComplexMessageItem::onStyleChanged()
{
    updateSenderControlColor();
}

void ComplexMessageItem::onActivityChanged(const bool isActive)
{
    for (auto block : Blocks_)
    {
        block->onActivityChanged(isActive);
    }

    const auto isInit = (isActive && !Initialized_);
    if (isInit)
    {
        Initialized_ = true;
        initialize();
    }
}

void ComplexMessageItem::onVisibilityChanged(const bool isVisible)
{
    for (auto block : Blocks_)
    {
        block->onVisibilityChanged(isVisible);
    }
}

void ComplexMessageItem::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    for (auto block : Blocks_)
    {
        block->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    }
}


void ComplexMessageItem::replaceBlockWithSourceText(IItemBlock *block)
{
    assert(block);
    assert(!Blocks_.empty());

    const auto isMenuBlockReplaced = (MenuBlock_ == block);
    if (isMenuBlockReplaced)
    {
        cleanupMenu();
    }

    const auto isHoveredBlockReplaced = (HoveredBlock_ == block);
    if (isHoveredBlockReplaced)
    {
        onHoveredBlockChanged(nullptr);
    }

    for (auto b : Blocks_)
    {
        if (b->replaceBlockWithSourceText(block))
        {
            Layout_->onBlockSizeChanged();
            return;
        }
    }

    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
    {
        assert(!"block is missing");
        return;
    }

    auto& existingBlock = *iter;
    assert(existingBlock);

    auto textBlock = new TextBlock(
        this,
        existingBlock->getSourceText());

    textBlock->onVisibilityChanged(true);
    textBlock->onActivityChanged(true);

    textBlock->show();

    existingBlock->deleteLater();
    existingBlock = textBlock;

    Layout_->onBlockSizeChanged();
}

void ComplexMessageItem::removeBlock(IItemBlock *block)
{
    assert(block);
    assert(!Blocks_.empty());

    const auto isMenuBlockReplaced = (MenuBlock_ == block);
    if (isMenuBlockReplaced)
    {
        cleanupMenu();
    }

    const auto isHoveredBlockReplaced = (HoveredBlock_ == block);
    if (isHoveredBlockReplaced)
    {
        onHoveredBlockChanged(nullptr);
    }

    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
    {
        assert(!"block is missing");
        return;
    }

    Blocks_.erase(iter);

    block->deleteLater();

    Layout_->onBlockSizeChanged();
}

void ComplexMessageItem::selectByPos(const QPoint& from, const QPoint& to)
{
    assert(!Blocks_.empty());

    const auto isEmptySelection = (from.y() == to.y());
    if (isEmptySelection)
    {
        return;
    }

    const auto isSelectionTopToBottom = (from.y() <= to.y());

    const auto &topPoint = (isSelectionTopToBottom ? from : to);
    const auto &bottomPoint = (isSelectionTopToBottom ? to : from);
    assert(topPoint.y() <= bottomPoint.y());

    const auto &blocksRect = Layout_->getBubbleRect();
    assert(!blocksRect.isEmpty());

    const QRect globalBlocksRect(
        mapToGlobal(blocksRect.topLeft()),
        mapToGlobal(blocksRect.bottomRight()));

    const auto isTopPointAboveBlocks = (topPoint.y() <= globalBlocksRect.top());
    const auto isTopPointBelowBlocks = (topPoint.y() >= globalBlocksRect.bottom());

    const auto isBottomPointAboveBlocks = (bottomPoint.y() <= globalBlocksRect.top());
    const auto isBottomPointBelowBlocks = (bottomPoint.y() >= globalBlocksRect.bottom());

    FullSelectionType_ = BlockSelectionType::Invalid;

    const auto isNotSelected = (
        (isTopPointAboveBlocks && isBottomPointAboveBlocks) ||
        (isTopPointBelowBlocks && isBottomPointBelowBlocks));
    if (isNotSelected)
    {
        FullSelectionType_ = BlockSelectionType::None;
    }

    const auto isFullSelection = (isTopPointAboveBlocks && isBottomPointBelowBlocks);
    if (isFullSelection)
    {
        FullSelectionType_ = BlockSelectionType::Full;
    }

    for (auto block : Blocks_)
    {
        const auto blockLayout = block->getBlockLayout();
        if (!blockLayout)
        {
            continue;
        }

        const auto blockGeometry = blockLayout->getBlockGeometry();

        const QRect globalBlockGeometry(
            mapToGlobal(blockGeometry.topLeft()),
            mapToGlobal(blockGeometry.bottomRight()));

        auto selectionType = BlockSelectionType::Invalid;

        if (isFullSelection)
        {
            selectionType = BlockSelectionType::Full;
        }
        else if (isNotSelected)
        {
            selectionType = BlockSelectionType::None;
        }
        else
        {
            selectionType = evaluateBlockSelectionType(globalBlockGeometry, topPoint, bottomPoint);
        }

        assert(selectionType != BlockSelectionType::Invalid);

        const auto blockNotSelected = (selectionType == BlockSelectionType::None);
        if (blockNotSelected)
        {
            block->clearSelection();
            continue;
        }

        block->selectByPos(topPoint, bottomPoint, selectionType);
    }
}

void ComplexMessageItem::setHasAvatar(const bool value)
{
    HistoryControlPageItem::setHasAvatar(value);

    if (!isOutgoing() && value)
    {
        loadAvatar();
    }
    else
    {
        Avatar_.reset();
    }

    onBlockSizeChanged();
}

void ComplexMessageItem::setItems(IItemBlocksVec blocks)
{
    assert(Blocks_.empty());
    assert(!blocks.empty());
    assert(!ShareButton_);
    assert(
        std::all_of(
            blocks.cbegin(),
            blocks.cend(),
            [](IItemBlock *block) { return block; }));

    Blocks_ = std::move(blocks);

    initializeShareButton();
}

void ComplexMessageItem::setMchatSender(const QString& sender)
{
    assert(!sender.isEmpty());

    if (isOutgoing() || Sender_)
    {
        return;
    }

    createSenderControl();

    Sender_->setText(sender);

    Sender_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    Sender_->setVisible(false);
}

void ComplexMessageItem::setLastStatus(LastStatus _lastStatus)
{
    if (getLastStatus() != _lastStatus)
    {
        HistoryControlPageItem::setLastStatus(_lastStatus);
        onBlockSizeChanged();
    }
}

void ComplexMessageItem::setTime(const int32_t time)
{
    assert(time >= -1);
    assert(Time_ == -1);

    Time_ = time;

    assert(!TimeWidget_);
    TimeWidget_ = new MessageTimeWidget(this);

    TimeWidget_->setContact(getAimid());
    TimeWidget_->setTime(time);

    TimeWidget_->hideAnimated();
}

int32_t ComplexMessageItem::getTime() const
{
    return Time_;
}

QSize ComplexMessageItem::sizeHint() const
{
    if (Layout_)
    {
        return Layout_->sizeHint();
    }

    return QSize(-1, MessageStyle::getMinBubbleHeight());
}

void ComplexMessageItem::updateWith(ComplexMessageItem &update)
{
    if (update.Id_ != -1)
    {
        assert((Id_ == -1) || (Id_ == update.Id_));
        Id_ = update.Id_;
    }
}

void ComplexMessageItem::leaveEvent(QEvent *event)
{
    event->ignore();

    MouseRightPressedOverItem_ = false;
    MouseLeftPressedOverAvatar_ = false;

    bubbleHovered_ = false;
    if (timestampHoverEnabled_)
        hideHiddenControls();

    onHoveredBlockChanged(nullptr);
    emit leave();

    MessageItemBase::leaveEvent(event);
}

void ComplexMessageItem::mouseMoveEvent(QMouseEvent *_event)
{
    _event->ignore();

    const auto mousePos = _event->pos();

    if (isOverAvatar(mousePos))
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    auto blockUnderCursor = findBlockUnder(mousePos);
    onHoveredBlockChanged(blockUnderCursor);

    const auto prevHovered = bubbleHovered_;
    bubbleHovered_ = Layout_->getBubbleRect().contains(mousePos);
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
}

void ComplexMessageItem::mousePressEvent(QMouseEvent *event)
{
    MouseLeftPressedOverAvatar_ = false;
    MouseRightPressedOverItem_ = false;

    const auto isLeftButtonPressed = (event->button() == Qt::LeftButton);
    const auto pressedOverAvatar = (
        isLeftButtonPressed &&
        isOverAvatar(event->pos()));
    if (pressedOverAvatar)
    {
        MouseLeftPressedOverAvatar_ = true;
    }

    const auto isRightButtonPressed = (event->button() == Qt::RightButton);
    if (isRightButtonPressed)
    {
        MouseRightPressedOverItem_ = true;
    }

    if (!bubbleHovered_ && !pressedOverAvatar)
        event->ignore();

    return MessageItemBase::mousePressEvent(event);
}

void ComplexMessageItem::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButtonReleased = (event->button() == Qt::LeftButton);
    const auto leftClickAvatar = (
        isLeftButtonReleased &&
        isOverAvatar(event->pos()) &&
        MouseLeftPressedOverAvatar_);
    if (leftClickAvatar)
    {
        Utils::openDialogOrProfile(SenderAimid_, ChatAimid_);
        event->accept();
    }

    const auto isRightButtonPressed = (event->button() == Qt::RightButton);
    const auto rightButtonClickOnWidget = (isRightButtonPressed && MouseRightPressedOverItem_);
    if (rightButtonClickOnWidget)
    {
        if (isOverAvatar(event->pos()))
        {
            emit avatarMenuRequest(SenderAimid_);
            return;
        }

        const auto globalPos = event->globalPos();
        trackMenu(globalPos);
    }

    MouseRightPressedOverItem_ = false;
    MouseLeftPressedOverAvatar_ = false;
}

void ComplexMessageItem::paintEvent(QPaintEvent *event)
{
    MessageItemBase::paintEvent(event);

    if (!Layout_)
    {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawAvatar(p);

    drawBubble(p, QuoteAnimation_.quoteColor());

    if (Style::isBlocksGridEnabled())
    {
        drawGrid(p);
    }

    const auto lastStatus = getLastStatus();
    if (lastStatus != LastStatus::None)
        drawLastStatusIcon(p, lastStatus, SenderAimid_, getSenderFriendly(), isOutgoing() ? 0 : MessageStyle::getTimeMaxWidth());
}

void ComplexMessageItem::hideEvent(QHideEvent *)
{
    if (TimeWidget_ && get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true))
        TimeWidget_->hideAnimated();
}

void ComplexMessageItem::onAvatarChanged(QString aimId)
{
    assert(!aimId.isEmpty());
    assert(!SenderAimid_.isEmpty());

    if (SenderAimid_ != aimId)
    {
        return;
    }

    if (!hasAvatar())
    {
        return;
    }

    loadAvatar();

    update();
}

void ComplexMessageItem::onMenuItemTriggered(QAction *action)
{
    const auto params = action->data().toMap();
    const auto command = params[qsl("command")].toString();

    const auto isDeveloperCommand = command.startsWith(ql1s("dev:"));
    if (isDeveloperCommand)
    {
        const auto subCommand = command.mid(4);

        if (subCommand.isEmpty())
        {
            assert(!"unknown subcommand");
            return;
        }

        if (onDeveloperMenuItemTriggered(subCommand))
        {
            return;
        }
    }

    const auto isProcessedByBlock = (
        MenuBlock_ &&
        MenuBlock_->onMenuItemTriggered(params));
    if (isProcessedByBlock)
    {
        return;
    }

    if (command == ql1s("copy"))
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Copy);
        return;
    }

    if (command == ql1s("quote"))
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Quote);
        return;
    }

    if (command == ql1s("forward"))
    {
        onCopyMenuItem(ComplexMessageItem::MenuItemType::Forward);
        return;
    }

    if ((command == ql1s("delete_all")) || (command == ql1s("delete")))
    {
        const auto isPendingMessage = (Id_ <= -1);
        if (isPendingMessage)
        {
            return;
        }

        const auto is_shared = command == ql1s("delete_all");

        assert(!SenderAimid_.isEmpty());

        const QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?" );

        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Delete message"),
            nullptr
        );

        if (confirm)
            GetDispatcher()->deleteMessages({ Id_ }, ChatAimid_, is_shared);

        return;
    }
}

void ComplexMessageItem::cleanupMenu()
{
    MenuBlock_ = nullptr;
}

void ComplexMessageItem::createSenderControl()
{
    if (Sender_)
    {
        return;
    }

    QColor color;
    Sender_ = new TextEmojiWidget(
        this,
        Fonts::appFont(Ui::MessageStyle::getSenderFont().pixelSize()),
        color);

    updateSenderControlColor();
}

void ComplexMessageItem::connectSignals()
{
    connect(Logic::GetAvatarStorage(),          &Logic::AvatarStorage::avatarChanged,          this, &ComplexMessageItem::onAvatarChanged);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showMessageHiddenControls, this, &ComplexMessageItem::showHiddenControls, Qt::QueuedConnection);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMessageHiddenControls, this, &ComplexMessageItem::hideHiddenControls, Qt::QueuedConnection);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setTimestampHoverActionsEnabled, this, &ComplexMessageItem::setTimestampHoverEnabled);
}

bool ComplexMessageItem::containsShareableBlocks() const
{
    assert(!Blocks_.empty());

     return std::any_of(
         Blocks_.cbegin(),
         Blocks_.cend(),
         []
         (const IItemBlock *block)
         {
             assert(block);
             return block->isSharingEnabled() || block->containSharingBlock();
         });
}

void ComplexMessageItem::drawAvatar(QPainter &p)
{
    if (!hasAvatar())
    {
        return;
    }

    assert(Avatar_);
    if (!Avatar_)
    {
        return;
    }

    const auto &avatarRect = Layout_->getAvatarRect();
    if (avatarRect.isEmpty())
    {
        return;
    }

    p.drawPixmap(avatarRect, *Avatar_);
}

void ComplexMessageItem::drawBubble(QPainter &p, const QColor& quote_color)
{
    const auto &bubbleRect = Layout_->getBubbleRect();

    if (bubbleRect.isNull())
    {
        return;
    }

    if (Style::isBlocksGridEnabled())
    {
        p.save();

        p.setPen(Qt::gray);
        p.drawRect(bubbleRect);

        p.restore();
    }

    if (!isBubbleRequired())
    {
        return;
    }

    if (bubbleRect != BubbleGeometry_)
    {
        Bubble_ = Utils::renderMessageBubble(bubbleRect, MessageStyle::getBorderRadius(), isOutgoing());

        BubbleGeometry_ = bubbleRect;
    }

    p.fillPath(Bubble_, MessageStyle::getBodyBrush(isOutgoing(), false, theme()->get_id()));

    if (quote_color.isValid())
        p.fillPath(Bubble_, QBrush(quote_color));
}

QString ComplexMessageItem::getBlocksText(const IItemBlocksVec &items, const bool isSelected, const bool isQuote) const
{
    QString result;

    // to reduce the number of reallocations
    result.reserve(1024);

    int selectedItemCount = 0;
    if (isSelected)
    {
        for (auto item : items)
        {
            if (item->isSelected())
                ++selectedItemCount;
        }
    }


    for (auto item : items)
    {
        const auto itemText = (
            isSelected ?
                item->getSelectedText(selectedItemCount > 1) :
                item->getTextForCopy());

        if (itemText.isEmpty())
        {
            continue;
        }

        result += itemText;
    }

    if (result.endsWith(QChar::LineFeed))
        result.truncate(result.length() - 1);

    return result;
}

void ComplexMessageItem::drawGrid(QPainter &p)
{
    assert(Style::isBlocksGridEnabled());

    p.setPen(Qt::blue);
    p.drawRect(Layout_->getBlocksContentRect());
}

IItemBlock* ComplexMessageItem::findBlockUnder(const QPoint &pos) const
{
    for (auto block : Blocks_)
    {
        assert(block);
        if (!block)
        {
            continue;
        }

        auto b = block->findBlockUnder(pos);
        if (b)
            return b;

        const auto blockLayout = block->getBlockLayout();
        assert(blockLayout);

        if (!blockLayout)
        {
            continue;
        }

        const auto &blockGeometry = blockLayout->getBlockGeometry();

        const auto topY = blockGeometry.top();
        const auto bottomY = blockGeometry.bottom();
        const auto posY = pos.y();

        const auto isPosOverBlock = ((posY >= topY) && (posY <= bottomY));
        if (isPosOverBlock)
        {
            return block;
        }
    }

    return nullptr;
}

QString ComplexMessageItem::getSenderFriendly() const
{
    assert(!SenderFriendly_.isEmpty());
    return SenderFriendly_;
}

QVector<Data::Quote> ComplexMessageItem::getQuotes(bool force) const
{
    QVector<Data::Quote> quotes;
    Data::Quote quote;
    for (auto b : Blocks_)
    {
        auto selectedText = force ? b->getTextForCopy() : b->getSelectedText();
        if (!selectedText.isEmpty())
        {
            auto q = b->getQuote();
            if (!q.isEmpty())
            {
                quotes.push_back(std::move(q));
            }
            else if (b->needFormatQuote())
            {
                if (quote.isEmpty())
                {
                    quote.senderId_ = isOutgoing() ? MyInfo()->aimId() : getSenderAimid();
                    quote.chatId_ = getChatAimid();
                    quote.time_ = getTime();
                    quote.msgId_ = getId();
                    QString senderFriendly = getSenderFriendly();
                    if (senderFriendly.isEmpty())
                        senderFriendly = Logic::getContactListModel()->getDisplayName(quote.senderId_);
                    if (isOutgoing())
                        senderFriendly = MyInfo()->friendlyName();
                    quote.senderFriendly_ = senderFriendly;
                    auto stickerInfo = b->getStickerInfo();
                    if (stickerInfo)
                    {
                        quote.setId_ = stickerInfo->SetId_;
                        quote.stickerId_ = stickerInfo->StickerId_;
                    }
                }
                quote.text_ += selectedText;

                switch (b->getContentType())
                {
                    case IItemBlock::ContentType::FileSharing:
                        quote.type_ = Data::Quote::Type::file_sharing;
                        break;
                    case IItemBlock::ContentType::Link:
                        quote.type_ = Data::Quote::Type::link;
                        break;
                    case IItemBlock::ContentType::Quote:
                        quote.type_ = Data::Quote::Type::quote;
                        break;
                    case IItemBlock::ContentType::Text:
                        quote.type_ = Data::Quote::Type::text;
                        break;
                    case IItemBlock::ContentType::Other:
                        quote.type_ = Data::Quote::Type::other;
                        break;
                }
            }
        }
    }

    if (!quote.isEmpty())
    {
        quote.mentions_ = mentions_;
        quotes.push_back(std::move(quote));
    }

    return quotes;
}

void ComplexMessageItem::setSourceText(QString text)
{
    SourceText_ = std::move(text);
}

void ComplexMessageItem::initialize()
{
    assert(Layout_);
    assert(!Blocks_.empty());

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    onBlockSizeChanged();
    updateGeometry();
    update();

    setMouseTracking(true);
}

void ComplexMessageItem::initializeShareButton()
{
    assert(!ShareButton_);
    assert(!Blocks_.empty());

    if (!containsShareableBlocks())
    {
        return;
    }

    ShareButton_ = new ActionButtonWidget(ActionButtonWidget::ResourceSet::ShareContent_, this);
    ShareButton_->setVisible(false);

    const auto success = QObject::connect(
        ShareButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        &ComplexMessageItem::onShareButtonClicked);
    assert(success);

    /// install QuoteBlockHover event filter
    emit eventFilterRequest(ShareButton_);
}

bool ComplexMessageItem::isBubbleRequired() const
{
      for (const auto block : Blocks_)
      {
          if (block->isBubbleRequired())
          {
              return true;
          }
      }

      return false;
}

int ComplexMessageItem::getMaxWidth() const
{
    int maxWidth = -1;

    for (const auto block : Blocks_)
    {
        int blockMaxWidth = block->getMaxWidth();

        if (blockMaxWidth > 0)
        {
            if (maxWidth == -1 || maxWidth > blockMaxWidth)
                maxWidth = blockMaxWidth;
        }
    }

    return maxWidth;
}

const Data::MentionMap& ComplexMessageItem::getMentions() const
{
    return mentions_;
}

bool ComplexMessageItem::isOverAvatar(const QPoint &pos) const
{
    assert(Layout_);

    if (Avatar_)
    {
        return Layout_->isOverAvatar(pos);
    }

    return false;
}

bool ComplexMessageItem::isSelected() const
{
    for (const auto block : Blocks_)
    {
        if (block->isSelected())
        {
            return true;
        }
    }

    return false;
}

bool ComplexMessageItem::isSenderVisible() const
{
    return (Sender_ && hasAvatar());
}

void ComplexMessageItem::loadAvatar()
{
    assert(hasAvatar());

    auto isDefault = false;

    Avatar_ = Logic::GetAvatarStorage()->GetRounded(
        SenderAimid_,
        getSenderFriendly(),
        Utils::scale_bitmap(MessageStyle::getAvatarSize()),
        QString(),
        Out isDefault,
        false,
        false);
}

void ComplexMessageItem::forwardRoutine()
{
    const auto quotes = getQuotes(true);
    if (quotes.size() == 1)
    {
        std::vector<IItemBlock::ContentType> typesInside;
        typesInside.reserve(Blocks_.size());
        for (auto b : Blocks_)
        {
            typesInside.push_back(b->getContentType());
        }

        if (typesInside.size() == 1 && (typesInside[0] == IItemBlock::ContentType::FileSharing || typesInside[0] == IItemBlock::ContentType::Link))
        {
            shareButtonRoutine(quotes.constFirst().text_);
        }
        else
        {
            emit forward(quotes);
        }
    }
    else
    {
        emit forward(quotes);
    }
}

void ComplexMessageItem::onCopyMenuItem(ComplexMessageItem::MenuItemType type)
{
    QString itemText;
    itemText.reserve(1024);

    bool isCopy = (type == ComplexMessageItem::MenuItemType::Copy);
    bool isQuote = (type == ComplexMessageItem::MenuItemType::Quote);
    bool isForward = (type == ComplexMessageItem::MenuItemType::Forward);

    if (isQuote || isForward)
    {
        itemText += getQuoteHeader();
    }

    if (isSelected())
    {
        itemText += getSelectedText(false);
    }
    else
    {
        itemText += getBlocksText(Blocks_, false, isQuote || isForward);
    }

    if (isCopy)
    {
        emit copy(itemText);
    }
    else if (isQuote)
    {
        emit quote(getQuotes(true));
    }
    else if (isForward)
    {
        forwardRoutine();
    }
}

bool ComplexMessageItem::onDeveloperMenuItemTriggered(const QString &cmd)
{
    assert(!cmd.isEmpty());

    if (!GetAppConfig().IsContextMenuFeaturesUnlocked())
    {
        assert(!"developer context menu is not unlocked");
        return true;
    }

    if (cmd == ql1s("copy_message_id"))
    {
        const auto idStr = QString::number(getId());

        QApplication::clipboard()->setText(idStr);

        return true;
    }

    return false;
}

void ComplexMessageItem::shareButtonRoutine(QString sourceText)
{
    assert(!sourceText.isEmpty());

    SelectContactsWidget shareDialog(
        nullptr,
        Logic::MembersWidgetRegim::SHARE_LINK,
        QT_TRANSLATE_NOOP("popup_window", "Share link"),
        QT_TRANSLATE_NOOP("popup_window", "COPY LINK"),
        Ui::MainPage::instance(),
        true);

    emit Utils::InterConnector::instance().searchEnd();

    const auto action = shareDialog.show();
    if (action != QDialog::Accepted)
    {
        return;
    }
    const auto contact = shareDialog.getSelectedContact();

    if (!contact.isEmpty())
    {
        emit Utils::InterConnector::instance().addPageToDialogHistory(Logic::getContactListModel()->selectedContact());
        Logic::getContactListModel()->setCurrent(contact, -1, true);
        Ui::GetDispatcher()->sendMessageToContact(contact, sourceText);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_send_preview);
        Utils::InterConnector::instance().onSendMessage(contact);
    }
    else
    {
        QApplication::clipboard()->setText(sourceText);
    }
}

void ComplexMessageItem::onShareButtonClicked()
{
    assert(HoveredBlock_);
    if (!HoveredBlock_)
    {
        return;
    }
    shareButtonRoutine(HoveredBlock_->getSourceText());
}

void ComplexMessageItem::trackMenu(const QPoint &globalPos)
{
    cleanupMenu();

    auto menu = new ContextMenu(this);

    menu->addActionWithIcon(qsl(":/context_menu/reply_100"), QT_TRANSLATE_NOOP("context_menu", "Reply"), makeData(qsl("quote")));

    MenuBlock_ = findBlockUnder(mapFromGlobal(globalPos));

    const auto hasBlock      = !!MenuBlock_;
    const auto isLinkCopyable = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagLinkCopyable  : false;
    const auto isFileCopyable = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagFileCopyable  : false;
    const auto isOpenable     = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagOpenInBrowser : false;
    const auto isCopyable     = hasBlock ? MenuBlock_->getMenuFlags() & IItemBlock::MenuFlagCopyable : false;

    if (hasBlock)
    {
        if (isLinkCopyable && MenuBlock_->getContentType() != IItemBlock::ContentType::Text)
        {
            menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/link_100"))), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData(qsl("copy_link")));
        }
        else
        {
            // check if message has a link without preview
            QString link = MenuBlock_->linkAtPos(globalPos);

            bool hasCopyableLink = false;
            if (!link.isEmpty())
            {
                link.remove(QRegularExpression(qsl("^mailto:")));
                Utils::UrlParser parser;
                parser.process(&link);
                if (parser.hasUrl() && !parser.getUrl().is_email())
                    hasCopyableLink = true;
            }
            if(hasCopyableLink)
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/link_100"))), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData(qsl("copy_link"), link));
            else if (isCopyable)
                menu->addActionWithIcon(qsl(":/context_menu/copy_100"), QT_TRANSLATE_NOOP("context_menu", "Copy text"), makeData(qsl("copy")));
        }
    }

    if (hasBlock && isFileCopyable)
        menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/attach_100"))), QT_TRANSLATE_NOOP("context_menu", "Copy to clipboard"), makeData(qsl("copy_file")));

    menu->addActionWithIcon(qsl(":/context_menu/forward_100"), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward")));

    if (hasBlock && isFileCopyable)
        menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/download_100"))), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData(qsl("save_as")));

    if (isOpenable)
        menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/browser_100"))), QT_TRANSLATE_NOOP("context_menu", "Open in browser"), makeData(qsl("open_in_browser")));

    menu->addActionWithIcon(qsl(":/context_menu/close_100"), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData(qsl("delete")));

    if (isOutgoing() || Logic::getContactListModel()->isYouAdmin(ChatAimid_))
    {
        menu->addActionWithIcon(
            qsl(":/context_menu/closeall_100"),
            QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
            makeData(qsl("delete_all")));
    }

    if (GetAppConfig().IsContextMenuFeaturesUnlocked())
    {
        menu->addActionWithIcon(qsl(":/resources/copy_100"), qsl("Copy Message ID"), makeData(qsl("dev:copy_message_id")));
    }

    connect(menu, &ContextMenu::triggered, this, &ComplexMessageItem::onMenuItemTriggered, Qt::QueuedConnection);
    connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
    connect(menu, &ContextMenu::aboutToShow, this, &ComplexMessageItem::contextMenuShow, Qt::QueuedConnection);
    connect(menu, &ContextMenu::aboutToHide, this, &ComplexMessageItem::contextMenuHide, Qt::QueuedConnection);
    connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

    menu->popup(globalPos);
}

void ComplexMessageItem::updateSenderControlColor()
{
    if (!Sender_)
    {
        return;
    }

    const auto currentTheme = theme();

    const auto color = currentTheme
        ? currentTheme->contact_name_.text_color_
        : MessageStyle::getSenderColor();

    Sender_->setColor(color);
}

void ComplexMessageItem::updateShareButtonGeometry()
{
    if (!ShareButton_)
    {
        return;
    }

    if (!HoveredBlock_)
    {
        ShareButton_->setVisible(false);
        return;
    }

    const auto buttonGeometry = Layout_->getShareButtonGeometry(
        *HoveredBlock_,
        ShareButton_->sizeHint(),
        HoveredBlock_->isBubbleRequired());

    ShareButton_->setVisible(true);

    ShareButton_->setGeometry(buttonGeometry);
}

void ComplexMessageItem::setQuoteSelection()
{
    for (auto& val : Blocks_)
    {
        val->setQuoteSelection();
    }
    QuoteAnimation_.startQuoteAnimation();
}

void ComplexMessageItem::contextMenuShow()
{
    ContextMenuCreateEvent* e = new ContextMenuCreateEvent();
    QApplication::postEvent(this, e);
}

void ComplexMessageItem::contextMenuHide()
{
    ContextMenuDestroyEvent* e = new ContextMenuDestroyEvent();
    QApplication::postEvent(this, e);
}

void ComplexMessageItem::showHiddenControls()
{
    if (TimeWidget_ && isVisible() && get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true))
        TimeWidget_->showAnimated();

    showMessageStatus();
}

void ComplexMessageItem::hideHiddenControls()
{
    const bool canHide = TimeWidget_
        && !bubbleHovered_
        && isVisible()
        && get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true);

    if (canHide)
        TimeWidget_->hideAnimated();

    hideMessageStatus();
}

void ComplexMessageItem::setTimestampHoverEnabled(const bool _enabled)
{
    timestampHoverEnabled_ = _enabled;
}

void ComplexMessageItem::setDeliveredToServer(const bool _isDeliveredToServer)
{
    if (!isOutgoing())
        return;

    if (IsDeliveredToServer_ != _isDeliveredToServer)
        IsDeliveredToServer_ = _isDeliveredToServer;
}

bool ComplexMessageItem::isQuoteAnimation() const
{
    return bQuoteAnimation_;
}

void ComplexMessageItem::setQuoteAnimation()
{
    bQuoteAnimation_ = true;
}

bool ComplexMessageItem::isObserveToSize() const
{
    return bObserveToSize_;
}

void ComplexMessageItem::onObserveToSize()
{
    bObserveToSize_ = true;
}

namespace
{
    BlockSelectionType evaluateBlockSelectionType(const QRect &blockGeometry, const QPoint &selectionTop, const QPoint &selectionBottom)
    {
        const auto isBlockAboveSelection = (blockGeometry.bottom() < selectionTop.y());
        const auto isBlockBelowSelection = (blockGeometry.top() > selectionBottom.y());
        if (isBlockAboveSelection || isBlockBelowSelection)
        {
            return BlockSelectionType::None;
        }

        const auto isTopPointAboveBlock = (blockGeometry.top() >= selectionTop.y());
        const auto isBottomPointBelowBlock = (blockGeometry.bottom() <= selectionBottom.y());

        if (isTopPointAboveBlock && isBottomPointBelowBlock)
        {
            return BlockSelectionType::Full;
        }

        if (isTopPointAboveBlock)
        {
            return BlockSelectionType::FromBeginning;
        }

        if (isBottomPointBelowBlock)
        {
            return BlockSelectionType::TillEnd;
        }

        return BlockSelectionType::PartialInternal;
    }

    QMap<QString, QVariant> makeData(const QString& command, const QString& arg)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = command;

        if (!arg.isEmpty())
            result[qsl("arg")] = arg;

        return result;
    }
}

UI_COMPLEX_MESSAGE_NS_END
