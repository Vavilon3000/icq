#include "stdafx.h"
#include "Store.h"

#include "../../controls/TransparentScrollBar.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../main_window/MainWindow.h"
#include "../../fonts.h"
#include "../../cache/stickers/stickers.h"
#include "../../core_dispatcher.h"
#include "../contact_list/ContactListModel.h"

using namespace Ui;
using namespace Stickers;

int getMarginLeft()
{
    return Utils::scale_value(20);
}

int getMarginRight()
{
    return Utils::scale_value(20);
}

int getHeaderHeight()
{
    return Utils::scale_value(52);
}

int getPackIconSize()
{
    return Utils::scale_value(92);
}

int getIconMarginV()
{
    return Utils::scale_value(4);
}

int getIconMarginH()
{
    return Utils::scale_value(8);
}

int getPackNameHeight()
{
    return Utils::scale_value(40);
}

int getPacksViewHeight()
{
    return (getPackIconSize() + getIconMarginV() + getPackNameHeight());
}

int getHScrollbarHeight()
{
    return Utils::scale_value(16);
}

int getMyStickerMarginBottom()
{
    return Utils::scale_value(12);
}

int getMyStickerHeight()
{
    return getMyStickerMarginBottom() + Utils::scale_value(52);
}

int getMyPackIconSize()
{
    return Utils::scale_value(52);
}

int getMyPackTextLeftMargin()
{
    return Utils::scale_value(12);
}

int getMyPackTextTopMargin()
{
    return Utils::scale_value(4);
}

int getMyPackDescTopMargin()
{
    return Utils::scale_value(28);
}

int getDelButtonHeight()
{
    return Utils::scale_value(20);
}

QColor getHoverColor()
{
    return QColor(0, 0, 0, (int)(255.0 * 0.1));
}

QString getEmptyMyStickersText()
{
    return QT_TRANSLATE_NOOP("stickers", "You have not added stickers yet");
}

QString getBotUin()
{
    return qsl("100500");
}

int getMyHeaderHeight()
{
    return Utils::scale_value(44);
}
//////////////////////////////////////////////////////////////////////////
// class PacksView
//////////////////////////////////////////////////////////////////////////
PacksView::PacksView(ScrollAreaWithTrScrollBar* _parent)
    : QWidget(_parent)
    , parent_(_parent)
    , hoveredPack_(-1)
    , animScroll_(nullptr)
{
    setFixedHeight(getPacksViewHeight());
    setCursor(QCursor(Qt::PointingHandCursor));
}

QRect PacksView::getStickerRect(const int _pos)
{
    return QRect(getMarginLeft() + _pos*(getPackIconSize() + 2*getIconMarginH()), 0, getPackIconSize(), getPackIconSize() + getPackNameHeight() + getIconMarginV());
}

void PacksView::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    static auto hoveredBrush = QBrush(getHoverColor());
    static auto font(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));

    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.intersects(_e->rect()))
        {
            const QRect imageRect(stickerRect.left(), stickerRect.top(), getPackIconSize(), getPackIconSize());

            if ((int) i == hoveredPack_)
            {
                p.save();

                p.setBrush(hoveredBrush);
                p.setPen(Qt::PenStyle::NoPen);
                p.setRenderHint(QPainter::HighQualityAntialiasing);

                const qreal radius = qreal(Utils::scale_value(8));

                p.drawRoundedRect(imageRect, radius, radius);

                p.restore();
            }

            if (!packs_[i].icon_.isNull())
            {
                p.drawPixmap(imageRect, packs_[i].icon_);
            }

            QRect textRect = stickerRect;
            textRect.setTop(stickerRect.height() - getPackNameHeight());

            static QFontMetrics nameMetrics(font);
            auto elidedString = nameMetrics.elidedText(packs_[i].name_, Qt::ElideRight, stickerRect.width());

            p.setFont(font);
            p.drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, elidedString);
        }
    }

    QWidget::paintEvent(_e);
}

