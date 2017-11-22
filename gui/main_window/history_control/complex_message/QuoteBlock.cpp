#include "stdafx.h"

#include "ComplexMessageItem.h"
#include "../MessageStyle.h"
#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../controls/PictureWidget.h"
#include "../../../controls/ContactAvatarWidget.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../my_info.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../contextMenuEvent.h"
#include "../MessagesModel.h"

#include "QuoteBlockLayout.h"
#include "Selection.h"
#include "Style.h"
#include "QuoteBlock.h"
#include "TextBlock.h"
#include "../../../gui/controls/TextEditEx.h"
#include "../../../gui/controls/CommonStyle.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QuoteBlockHoverPainter::QuoteBlockHoverPainter(QWidget* parent) :
	QWidget(parent),
	Opacity_(0.0)
{
	QGraphicsOpacityEffect * effect = new QGraphicsOpacityEffect(this);
	Opacity_ = 0.0;
	effect->setOpacity(0.0);
	setGraphicsEffect(effect);
}

void QuoteBlockHoverPainter::paintEvent(QPaintEvent * e)
{
	QPainter p(this);

	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);

	QPen pen(Qt::NoPen);
	p.setPen(pen);

	static const QBrush b(QColor(0, 0, 0, 25));
	p.setBrush(b);

	static const auto radius = Utils::scale_value(8);
	p.drawRoundedRect(rect(), radius, radius);
}

void QuoteBlockHoverPainter::startAnimation(qreal begin, qreal end_opacity)
{
	QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(eff);

	QPropertyAnimation *a = new QPropertyAnimation(eff, QByteArrayLiteral("opacity"), eff);

	int duration = abs(begin - end_opacity) * 20;
	if (duration < 5)
		duration = 5;

	a->setDuration(duration);
	a->setStartValue(begin);
	a->setEndValue(end_opacity);
	a->setEasingCurve(QEasingCurve::InBack);
	a->start();

	connect(eff, &QGraphicsOpacityEffect::opacityChanged, this, &QuoteBlockHoverPainter::onOpacityChanged);
}

void QuoteBlockHoverPainter::startAnimation(qreal end_opacity)
{
    if (fabs(Opacity_ - end_opacity) > 0.02)
	    startAnimation(Opacity_, end_opacity);
}

void QuoteBlockHoverPainter::onOpacityChanged(qreal o)
{
	Opacity_ = o;
}

/////////////////////////////////////////////////////////////////////
QuoteBlockHover::QuoteBlockHover(QuoteBlockHoverPainter* painter, QWidget* parent, QuoteBlock* block) :
	QWidget(parent),
	Painter_(painter),
	Block_(block),
    Text_(nullptr),
    bAnchorClicked_(false)
{
    setMouseTracking(true);
}

bool QuoteBlockHover::eventFilter(QObject * obj, QEvent * e)
{
    if (e->type() == QEvent::MouseMove ||
        e->type() == QEvent::Leave ||
        e->type() == ContextMenuCreateEvent::type() ||
        e->type() == ContextMenuDestroyEvent::type())
    {
        auto pos = this->mapFromGlobal(QCursor::pos());
        bool bInside = rect().contains(pos);

        if (bInside)
            Painter_->startAnimation(1.0);
        else
            Painter_->startAnimation(0.0);
    }
    return QWidget::eventFilter(obj, e);
}

void QuoteBlockHover::mouseMoveEvent(QMouseEvent * e)
{
    e->ignore();
}

void QuoteBlockHover::mousePressEvent(QMouseEvent * e)
{
    bAnchorClicked_ = false;

    if (Text_)
    {
        auto pos = Text_->mapFromGlobal(e->globalPos());
        QMouseEvent ev(QEvent::MouseButtonPress, pos, e->button(), e->buttons(), e->modifiers());
        QApplication::sendEvent(Text_, &ev);
    }

	if ((e->buttons() & Qt::RightButton) ||
		(e->button() == Qt::RightButton))
	{
		/// context menu
		const auto globalPos = e->globalPos();
		emit contextMenu(globalPos);

        e->accept();
	}
    else
        e->ignore();
}

