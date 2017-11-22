#include "stdafx.h"
#include "InputWidget.h"

#include "../ContactDialog.h"
#include "../history_control/MessageStyle.h"
#include "../history_control/KnownFileTypes.h"
#include "../history_control/MentionCompleter.h"
#include "../history_control/HistoryControlPage.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/PictureWidget.h"
#include "../../themes/ThemePixmap.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/history_control/MessagesModel.h"
#include "../../main_window/sounds/SoundsManager.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

#ifdef __APPLE__
#   include "../../utils/SChar.h"
#endif

const int widget_max_height = 230;
const int document_min_height = 32;
const int quote_line_width = 2;
const int max_quote_height = 66;
const int preview_max_height = 36;
const int preview_offset = 12;
const int preview_max_width = 124;
const int preview_offset_hor = 18;
const int preview_text_offset = 16;
const int icon_size = 32;

namespace Ui
{
    HistoryTextEdit::HistoryTextEdit(QWidget* _parent)
        : TextEditEx(_parent, MessageStyle::getTextFont(), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), true, false)
    {
        limitCharacters(Utils::getInputMaximumChars());
        setUndoRedoEnabled(true);

        document()->setDefaultStyleSheet(ql1s("a { color:") % CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT).name() % ql1s("; text-decoration:none;}"));
    }

    bool HistoryTextEdit::catchEnter(int _modifiers)
    {
        auto key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

         // spike for fix invalid setting
        if (key1_to_send == KeyToSendMessage::Enter_Old)
            key1_to_send = KeyToSendMessage::Enter;

        switch (key1_to_send)
        {
        case Ui::KeyToSendMessage::Enter:
            return _modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier;
        case Ui::KeyToSendMessage::Shift_Enter:
            return _modifiers & Qt::ShiftModifier;
        case Ui::KeyToSendMessage::Ctrl_Enter:
            return _modifiers & Qt::ControlModifier;
        case Ui::KeyToSendMessage::CtrlMac_Enter:
            return _modifiers & Qt::MetaModifier;
        default:
            break;
        }
        return false;
    }

    bool HistoryTextEdit::catchNewLine(const int _modifiers)
    {
        auto key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);

        // spike for fix invalid setting
        if (key1_to_send == KeyToSendMessage::Enter_Old)
            key1_to_send = KeyToSendMessage::Enter;

        switch (key1_to_send)
        {
        case Ui::KeyToSendMessage::Enter:
            return _modifiers & Qt::ShiftModifier
                    || _modifiers & Qt::ControlModifier
                    || _modifiers & Qt::MetaModifier;
        case Ui::KeyToSendMessage::Shift_Enter:
            return _modifiers & Qt::ControlModifier
                    || _modifiers & Qt::MetaModifier
                    || (_modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier);
        case Ui::KeyToSendMessage::Ctrl_Enter:
            return _modifiers & Qt::ShiftModifier
                    || _modifiers & Qt::MetaModifier
                    || (_modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier);
        case Ui::KeyToSendMessage::CtrlMac_Enter:
            return _modifiers & Qt::ShiftModifier
                    || _modifiers & Qt::ControlModifier
                    || (_modifiers == Qt::NoModifier || _modifiers == Qt::KeypadModifier);
        default:
            break;
        }
        return false;
    }

    void HistoryTextEdit::keyPressEvent(QKeyEvent* _e)
    {
        auto page = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact());
        if (!page)
            return;

        const auto mc = page->getMentionCompleter();
        if (mc && mc->completerVisible())
        {
            static const auto keys = {
                Qt::Key_Up,
                Qt::Key_Down,
                Qt::Key_PageUp,
                Qt::Key_PageDown,
                Qt::Key_Enter,
                Qt::Key_Return
            };
            if (std::find(keys.begin(), keys.end(), _e->key()) != keys.end())
            {
                QApplication::sendEvent(mc, _e);
                if (_e->isAccepted())
                    return;
            }
        }

        const auto atTextStart = textCursor().atStart();
        const auto atTextEnd = textCursor().atEnd();

        if (atTextStart)
        {
            const auto applePageUp = (platform::is_apple() && _e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && _e->key() == Qt::Key_Up);
            if (_e->key() == Qt::Key_Up || _e->key() == Qt::Key_PageUp || applePageUp)
            {
                QApplication::sendEvent(page, _e);
                _e->accept();
                return;
            }
        }

        if (atTextEnd)
        {
            const auto applePageDown = (platform::is_apple() && _e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && _e->key() == Qt::Key_Down);
            const auto applePageEnd = (platform::is_apple() && ((_e->modifiers().testFlag(Qt::KeyboardModifier::MetaModifier) && _e->key() == Qt::Key_Right) || _e->key() == Qt::Key_End));
            const auto ctrlEnd = atTextStart == atTextEnd && ((_e->modifiers() == Qt::CTRL && _e->key() == Qt::Key_End) || applePageEnd);

            if (_e->key() == Qt::Key_Down || _e->key() == Qt::Key_PageDown || applePageDown || ctrlEnd)
            {
                QApplication::sendEvent(page, _e);
                _e->accept();
                return;
            }
        }

        const auto prevCursor = textCursor();
        TextEditEx::keyPressEvent(_e);

        auto cursor = textCursor();
        const auto moveMode = _e->modifiers().testFlag(Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
        bool set = false;
        if (_e->key() == Qt::Key_Up || _e->key() == Qt::Key_PageUp)
            set = cursor.movePosition(QTextCursor::Start, moveMode);
        else if (_e->key() == Qt::Key_Down || _e->key() == Qt::Key_PageDown)
            set = cursor.movePosition(QTextCursor::End, moveMode);

        if (set && cursor.blockNumber() == prevCursor.blockNumber())
            setTextCursor(cursor);
    }

    InputWidget::InputWidget(QWidget* parent, int height, const QString& styleSheet, bool messageOnly)
        : QWidget(parent)
        , smiles_button_(new QPushButton(this))
        , send_button_(new QPushButton(this))
        , file_button_(new QPushButton(this))
        , anim_height_(nullptr)
        , text_edit_(new HistoryTextEdit(this))
        , is_initializing_(false)
        , message_only_(messageOnly)
        , mentionSignIndex_(-1)
    {
        default_height_ = height == -1 ? CommonStyle::getBottomPanelHeight() : height;
        active_height_ = default_height_;
        need_height_ = active_height_;

        setVisible(false);
        setStyleSheet(Utils::LoadStyle(styleSheet.isEmpty() ? qsl(":/qss/input_widget") : styleSheet));

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto vLayout = Utils::emptyVLayout();

        quote_block_ = new QWidget(this);
        quote_block_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto quoteLayout = Utils::emptyHLayout(quote_block_);
        quoteLayout->setAlignment(Qt::AlignVCenter);
        quoteLayout->setContentsMargins(Utils::scale_value(52), Utils::scale_value(12), Utils::scale_value(16), 0);

        quote_line_ = new QWidget(quote_block_);
        quote_line_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        quote_line_->setStyleSheet(qsl("background-color: %1;")
            .arg(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL).name()));
        quote_line_->setFixedWidth(Utils::scale_value(quote_line_width));
        quoteLayout->addWidget(quote_line_);
        quote_text_widget_ = new QWidget(quote_block_);
        auto quote_text_layout = Utils::emptyVLayout(quote_text_widget_);
        quote_text_layout->setContentsMargins(Utils::scale_value(10), Utils::scale_value(4), 0, 0);

        quote_text_ = new TextEditEx(quote_block_, Fonts::appFontScaled(12), QColor(), false, false);
        quote_text_->setSize(QSizePolicy::Preferred, QSizePolicy::Preferred);
        quote_text_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        quote_text_->setStyleSheet(qsl("background-color: %1;")
            .arg(CommonStyle::getFrameColor().name()));
        quote_text_->setFrameStyle(QFrame::NoFrame);
        quote_text_->document()->setDocumentMargin(0);
        quote_text_->setOpenLinks(true);
        quote_text_->setOpenExternalLinks(true);
        quote_text_->setWordWrapMode(QTextOption::NoWrap);
        quote_text_->setContextMenuPolicy(Qt::NoContextMenu);

        QPalette p = quote_text_->palette();
        p.setColor(QPalette::Text, CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY));
        p.setColor(QPalette::Link, CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
        p.setColor(QPalette::LinkVisited, CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
        quote_text_->setPalette(p);
        quote_text_layout->addWidget(quote_text_);
        quoteLayout->addWidget(quote_text_widget_);
        cancel_quote_ = new QPushButton(quote_block_);
        cancel_quote_->setObjectName(qsl("cancel_quote"));
        cancel_quote_->setCursor(QCursor(Qt::PointingHandCursor));
        quoteLayout->addWidget(cancel_quote_);
        vLayout->addWidget(quote_block_);
        quote_block_->hide();

        auto hLayout = Utils::emptyHLayout();

        auto smiles_layout = Utils::emptyVLayout();
        smiles_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

        smiles_button_->setObjectName(qsl("smiles_button"));
        smiles_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        smiles_button_->setCursor(QCursor(Qt::PointingHandCursor));
        smiles_button_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Smileys and stickers"));
        smiles_button_->setFocusPolicy(Qt::NoFocus);
        smiles_button_->setCheckable(true);
        smiles_button_->setChecked(false);

        smiles_layout->addWidget(smiles_button_);
        smiles_layout->addSpacerItem(new QSpacerItem(0, default_height_ / 2 - Utils::scale_value(icon_size) / 2, QSizePolicy::Preferred, QSizePolicy::Fixed));

        cancel_files_ = new QPushButton(this);
        cancel_files_->setObjectName(qsl("cancel_files"));
        cancel_files_->setCursor(QCursor(Qt::PointingHandCursor));
        hLayout->addLayout(smiles_layout);
        hLayout->addWidget(cancel_files_);

        connect(cancel_files_, &QPushButton::clicked, this, &InputWidget::onFilesCancel, Qt::QueuedConnection);

        cancel_files_->hide();

        setObjectName(qsl("input_edit_control"));
        if (message_only_ && platform::is_apple())
            text_edit_->setObjectName(qsl("input_edit_control_mac"));
        else
            text_edit_->setObjectName(qsl("input_edit_control"));

        text_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("input_widget","Message"));
        text_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        text_edit_->setAutoFillBackground(false);
        text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        text_edit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        hLayout->addWidget(text_edit_);

        files_block_ = new QWidget(this);
        files_block_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto filesLayout = Utils::emptyHLayout(files_block_);
        filesLayout->setContentsMargins(Utils::scale_value(preview_offset), 0, 0, 0);
        file_preview_ = new PictureWidget(files_block_);
        file_preview_->setObjectName(qsl("files_preview"));
        file_preview_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        filesLayout->addWidget(file_preview_);
        filesLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(preview_offset), 0, QSizePolicy::Fixed));
        files_label_ = new LabelEx(this);
        files_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        files_label_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        files_label_->setFont(Fonts::appFontScaled(16));
        filesLayout->addWidget(files_label_);
        hLayout->addWidget(files_block_);
        files_block_->hide();

        input_disabled_block_ = new QWidget(this);
        input_disabled_block_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto disabled_layout = Utils::emptyHLayout(input_disabled_block_);
        disabled_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        disable_label_ = new LabelEx(this);
        disable_label_->setFont(Fonts::appFontScaled(16));
        disable_label_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        disable_label_->setAlignment(Qt::AlignCenter);
        disable_label_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        disable_label_->setWordWrap(true);
        disabled_layout->addWidget(disable_label_);
        connect(disable_label_, &Ui::LabelEx::linkActivated, this, &InputWidget::disableActionClicked, Qt::QueuedConnection);
        disabled_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        hLayout->addWidget(input_disabled_block_);
        input_disabled_block_->hide();

        auto send_layout = Utils::emptyVLayout();
        send_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
        send_button_->setObjectName(qsl("send_button"));
        send_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        send_button_->setCursor(QCursor(Qt::PointingHandCursor));
        send_button_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Send message"));
        send_button_->setVisible(false);
        Testing::setAccessibleName(send_button_, qsl("SendMessageButton"));
        send_layout->addWidget(send_button_);
        send_layout->addSpacerItem(new QSpacerItem(0, default_height_ / 2 - Utils::scale_value(icon_size) / 2, QSizePolicy::Preferred, QSizePolicy::Fixed));

        auto file_button_layout = Utils::emptyVLayout();
        file_button_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

        const int file_button_icon_size = Utils::scale_value(24);

        file_button_->setObjectName(qsl("file_button"));
        file_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        file_button_->setFocusPolicy(Qt::NoFocus);
        file_button_->setCursor(QCursor(Qt::PointingHandCursor));
        file_button_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Send file"));
        file_button_->raise();
        Testing::setAccessibleName(file_button_, qsl("SendFileButton"));
        file_button_->setVisible(!messageOnly);
        file_button_layout->addWidget(file_button_);
        file_button_layout->addSpacerItem(new QSpacerItem(0, default_height_ / 2 - file_button_icon_size / 2, QSizePolicy::Preferred, QSizePolicy::Fixed));

        hLayout->addLayout(send_layout);
        hLayout->addLayout(file_button_layout);

        vLayout->addLayout(hLayout);
        setLayout(vLayout);

        set_current_height(active_height_);

        connect(text_edit_->document(), &QTextDocument::contentsChanged, this, &InputWidget::editContentChanged, Qt::QueuedConnection);
        connect(text_edit_, &HistoryTextEdit::cursorPositionChanged, this, &InputWidget::cursorPositionChanged);
        connect(text_edit_, &HistoryTextEdit::enter, this, &InputWidget::enterPressed, Qt::QueuedConnection);
        connect(text_edit_, &HistoryTextEdit::enter, this, &InputWidget::statsMessageEnter, Qt::QueuedConnection);
        connect(text_edit_, &TextEditEx::textChanged, this, &InputWidget::typed);
        connect(text_edit_, &TextEditEx::mentionErased, this, &InputWidget::clearMentions);
        connect(text_edit_, &TextEditEx::focusOut, this, &InputWidget::editFocusOut);

        connect(send_button_, &QPushButton::clicked, this, &InputWidget::send, Qt::QueuedConnection);
        connect(send_button_, &QPushButton::clicked, this, &InputWidget::statsMessageSend, Qt::QueuedConnection);
        connect(file_button_, &QPushButton::clicked, this, &InputWidget::attach_file, Qt::QueuedConnection);
        connect(file_button_, &QPushButton::clicked, this, &InputWidget::stats_attach_file, Qt::QueuedConnection);
        connect(cancel_quote_, &QPushButton::clicked, this, &InputWidget::clear, Qt::QueuedConnection);
        connect(this, &InputWidget::needUpdateSizes, this, &InputWidget::updateSizes, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::youRoleChanged, this, &InputWidget::chatRoleChanged, Qt::QueuedConnection);

        if (!message_only_)
        {
            connect(smiles_button_, &QPushButton::clicked, [this]()
            {
                text_edit_->setFocus();
                emit smilesMenuSignal();
            });
        }

        active_document_height_ = text_edit_->document()->size().height();

        Testing::setAccessibleName(text_edit_, qsl("InputTextEdit"));
    }

    void InputWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

    void InputWidget::resizeEvent(QResizeEvent * _e)
    {
        updateSizes();
        return QWidget::resizeEvent(_e);
    }

    void InputWidget::keyPressEvent(QKeyEvent * _e)
    {
        if (_e->matches(QKeySequence::Paste) && !message_only_)
        {
            auto mimedata = QApplication::clipboard()->mimeData();
            if (mimedata && !Utils::haveText(mimedata))
            {
                bool hasUrls = false;
                if (mimedata->hasUrls())
                {
                    files_to_send_[contact_].clear();
                    const QList<QUrl> urlList = mimedata->urls();
                    for (const auto& url : urlList)
                    {
                        if (url.isLocalFile())
                        {
                            auto path = url.toLocalFile();
                            auto fi = QFileInfo(path);
                            if (fi.exists() && !fi.isBundle() && !fi.isDir())
                                files_to_send_[contact_] << path;
                            hasUrls = true;
                        }
                    }
                }

                if (!hasUrls && mimedata->hasImage())
                {
#ifdef _WIN32
                    if (OpenClipboard(nullptr))
                    {
                        HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
                        image_buffer_[contact_] = qt_pixmapFromWinHBITMAP(hBitmap);
                        CloseClipboard();
                    }
#else
                    image_buffer_[contact_] = qvariant_cast<QPixmap>(mimedata->imageData());
#endif //_WIN32
                }
                editContentChanged();
                initFiles(contact_);
            }
        }

        if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            Qt::KeyboardModifiers modifiers = _e->modifiers();

            if (text_edit_->catchEnter(modifiers))
            {
                send();
            }
        }

        const auto mc = getMentionCompleter();
        const auto canOfferCompleter =
            text_edit_->isVisible() &&
            text_edit_->isEnabled() &&
            shouldOfferMentions() &&
            (mentionSignIndex_ == -1 || (mc && !mc->completerVisible()));

        if (_e->key() == Qt::Key_At && canOfferCompleter)
        {
            auto page = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact());
            if (page)
            {
                page->showMentionCompleter();
                mentionSignIndex_ = text_edit_->textCursor().position() - 1;
            }
        }

        if (_e->matches(QKeySequence::Find))
        {
            emit ctrlFPressedInInputWidget();
        }

        return QWidget::keyPressEvent(_e);
    }

    void InputWidget::keyReleaseEvent(QKeyEvent *_e)
    {
        if (platform::is_apple())
        {
            if (_e->modifiers() & Qt::AltModifier)
            {
                QTextCursor cursor = text_edit_->textCursor();
                QTextCursor::MoveMode selection = (_e->modifiers() & Qt::ShiftModifier)?
                    QTextCursor::MoveMode::KeepAnchor:
                    QTextCursor::MoveMode::MoveAnchor;

                if (_e->key() == Qt::Key_Left)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::PreviousWord, selection);
                    text_edit_->setTextCursor(cursor);
                }
                else if (_e->key() == Qt::Key_Right)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::NextWord, selection);
                    text_edit_->setTextCursor(cursor);
                }
            }
        }

        QWidget::keyReleaseEvent(_e);
    }

    void InputWidget::resize_to(int _height)
    {
        if (get_current_height() == _height)
            return;

        QEasingCurve easing_curve = QEasingCurve::Linear;
        int duration = 100;

        int start_value = get_current_height();
        int end_value = _height;

        if (!anim_height_)
            anim_height_ = new QPropertyAnimation(this, QByteArrayLiteral("current_height"), this);

        anim_height_->stop();
        anim_height_->setDuration(duration);
        anim_height_->setStartValue(start_value);
        anim_height_->setEndValue(end_value);
        anim_height_->setEasingCurve(easing_curve);
        anim_height_->start();
    }

    void InputWidget::editContentChanged()
    {
        const QString input_text = text_edit_->getPlainText();
        const int widget_min_height = default_height_;
        Logic::getContactListModel()->setInputText(contact_, input_text);

        const auto isAllAttachsEmpty = isAllAttachmentsEmpty(contact_);

        send_button_->setVisible(!QStringRef(&input_text).trimmed().isEmpty() || !isAllAttachsEmpty);
        file_button_->setVisible(input_text.isEmpty() && isAllAttachsEmpty && !message_only_ && !disabled_.contains(contact_));

        int doc_height = text_edit_->document()->size().height();

        int diff = doc_height - active_document_height_;

        active_document_height_ = doc_height;

        need_height_ = need_height_ + diff;

        int new_height = need_height_;

        if (need_height_ <= widget_min_height || doc_height <= Utils::scale_value(document_min_height))
        {
            new_height = widget_min_height;
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        else if (need_height_ > Utils::scale_value(widget_max_height))
        {
            new_height = Utils::scale_value(widget_max_height);
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else
        {
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }

        resize_to(new_height);
    }

    void InputWidget::quote(const QVector<Data::Quote>& _quotes)
    {
        if (_quotes.isEmpty() || disabled_.contains(contact_))
            return;

        text_edit_->setFocus();

        quotes_[contact_] += _quotes;
        quote_block_->setVisible(true);
        initQuotes(contact_);
        initFiles(contact_);

        editContentChanged();
    }

    void InputWidget::insert_emoji(int32_t _main, int32_t _ext)
    {
#ifdef __APPLE__
        text_edit_->insertPlainText(Utils::SChar(_main, _ext).ToQString());
#else
        text_edit_->insertEmoji(_main, _ext);
#endif
        text_edit_->setFocus();
    }

    void InputWidget::send_sticker(int32_t _set_id, int32_t _sticker_id)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        //ext:1:sticker:5
        collection.set_value_as_bool("sticker", true);
        collection.set_value_as_int("sticker/set_id", _set_id);
        collection.set_value_as_int("sticker/id", _sticker_id);

        if (!quotes_[contact_].isEmpty())
        {
            core::ifptr<core::iarray> quotesArray(collection->create_array());
            quotesArray->reserve(quotes_[contact_].size());
            for (auto quote : quotes_[contact_])
            {
                core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                quote.serialize(quoteCollection.get());
                core::coll_helper coll(collection->create_collection(), true);
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());
            }
            collection.set_value_as_array("quotes", quotesArray.get());
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_send_answer);

            core::stats::event_props_type props;
            props.emplace_back(std::make_pair("Quotes_MessagesCount", std::to_string(quotes_[contact_].size())));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, props);
        }

        Data::MentionMap quoteMentions;
        for (const auto& q : quotes_[contact_])
            quoteMentions.insert(q.mentions_.begin(), q.mentions_.end());

        if (!quoteMentions.empty())
        {
            core::ifptr<core::iarray> mentArray(collection->create_array());
            mentArray->reserve(quoteMentions.size());
            for (const auto& m : quoteMentions)
            {
                core::ifptr<core::icollection> mentCollection(collection->create_collection());
                Ui::gui_coll_helper coll(mentCollection.get(), false);
                coll.set_value_as_qstring("aimid", m.first);
                coll.set_value_as_qstring("friendly", m.second);

                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(mentCollection.get());
                mentArray->push_back(val.get());
            }
            collection.set_value_as_array("mentions", mentArray.get());
        }

        Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());

        clearQuotes(contact_);

        emit sendMessage(contact_);
    }

    void InputWidget::smilesMenuHidden()
    {
        smiles_button_->setChecked(false);
    }

    void InputWidget::insertMention(const QString & _aimId, const QString & _friendly)
    {
        mentions_[contact_].emplace(std::make_pair(_aimId, _friendly));

        auto insertPos = -1;
        if (mentionSignIndex_ != -1)
        {
            insertPos = mentionSignIndex_;

            auto cursor = text_edit_->textCursor();
            auto posShift = 0;

            auto mc = getMentionCompleter();
            if (mc)
                posShift = mc->getSearchPattern().length() + 1;
            else if (cursor.position() >= mentionSignIndex_)
                posShift = cursor.position() - mentionSignIndex_;

            cursor.setPosition(mentionSignIndex_);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posShift);
            cursor.removeSelectedText();

            mentionSignIndex_ = -1;
        }

        if (insertPos != -1)
        {
            auto cursor = text_edit_->textCursor();
            cursor.setPosition(insertPos);
            text_edit_->setTextCursor(cursor);
        }
        text_edit_->insertMention(_aimId, _friendly);
    }

    void InputWidget::cursorPositionChanged()
    {
        auto mc = getMentionCompleter();
        const auto cursorPos = text_edit_->textCursor().position();
        if (mc && mentionSignIndex_ != -1)
        {
            if (mc->completerVisible())
            {
                if (cursorPos <= mentionSignIndex_)
                    mc->hide();
            }
            else if (cursorPos > mentionSignIndex_)
                mc->show();
        }
    }

    void InputWidget::enterPressed()
    {
        const auto mc = getMentionCompleter();
        if (mc && mc->completerVisible() && mc->hasSelectedItem())
            mc->insertCurrent();
        else
            send();
    }

    void InputWidget::send()
    {
        auto text = text_edit_->getPlainText().trimmed();

        auto& quotes = quotes_[contact_];

        if (text.isEmpty() && quotes.isEmpty() && files_to_send_[contact_].isEmpty() && image_buffer_[contact_].isNull())
        {
            return;
        }

        if (!predefined_.isEmpty())
            text = predefined_ % ql1c(' ') % text;

        if (!image_buffer_[contact_].isNull())
        {
            QByteArray array;
            QBuffer b(&array);
            b.open(QIODevice::WriteOnly);
            image_buffer_[contact_].save(&b, "PNG");
            Ui::GetDispatcher()->uploadSharedFile(contact_, array, qsl(".png"));
            onFilesCancel();
            return;
        }

        if (!files_to_send_[contact_].isEmpty())
        {
            for (const auto &filename : Utils::as_const(files_to_send_[contact_]))
            {
                QFileInfo fileInfo(filename);
                if (fileInfo.size() == 0)
                    continue;

                Ui::GetDispatcher()->uploadSharedFile(contact_, filename);

                core::stats::event_props_type props_file;
                props_file.emplace_back(std::make_pair("Filesharing_Filesize", std::to_string(1.0 * fileInfo.size() / 1024 / 1024)));
                props_file.emplace_back(std::make_pair("Filesharing_Is_Image", std::to_string(Utils::is_image_extension(fileInfo.suffix()))));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent, props_file);
            }

            onFilesCancel();
            return;
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        const auto msg = text.toUtf8();
        collection.set<QString>("contact", contact_);
        collection.set_value_as_string("message", msg.data(), msg.size());
        if (!quotes.isEmpty())
        {
            core::ifptr<core::iarray> quotesArray(collection->create_array());
            quotesArray->reserve(quotes.size());
            for (const auto& quote : Utils::as_const(quotes))
            {
                core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                quote.serialize(quoteCollection.get());
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());
            }
            collection.set_value_as_array("quotes", quotesArray.get());
            Ui::GetDispatcher()->post_stats_to_core(text.isEmpty() ? core::stats::stats_event_names::quotes_send_alone : core::stats::stats_event_names::quotes_send_answer);

            core::stats::event_props_type props;
            props.emplace_back(std::make_pair("Quotes_MessagesCount", std::to_string(quotes.size())));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, props);
        }

        auto& mentions = mentions_[contact_];
        for (const auto& q : Utils::as_const(quotes))
            mentions.insert(q.mentions_.begin(), q.mentions_.end());

        if (!mentions.empty())
        {
            QString textWithMentions;
            textWithMentions += text;
            for (const auto& q : Utils::as_const(quotes))
                textWithMentions.append(q.text_);

            for (auto it = mentions.cbegin(); it != mentions.cend();)
            {
                if (!textWithMentions.contains(ql1s("@[") % it->first % ql1c(']')))
                    it = mentions.erase(it);
                else
                    ++it;
            }
        }

        if (!mentions.empty())
        {
            core::ifptr<core::iarray> mentArray(collection->create_array());
            mentArray->reserve(mentions.size());
            for (const auto& m : Utils::as_const(mentions))
            {
                core::ifptr<core::icollection> mentCollection(collection->create_collection());
                Ui::gui_coll_helper coll(mentCollection.get(), false);
                coll.set_value_as_qstring("aimid", m.first);
                coll.set_value_as_qstring("friendly", m.second);

                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(mentCollection.get());
                mentArray->push_back(val.get());
            }
            collection.set_value_as_array("mentions", mentArray.get());
        }

        Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());

        text_edit_->clear();
        clearMentions(contact_);
        clearQuotes(contact_);
        onFilesCancel();

        GetSoundsManager()->playOutgoingMessage();

        emit sendMessage(contact_);
    }

    void InputWidget::attach_file()
    {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setViewMode(QFileDialog::Detail);
        dialog.setDirectory(get_gui_settings()->get_value<QString>(settings_upload_directory, QString()));
        if (platform::is_apple())
            dialog.setFilter(QDir::Files); // probably it'll also be useful for other platforms
        if (!dialog.exec())
            return;

        get_gui_settings()->set_value<QString>(settings_upload_directory, dialog.directory().absolutePath());

        const QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.isEmpty())
            return;

        core::stats::event_props_type props;
        props.emplace_back(std::make_pair("Filesharing_Count", std::to_string(selectedFiles.size())));
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent_count, props);

        for (const auto &filename : selectedFiles)
        {
            QFileInfo fileInfo(filename);
            if (fileInfo.size() == 0)
                continue;

            Ui::GetDispatcher()->uploadSharedFile(contact_, filename);

            core::stats::event_props_type props_file;
            props_file.emplace_back(std::make_pair("Filesharing_Filesize", std::to_string(1.0 * fileInfo.size() / 1024 / 1024)));
            props_file.emplace_back(std::make_pair("Filesharing_Is_Image", std::to_string(Utils::is_image_extension(fileInfo.suffix()))));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent, props_file);
        }
    }

    void InputWidget::contactSelected(const QString& _contact)
    {
        if (contact_ == _contact)
            return;

        if (!quotes_[_contact].isEmpty())
        {
            initQuotes(_contact);
            quote_block_->setVisible(true);

            editContentChanged();
        }
        else
        {
            clearQuotes(_contact);
        }

        if (!files_to_send_[_contact].isEmpty() || !image_buffer_[_contact].isNull())
        {
            initFiles(_contact);

            editContentChanged();
        }
        else
        {
            clearFiles(_contact);
        }

        contact_ = _contact;

        setVisible(true);
        activateWindow();
        is_initializing_ = true;
        text_edit_->setPlainText(Logic::getContactListModel()->getInputText(contact_), false);
        is_initializing_ = false;

        if (text_edit_->getPlainText().isEmpty())
            mentions_[contact_].clear();

        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        if (contactDialog)
            contactDialog->hideSmilesMenu();

        auto role = Logic::getContactListModel()->getYourRole(contact_);
        if (role == ql1s("notamember"))
            disable(NotAMember);
        else if (role == ql1s("readonly"))
            hide();
        else
            enable();

        text_edit_->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void InputWidget::hide()
    {
        contact_.clear();
        setVisible(false);
        text_edit_->setPlainText(QString(), false);
    }

    void InputWidget::hideNoClear()
    {
        setVisible(false);
    }

    void InputWidget::disable(DisableReason reason)
    {
        disabled_.insert(contact_, reason);
        text_edit_->clear();
        text_edit_->setEnabled(false);
        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        if (contactDialog)
            contactDialog->hideSmilesMenu();

        if (reason == NotAMember)
        {
            disable_label_->setFixedWidth(width());
            disable_label_->setVisible(true);
            disable_label_->setText(
                QT_TRANSLATE_NOOP("input_widget", "You are not a member of this chat. ")
                % ql1s("<a style=\"text-decoration:none\" href=\"leave\"><span style=\"color:")
                % CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT).name()
                % ql1s(";\">")
                % QT_TRANSLATE_NOOP("input_widget", "Leave and delete")
                % ql1s("</span></a>")
            );
        }

        clear();
        smiles_button_->hide();
        text_edit_->hide();
        file_button_->hide();
        send_button_->hide();
        input_disabled_block_->setVisible(true);
        input_disabled_block_->setFixedHeight(default_height_);
    }

    void InputWidget::enable()
    {
        disabled_.remove(contact_);
        text_edit_->setEnabled(true);
        initQuotes(contact_);
        smiles_button_->show();
        text_edit_->show();
        file_button_->show();
        send_button_->setVisible(text_edit_->getPlainText().isEmpty());
        initFiles(contact_);
        editContentChanged();
        input_disabled_block_->hide();

        emit needUpdateSizes();
    }

    void InputWidget::predefined(const QString& _contact, const QString& _text)
    {
        contact_ = _contact;
        predefined_ = _text;
    }

    void InputWidget::set_current_height(int _val)
    {
        text_edit_->setFixedHeight(_val);

        active_height_ = _val;

        emit sizeChanged();
    }

    int InputWidget::get_current_height() const
    {
        return active_height_;
    }

    void InputWidget::setFocusOnInput()
    {
        text_edit_->setFocus();
    }

    void InputWidget::setFontColor(const QColor& _color)
    {
        QPalette p = text_edit_->palette();
        p.setColor(QPalette::Text, _color);
        text_edit_->setPalette(p);
    }

    QPixmap InputWidget::getFilePreview(const QString& contact)
    {
        QPixmap result ;
        if (files_to_send_[contact].isEmpty())
        {
            if (image_buffer_[contact].isNull())
                return result;

            result = image_buffer_[contact];
        }

        if (!files_to_send_[contact].isEmpty())
        {
            auto file = files_to_send_[contact].at(0);

            if (Utils::is_image_extension(QFileInfo(file).suffix()))
            {
                auto reader = std::make_unique<QImageReader>(file);

                QString type = reader->format();

                if (type.isEmpty())
                {
                    reader = std::make_unique<QImageReader>(file);
                    reader->setDecideFormatFromContent(true);
                    type = reader->format();
                }

                result = QPixmap::fromImageReader(reader.get());
            }
            else
            {
                auto fileTypeIcon = History::GetIconByFilename(file);
                result = fileTypeIcon->GetPixmap();
            }
        }

        int h = Utils::scale_bitmap(Utils::scale_value(preview_max_height));
        int w = Utils::scale_bitmap(Utils::scale_value(preview_max_width));
        result = result.scaledToHeight(h, Qt::SmoothTransformation);
        if (result.width() > w)
        {
            result = result.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }

        return result;
    }

    QString InputWidget::getFileSendText(const QString& contact) const
    {
        const auto it = files_to_send_.find(contact);
        if (it == files_to_send_.end() || it.value().isEmpty())
        {
            const auto imageIt = image_buffer_.find(contact);
            if (imageIt != image_buffer_.end() && !imageIt.value().isNull())
                return QT_TRANSLATE_NOOP("input_widget", "Send image");

            return QString();
        }

        const auto& files = it.value();

        auto file = files.at(0);
        bool image = true;
        for (const auto& file_iter : files)
        {
            if (!Utils::is_image_extension(QFileInfo(file_iter).suffix()))
            {
                image = false;
                break;
            }
        }

        if (files.size() == 1)
        {
            if (image)
                return QT_TRANSLATE_NOOP("input_widget", "Send image");
            else
                return QT_TRANSLATE_NOOP("input_widget", "Send file");
        }

        QString numberString;
        if (image)
            numberString = Utils::GetTranslator()->getNumberString(files.size(), QT_TRANSLATE_NOOP3("input_widget", "image", "1"), QT_TRANSLATE_NOOP3("input_widget", "images", "2"), QT_TRANSLATE_NOOP3("input_widget", "images", "5"), QT_TRANSLATE_NOOP3("input_widget", "images", "21"));
        else
            numberString = Utils::GetTranslator()->getNumberString(files.size(), QT_TRANSLATE_NOOP3("input_widget", "file", "1"), QT_TRANSLATE_NOOP3("input_widget", "files", "2"), QT_TRANSLATE_NOOP3("input_widget", "files", "5"), QT_TRANSLATE_NOOP3("input_widget", "files", "21"));

        return QT_TRANSLATE_NOOP("input_widget", "Send %1 %2 %3 %4").arg(qsl("<font color=black>"), QString::number(files.size()), qsl("</font>"), numberString);
    }

    bool InputWidget::isAllAttachmentsEmpty(const QString & contact) const
    {
        {
            const auto it = quotes_.constFind(contact);
            if (it != quotes_.cend() && !it.value().isEmpty())
                return false;
        }
        {
            const auto it = files_to_send_.constFind(contact);
            if (it != files_to_send_.cend() && !it.value().isEmpty())
                return false;
        }
        {
            const auto it = image_buffer_.constFind(contact);
            if (it != image_buffer_.cend() && !it.value().isNull())
                return false;
        }
        return true;
    }

    bool InputWidget::shouldOfferMentions() const
    {
        auto cursor = text_edit_->textCursor();
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);

        const auto text = cursor.selectedText();
        if (text.isEmpty())
            return true;

        const auto lastChar = text.at(0);
        if (lastChar.isSpace() || qsl(".,:;_-+=!?\"\'#").contains(lastChar))
            return true;

        return false;
    }

    MentionCompleter* InputWidget::getMentionCompleter()
    {
        const auto currentContact = Logic::getContactListModel()->selectedContact();
        if (!currentContact.isEmpty())
        {
            auto page = Utils::InterConnector::instance().getHistoryPage(currentContact);
            if (page)
                return page->getMentionCompleter();
        }

        return nullptr;
    }

    void InputWidget::typed()
    {
        if (is_initializing_)
            return;

        if (!isVisible())
            return;

        if (mentionSignIndex_ != -1)
        {
            const auto text = text_edit_->document()->toPlainText();
            const auto noAtSign = text.length() <= mentionSignIndex_ || text.at(mentionSignIndex_) != ql1c('@');

            if (noAtSign)
            {
                mentionSignIndex_ = -1;
                emit Utils::InterConnector::instance().hideMentionCompleter();
                return;
            }

            auto mc = getMentionCompleter();
            if (mc)
            {
                auto cursor = text_edit_->textCursor();
                cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, cursor.position() - mentionSignIndex_ - 1);
                mc->setSearchPattern(cursor.selectedText());
            }
        }

        emit inputTyped();
    }

    void InputWidget::statsMessageEnter()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_enter_button);
    }

    void InputWidget::statsMessageSend()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_send_button);
    }

    void InputWidget::stats_attach_file()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dialog);
    }

    void InputWidget::clearQuotes(const QString& contact)
    {
        quotes_[contact].clear();
        quote_text_->clear();
        quote_block_->setVisible(false);
        editContentChanged();
    }

    void InputWidget::clearFiles(const QString& contact)
    {
        files_to_send_[contact].clear();
        initFiles(contact);
        editContentChanged();
    }

    void InputWidget::clearMentions(const QString& contact)
    {
        mentions_[contact].clear();
    }

    void InputWidget::clear()
    {
        clearQuotes(contact_);
        clearFiles(contact_);
        clearMentions(contact_);
    }

    void InputWidget::updateSizes()
    {
        initQuotes(contact_);
        editContentChanged();

        disable_label_->setFixedWidth(width());
    }

    void InputWidget::initQuotes(const QString& contact)
    {
        const auto it = quotes_.constFind(contact);
        if (it == quotes_.cend() || it.value().isEmpty())
            return;

        quote_text_->clear();

        const auto& currentQuotes = it.value();

        QString text;
        int i = 0;
        int needHeight = 0;
        for (const auto& quote : currentQuotes)
        {
            if (!text.isEmpty())
                text += ql1c('\n');

            QString quoteText = quote.senderFriendly_ % ql1s(": ") % Utils::convertMentions(quote.text_, quote.mentions_);
            quoteText.replace(QChar::LineSeparator, QChar::Space);
            quoteText.replace(QChar::LineFeed, QChar::Space);

            QFontMetrics m(quote_text_->font());
            auto elided = m.elidedText(quoteText, Qt::ElideRight, width() - cancel_quote_->width() - Utils::scale_value(100));
            text += elided;
            if (++i <= 3)
                needHeight += m.size(Qt::TextSingleLine, elided).height();
        }

        quote_text_->setPlainText(text, false);

        if (currentQuotes.size() <= 3)
        {
            auto docSize = quote_text_->document()->documentLayout()->documentSize().toSize();
            auto height = docSize.height();
            needHeight = height;
        }

        needHeight += Utils::scale_value(20);

        quote_text_widget_->setFixedHeight(needHeight);
        quote_block_->setFixedHeight(needHeight);
        quote_line_->setFixedHeight(needHeight - Utils::scale_value(2));
    }

    void InputWidget::initFiles(const QString& contact)
    {
        const auto haveFilesToSend = !files_to_send_[contact].isEmpty();
        const auto haveImageBuffer = !image_buffer_[contact].isNull();

        const auto show = haveFilesToSend || haveImageBuffer;

        QString first;
        if (haveFilesToSend)
            first = files_to_send_[contact].at(0);

        auto p = getFilePreview(contact);
        files_label_->setText(getFileSendText(contact));
        files_label_->setTextFormat(Qt::RichText);
        file_preview_->setFixedSize(p.width() / Utils::scale_bitmap(1), p.height() / Utils::scale_bitmap(1) + Utils::scale_value(preview_offset));
        file_preview_->setImage(p, Utils::is_image_extension(QFileInfo(first).suffix()) || haveImageBuffer ? Utils::scale_value(8) : -1);

        smiles_button_->setVisible(!show);
        cancel_files_->setVisible(show);

        files_block_->setVisible(show);
        text_edit_->setVisible(!show);

        file_button_->setVisible(!show && text_edit_->getPlainText().isEmpty());

        quote_block_->setVisible(!show && !quotes_[contact].isEmpty());

        if (show)
        {
            auto contactDialog = Utils::InterConnector::instance().getContactDialog();
            if (contactDialog)
                contactDialog->hideSmilesMenu();
            files_block_->setFixedHeight(default_height_);
        }
    }

    void InputWidget::onFilesCancel()
    {
        files_to_send_[contact_].clear();
        image_buffer_[contact_] = QPixmap();
        initFiles(contact_);
        editContentChanged();

        emit needUpdateSizes();
    }

    void InputWidget::disableActionClicked(const QString& action)
    {
        if (action == ql1s("leave") && disabled_.contains(contact_) && disabled_[contact_] == NotAMember)
            Logic::getContactListModel()->removeContactFromCL(contact_);
    }

    void InputWidget::chatRoleChanged(QString contact)
    {
        const auto role = Logic::getContactListModel()->getYourRole(contact);

        const bool isNotMember = (role == ql1s("notamember"));
        const bool isReadonly = !isNotMember && (role == ql1s("readonly"));
        if (contact_ == contact)
        {
            if (isNotMember)
            {
                disable(NotAMember);
                return;
            }
            else if (isReadonly)
            {
                hide();
                return;
            }
        }

        if (disabled_.contains(contact))
        {
            if (contact_ == contact)
                enable();
            else if (isNotMember)
                disabled_[contact] = NotAMember;
            else
                disabled_.remove(contact);
        }
    }
}
