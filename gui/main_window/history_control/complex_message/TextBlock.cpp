#include "stdafx.h"

#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEditEx.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"

#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "TextBlockLayout.h"
#include "Selection.h"

#include "TextBlock.h"
#include "QuoteBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    const QString SELECTION_STYLE =
        qsl("background: transparent; selection-background-color: %1;")
        .arg(Utils::rgbaStringFromColor(Utils::getSelectionColor()));

    struct ExtractResult
    {
        QString trimmedText;
        QString trailingSpaces;
    };

    ExtractResult extractTrailingSpaces(const QString &text)
    {
        assert(!text.isEmpty());

        QStringRef trimmedTextRef(&text);

        if (trimmedTextRef.length() > 1 && trimmedTextRef.startsWith(QChar::LineFeed))
        {
            trimmedTextRef = trimmedTextRef.mid(1, trimmedTextRef.length() - 1);
        }

        QString trailingSpaces;

        if (text.isEmpty())
        {
            return { trimmedTextRef.toString(), std::move(trailingSpaces) };
        }

        trailingSpaces.reserve(trimmedTextRef.length());

        while(!trimmedTextRef.isEmpty())
        {
            const auto lastCharIndex = (trimmedTextRef.length() - 1);

            const auto lastChar = trimmedTextRef.at(lastCharIndex);

            if (!lastChar.isSpace())
            {
                break;
            }

            trailingSpaces += lastChar;
            trimmedTextRef.truncate(lastCharIndex);
        }

        return { trimmedTextRef.toString(), std::move(trailingSpaces) };
    }
}

TextBlock::TextBlock(ComplexMessageItem *parent, const QString &text, const bool _hideLinks)
    : GenericBlock(parent, text, MenuFlagCopyable, false)
    , Text_(text)
    , Layout_(nullptr)
    , TextCtrl_(nullptr)
    , Selection_(BlockSelectionType::None)
    , TextFontSize_(-1)
    , TextOpacity_(1.0)
    , hideLinks_(_hideLinks)

{
    assert(!Text_.isEmpty());

    auto result = extractTrailingSpaces(Text_);
    TrimmedText_ = std::move(result.trimmedText);
    TrailingSpaces_ = std::move(result.trailingSpaces);

    Layout_ = new TextBlockLayout();
    setLayout(Layout_);

    connect(this, &TextBlock::selectionChanged, parent, &ComplexMessageItem::selectionChanged);
    connect(this, &TextBlock::setTextEditEx, parent, &ComplexMessageItem::setTextEditEx);
}

TextBlock::~TextBlock()
{
}

void TextBlock::clearSelection()
{
    if (TextCtrl_)
    {
        TextCtrl_->clearSelection();
    }

    Selection_ = BlockSelectionType::None;
}

IItemBlockLayout* TextBlock::getBlockLayout() const
{
    return Layout_;
}

QString TextBlock::getSelectedText(bool isFullSelect) const
{
    if (!TextCtrl_)
    {
        return QString();
    }

    switch (Selection_)
    {
        case BlockSelectionType::Full:
            return Text_;

        case BlockSelectionType::TillEnd:
            return (TextCtrl_->selection() + TrailingSpaces_);
    }

    return TextCtrl_->selection();
}

bool TextBlock::isDraggable() const
{
    return false;
}

bool TextBlock::isSelected() const
{
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    auto selected = TextCtrl_ && !TextCtrl_->selection().isEmpty();
    return (Selection_ != BlockSelectionType::None || selected);
}

bool TextBlock::isSharingEnabled() const
{
    return false;
}

void TextBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    assert(selection > BlockSelectionType::Min);
    assert(selection < BlockSelectionType::Max);
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    if (!TextCtrl_)
    {
        return;
    }

    Selection_ = selection;

    const auto selectAll = (selection == BlockSelectionType::Full);
    if (selectAll)
    {
        TextCtrl_->selectWholeText();
        return;
    }

    const auto selectFromBeginning = (selection == BlockSelectionType::FromBeginning);
    if (selectFromBeginning)
    {
        TextCtrl_->selectFromBeginning(to);
        return;
    }

    const auto selectTillEnd = (selection == BlockSelectionType::TillEnd);
    if (selectTillEnd)
    {
        TextCtrl_->selectTillEnd(from);
        return;
    }

    TextCtrl_->selectByPos(from, to);
}

