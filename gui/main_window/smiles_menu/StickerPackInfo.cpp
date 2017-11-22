#include "stdafx.h"
#include "StickerPackInfo.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/TextEditEx.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../../controls/TransparentScrollBar.h"
#include "SmilesMenu.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/CustomButton.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListModel.h"

using namespace Ui;

int32_t getStickerItemSize()
{
    return Utils::scale_value(80);
}

int32_t getStickerSize()
{
    return Utils::scale_value(72);
}

int32_t getButtonMargin()
{
    return Utils::scale_value(16);
}

int32_t getShareButtonWidth()
{
    return Utils::scale_value(32);
}

QString getStickerpackUrl(const QString& _storeId)
{
    return ql1s("https://cicq.org/s/") % _storeId;
}




StickersView::StickersView(QWidget* _parent, Smiles::StickersTable* _stickers)
    : QWidget(_parent)
    , stickers_(_stickers)
    , previewActive_(false)
{
    setMouseTracking(true);

    setCursor(Qt::PointingHandCursor);

    connect(stickers_, &Smiles::StickersTable::stickerPreview, this, [this](int32_t _setId, int32_t _stickerId)
    {
        previewActive_ = true;

        emit stickerPreview(_setId, _stickerId);
    });

    connect(stickers_, &Smiles::StickersTable::stickerHovered, this, &StickersView::stickerHovered);
}

void StickersView::mouseReleaseEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::MouseButton::LeftButton)
    {
        if (previewActive_)
        {
            previewActive_ = false;

            emit stickerPreviewClose();
        }
        else
        {
            const auto rc = stickers_->geometry();

            stickers_->mouseReleaseEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
        }
    }

    QWidget::mouseReleaseEvent(_e);
}

void StickersView::mousePressEvent(QMouseEvent* _e)
{
    if (_e->button() == Qt::MouseButton::LeftButton)
    {
        const auto rc = stickers_->geometry();

        stickers_->mousePressEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
    }

    QWidget::mousePressEvent(_e);
}

void StickersView::mouseMoveEvent(QMouseEvent* _e)
{
    const auto rc = stickers_->geometry();

    stickers_->mouseMoveEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));

    QWidget::mouseMoveEvent(_e);
}

void StickersView::leaveEvent(QEvent* _e)
{
    stickers_->leaveEvent();

    QWidget::leaveEvent(_e);
}


PackWidget::PackWidget(QWidget* _parent)
    : QWidget(_parent)
    , viewArea_(nullptr)
    , stickers_(nullptr)
    , stickerPreview_(nullptr)
    , addButton_(nullptr)
    , removeButton_(nullptr)
    , setId_(0)
    , shareButton_(nullptr)
    , loadingText_(new QLabel(this))
    , descriptionControl_(nullptr)
    , dialog_(nullptr)
{
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/pack_info")));

    setFixedWidth(Utils::scale_value(360));
    setFixedHeight(Utils::scale_value(400));

    rootVerticalLayout_ = Utils::emptyVLayout(this);

    loadingText_->setObjectName(qsl("loading_state"));
    loadingText_->setAlignment(Qt::AlignCenter);
    loadingText_->setText(QT_TRANSLATE_NOOP("stickers", "Loading..."));

    rootVerticalLayout_->addWidget(loadingText_);

    setLayout(rootVerticalLayout_);
}

void PackWidget::onStickerEvent(const qint32 _error, const qint32 _setId, const qint32 _stickerId)
{
    if (stickers_)
    {
        stickers_->onStickerUpdated(_setId, _stickerId);
    }
}

void PackWidget::setParentDialog(QWidget* _dialog)
{
    dialog_ = _dialog;
}

