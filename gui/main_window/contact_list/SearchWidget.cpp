#include "stdafx.h"
#include "SearchWidget.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

namespace Ui
{
    SearchWidget::SearchWidget(QWidget* _parent, int _add_hor_space, int _add_ver_space)
        : QWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(32) + _add_ver_space * 2);
        active_ = false;
        vMainLayout_ = Utils::emptyVLayout(this);
        vMainLayout_->setContentsMargins(Utils::scale_value(12) + _add_hor_space, _add_ver_space, Utils::scale_value(12) + _add_hor_space, _add_ver_space);
        hMainLayout = Utils::emptyHLayout();

        searchWidget_ = new QWidget(this);
        Testing::setAccessibleName(searchWidget_, qsl("AS searchWidget_"));

        hSearchLayout_ = Utils::emptyHLayout(searchWidget_);
        hSearchLayout_->setContentsMargins(Utils::scale_value(4), 0, Utils::scale_value(8), 0);

        searchEdit_ = new LineEditEx(this);
        searchEdit_->setFixedHeight(Utils::scale_value(24));
        searchEdit_->setFont(Fonts::appFontScaled(14));
        searchEdit_->setFrame(QFrame::NoFrame);

        searchEdit_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
        searchEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        Testing::setAccessibleName(searchEdit_, qsl("search_edit"));
        hSearchLayout_->addSpacing(0);
        hSearchLayout_->addWidget(searchEdit_);

        closeIcon_ = new Ui::CustomButton(this, qsl(":/controls/close_a_100"));
        closeIcon_->setActiveImage(qsl(":/controls/close_d_100"));
        closeIcon_->setHoverImage(qsl(":/controls/close_d_100"));
        closeIcon_->setFixedSize(Utils::scale_value(28), Utils::scale_value(24));
        closeIcon_->setStyleSheet(qsl("border: none;"));
        closeIcon_->setFocusPolicy(Qt::NoFocus);
        closeIcon_->setCursor(Qt::PointingHandCursor);
        Testing::setAccessibleName(closeIcon_, qsl("AS closeIcon_"));
        hSearchLayout_->addWidget(closeIcon_);
        closeIcon_->hide();

        hMainLayout->addWidget(searchWidget_);
        vMainLayout_->addLayout(hMainLayout);

        auto style = qsl("border-style: solid; border-width: 1dip; border-color: %1; border-radius: 16dip; background-color: %2; color: %3;")
            .arg(
                CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_DARK).name(),
                CommonStyle::getFrameColor().name(),
                CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY).name()
            );
        Utils::ApplyStyle(this, style);
        searchEdit_->setStyleSheet(qsl("border: none; background: transparent;"));

        connect(searchEdit_, &LineEditEx::textEdited, this, &SearchWidget::searchChanged, Qt::QueuedConnection);
        connect(searchEdit_, &LineEditEx::clicked, this, &SearchWidget::searchStarted, Qt::QueuedConnection);
        connect(searchEdit_, &LineEditEx::escapePressed, this, &SearchWidget::onEscapePress, Qt::QueuedConnection);
        connect(searchEdit_, &LineEditEx::enter, this, &SearchWidget::editEnterPressed, Qt::QueuedConnection);
        connect(searchEdit_, &LineEditEx::upArrow, this, &SearchWidget::editUpPressed, Qt::QueuedConnection);
        connect(searchEdit_, &LineEditEx::downArrow, this, &SearchWidget::editDownPressed, Qt::QueuedConnection);
        connect(searchEdit_, &LineEditEx::focusOut, this, &SearchWidget::focusedOut, Qt::QueuedConnection);
        connect(closeIcon_, &QPushButton::clicked, this, &SearchWidget::searchCompleted, Qt::QueuedConnection);

        setDefaultPlaceholder();
        setActive(false);
    }

    SearchWidget::~SearchWidget()
    {
    }

    void SearchWidget::setFocus()
    {
        searchEdit_->setFocus(Qt::MouseFocusReason);
        setActive(true);
    }

    void SearchWidget::clearFocus()
    {
        searchEdit_->clearFocus();
        setActive(false);
    }

    bool SearchWidget::isActive() const
    {
        return active_;
    }

    void SearchWidget::setDefaultPlaceholder()
    {
        setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Search"));
    }

    void SearchWidget::setPlaceholderText(const QString & _text)
    {
        searchEdit_->setPlaceholderText(_text);
    }

    void SearchWidget::setActive(bool _active)
    {
        if (active_ == _active)
            return;

        auto style = qsl("border-style: solid; border-width: 1dip; border-color: %1; border-radius: 16dip; background-color: %2; color: %3;")
            .arg(
                CommonStyle::getColor(_active ? CommonStyle::Color::GREEN_FILL : CommonStyle::Color::GRAY_FILL_DARK).name(),
                CommonStyle::getFrameColor().name(),
                CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY).name()
            );
        Utils::ApplyStyle(this, style);

        active_ = _active;
        closeIcon_->setVisible(_active);
        setDefaultPlaceholder();
        emit activeChanged(_active);
    }

    void SearchWidget::searchStarted()
    {
        setActive(true);
    }

    QString SearchWidget::getText() const
    {
        return searchEdit_->text();
    }

    bool SearchWidget::isEmpty() const
    {
        return getText().isEmpty();
    }

    void SearchWidget::clearInput()
    {
        searchEdit_->clear();
    }

    void SearchWidget::searchCompleted()
    {
        clearInput();
        setActive(false);
        searchEdit_->clearFocus();
        emit searchChanged(QString());
        emit searchEnd();
        Utils::InterConnector::instance().setFocusOnInput();
    }

    void SearchWidget::onEscapePress()
    {
        searchCompleted();
        emit escapePressed();
    }

    void SearchWidget::searchChanged(const QString& _text)
    {
        if (!_text.isEmpty())
            setActive(true);

        if (!_text.isEmpty())
        {
            emit searchBegin();
        }
        else
        {
            emit inputEmpty();
        }

        emit search(platform::is_apple() ? _text.toLower() : _text);
    }

    void SearchWidget::editEnterPressed()
    {
        emit enterPressed();
    }

    void SearchWidget::editUpPressed()
    {
        emit upPressed();
    }

    void SearchWidget::editDownPressed()
    {
        emit downPressed();
    }

    void SearchWidget::focusedOut()
    {
        setActive(false);
    }
}
