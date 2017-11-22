#include "stdafx.h"
#include "toolbar.h"

#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../controls/PictureWidget.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

namespace Ui
{
    using namespace Smiles;

    AttachedView::AttachedView(QWidget* _view, QWidget* _viewParent)
        : View_(_view),
        ViewParent_(_viewParent)
    {

    }

    QWidget* AttachedView::getView()
    {
        return View_;
    }

    QWidget* AttachedView::getViewParent()
    {
        return ViewParent_;
    }




    //////////////////////////////////////////////////////////////////////////
    // AddButton
    //////////////////////////////////////////////////////////////////////////
    AddButton::AddButton(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedSize(Utils::scale_value(52), Utils::scale_value(_parent->geometry().height()));

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, Utils::scale_value(12), 0);
        layout->setAlignment(Qt::AlignRight);

        auto button = new CustomButton(this, qsl(":/smiles_menu/i_add_a_100"));
        button->setActiveImage(qsl(":/smiles_menu/i_add_a_100"));
        button->setFillColor(CommonStyle::getFrameColor());
        button->setFixedSize(Utils::scale_value(32), Utils::scale_value(32));
        button->setStyleSheet(qsl("border: none;"));
        button->setFocusPolicy(Qt::NoFocus);
        button->setCursor(QCursor(Qt::PointingHandCursor));

        QObject::connect(button, &QPushButton::clicked, this, [this](const bool)
        {
            emit clicked();
        });

        layout->addWidget(button);