QPixmap scaleIcon(const QPixmap& _icon, const int _toSize)
{
    int coeff = Utils::is_mac_retina() ? 2 : 1;

    QSize szNeed(_toSize * coeff, _toSize * coeff);

    return _icon.scaled(szNeed, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void PacksView::onSetIcon(const int32_t _setId)
{
    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        if (packs_[i].id_ == _setId)
        {
            auto pack = Stickers::getStoreSet(_setId);
            if (pack)
            {
                packs_[i].icon_ = scaleIcon(pack->getBigIcon(), getPackIconSize());

                Utils::check_pixel_ratio(packs_[i].icon_);
            }

            const QRect stickerRect = getStickerRect(i);

            repaint(stickerRect);

            break;
        }
    }
}

void PacksView::addPack(PackInfo _pack)
{
    packs_.push_back(std::move(_pack));
}

void PacksView::updateSize()
{
    setFixedWidth(getMarginLeft() + packs_.size() * (getPackIconSize() + getIconMarginH() * 2) + getMarginRight());
}

void PacksView::wheelEvent(QWheelEvent* _e)
{
    const int numDegrees = _e->delta() / 8;
    const int numSteps = numDegrees / 15;

    if (!numSteps || !numDegrees)
        return;

    if (numSteps > 0)
        scrollStep(direction::left);
    else
        scrollStep(direction::right);

    _e->accept();

    parent_->fadeIn();
}

void PacksView::scrollStep(direction _direction)
{
    QRect viewRect = parent_->viewport()->geometry();
    auto scrollbar = parent_->horizontalScrollBar();

    int maxVal = scrollbar->maximum();
    int minVal = scrollbar->minimum();
    int curVal = scrollbar->value();

    int step = viewRect.width() / 2;

    int to = 0;

    if (_direction == PacksView::direction::right)
    {
        to = curVal + step;
        if (to > maxVal)
        {
            to = maxVal;
        }
    }
    else
    {
        to = curVal - step;
        if (to < minVal)
        {
            to = minVal;
        }

    }

    QEasingCurve easing_curve = QEasingCurve::InQuad;
    int duration = 300;

    if (!animScroll_)
        animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);

    animScroll_->stop();
    animScroll_->setDuration(duration);
    animScroll_->setStartValue(curVal);
    animScroll_->setEndValue(to);
    animScroll_->setEasingCurve(easing_curve);
    animScroll_->start();
}

void PacksView::mousePressEvent(QMouseEvent* _e)
{
    QWidget::mousePressEvent(_e);

    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.contains(_e->pos()))
        {
            Stickers::showStickersPack(packs_[i].id_);

            return;
        }
    }
}

void PacksView::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    if (hoveredPack_ >= 0)
    {
        const auto prevHoveredPack = hoveredPack_;

        hoveredPack_ = -1;

        const auto prevRect = getStickerRect(prevHoveredPack);

        repaint(prevRect);
    }
}

void PacksView::mouseMoveEvent(QMouseEvent* _e)
{
    QWidget::mouseMoveEvent(_e);

    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.contains(_e->pos()))
        {
            if (hoveredPack_ == (int) i)
                return;

            const auto prevHoveredPack = hoveredPack_;

            hoveredPack_ = i;

            if (prevHoveredPack >= 0)
            {
                const auto prevRect = getStickerRect(prevHoveredPack);

                repaint(prevRect);
            }

            repaint(stickerRect);

            return;
        }
    }
}

void PacksView::clear()
{
    packs_.clear();
}


//////////////////////////////////////////////////////////////////////////
// class PacksWidget
//////////////////////////////////////////////////////////////////////////
PacksWidget::PacksWidget(QWidget* _parent)
    : QWidget(_parent)
{
    Ui::GetDispatcher()->getStickersStore();

    setFixedHeight(getPacksViewHeight() + getHeaderHeight() + getHScrollbarHeight() + Utils::scale_value(8));

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setAlignment(Qt::AlignTop);
    setLayout(rootLayout);

    QHBoxLayout* headerLayout = new QHBoxLayout(nullptr);
    headerLayout->setContentsMargins(getMarginLeft(), 0, 0, 0);
    auto header = new QLabel(this);
    header->setFixedHeight(getHeaderHeight());
    header->setText(QT_TRANSLATE_NOOP("stickers", "Popular"));
    header->setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Medium));
    headerLayout->addWidget(header);
    rootLayout->addLayout(headerLayout);

    scrollArea_ = CreateScrollAreaAndSetTrScrollBarH(this);
    scrollArea_->setFocusPolicy(Qt::NoFocus);
    scrollArea_->setFixedHeight(getPacksViewHeight() + getHScrollbarHeight());
    Utils::grabTouchWidget(scrollArea_->viewport(), true);

    packs_ = new PacksView(scrollArea_);

    scrollArea_->setWidget(packs_);
    scrollArea_->setWidgetResizable(true);

    rootLayout->addWidget(scrollArea_);

    init(false);
}

