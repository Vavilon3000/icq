#pragma once

namespace CommonStyle
{
    enum class Color
    {
        Min,

        GREEN_FILL,
        GREEN_TEXT,
        GRAY_FILL_LIGHT,
        GRAY_FILL_DARK,
        GRAY_BORDER,
        TEXT_PRIMARY,
        TEXT_SECONDARY,
        TEXT_LIGHT,
        RED_FILL,
        TEXT_RED,

        Max,
    };

    enum class State
    {
        Min,

        NORMAL,
        HOVER,
        PRESSED,

        Max,
    };

    QColor getColor(const Color _color);

    QString getCloseButtonStyle();
    QString getMinimizeButtonStyle();
    QString getMaximizeButtonStyle();
    QString getRestoreButtonStyle();

    QString getDisabledButtonStyle();
    QString getGrayButtonStyle();
    QString getGreenButtonStyle();
    QString getRedButtonStyle();

    const QColor getFrameColor();
    const int getBottomPanelHeight();
    const int getTopPanelHeight();

    QString getLineEditStyle();
    QString getTextEditStyle();
    QString getLineEditErrorStyle();
}