void QuoteBlockHover::mouseReleaseEvent(QMouseEvent * e)
{
    if (Text_)
    {
        auto pos = Text_->mapFromGlobal(e->globalPos());
        QMouseEvent ev(QEvent::MouseButtonRelease, pos, e->button(), e->buttons(), e->modifiers());
        QApplication::sendEvent(Text_, &ev);
    }

	/// press when below avatar_name
    int max_y = Utils::scale_value(0); //Change to 35 to create an AvatarClick action (by s.skakov)
    if (!Block_->isSelected() && !bAnchorClicked_)
    {
        if ((e->buttons() & Qt::LeftButton) ||
            (e->button() == Qt::LeftButton))
        {
            if (e->localPos().y() > max_y)
            {
                /// press to message
                emit openMessage();
            }
            else
            {
                /// press to avatar zone
                emit openAvatar();
            }
            e->accept();
        }
        else
            e->ignore();
    }
    else
        e->ignore();

    bAnchorClicked_ = false;
}

void Ui::ComplexMessage::QuoteBlockHover::onLeave()
{
    Painter_->startAnimation(0.0);
}

void Ui::ComplexMessage::QuoteBlockHover::onAnchorClicked(const QUrl&)
{
    bAnchorClicked_ = true;
}

void Ui::ComplexMessage::QuoteBlockHover::onSetTextEditEx(Ui::TextEditEx* _text)
{
    Text_ = _text;
    Text_->installEventFilter(this);
    connect(Text_, &Ui::TextEditEx::anchorClicked, this, &QuoteBlockHover::onAnchorClicked, Qt::UniqueConnection);
}

void Ui::ComplexMessage::QuoteBlockHover::onEventFilterRequest(QWidget* w)
{
    w->installEventFilter(this);
    w->setMouseTracking(true);
}

/////////////////////////////////////////////////////////////////////
QuoteBlock::QuoteBlock(ComplexMessageItem *parent, const Data::Quote& quote)
    : GenericBlock(parent, quote.senderFriendly_, MenuFlagCopyable, false)
    , Quote_(quote)
    , Layout_(new QuoteBlockLayout())
    , forwardLabel_(new LabelEx(this))
    , TextCtrl_(nullptr)
    , Selection_(BlockSelectionType::None)
    , Parent_(parent)
    , ReplyBlock_(nullptr)
    , ForwardIcon_(new PictureWidget(this, qsl(":/resources/forwardmsg_100.png")))
	, QuoteHoverPainter_(nullptr)
    , QuoteHover_(nullptr)
	, MessagesCount_(0)
	, MessageIndex_(0)
{
    setLayout(Layout_);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    forwardLabel_->setFont(Fonts::appFontScaled(13));
    forwardLabel_->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
    forwardLabel_->setText(quote.chatName_);
    forwardLabel_->adjustSize();
    forwardLabel_->setVisible(needForwardBlock());

    ForwardIcon_->setVisible(needForwardBlock());
    ForwardIcon_->setFixedSize(Utils::scale_value(12), Utils::scale_value(12));

    connect(this, &QuoteBlock::observeToSize, parent, &ComplexMessageItem::onObserveToSize);
}

QuoteBlock::~QuoteBlock()
{

}

void QuoteBlock::clearSelection()
{
    for (auto b : Blocks_)
    {
        b->clearSelection();
    }
    update();
}

IItemBlockLayout* QuoteBlock::getBlockLayout() const
{
    return Layout_;
}

QString QuoteBlock::getSelectedText(bool isFullSelect) const
{
    QString result;
    for (auto b : Blocks_)
    {
        if (!b->isSelected())
            continue;

        if (isFullSelect)
            result += qsl("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString(qsl("dd.MM.yyyy hh:mm")));

        result += b->getSelectedText();
        result += QChar::LineFeed;
    }

    return result;
}

QString QuoteBlock::getSourceText() const
{
    QString result;
    for (auto b : Blocks_)
    {
        result += qsl("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString(qsl("dd.MM.yyyy hh:mm")));
        result += b->getSourceText();
        result += QChar::LineFeed;
    }

    return result;
}

QString QuoteBlock::formatRecentsText() const
{
    if (standaloneText())
        return ReplyBlock_ ? ReplyBlock_->formatRecentsText() : QString();

    QString result;
    for (auto b : Blocks_)
    {
        result += b->getSourceText();
        result += QChar::LineFeed;
    }
    return result;
}

bool QuoteBlock::standaloneText() const
{
    return !quoteOnly() && !Quote_.isForward_;
}

Data::Quote QuoteBlock::getQuote() const
{
    Data::Quote quote;
    if (quoteOnly() || Quote_.isForward_)
        quote = Quote_;
    else
        quote.id_ = Quote_.id_;

   return quote;
}

IItemBlock* QuoteBlock::findBlockUnder(const QPoint &pos) const
{
    for (auto block : Blocks_)
    {
        assert(block);
        if (!block)
        {
            continue;
        }

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

bool QuoteBlock::isSelected() const
{
    for (auto b : Blocks_)
    {
        if (b->isSelected() && !b->getSelectedText().isEmpty())
            return true;
    }

    return false;
}

void QuoteBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    for (auto b : Blocks_)
    {
        b->selectByPos(from, to, selection);
    }
}

bool QuoteBlock::needForwardBlock() const
{
    return Quote_.isForward_ && Quote_.isFirstQuote_;
}

void QuoteBlock::setReplyBlock(GenericBlock* block)
{
    if (!ReplyBlock_)
        ReplyBlock_ = block;
}

void QuoteBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);
    for (auto block : Blocks_)
    {
        block->onVisibilityChanged(isVisible);
    }
}

void QuoteBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    GenericBlock::onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    for (auto block : Blocks_)
    {
        block->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    }
}