void TextBlock::setFontSize(int size)
{
    TextFontSize_ = size;

    if (TextCtrl_)
    {
        TextCtrl_->setFont(MessageStyle::getTextFont(TextFontSize_));
    }
}

void TextBlock::setTextOpacity(double opacity)
{
    TextOpacity_ = opacity;
    if (TextCtrl_)
    {
        QPalette p = TextCtrl_->palette();
        p.setColor(QPalette::Text, MessageStyle::getTextColor(TextOpacity_));
        TextCtrl_->setPalette(p);
    }
}

void TextBlock::drawBlock(QPainter & p, const QRect& _rect, const QColor& quote_color)
{
}

void TextBlock::initialize()
{
    GenericBlock::initialize();

    assert(!TextCtrl_);
    assert(!Text_.isEmpty());
    TextCtrl_ = createTextEditControl(Text_);
    TextCtrl_->setVisible(true);
    TextCtrl_->raise();

    QObject::connect(
        TextCtrl_,
        &QTextBrowser::selectionChanged,
        this,
        &TextBlock::selectionChanged);
}

TextEditEx* TextBlock::createTextEditControl(const QString &text)
{
    assert(!text.isEmpty());

    QSignalBlocker sb(this);
    setUpdatesEnabled(false);

    QPalette p;
    p.setColor(QPalette::Text, MessageStyle::getTextColor(TextOpacity_));

    auto textControl = new Ui::TextEditEx(
        this,
        MessageStyle::getTextFont(TextFontSize_),
        p,
        false,
        false);

    textControl->document()->setDefaultStyleSheet(MessageStyle::getMessageStyle());

    textControl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    textControl->setStyle(QApplication::style());
    textControl->setFrameStyle(QFrame::NoFrame);
    textControl->setFocusPolicy(Qt::NoFocus);
    textControl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setOpenLinks(false);
    textControl->setOpenExternalLinks(false);
    textControl->setWordWrapMode(QTextOption::WordWrap);
    textControl->setStyleSheet(qsl("background: transparent"));
    textControl->setContextMenuPolicy(Qt::NoContextMenu);
    textControl->setReadOnly(true);
    textControl->setUndoRedoEnabled(false);
    textControl->setMentions(getParentComplexMessage()->getMentions());

    setTextEditTheme(textControl);

    textControl->verticalScrollBar()->blockSignals(true);

    Logic::Text4Edit(TrimmedText_, *textControl, Logic::Text2DocHtmlMode::Escape, !hideLinks_, true);

    textControl->document()->setDocumentMargin(0);

    textControl->verticalScrollBar()->blockSignals(false);

    setUpdatesEnabled(true);

    sb.unblock();
    emit setTextEditEx(textControl);

    return textControl;
}

void TextBlock::setTextEditTheme(TextEditEx *textControl)
{
    assert(textControl);

    Utils::ApplyStyle(textControl, SELECTION_STYLE);
    auto textColor = MessageStyle::getTextColor(TextOpacity_);
    textColor.setAlpha(TextOpacity_ * 255);

    QPalette palette = textControl->palette();
    palette.setColor(QPalette::Text, textColor);
    textControl->document()->setDefaultStyleSheet(MessageStyle::getMessageStyle());
    textControl->setPalette(palette);
}

void TextBlock::connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover)
{
    if (hover && TextCtrl_)
    {
        TextCtrl_->installEventFilter(hover);
        installEventFilter(hover);
    }
}

bool TextBlock::isBubbleRequired() const
{
    return true;
}

QString TextBlock::linkAtPos(const QPoint& pos) const
{
    return TextCtrl_->anchorAt(TextCtrl_->mapFromGlobal(pos));
}

UI_COMPLEX_MESSAGE_NS_END