void PacksWidget::init(const bool _fromServer)
{
    packs_->clear();

    for (const auto _setId : Stickers::getStoreStickersSets())
    {
        const auto stickersSet = Stickers::getStoreSet(_setId);

        if (!stickersSet)
            continue;

        if (!stickersSet->isPurchased())
        {
            packs_->addPack(PackInfo(
                _setId,
                stickersSet->getName(),
                stickersSet->getDescription(),
                stickersSet->getStoreId(),
                stickersSet->getBigIcon()));
        }
    }

    packs_->updateSize();
}

void PacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}




//////////////////////////////////////////////////////////////////////////
// MyPacksView
//////////////////////////////////////////////////////////////////////////
MyPacksView::MyPacksView(QWidget* _parent)
{
    setCursor(QCursor(Qt::PointingHandCursor));
}

void MyPacksView::addPack(PackInfo _pack)
{
    packs_.push_back(std::move(_pack));
}

void MyPacksView::updateSize()
{
    setFixedHeight(packs_.size() * getMyStickerHeight());
}

void MyPacksView::clear()
{
    packs_.clear();
}

QRect MyPacksView::getStickerRect(const int _pos)
{
    return QRect(
        0,
        _pos * getMyStickerHeight(),
        geometry().width(),
        getMyStickerHeight());
}

QRect MyPacksView::getDelButtonRect(const QRect& _stickerRect)
{
    return QRect(
        _stickerRect.width() - getDelButtonHeight(),
        _stickerRect.top() + (_stickerRect.height() - getDelButtonHeight())/2,
        getDelButtonHeight(),
        getDelButtonHeight());
}


QFont getNameFont()
{
    return Fonts::appFontScaled(14);
}

QFont getDescFont()
{
    return Fonts::appFontScaled(12);
}


TextEmojiWidget& getNameWidget()
{
    static TextEmojiWidget nameWidget(nullptr, getNameFont(), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));

    nameWidget.setStyleSheet("background: transparent");

    return nameWidget;
}

TextEmojiWidget& getDescWidget()
{
    static TextEmojiWidget descWidget(nullptr, getDescFont(), CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));

    descWidget.setStyleSheet("background: transparent");

    return descWidget;
}

CustomButton& getDelButton()
{
    static CustomButton button(nullptr, qsl(":/smiles_menu/i_delete_a_100"));

    button.setStyleSheet("background: transparent");

    button.setFixedWidth(getDelButtonHeight());
    button.setFixedHeight(getDelButtonHeight());

    return button;
}



void MyPacksView::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        const QRect iconRect(stickerRect.left(), stickerRect.top(), getMyPackIconSize(), getMyPackIconSize());

        if (stickerRect.intersects(_e->rect()))
        {
            if (!packs_[i].icon_.isNull())
            {
                p.drawPixmap(iconRect, packs_[i].icon_);
            }

            p.save();
            p.translate(stickerRect.topLeft());

            static auto nameFont = getNameFont();
            static auto descFont = getDescFont();

            const int textWidth = stickerRect.width() - iconRect.right() - getDelButtonHeight() - 2 * getMarginRight();

            static QFontMetrics nameMetrics(nameFont);
            auto elidedString = nameMetrics.elidedText(packs_[i].name_, Qt::ElideRight, textWidth);

            static auto& nameWidget = getNameWidget();
            nameWidget.setText(elidedString);
            nameWidget.render(&p, QPoint(getMyStickerHeight() + getMyPackTextLeftMargin(), getMyPackTextTopMargin()));

            static QFontMetrics descMetrics(descFont);
            elidedString = descMetrics.elidedText(packs_[i].comment_, Qt::ElideRight, textWidth);

            static auto& commentWidget = getDescWidget();
            commentWidget.setText(elidedString);
            commentWidget.render(&p, QPoint(getMyStickerHeight() + getMyPackTextLeftMargin(), getMyPackDescTopMargin()));

            p.restore();

            p.save();

            p.translate(rect().topLeft());

            auto btnRect = getDelButtonRect(stickerRect);
            static auto& delButton = getDelButton();
            delButton.render(&p, btnRect.topLeft());

            p.restore();

        }
    }

    QWidget::paintEvent(_e);
}