void PackWidget::onStickersPackInfo(std::shared_ptr<Ui::Stickers::Set> _set, const bool _result, const bool _purchased)
{
    if (viewArea_)
        return;

    if (_result)
    {
        loadingText_->setVisible(false);

        setId_ = _set->getId();
        storeId_ = _set->getStoreId();
        description_ = _set->getDescription();

        if (!description_.isEmpty())
        {
            QHBoxLayout* textLayout = new QHBoxLayout(this);
            textLayout->setContentsMargins(getButtonMargin(), 0, getButtonMargin(), 0);

            int textWidth = width() - 2 * getButtonMargin();

            descriptionControl_ = new TextEditEx(this, Fonts::appFontScaled(11), CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT), false, false);
            descriptionControl_->setWordWrapMode(QTextOption::WordWrap);
            descriptionControl_->setPlainText(description_);
            descriptionControl_->setObjectName(qsl("description"));
            descriptionControl_->adjustHeight(textWidth);
            descriptionControl_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            descriptionControl_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            descriptionControl_->setFrameStyle(QFrame::NoFrame);

            textLayout->addWidget(descriptionControl_);

            rootVerticalLayout_->addLayout(textLayout);
        }

        viewArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        viewArea_->setFocusPolicy(Qt::NoFocus);
        Utils::grabTouchWidget(viewArea_->viewport(), true);
        connect(QScroller::scroller(viewArea_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

        stickers_ = new Smiles::StickersTable(this, setId_, getStickerSize(), getStickerItemSize());

        StickersView* scroll_area_widget = new StickersView(this, stickers_);
        scroll_area_widget->setObjectName(qsl("scroll_area_widget"));
        viewArea_->setWidget(scroll_area_widget);
        viewArea_->setWidgetResizable(true);

        rootVerticalLayout_->addWidget(viewArea_);

        stickersLayout_ = Utils::emptyVLayout(this);
        stickersLayout_->setContentsMargins(Utils::scale_value(20), 0, 0, 0);
        stickersLayout_->setAlignment(Qt::AlignTop);

        scroll_area_widget->setLayout(stickersLayout_);

        stickersLayout_->addWidget(stickers_);

        // init buttons
        QHBoxLayout* buttonsLayout = new QHBoxLayout(this);

        buttonsLayout->setContentsMargins(0, getButtonMargin(), 0, getButtonMargin());

        if (!_purchased)
        {
            addButton_ = new QPushButton(this);

            Utils::ApplyStyle(addButton_, CommonStyle::getGreenButtonStyle());

            addButton_->setFlat(true);
            addButton_->setCursor(QCursor(Qt::PointingHandCursor));
            addButton_->setText(QT_TRANSLATE_NOOP("popup_window", "ADD"));
            addButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            addButton_->adjustSize();
            buttonsLayout->addWidget(addButton_);

            QObject::connect(addButton_, &QPushButton::clicked, this, &PackWidget::onAddButton, Qt::QueuedConnection);
        }
        else
        {
            removeButton_ = new QPushButton(this);

            Utils::ApplyStyle(removeButton_, CommonStyle::getRedButtonStyle());

            removeButton_->setFlat(true);
            removeButton_->setCursor(QCursor(Qt::PointingHandCursor));
            removeButton_->setText(QT_TRANSLATE_NOOP("popup_window", "REMOVE"));
            removeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            removeButton_->adjustSize();

            QObject::connect(removeButton_, &QPushButton::clicked, this, &PackWidget::onRemoveButton, Qt::QueuedConnection);

            buttonsLayout->addWidget(removeButton_);
        }

        shareButton_ = new CustomButton(this, qsl(":/resources/smiles_menu/share_100.png"));
        shareButton_->setHoverImage(qsl(":/resources/smiles_menu/share_100_hover.png"));

        shareButton_->setFixedHeight(getShareButtonWidth());
        shareButton_->setFixedHeight(getShareButtonWidth());
        shareButton_->setCursor(QCursor(Qt::PointingHandCursor));
        shareButton_->show();
        shareButton_->raise();
        shareButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Share"));

        moveShareButton();

        QObject::connect(shareButton_, &QPushButton::clicked, this, &PackWidget::onShareButton, Qt::QueuedConnection);

        QObject::connect(scroll_area_widget, &StickersView::stickerPreviewClose, this, &PackWidget::onStickerPreviewClose);
        QObject::connect(scroll_area_widget, &StickersView::stickerPreview, this, &PackWidget::onStickerPreview);
        QObject::connect(scroll_area_widget, &StickersView::stickerHovered, this, &PackWidget::onStickerHovered);

        rootVerticalLayout_->addLayout(buttonsLayout);
    }
}

void PackWidget::touchScrollStateChanged(QScroller::State _state)
{
    stickers_->blockSignals(_state != QScroller::Inactive);
}

void PackWidget::onAddButton(bool _checked)
{
    GetDispatcher()->addStickersPack(setId_, storeId_);

    emit buttonClicked();
}

void PackWidget::onRemoveButton(bool _checked)
{
    GetDispatcher()->removeStickersPack(setId_, storeId_);

    emit buttonClicked();
}

void PackWidget::onShareButton(bool _checked)
{
    emit shareClicked();
}

void PackWidget::onStickerPreviewClose()
{
    if (!stickerPreview_)
        return;

    stickerPreview_->hide();
    delete stickerPreview_;
    stickerPreview_ = nullptr;
}

void PackWidget::onStickerPreview(const int32_t _setId, const int32_t _stickerId)
{
    if (!stickerPreview_)
    {
        stickerPreview_ = new Smiles::StickerPreview(dialog_, _setId, _stickerId);
        stickerPreview_->setGeometry(dialog_->rect());
        stickerPreview_->show();
        stickerPreview_->raise();
    }
}

void PackWidget::onStickerHovered(const int32_t _setId, const int32_t _stickerId)
{
    if (stickerPreview_)
    {
        stickerPreview_->showSticker(_setId, _stickerId);
    }
}

void PackWidget::paintEvent(QPaintEvent* _e)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    return QWidget::paintEvent(_e);
}