QRect QuoteBlock::setBlockGeometry(const QRect &ltr)
{
    QRect b = ltr;

    b.moveLeft(b.x() + Style::Quote::getQuoteOffsetLeft());
    QRect r = b;
    b.moveTop(b.y() + Style::Quote::getQuoteOffsetTop());
    b.moveTopLeft(GenericBlock::setBlockGeometry(b).bottomLeft());

    if (needForwardBlock())
       b.moveTop(b.y() + forwardLabel_->height() + Style::Quote::getForwardLabelBottomMargin());

    int i = 1;
    for (auto block : Blocks_)
    {
        block->setMaxPreviewWidth(Style::Quote::getMaxImageWidthInQuote());
        auto blockGeometry = block->setBlockGeometry(b);
        auto bl = QPoint(
            blockGeometry.bottomLeft().x(),
            blockGeometry.bottomLeft().y() + Style::Quote::getQuoteBlockSpacing());
        b.moveTopLeft(bl);
        r.setBottomLeft(i == Blocks_.size() ? blockGeometry.bottomLeft() : bl);
        ++i;
    }
    r.moveLeft(r.x() - Style::Quote::getQuoteOffsetLeft());
    r.moveLeft(r.x() - Style::Quote::getForwardIconOffset());

    if (!quoteOnly())
    {
        if (Quote_.isLastQuote_)
            r.setHeight(r.height() + Style::Quote::getQuoteOffsetBottom());
    }

    r.setHeight(r.height() - Style::Quote::getQuoteSpacing());

	int offset = 0;
	int offset_hover = 6;
	if (MessagesCount_ > 1)
	{
		if (ReplyBlock_)
		{
			if (MessagesCount_ == MessageIndex_ + 2)
				offset = 0;
			else
			{
				offset = 4;
				offset_hover = -4;
			}
		}
		else
		{
			if (MessagesCount_ == MessageIndex_ + 1)
			{
				/// last
				offset = 15;
			}
			else
			{
				offset_hover = 6;
				offset = 4;
			}
		}
	}
	else if (!ReplyBlock_)
		offset = 15;

	if (QuoteHover_ && QuoteHoverPainter_)
	{
		auto rect = QRect(ltr.left() + Utils::scale_value(6),
			ltr.top() + Utils::scale_value(4),
			ltr.right() - ltr.left(),
			quoteOnly() ? r.height() - Utils::scale_value(6) + Style::Quote::getQuoteOffsetBottom() : r.height() - Utils::scale_value(offset_hover));

		QuoteHover_->setGeometry(rect);
		QuoteHoverPainter_->setGeometry(rect);
	}

	QRect ret(
		r.left(),
		r.top(),
		r.width(),
		r.height() + Utils::scale_value(offset));

	QRect set_rect(
		r.left(),
		r.top(),
		r.width(),
		r.height() + Utils::scale_value(offset+4));

	Geometry_ = set_rect;
	setGeometry(set_rect);

    return ret;
}

void QuoteBlock::onActivityChanged(const bool isVisible)
{
    GenericBlock::onActivityChanged(isVisible);
    for (auto block : Blocks_)
    {
        block->onActivityChanged(isVisible);
    }
}

void QuoteBlock::addBlock(GenericBlock* block)
{
    block->setBubbleRequired(false);
    block->setFontSize(Style::Quote::getQuoteFont().pixelSize());
    block->setTextOpacity(0.7);
    Blocks_.push_back(block);

    /// connect(block, SIGNAL(clicked()), this, SLOT(blockClicked()), Qt::QueuedConnection);
}

bool QuoteBlock::quoteOnly() const
{
    return !ReplyBlock_;
}

