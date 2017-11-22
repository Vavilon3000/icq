#include "stdafx.h"

#include "CommonStyle.h"
#include "ContextMenu.h"

#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const QString MENU_STYLE = qsl(
        "QMenu { background-color: %3; border: 1px solid %4; }"
        "QMenu::item { background-color: transparent;"
        "height: %2; padding-left: %1; padding-right: 12dip; color: %7;}"
        "QMenu::item:selected { background-color: %6;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::item:disabled { background-color: transparent; color: %5;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::item:disabled:selected { background-color: transparent; color: %5;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::icon { padding-left: 22dip; }");
}

namespace Ui
{
    int MenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        if (_metric == QStyle::PM_SmallIconSize)
            return Utils::scale_value(20);
        return QProxyStyle::pixelMetric(_metric, _option, _widget);
    }

    ContextMenu::ContextMenu(QWidget* _parent)
        : QMenu(_parent)
        , InvertRight_(false)
        , Indent_(0)
    {
        applyStyle(this, true, Utils::scale_value(15), Utils::scale_value(36));
    }

    void ContextMenu::applyStyle(QMenu* _menu, bool _withPadding, int _fontSize, int _height)
    {
        _menu->setWindowFlags(_menu->windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        _menu->setStyle(new MenuStyle());
        QString style = MENU_STYLE
            .arg(
            QString::number(Utils::scale_value(_withPadding ? 40 : 12)),
            QString::number(_height),
            CommonStyle::getFrameColor().name(),
            CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER).name(),
            CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT).name(),
            CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_LIGHT).name(),
            CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY).name()
            );
        Utils::ApplyStyle(_menu, style);
        auto font = Fonts::appFont(_fontSize);
        _menu->setFont(font);
        if (platform::is_apple())
        {
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, _menu, &QMenu::close);
        }
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data)
    {
        QAction* action = addAction(_icon, _name);
        action->setData(_data);
        return action;
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QObject* _receiver, const char* _member)
    {
        return addAction(_icon, _name, _receiver, _member);
    }

    QAction* ContextMenu::addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data)
    {
        return addActionWithIcon(QIcon(Utils::parse_image_name(_iconPath)), _name, _data);
    }

    bool ContextMenu::hasAction(const QString& _command)
    {
        assert(!_command.isEmpty());
        const auto actions = this->actions();
        for (const auto& action : actions)
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find(qsl("command"));
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == _command)
            {
                return true;
            }
        }

        return false;
    }

    void ContextMenu::removeAction(const QString& _command)
    {
        assert(!_command.isEmpty());
        const auto actions = this->actions();
        for (const auto& action : actions)
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find(qsl("command"));
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == _command)
            {
                QWidget::removeAction(action);

                return;
            }
        }
    }

    void ContextMenu::invertRight(bool _invert)
    {
        InvertRight_ = _invert;
    }

    void ContextMenu::setIndent(int _indent)
    {
        Indent_ = _indent;
    }

    void ContextMenu::popup(const QPoint& _pos, QAction* _at)
    {
        Pos_ = _pos;
        QMenu::popup(_pos, _at);
    }

    void ContextMenu::clear()
    {
        QMenu::clear();
    }

    void ContextMenu::hideEvent(QHideEvent* _e)
    {
        QMenu::hideEvent(_e);
    }

    void ContextMenu::showEvent(QShowEvent*)
    {
        if (InvertRight_ || Indent_ != 0)
        {
            QPoint p;
            if (pos().x() != Pos_.x())
                p = Pos_;
            else
                p = pos();

            if (InvertRight_)
                p.setX(p.x() - width() - Indent_);
            else
                p.setX(p.x() + Indent_);
            move(p);
        }
    }

    void ContextMenu::focusOutEvent(QFocusEvent *_e)
    {
        if (parentWidget() && !parentWidget()->isActiveWindow())
        {
            close();
            return;
        }
        QMenu::focusOutEvent(_e);
    }

}