void PackWidget::moveShareButton()
{
    if (shareButton_)
    {
        QRect rcParent = rect();

        shareButton_->setGeometry(
            rcParent.right() - getButtonMargin() - getShareButtonWidth(),
            rcParent.bottom() - getButtonMargin() - getShareButtonWidth(),
            getShareButtonWidth(),
            getShareButtonWidth());
    }
}

void PackWidget::resizeEvent(QResizeEvent* _e)
{
    moveShareButton();

    QWidget::resizeEvent(_e);
}








StickerPackInfo::StickerPackInfo(QWidget* _parent, const int32_t _set_id, const QString& _store_id)
    : QWidget(_parent)
    , set_id_(_set_id)
    , store_id_(_store_id)
{
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/pack_info")));

    pack_ = new PackWidget(this);

    parentDialog_.reset(new GeneralDialog(pack_, Utils::InterConnector::instance().getMainWindow()));
    parentDialog_->addHead();

    pack_->setParentDialog(parentDialog_->getMainHost());

    Ui::GetDispatcher()->getStickersPackInfo(set_id_, _store_id.toStdString());

    connect(Ui::GetDispatcher(), &core_dispatcher::onStickerpackInfo, this, &StickerPackInfo::onStickerpackInfo);
    connect(Ui::GetDispatcher(), &core_dispatcher::onSticker, this, &StickerPackInfo::stickerEvent);

    connect(pack_, &PackWidget::buttonClicked, this, [this]()
    {
        parentDialog_->close();
    });

    connect(pack_, &PackWidget::shareClicked, this, &StickerPackInfo::onShareClicked);

}

StickerPackInfo::~StickerPackInfo()
{
}

void StickerPackInfo::onShareClicked()
{
    parentDialog_->hide();

    SelectContactsWidget shareDialog(
        nullptr,
        Logic::MembersWidgetRegim::SHARE_LINK,
        QT_TRANSLATE_NOOP("stickers", "Share"),
        QT_TRANSLATE_NOOP("popup_window", "SEND"),
        Ui::MainPage::instance(),
        true);

    emit Utils::InterConnector::instance().searchEnd();

    QString sourceText = getStickerpackUrl(store_id_);

    const auto action = shareDialog.show();

    parentDialog_->close();

    if (action != QDialog::Accepted)
    {
        return;
    }

    const auto contact = shareDialog.getSelectedContact();

    if (!contact.isEmpty())
    {
        Logic::getContactListModel()->setCurrent(contact, -1, true);

        Ui::GetDispatcher()->sendMessageToContact(contact, sourceText);

        Utils::InterConnector::instance().onSendMessage(contact);
    }
    else
    {
        QApplication::clipboard()->setText(sourceText);
    }
}

void StickerPackInfo::stickerEvent(const qint32 _error, const qint32 _setId, const qint32 _stickerId)
{
    pack_->onStickerEvent(_error, _setId, _stickerId);
}


void StickerPackInfo::show()
{
    parentDialog_->showInPosition(-1, -1);
}

void StickerPackInfo::onStickerpackInfo(const bool _result, std::shared_ptr<Ui::Stickers::Set> _set)
{
    if (!_result)
        return;

    if (!_set)
        return;

    if (set_id_ != -1 && _set->getId() != set_id_)
        return;

    if (!store_id_.isEmpty() && store_id_ != _set->getStoreId())
        return;

    set_id_ = _set->getId();
    store_id_ = _set->getStoreId();

    pack_->onStickersPackInfo(_set, _result, _set->isPurchased());

    if (_result)
    {
        parentDialog_->addLabel(_set->getName());
    }
}

void StickerPackInfo::paintEvent(QPaintEvent* _e)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    return QWidget::paintEvent(_e);
}