void MyPacksView::mousePressEvent(QMouseEvent* _e)
{
    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        const QRect stickerRect = getStickerRect(i);

        if (stickerRect.contains(_e->pos()))
        {
            const QRect delButtonRect = getDelButtonRect(stickerRect);

            if (delButtonRect.contains(_e->pos()))
            {
                const auto confirm = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
                    QT_TRANSLATE_NOOP("popup_window", "YES"),
                    QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to remove this sticker pack?"),
                    QT_TRANSLATE_NOOP("popup_window", "Remove sticker pack"),
                    nullptr);

                if (confirm)
                {
                    GetDispatcher()->removeStickersPack(packs_[i].id_, QString());
                }

                return;
            }

            Stickers::showStickersPack(packs_[i].id_);

            return;
        }
    }
}


void MyPacksView::onSetIcon(const int32_t _setId)
{
    for (unsigned int i = 0; i < packs_.size(); ++i)
    {
        if (packs_[i].id_ == _setId)
        {
            auto pack = Stickers::getStoreSet(_setId);
            if (pack)
            {
                packs_[i].icon_ = scaleIcon(pack->getBigIcon(), Utils::scale_value(52));

                Utils::check_pixel_ratio(packs_[i].icon_);
            }

            const QRect stickerRect = getStickerRect(i);

            repaint(stickerRect);

            break;
        }
    }
}


bool MyPacksView::empty() const
{
    return packs_.empty();
}


MyPacksHeader::MyPacksHeader(QWidget* _parent)
    : font_(Fonts::appFontScaled(16, Fonts::FontWeight::Medium))
    , createStickerPack_(QT_TRANSLATE_NOOP("stickers", "Create stickerpack"))
    , metrics_(font_)
    , packLength_(metrics_.width(createStickerPack_))
    , marginText_(Utils::scale_value(8))
    , buttonWidth_(Utils::scale_value(20))
{
    setFixedHeight(getMyHeaderHeight());

    buttonAdd_ = new QPushButton(this);
    buttonAdd_->setStyleSheet("border: none;");
    buttonAdd_->show();
    buttonAdd_->setCursor(Qt::PointingHandCursor);

    QObject::connect(buttonAdd_, &QPushButton::clicked, this, &MyPacksHeader::clicked);


}

void MyPacksHeader::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    static const QString myText = QT_TRANSLATE_NOOP("stickers", "My");

    static const auto penMy(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
    static const auto penPack(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));

    static const int xheight = metrics_.boundingRect('a').height();
    static QPixmap addPackButtonImage(Utils::ScaleStyle(":/smiles_menu/i_plus_100", Utils::getScaleCoefficient()));

    Utils::check_pixel_ratio(addPackButtonImage);

    const int upMargin = (height() - xheight) / 2 + Utils::scale_value(1);

    p.setFont(font_);
    p.setPen(penMy);
    p.drawText(QPoint(0, upMargin + xheight), myText);

    const int textX = rect().width() - packLength_;

    p.setPen(penPack);
    p.drawText(QPoint(rect().width() - packLength_, upMargin + xheight), createStickerPack_);

    const int offsetY = (height() - buttonWidth_) / 2;

    QRect addPackRect(textX - buttonWidth_ - marginText_, offsetY, buttonWidth_, buttonWidth_);

    p.drawPixmap(addPackRect, addPackButtonImage);

    QWidget::paintEvent(_e);
}

void MyPacksHeader::resizeEvent(QResizeEvent * event)
{
    auto rc = geometry();

    const int buttonWidthAll = packLength_ + marginText_ + buttonWidth_;

    buttonAdd_->setGeometry(rc.width() - buttonWidthAll, 0, buttonWidthAll, rc.height());
}



//////////////////////////////////////////////////////////////////////////
// MyPacksWidget
//////////////////////////////////////////////////////////////////////////
MyPacksWidget::MyPacksWidget(QWidget* _parent)
    : QWidget(_parent)
    , syncedWithServer_(false)
    , ref_(new bool(false))
{
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(getMarginLeft(), 0, getMarginRight(), 0);
    rootLayout->setSpacing(0);
    rootLayout->setAlignment(Qt::AlignTop);

    MyPacksHeader* headerWidget = new MyPacksHeader(this);

    QObject::connect(headerWidget, &MyPacksHeader::clicked, this, &MyPacksWidget::createPackButtonClicked);

    rootLayout->addWidget(headerWidget);

    packs_ = new MyPacksView(this);

    rootLayout->addWidget(packs_);

    setLayout(rootLayout);

    init(false);
}