void QuoteBlock::blockClicked()
{
    if (Quote_.isForward_)
    {
        if (Logic::getContactListModel()->contains(Quote_.chatId_))
        {
            const auto selectedContact = Logic::getContactListModel()->selectedContact();
            if (selectedContact != Quote_.chatId_)
                emit Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

            emit Logic::getContactListModel()->select(Quote_.chatId_, Quote_.msgId_, Quote_.msgId_, Logic::UpdateChatSelection::Yes);
            if (Quote_.msgId_ > 0)
                Logic::GetMessagesModel()->emitQuote(Quote_.msgId_);
        }
        else if (!Quote_.chatStamp_.isEmpty())
        {
            Logic::getContactListModel()->joinLiveChat(Quote_.chatStamp_, false);
        }
    }
    else
    {
        Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), Quote_.msgId_, true, nullptr, Quote_.msgId_);

        if (Quote_.msgId_ > 0)
            Logic::GetMessagesModel()->emitQuote(Quote_.msgId_);
    }
}

void QuoteBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor)
{
    auto pen = Style::Quote::getQuoteSeparatorPen();
    p.save();
    p.setPen(pen);
    auto end = rect().bottomLeft();
    end.setX(end.x() + Style::Quote::getLineOffset());
    if (Quote_.isLastQuote_)
	{
		end.setY(end.y() - Style::Quote::getQuoteOffsetBottom() - Utils::scale_value(4));
	}

    auto begin = rect().topLeft();
    begin.setX(begin.x() + Style::Quote::getLineOffset());
    if (Quote_.isFirstQuote_)
    {
        begin.setY(begin.y() + Style::Quote::getFirstQuoteOffset());
        if (needForwardBlock())
          begin.setY(begin.y() + forwardLabel_->height() + Style::Quote::getForwardLabelBottomMargin());
    }

    p.drawLine(begin, end);

	p.restore();
}

void QuoteBlock::initialize()
{
    GenericBlock::initialize();

    TextCtrl_ = new TextEmojiWidget(this, Fonts::appFontScaled(12), CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));

    Avatar_ = new ContactAvatarWidget(this, Quote_.senderId_, Quote_.senderFriendly_, Utils::scale_value(20), true);

    TextCtrl_->setText(Quote_.senderFriendly_);
    TextCtrl_->show();
    Avatar_->show();
}

bool QuoteBlock::replaceBlockWithSourceText(IItemBlock *block)
{
    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);

    if (iter == Blocks_.end())
    {
        return false;
    }

    auto &existingBlock = *iter;
    assert(existingBlock);

    auto textBlock = new TextBlock(Parent_, existingBlock->getSourceText());
    emit observeToSize();

    textBlock->onVisibilityChanged(true);
    textBlock->onActivityChanged(true);

    textBlock->show();

    connect(textBlock, &Ui::ComplexMessage::TextBlock::clicked, this, &QuoteBlock::blockClicked, Qt::QueuedConnection);

    existingBlock->deleteLater();
    existingBlock = textBlock;

    textBlock->connectToHover(QuoteHover_);

    return true;
}

bool QuoteBlock::isSharingEnabled() const
{
    return false;
}

bool QuoteBlock::containSharingBlock() const
{
    for (auto b : Blocks_)
    {
        if (b->isSharingEnabled())
            return true;
    }

    return false;
}

void QuoteBlock::createQuoteHover(ComplexMessage::ComplexMessageItem* complex_item)
{
    QuoteHoverPainter_ = new QuoteBlockHoverPainter((QWidget*)parent());
    QuoteHoverPainter_->lower();

    QuoteHover_ = new QuoteBlockHover(QuoteHoverPainter_, (QWidget*)parent(), this);
    QuoteHover_->raise();

    connect(QuoteHover_, &QuoteBlockHover::openMessage, this, &QuoteBlock::blockClicked);
    connect(QuoteHover_, &QuoteBlockHover::contextMenu, complex_item, &ComplexMessageItem::trackMenu);
    connect(complex_item, &ComplexMessage::ComplexMessageItem::eventFilterRequest, QuoteHover_, &QuoteBlockHover::onEventFilterRequest);

    for (auto& val : Blocks_)
        val->connectToHover(QuoteHover_);

    Parent_->installEventFilter(QuoteHover_);
    QuoteHover_->setCursor(Qt::PointingHandCursor);

    connect(complex_item, &ComplexMessage::ComplexMessageItem::setTextEditEx, QuoteHover_, &QuoteBlockHover::onSetTextEditEx);
    connect(complex_item, &ComplexMessage::ComplexMessageItem::leave, QuoteHover_, &QuoteBlockHover::onLeave);
}

void QuoteBlock::setMessagesCountAndIndex(int count, int index)
{
	MessagesCount_ = count;
	MessageIndex_ = index;
}

UI_COMPLEX_MESSAGE_NS_END
