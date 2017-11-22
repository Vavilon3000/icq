#include "stdafx.h"
#include "TopPanel.h"
#include "../TitleBar.h"
#include "../../my_info.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/SChar.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../main_window/contact_list/SearchWidget.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

namespace
{
    const int left_margin = 16;
    const int left_margin_linux = 12;

    const int right_margin = 12;
    const int right_margin_linux = 6;

    const int burger_width = 24;
    const int burger_width_compact = 30;
    const int burger_height = 20;
}

namespace Ui
{
    BurgerWidget::BurgerWidget(QWidget* parent)
        : QWidget(parent)
        , Back_(false)
    {
    }

    void BurgerWidget::paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        QImage pix(QSize(burger_width * 2, burger_height * 2), QImage::Format_ARGB32);
        QPainter painter(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(ql1s("#454545")));
        pen.setWidth(4);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        pix.fill(Qt::transparent);

        const static std::array<QLine, 3> lines =
        {
            QLine(QPoint(2, 6), QPoint(44, 6)),
            QLine(QPoint(2, 20), QPoint(44, 20)),
            QLine(QPoint(2, 34), QPoint(44, 34))
        };
        painter.drawLines(lines.data(), lines.size());

        painter.end();
        Utils::check_pixel_ratio(pix);
        if (Utils::is_mac_retina())
            pix = pix.scaled(QSize(width() * 2, height() * 2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            pix = pix.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        p.fillRect(rect(), CommonStyle::getFrameColor());
        p.drawImage(0, 0, pix);
    }

    void BurgerWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        if (Back_)
            emit back();
        else
            emit clicked();

        return QWidget::mouseReleaseEvent(e);
    }

    void BurgerWidget::setBack(bool _back)
    {
        Back_ = _back;
        update();
    }

    TopPanelWidget::TopPanelWidget(QWidget* parent, SearchWidget* searchWidget)
        : QWidget(parent)
        , Mode_(NORMAL)
    {
        mainLayout = Utils::emptyHLayout();
        LeftSpacer_ = new QWidget(this);
        LeftSpacer_->setFixedWidth(Utils::scale_value(left_margin));
        LeftSpacer_->setStyleSheet(
            qsl("border-style: none; border-bottom-style:solid; background-color: %1;")
            .arg(CommonStyle::getFrameColor().name())
        );
        mainLayout->addWidget(LeftSpacer_);
        Burger_ = new BurgerWidget(this);
        Burger_->setFixedSize(Utils::scale_value(burger_width), Utils::scale_value(burger_height));
        Burger_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(Burger_);
	    AdditionalSpacerLeft_ = new QWidget(this);
        mainLayout->addWidget(AdditionalSpacerLeft_);
        AdditionalSpacerLeft_->setFixedWidth(Utils::scale_value(right_margin_linux));
        AdditionalSpacerLeft_->setStyleSheet("background: transparent; border-style: none;");
	    AdditionalSpacerLeft_->hide();
        searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mainLayout->addWidget(searchWidget);
        Search_ = searchWidget;
        RightSpacer_ = new QWidget(this);
        RightSpacer_->setStyleSheet(
            qsl("border-style: none; border-bottom-style:solid; border-right-style:solid; background-color: %1;")
            .arg(CommonStyle::getFrameColor().name())
            );
        mainLayout->addWidget(RightSpacer_);
	    Additional_ = Utils::emptyHLayout();
	    mainLayout->addLayout(Additional_);
	    AdditionalSpacerRight_ = new QWidget(this);
        mainLayout->addWidget(AdditionalSpacerRight_);
        AdditionalSpacerRight_->setStyleSheet(qsl("background: transparent; border-style: none;"));
        AdditionalSpacerRight_->setFixedWidth(Utils::scale_value(right_margin_linux));
	    AdditionalSpacerRight_->hide();
        RightSpacer_->setFixedWidth(Utils::scale_value(right_margin));
        setLayout(mainLayout);

        Utils::ApplyStyle(this,
            qsl("background-color: %1;"
                "border-style: solid;"
                "border-color: %2;"
                "border-width: 1dip;"
                "border-top: none;"
                "border-left: none;"
                "border-bottom: none;")
            .arg(CommonStyle::getFrameColor().name())
            .arg(CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER).name())
        );

        connect(Burger_, &Ui::BurgerWidget::back, this, &TopPanelWidget::back, Qt::QueuedConnection);
        connect(Burger_, &Ui::BurgerWidget::clicked, this, &TopPanelWidget::burgerClicked, Qt::QueuedConnection);
        connect(searchWidget, &Ui::SearchWidget::activeChanged, this, &TopPanelWidget::searchActiveChanged, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::titleButtonsUpdated, this, &TopPanelWidget::titleButtonsUpdated, Qt::QueuedConnection);
    }

    void TopPanelWidget::setMode(Mode _mode)
    {
        const auto isCompact = (_mode == Ui::TopPanelWidget::COMPACT);

        Burger_->setFixedWidth(Utils::scale_value(isCompact ? burger_width_compact : burger_width));
        Burger_->setVisible(_mode != SPREADED);
        Search_->setVisible(!isCompact);
        LeftSpacer_->setVisible(_mode != SPREADED);
#ifdef __linux__
	    LeftSpacer_->setFixedWidth(Utils::scale_value(isCompact ? left_margin : left_margin_linux));
#endif //__linux__

        if (isCompact)
        {
            RightSpacer_->setStyleSheet(
                qsl("border-style: none; border-bottom-style:solid; border-right-style:none; background-color: %1;")
                .arg(CommonStyle::getFrameColor().name())
                );
            RightSpacer_->show();
#ifdef __linux__
	        emit Utils::InterConnector::instance().hideTitleButtons();
	        AdditionalSpacerRight_->hide();
            AdditionalSpacerLeft_->hide();
#endif //__linux__
        }
        else
        {
            RightSpacer_->setStyleSheet(
                qsl("border-style: none; border-bottom-style:solid; border-right-style:solid; background-color: %1;")
                .arg(CommonStyle::getFrameColor().name())
                );
            RightSpacer_->hide();
#ifdef __linux__
	        emit Utils::InterConnector::instance().updateTitleButtons();
	        AdditionalSpacerRight_->show();
            AdditionalSpacerLeft_->show();
#endif //__linux__
        }
        RightSpacer_->setStyle(QApplication::style());

        Mode_ = _mode;
    }

    void TopPanelWidget::searchActiveChanged(bool active)
    {
#ifdef __linux__
        if (active)
	        emit Utils::InterConnector::instance().hideTitleButtons();
        else
            emit Utils::InterConnector::instance().updateTitleButtons();
#endif //__linux__
    }

    void TopPanelWidget::titleButtonsUpdated()
    {
#ifdef __linux__
        if (Mode_ == Ui::TopPanelWidget::COMPACT)
            emit Utils::InterConnector::instance().hideTitleButtons();
#endif //__linux__
    }

    void TopPanelWidget::setBack(bool _back)
    {
        if (Mode_ == Ui::TopPanelWidget::COMPACT)
            Burger_->setBack(_back);
    }

    void TopPanelWidget::searchActivityChanged(bool _active)
    {
        Burger_->setVisible(Mode_ != Ui::TopPanelWidget::SPREADED);
    }

    void TopPanelWidget::addWidget(QWidget* w)
    {
	    Additional_->addWidget(w);
    }

    void TopPanelWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
