#include "stdafx.h"
#include "CommonStyle.h"

#include "../utils/utils.h"

namespace CommonStyle
{
    QColor getColor(const Color _color)
    {
        QColor result;

        switch (_color)
        {
        case Color::GREEN_FILL:
            result = QColor(ql1s("#57b359"));
            break;

        case Color::GREEN_TEXT:
            result = QColor(ql1s("#43a047"));
            break;

        case Color::GRAY_FILL_LIGHT:
            result = QColor(ql1s("#f2f2f2"));
            break;

        case Color::GRAY_FILL_DARK:
            result = QColor(ql1s("#cbcbcb"));
            break;

        case Color::GRAY_BORDER:
            result = QColor(ql1s("#d7d7d7"));
            break;

        case Color::TEXT_PRIMARY:
            result = QColor(ql1s("#000000"));
            break;

        case Color::TEXT_SECONDARY:
            result = QColor(ql1s("#767676"));
            break;

        case Color::TEXT_LIGHT:
            result = QColor(ql1s("#999999"));
            break;

        case Color::TEXT_RED:
            result = QColor(ql1s("#d0021b"));
            break;

        case Color::RED_FILL:
            result = QColor(ql1s("#ef5350"));
            break;

        default:
            assert(!"unexpected color");
            break;
        }

        return result;
    }

    QString getCloseButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "background-color: transparent;"
            "background-image: url(:/titlebar/close_100);"
            "background-position: center; background-repeat: no-repeat; border: none;"
            "padding: 0; margin: 0;"
            "width: 46dip; height: 36dip; }"
            "QPushButton:hover {"
            "background-image: url(:/titlebar/close_100_hover);"
            "background-color: #e81123; }"
            "QPushButton:hover:pressed { background-color: #e81123; }"
            "QPushButton:focus { outline: none; }"
        );
    }

    QString getMinimizeButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "background-color: transparent;"
            "background-image: url(:/titlebar/minimize_100);"
            "background-position: center; background-repeat: no-repeat; border: none;"
            "padding: 0; margin: 0;"
            "width: 46dip; height: 36dip; }"
            "QPushButton:hover { background-color: #d3d3d3; }"
            "QPushButton:hover:pressed { background-color: #c8c8c8; }"
            "QPushButton:focus { outline: none; }"
        );
    }

    QString getMaximizeButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "background-color: transparent;"
            "background-image: url(:/titlebar/bigwindow_100);"
            "background-position: center; background-repeat: no-repeat; border: none;"
            "padding: 0; margin: 0;"
            "width: 46dip; height: 36dip; }"
            "QPushButton:hover { background-color: #d3d3d3; }"
            "QPushButton:hover:pressed { background-color: #c8c8c8; }"
            "QPushButton:focus { outline: none; }"
        );
    }
    QString getRestoreButtonStyle()
    {
        return qsl(
            "QPushButton { "
            "background-color: transparent;"
            "background-image: url(:/titlebar/smallwindow_100);"
            "background-position: center; background-repeat: no-repeat; border: none;"
            "padding: 0; margin: 0;"
            "width: 46dip; height: 36dip; }"
            "QPushButton:hover { background-color: #d3d3d3; }"
            "QPushButton:hover:pressed { background-color: #c8c8c8; }"
            "QPushButton:focus { outline: none; }"
        );
    }

    QString getDisabledButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "color: #ffffff;"
            "font-size: 14dip; font-family: %FONT_FAMILY_MEDIUM%; font-weight: %FONT_WEIGHT_MEDIUM%;"
            "background-color: %1;"
            "border-style: none; border-radius: 4dip;"
            "margin: 0;"
            "padding-left: 16dip; padding-right: 16dip;"
            "min-width: 80dip;"
            "max-height: 32dip; min-height: 32dip; }"
            "QPushButton:focus { outline: none; }"
        ).arg(CommonStyle::getColor(CommonStyle::Color::GRAY_FILL_DARK).name());
    }

    QString getGrayButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "font-size: 14dip; font-family: %FONT_FAMILY_MEDIUM%; font-weight: %FONT_WEIGHT_MEDIUM%;"
            "color: #999999;"
            "background-color: #ffffff;"
            "border-style: solid; border-width: 1dip; border-color: #cbcbcb; border-radius: 4dip;"
            "margin: 0;"
            "padding-left: 16dip; padding-right: 16dip;"
            "min-width: 80dip;"
            "max-height: 30dip; min-height: 30dip; }"
            "QPushButton:focus { outline: none; }");
    }

    QString getGreenButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "color: #ffffff;"
            "font-size: 14dip; font-family: %FONT_FAMILY_MEDIUM%; font-weight: %FONT_WEIGHT_MEDIUM%;"
            "background-color: %1;"
            "border-style: none; border-radius: 4dip;"
            "margin: 0;"
            "padding-left: 16dip; padding-right: 16dip;"
            "min-width: 80dip;"
            "max-height: 32dip; min-height: 32dip;"
            "text-align: center; }"
            "QPushButton:hover { background-color: #43a047; }"
            "QPushButton:focus { outline: none; }"
        ).arg(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL).name());
    }

    QString getRedButtonStyle()
    {
        return qsl(
            "QPushButton {"
            "color: #ffffff;"
            "font-size: 14dip; font-family: %FONT_FAMILY_MEDIUM%; font-weight: %FONT_WEIGHT_MEDIUM%;"
            "background-color: %1;"
            "border-style: none; border-radius: 4dip;"
            "margin: 0;"
            "padding-left: 16dip; padding-right: 16dip;"
            "min-width: 80dip;"
            "max-height: 32dip; min-height: 32dip;"
            "text-align: center; }"
            "QPushButton:hover { background-color: #ef5350; }"
            "QPushButton:focus { outline: none; }"
        ).arg(CommonStyle::getColor(CommonStyle::Color::RED_FILL).name());
    }

    const QColor getFrameColor() { return QColor(ql1s("#ffffff")); }
    const int getBottomPanelHeight() { return  Utils::scale_value(48); }
    const int getTopPanelHeight() { return  Utils::scale_value(56); }

    QString getLineEditStyle()
    {
        return qsl(
            "QLineEdit {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %1;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
            "QLineEdit:focus {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %2;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }")
            .arg(CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER).name())
            .arg(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL).name());
    }

    QString getTextEditStyle()
    {
        return qsl(
            "QTextBrowser {"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %1;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid;}"
            "QTextBrowser:focus {"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %2;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }")
            .arg(CommonStyle::getColor(CommonStyle::Color::GRAY_BORDER).name())
            .arg(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL).name());
    }

    QString getLineEditErrorStyle()
    {
        return qsl(
            "QLineEdit {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "color: %1;"
            "border-style: none;"
            "border-bottom-color: %1;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }")
            .arg(CommonStyle::getColor(CommonStyle::Color::TEXT_RED).name());
    }
}
