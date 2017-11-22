#include "stdafx.h"

#include "MessageStyle.h"

#include "../../cache/themes/themes.h"
#include "../../fonts.h"
#include "../../controls/CommonStyle.h"
#include "../../theme_settings.h"
#include "../../utils/utils.h"

UI_MESSAGE_STYLE_NS_BEGIN

QFont getTextFont(int size)
{
    const int defaultFontSize = (platform::is_windows_vista_or_late() || platform::is_linux()) ? 15 : 14;

    const int fontSize = size == -1 ? defaultFontSize : size;

    return Fonts::appFontScaled(fontSize);
}

QColor getTextColor(double opacity)
{
    QColor textColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
    textColor.setAlphaF(opacity);
    return textColor;
}

QColor getTimeColor()
{
    return CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT);
}

QColor getChatEventColor()
{
    return CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT);
}

QColor getTypingColor()
{
    return QColor(ql1s("#454545"));
}

QColor getSenderColor()
{
    return QColor(ql1s("#454545"));
}

QFont getSenderFont()
{
    return Fonts::appFontScaled(12);
}

QFont getTimeFont()
{
    return Fonts::appFontScaled(10, Fonts::FontWeight::Normal);
}

int32_t getTimeMarginX()
{
    return Utils::scale_value(4);
}
int32_t getTimeMarginY()
{
    return Utils::scale_value(4);
}

int32_t getTimeMaxWidth()
{
    return Utils::scale_value(32);
}

QColor getIncomingBodyColor()
{
    return QColor(ql1s("#ffffff"));
}

QColor getOutgoingBodyColor()
{
    return QColor(ql1s("#d8d4ce"));
}

QString getLinkColorS()
{
    return (CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT).name());
}

QString getMessageStyle()
{
    return ql1s("a {text-decoration:none;color:") % MessageStyle::getLinkColorS() % ql1s(";}");
}

QBrush getBodyBrush(
    const bool isOutgoing,
    const bool isSelected,
    const int _theme_id)
{
    auto _theme = get_qt_theme_settings()->themeForId(_theme_id);

    QColor bodyColor;

    QColor selectionColor(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL));

    if (isSelected)
    {
        bodyColor = selectionColor;
    }
    else
    {
        const auto color = isOutgoing ? _theme->outgoing_bubble_.bg_color_ : _theme->incoming_bubble_.bg_color_;
        bodyColor = color;
    }

    QBrush result(bodyColor);

    return result;
}

int32_t getMinBubbleHeight()
{
    return Utils::scale_value(32);
}

int32_t getBorderRadius()
{
    return Utils::scale_value(12);
}

int32_t getTopMargin(const bool hasTopMargin)
{
    return Utils::scale_value(
        hasTopMargin ? 12 : 4
    );
}

int32_t getLeftMargin(const bool isOutgoing)
{
    return Utils::scale_value(
        isOutgoing ? 90 : 28
    );
}

int32_t getRightMargin(const bool isOutgoing)
{
    return Utils::scale_value(
        isOutgoing ? 36 : 64
    );
}

int32_t getAvatarSize()
{
    return Utils::scale_value(32);
}

int32_t getAvatarRightMargin()
{
    return Utils::scale_value(6);
}

int32_t getBubbleHorPadding()
{
    return Utils::scale_value(16);
}

int32_t getLastReadAvatarSize()
{
    return Utils::scale_value(16);
}

QSize getLastStatusIconSize()
{
    return QSize(Utils::scale_value(16), Utils::scale_value(12));
}

QSize getLastStatusCheckMarkSize()
{
    return { Utils::scale_value(8), Utils::scale_value(6) };
}

int32_t getLastReadAvatarMargin()
{
    return Utils::scale_value(16);
}

int32_t getLastStatusIconMargin()
{
    return Utils::scale_value(20);
}

int32_t getHistoryWidgetMaxWidth()
{
    return Utils::scale_value(640);
}

int32_t getSenderHeight()
{
    return Utils::scale_value(16);
}

QFont getRotatingProgressBarTextFont()
{
    using namespace Utils;

    return Fonts::appFontScaled(15);
}

QPen getRotatingProgressBarTextPen()
{
    return QPen(QColor(ql1s("#ffffff")));
}

int32_t getRotatingProgressBarTextTopMargin()
{
    return Utils::scale_value(16);
}

int32_t getRotatingProgressBarPenWidth()
{
    return Utils::scale_value(2);
}

QPen getRotatingProgressBarPen()
{
    return QPen(
        CommonStyle::getColor(CommonStyle::Color::GREEN_FILL),
        getRotatingProgressBarPenWidth());
}

int32_t getSenderBottomMargin()
{
    return Utils::scale_value(4);
}

int32_t getTextWidthStep()
{
    return Utils::scale_value(20);
}

int32_t roundTextWidthDown(const int32_t width)
{
    assert(width > 0);

    return ((width / getTextWidthStep()) * getTextWidthStep());
}

int32_t getSnippetMaxWidth()
{
    return Utils::scale_value(420);
}

int32_t getImagePreviewLinkFontSize()
{
    return 14;
}

bool isShowLinksInImagePreview()
{
    return false;
}

UI_MESSAGE_STYLE_NS_END
