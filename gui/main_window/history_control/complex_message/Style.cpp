#include "stdafx.h"

#include "../../../fonts.h"
#include "../MessageStyle.h"
#include "../../../utils/utils.h"
#include "../../../controls/CommonStyle.h"

#include "Style.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace Style
{

    int32_t getBlocksSeparatorVertMargins()
    { return Utils::scale_value(16); }

    int32_t getBlockMaxWidth()
    { return Utils::scale_value(320); }

    const QMargins& getDefaultBlockBubbleMargins()
    {
        static const QMargins margins(
            MessageStyle::getBubbleHorPadding(),
            MessageStyle::getBubbleHorPadding(),
            MessageStyle::getBubbleHorPadding(),
            MessageStyle::getBubbleHorPadding());

        return margins;
    }

    bool isBlocksGridEnabled()
    {
        if (!build::is_debug())
        {
            return false;
        }

        return false;
    }

    int32_t getMinFileSize4ProgressBar()
    {
        return (15 * 1024);
    }

    int getDragDistance()
    {
        return Utils::scale_value(50);
    }

    namespace Preview
    {
        int32_t getImageHeightMax()
        {
            return Utils::scale_value(400);
        }

        int32_t getImageWidthMax()
        {
            return Utils::scale_value(388);
        }

        QBrush getImageShadeBrush()
        {
            QColor imageShadeColor(ql1s("#000000"));
            imageShadeColor.setAlphaF(0.4);
            return imageShadeColor;
        }

        QSize getMinPreviewSize()
        {
            return Utils::scale_value(QSize(48, 48));
        }

        QSizeF getMinPreviewSizeF()
        {
            return Utils::scale_value(QSizeF(48, 48));
        }

        QBrush getImagePlaceholderBrush()
        {
            QColor imagePlaceholderColor(ql1s("#000000"));
            imagePlaceholderColor.setAlphaF(0.15);
            return imagePlaceholderColor;
        }

        QSize getImagePlaceholderSize()
        {
            return Utils::scale_value(QSize(320, 240));
        }

    }

    namespace Snippet
    {
        QSize getFaviconPlaceholderSize()
        { return QSize(12, 12); }

        QSize getFaviconSizeUnscaled()
        {
            return QSize(12, 12);
        }

        QSize getImagePreloaderSizeDip()
        {
            return QSize(388, 180);
        }

        int32_t getLinkPreviewHeightMax()
        {
            return Utils::scale_value(180);
        }

        QColor getSiteNameColor()
        {
            return CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT);
        }

        QFont getSiteNameFont()
        {
            return Fonts::appFontScaled(12, platform::is_windows_vista_or_late() ?
                Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
        }

        QSize getSiteNamePlaceholderSize()
        { return Utils::scale_value(QSize(64, 12)); }

        QSize getTitlePlaceholderSize()
        { return Utils::scale_value(QSize(272, 20)); }

        int32_t getTitlePlaceholderTopOffset()
        { return Utils::scale_value(8); }

        int32_t getTitleTopOffset()
        { return Utils::scale_value(12); }

        QFont getYoutubeTitleFont()
        { return Fonts::appFontScaled(15, Fonts::FontWeight::Medium); }

        int32_t getSiteNameLeftPadding()
        { return Utils::scale_value(4); }

        int32_t getSiteNameTopPadding()
        { return Utils::scale_value(18); }

        int32_t getFaviconTopPadding()
        { return Utils::scale_value(8); }

        QBrush getPreloaderBrush()
        {
            QLinearGradient grad(0, 0, 0.5, 0);
            grad.setCoordinateMode(QGradient::StretchToDeviceMode);
            grad.setSpread(QGradient::ReflectSpread);

            QColor colorEdge(ql1s("#000000"));
            colorEdge.setAlphaF(0.07);
            grad.setColorAt(0, colorEdge);

            QColor colorCenter(ql1s("#000000"));
            colorCenter.setAlphaF(0.12);
            grad.setColorAt(0.5, colorCenter);

            grad.setColorAt(1, colorEdge);

            QBrush result(grad);
            result.setColor(Qt::transparent);

            return result;
        }
    }

    namespace Quote
    {
        const QPen& getQuoteSeparatorPen()
        {
            static const QPen separatorPen(
                QBrush(CommonStyle::getColor(CommonStyle::Color::GREEN_FILL)),
                (qreal)Utils::scale_value(2));

            return separatorPen;
        }

        int32_t getQuoteAvatarOffset()
        { return Utils::scale_value(24); }

        QSize getQuoteAvatarSize()
        { return QSize(Utils::scale_value(20), Utils::scale_value(20)); }

        QFont getQuoteFont()
        { return Fonts::appFont(14); }

        int32_t getQuoteOffsetLeft() //The offset to the right of the green line
        {
            return (Utils::scale_value(8) + Files::getFileSharingFramePen().width());
        }

        int32_t getQuoteOffsetTop() //Vertical offset between avatar and content
        {
            return (
                Utils::scale_value(8)
                + getQuoteAvatarSize().height()
                + getFirstQuoteOffset()
                - getDefaultBlockBubbleMargins().top()
                );
        }

        int32_t getQuoteOffsetBottom() //Vertical offset between quote and answer
        { return Utils::scale_value(12); }

        int32_t getQuoteSpacing() //Vertical correction between quotes
        { return Utils::scale_value(4); }

        int32_t getFirstQuoteOffset() //Vertical offset above the first quote
        { return Utils::scale_value(12); }

        int32_t getMaxImageWidthInQuote()
        { return Utils::scale_value(200); }

        int32_t getQuoteUsernameOffsetTop()
        { return Utils::scale_value(2); }

        int32_t getForwardLabelBottomMargin()
        { return Utils::scale_value(12); }

        int32_t getForwardIconOffset() //Forward icon correction
        { return Utils::scale_value(6); }

        int32_t getLineOffset()
        { return Utils::scale_value(6); }

        int32_t getQuoteBlockSpacing() //Vertical padding between content inside one quote
        { return Utils::scale_value(4); }
    }

    namespace Files
    {
        QPen getFileSharingFramePen()
        {
            QColor color(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
            const auto width = Utils::scale_value(1);
            return QPen(color, width);
        }

        int32_t getFileBubbleHeight()
        { return Utils::scale_value(64); }

        QFont getFilenameFont()
        { return Fonts::appFontScaled(16); }

        QColor getFileSizeColor()
        {
            return CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT);
        }

        QFont getFileSizeFont()
        { return Fonts::appFontScaled(12); }

        QFont getShowInDirLinkFont()
        { return Fonts::appFontScaled(12); }

        int32_t getCtrlIconLeftMargin()
        { return Utils::scale_value(16); }

        int32_t getFilenameBaseline()
        { return Utils::scale_value(28); }

        int32_t getFilenameLeftMargin()
        { return Utils::scale_value(12); }

        int32_t getFileSizeBaseline()
        { return Utils::scale_value(48); }

        int32_t getShowInDirLinkLeftMargin()
        { return Utils::scale_value(16); }
    }

    namespace Ptt
    {
        int32_t getPttBubbleHeight()
        { return Utils::scale_value(56); }

        int32_t getCtrlButtonMarginLeft()
        { return Utils::scale_value(16); }

        int32_t getCtrlButtonMarginTop()
        { return Utils::scale_value(8); }

        int32_t getDecodedTextHorPadding()
        { return Utils::scale_value(16); }

        int32_t getDecodedTextVertPadding()
        { return Utils::scale_value(16); }

        int32_t getTextButtonMarginRight()
        { return Utils::scale_value(16); }

        int32_t getTextButtonMarginTop()
        { return Utils::scale_value(16); }

        QColor getPttProgressColor()
        { return QColor(ql1s("#000000")); }

        int32_t getPttProgressWidth()
        { return Utils::scale_value(2); }

        QPen getDecodedTextSeparatorPen()
        {
            QColor pttSeparatorColor(ql1s("#000000"));
            pttSeparatorColor.setAlphaF(0.15);
            return QPen(pttSeparatorColor);
        }

        QFont getDurationTextFont()
        { return Fonts::appFontScaled(15); }

        QPen getDurationTextPen()
        {
            return QPen(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
        }

        QBrush getPlaybackProgressBrush()
        {
            QColor playbackColor(ql1s("#000000"));
            playbackColor.setAlphaF(0.12);

            return QBrush(playbackColor);
        }
    }
}

UI_COMPLEX_MESSAGE_NS_END