        setLayout(layout);

    }

    void AddButton::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }


    void AddButton::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        
    }


    //////////////////////////////////////////////////////////////////////////
    // TabButton class
    //////////////////////////////////////////////////////////////////////////
    TabButton::TabButton(QWidget* _parent)
        : CustomButton(_parent, nullptr)
        , AttachedView_(nullptr, nullptr)
        , Fixed_(false)

    {
        Init();
    }

    TabButton::TabButton(QWidget* _parent, const QString& _resource)
        : CustomButton(_parent, _resource)
        , resource_(_resource)
        , AttachedView_(nullptr)
        , Fixed_(false)
    {
        Init();
    }

    TabButton::~TabButton()
    {

    }

    void TabButton::Init()
    {
        setObjectName(qsl("tab_button_Widget"));

        setCursor(QCursor(Qt::PointingHandCursor));
        setCheckable(true);

        setFocusPolicy(Qt::NoFocus);
    }

    void TabButton::AttachView(const AttachedView& _view)
    {
        AttachedView_ = _view;
    }

    const AttachedView& TabButton::GetAttachedView() const
    {
        return AttachedView_;
    }

    void TabButton::setFixed(const bool _isFixed)
    {
        Fixed_ = _isFixed;
    }

    bool TabButton::isFixed() const
    {
        return Fixed_;
    }



    //////////////////////////////////////////////////////////////////////////
    // Toolbar class
    //////////////////////////////////////////////////////////////////////////
    Toolbar::Toolbar(QWidget* _parent, buttons_align _align)
        : QFrame(_parent)
        , align_(_align)
        , buttonStore_(nullptr)
        , AnimScroll_(nullptr)
    {
        setObjectName(qsl("toolbar_Widget"));

        auto rootLayout = Utils::emptyHLayout();
        setLayout(rootLayout);

        viewArea_ = new QScrollArea(this);
        viewArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QWidget* scrollAreaWidget = new QWidget(viewArea_);
        scrollAreaWidget->setObjectName(qsl("scroll_area_widget"));
        viewArea_->setWidget(scrollAreaWidget);
        viewArea_->setWidgetResizable(true);
        viewArea_->setFocusPolicy(Qt::NoFocus);

        Utils::grabTouchWidget(viewArea_->viewport(), true);
        Utils::grabTouchWidget(scrollAreaWidget);
        connect(QScroller::scroller(viewArea_->viewport()), &QScroller::stateChanged, this, &Toolbar::touchScrollStateChanged, Qt::QueuedConnection);

        rootLayout->addWidget(viewArea_);

        horLayout_ = Utils::emptyHLayout();

        scrollAreaWidget->setLayout(horLayout_);

        QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horLayout_->addSpacerItem(horizontalSpacer);

        if (_align == buttons_align::center)
        {
            QSpacerItem* horizontalSpacerCenter = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            horLayout_->addSpacerItem(horizontalSpacerCenter);
        }

        initScroll();
    }

    Toolbar::~Toolbar()
    {

    }

    void Toolbar::addButtonStore()
    {
        buttonStore_ = new AddButton(this);
        buttonStore_->show();

        QObject::connect(buttonStore_, &AddButton::clicked, this, &Toolbar::buttonStoreClick);

        horLayout_->setContentsMargins(0, 0, buttonStore_->width(), 0);
    }

    void Toolbar::buttonStoreClick()
    {
        emit Utils::InterConnector::instance().showStickersStore();
    }

    void Toolbar::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        if (buttonStore_)
        {
            QRect thisRect = geometry();

            buttonStore_->setFixedHeight(thisRect.height() - Utils::scale_value(1));
            buttonStore_->move(thisRect.width() - buttonStore_->width(), 0);
        }
    }

    void Toolbar::wheelEvent(QWheelEvent* _e)
    {
        int numDegrees = _e->delta() / 8;
        int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;

        if (numSteps > 0)
            scrollStep(direction::left);
        else
            scrollStep(direction::right);

        qDebug() << "Toolbar::wheelEvent" << " numDegrees=" << numDegrees << ", numSteps=" << numSteps;

        QWidget::wheelEvent(_e);
    }

    void Toolbar::initScroll()
    {
    }

    void Toolbar::touchScrollStateChanged(QScroller::State state)
    {
        for (auto iter : buttons_)
        {
            iter->setAttribute(Qt::WA_TransparentForMouseEvents, state != QScroller::Inactive);
        }
    }

    void Toolbar::scrollStep(direction _direction)
    {
        QRect viewRect = viewArea_->viewport()->geometry();
        auto scrollbar = viewArea_->horizontalScrollBar();

        int maxVal = scrollbar->maximum();
        int minVal = scrollbar->minimum();
        int curVal = scrollbar->value();

        int step = viewRect.width() / 2;

        int to = 0;

        if (_direction == Toolbar::direction::right)
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

        if (!AnimScroll_)
            AnimScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);

        AnimScroll_->stop();
        AnimScroll_->setDuration(duration);
        AnimScroll_->setStartValue(curVal);
        AnimScroll_->setEndValue(to);
        AnimScroll_->setEasingCurve(easing_curve);
        AnimScroll_->start();
    }

    void Toolbar::addButton(TabButton* _button)
    {
        _button->setAutoExclusive(true);

        int index = 0;
        if (align_ == buttons_align::left)
            index = horLayout_->count() - 1;
        else if (align_ == buttons_align::right)
            index = horLayout_->count();
        else
            index = horLayout_->count() - 1;


        Utils::grabTouchWidget(_button);

        horLayout_->insertWidget(index, _button);

        buttons_.push_back(_button);
    }

    void Toolbar::Clear(const bool _delFixed)
    {
        for (auto iter = buttons_.begin(); iter != buttons_.end();)
        {
            auto button = *iter;

            if (!_delFixed && button->isFixed())
            {
                ++iter;
                continue;
            }

            horLayout_->removeWidget(button);
            button->deleteLater();

            iter = buttons_.erase(iter);
        }
    }

    TabButton* Toolbar::addButton(const QString& _resource)
    {
        TabButton* button = new TabButton(this, _resource);

        addButton(button);

        return button;
    }

    TabButton* Toolbar::addButton(const QPixmap& _icon)
    {
        TabButton* button = new TabButton(this);

        button->setImage(_icon);

        addButton(button);

        return button;
    }

    void Toolbar::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    const std::list<TabButton*>& Toolbar::GetButtons() const
    {
        return buttons_;
    }

    void Toolbar::scrollToButton(TabButton* _button)
    {
        viewArea_->ensureWidgetVisible(_button);
    }
}