void MyPacksWidget::init(const bool _fromServer)
{
    if (_fromServer)
        syncedWithServer_ = true;

    packs_->clear();

    for (const auto _setId : Stickers::getStoreStickersSets())
    {
        const auto stickersSet = Stickers::getStoreSet(_setId);

        if (!stickersSet)
            continue;

        if (stickersSet->isPurchased())
        {
            packs_->addPack(PackInfo(
                _setId,
                stickersSet->getName(),
                stickersSet->getDescription(),
                stickersSet->getStoreId(),
                stickersSet->getBigIcon()));
        }
    }

    packs_->updateSize();

    QRect rcText(0, packs_->geometry().top(), rect().width(), rect().height() - packs_->geometry().top());

    repaint(rcText);
}

void MyPacksWidget::onSetIcon(const int32_t _setId)
{
    packs_->onSetIcon(_setId);
}

void MyPacksWidget::paintEvent(QPaintEvent* _e)
{
    if (syncedWithServer_ && packs_->empty())
    {
        QPainter p(this);

        static auto font(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));

        static const auto pen(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));

        static auto emptyText = getEmptyMyStickersText();

        p.setFont(font);

        p.setPen(pen);

        auto rcText = rect();

        p.drawText(rcText, emptyText, Qt::AlignVCenter | Qt::AlignHCenter);
    }

    QWidget::paintEvent(_e);
}

void MyPacksWidget::createPackButtonClicked(bool)
{
    std::weak_ptr<bool> wr_ref = ref_;

    const auto bot = getBotUin();

    Logic::getContactListModel()->setCurrent(bot, -1, true);

    Logic::getContactListModel()->addContactToCL(bot, [this, wr_ref, bot](bool _res)
    {
        auto ref = wr_ref.lock();
        if (!ref)
            return;

        Logic::getContactListModel()->setCurrent(bot, -1, true);
    });
}


//////////////////////////////////////////////////////////////////////////
// class Store
//////////////////////////////////////////////////////////////////////////
Store::Store(QWidget* _parent)
    : QWidget(_parent)
{
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/store")));

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setAlignment(Qt::AlignTop);
    setLayout(rootLayout);

    rootScrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    rootScrollArea_->setFocusPolicy(Qt::NoFocus);
    Utils::grabTouchWidget(rootScrollArea_->viewport(), true);

    connect(QScroller::scroller(rootScrollArea_->viewport()), &QScroller::stateChanged, this, &Store::touchScrollStateChanged, Qt::QueuedConnection);

    QWidget* scrollAreaWidget = new QWidget(rootScrollArea_);
    scrollAreaWidget->setObjectName("RootWidget");

    rootScrollArea_->setWidget(scrollAreaWidget);
    rootScrollArea_->setWidgetResizable(true);

    rootLayout->addWidget(rootScrollArea_);

    QVBoxLayout* widgetLayout = new QVBoxLayout();
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setAlignment(Qt::AlignTop);
    scrollAreaWidget->setLayout(widgetLayout);

    packsView_ = new PacksWidget(this);
    widgetLayout->addWidget(packsView_);
    Utils::grabTouchWidget(packsView_);

    myPacks_ = new MyPacksWidget(this);
    widgetLayout->addWidget(myPacks_);
    Utils::grabTouchWidget(myPacks_);

    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::onSetBigIcon, this, &Store::onSetBigIcon);
    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::onStore, this, &Store::onStore);
    QObject::connect(Ui::GetDispatcher(), &core_dispatcher::onStickers, this, &Store::onStickers);
}

void Store::touchScrollStateChanged(QScroller::State _state)
{
    packsView_->blockSignals(_state != QScroller::Inactive);
}

void Store::onStore()
{
    packsView_->init(true);
    myPacks_->init(true);
}

void Store::onStickers()
{
    Ui::GetDispatcher()->getStickersStore();
}


void Store::onSetBigIcon(const qint32 _error, const qint32 _setId)
{
    if (_error != 0)
        return;


    packsView_->onSetIcon(_setId);
    myPacks_->onSetIcon(_setId);
}

void Store::resizeEvent(QResizeEvent * event)
{
    auto rc = geometry();

    packsView_->setFixedWidth(rc.width());
}

void Store::paintEvent(QPaintEvent* _e)
{
    /*QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);*/

    return QWidget::paintEvent(_e);
}
