#include "stdafx.h"
#include "SemitransparentWindowAnimated.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"

namespace
{
    const int min_step = 0;
    const int max_step = 100;
    const char* semiWindowsCountProperty          = "SemiwindowsCount";
    const char* semiWindowsTouchSwallowedProperty = "SemiwindowsTouchSwallowed";
}

namespace Ui
{
    SemitransparentWindowAnimated::SemitransparentWindowAnimated(QWidget* _parent, int _duration)
        : QWidget(_parent)
        , Step_(min_step)
        , main_(false)
        , isMainWindow_(false)
    {
        Animation_ = new QPropertyAnimation(this, QByteArrayLiteral("step"), this);
        Animation_->setDuration(_duration);

        updateSize();

        connect(Animation_, &QPropertyAnimation::finished, this, &SemitransparentWindowAnimated::finished, Qt::QueuedConnection);
    }

    void SemitransparentWindowAnimated::Show()
    {
        main_ = (getSemiwindowsCount() == 0);
        incSemiwindowsCount();

        Animation_->stop();
        Animation_->setCurrentTime(0);
        setStep(min_step);
        show();
        Animation_->setStartValue(min_step);
        Animation_->setEndValue(max_step);
        Animation_->start();
    }

    void SemitransparentWindowAnimated::Hide()
    {
        Animation_->stop();
        Animation_->setCurrentTime(0);
        setStep(max_step);
        Animation_->setStartValue(max_step);
        Animation_->setEndValue(min_step);
        Animation_->start();
    }

    void SemitransparentWindowAnimated::forceHide()
    {
        hide();
        decSemiwindowsCount();
    }

    bool SemitransparentWindowAnimated::isSemiWindowVisible() const
    {
        return getSemiwindowsCount() != 0;
    }

    void SemitransparentWindowAnimated::finished()
    {
        if (Animation_->endValue() == min_step)
        {
            setStep(min_step);
            forceHide();
        }
    }

    void SemitransparentWindowAnimated::paintEvent(QPaintEvent* _e)
    {
        if (main_)
        {
            QPainter p(this);
            QColor windowOpacity(ql1s("#000000"));
            windowOpacity.setAlphaF(0.7 * (Step_ / (double)max_step));
            p.fillRect(rect(), windowOpacity);
        }
    }

    void SemitransparentWindowAnimated::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);
        if (!isSemiWindowsTouchSwallowed())
        {
            setSemiwindowsTouchSwallowed(true);
            emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
        }
    }

    bool SemitransparentWindowAnimated::isMainWindow() const
    {
        return isMainWindow_;
    }

    void SemitransparentWindowAnimated::updateSize()
    {
        if (parentWidget())
        {
            const auto rect = parentWidget()->rect();
            auto width = rect.width();
            auto height = rect.height();

            isMainWindow_ = (qobject_cast<MainWindow*>(parentWidget()) != 0);

            auto titleHeight = isMainWindow_ ? Utils::InterConnector::instance().getMainWindow()->getTitleHeight() : 0;
            setFixedHeight(height - titleHeight);
            setFixedWidth(width);

            move(0, titleHeight);
        }
    }

    void SemitransparentWindowAnimated::decSemiwindowsCount()
    {
        if (window())
        {
            auto variantCount = window()->property(semiWindowsCountProperty);
            if (variantCount.isValid())
            {
                int count = variantCount.toInt() - 1;
                if (count < 0)
                {
                    count = 0;
                }

                if (count == 0)
                    setSemiwindowsTouchSwallowed(false);

                window()->setProperty(semiWindowsCountProperty, count);
            }
        }
    }

    void SemitransparentWindowAnimated::incSemiwindowsCount()
    {
        if (window())
        {
            int count = 0;
            auto variantCount = window()->property(semiWindowsCountProperty);
            if (variantCount.isValid())
            {
                count = variantCount.toInt();
            }
            count++;

            window()->setProperty(semiWindowsCountProperty, count);
        }
    }

    int  SemitransparentWindowAnimated::getSemiwindowsCount() const
    {
        int res = 0;
        if (window())
        {
            auto variantCount = window()->property(semiWindowsCountProperty);
            if (variantCount.isValid())
            {
                res = variantCount.toInt();
            }
        }
        return res;
    }

    bool SemitransparentWindowAnimated::isSemiWindowsTouchSwallowed() const
    {
        bool res = false;
        if (window())
        {
            auto variantCount = window()->property(semiWindowsTouchSwallowedProperty);
            if (variantCount.isValid())
            {
                res = variantCount.toBool();
            }
        }
        return res;
    }

    void SemitransparentWindowAnimated::setSemiwindowsTouchSwallowed(bool _val)
    {
        if (window())
        {
            window()->setProperty(semiWindowsTouchSwallowedProperty, _val);
        }
    }

    void SemitransparentWindowAnimated::hideEvent(QHideEvent * event)
    {
        decSemiwindowsCount();
    }
}
