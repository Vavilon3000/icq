#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "FileSharingBlock.h"
#include "Style.h"

#include "FileSharingImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingImagePreviewBlockLayout::FileSharingImagePreviewBlockLayout()
{

}

FileSharingImagePreviewBlockLayout::~FileSharingImagePreviewBlockLayout()
{

}

QSize FileSharingImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    auto &block = *blockWidget<FileSharingBlock>();

    auto previewSize = Utils::scale_value(block.getOriginalPreviewSize());

    const auto maxSizeWidth = std::min(Utils::scale_value(maxWidth), Style::Preview::getImageWidthMax());
    const QSize maxSize(maxSizeWidth, Style::Preview::getImageHeightMax());
    previewSize = limitSize(previewSize, maxSize);

    const auto minPreviewSize = Style::Preview::getMinPreviewSize();

    const auto shouldScaleUp =
        (previewSize.width() < minPreviewSize.width()) &&
        (previewSize.height() < minPreviewSize.height());
    if (shouldScaleUp)
    {
        previewSize = previewSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
    }

    QSize blockSize(previewSize);

    return blockSize;
}

const QRect& FileSharingImagePreviewBlockLayout::getContentRect() const
{
    return PreviewRect_;
}

const QRect& FileSharingImagePreviewBlockLayout::getFilenameRect() const
{
    static QRect empty;
    return empty;
}

QRect FileSharingImagePreviewBlockLayout::getFileSizeRect() const
{
    return QRect();
}

QRect FileSharingImagePreviewBlockLayout::getShowInDirLinkRect() const
{
    return QRect();
}

const IItemBlockLayout::IBoxModel& FileSharingImagePreviewBlockLayout::getBlockBoxModel() const
{
    static const QMargins margins(
        Utils::scale_value(16),
        Utils::scale_value(12),
        Utils::scale_value(8),
        Utils::scale_value(16));

    static const BoxModel boxModel(
        true,
        margins);

    return boxModel;
}

QSize FileSharingImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &block = *blockWidget<FileSharingBlock>();

    PreviewRect_ = evaluatePreviewRect(block, geometry.width());
    setCtrlButtonGeometry(block, PreviewRect_);

    auto blockSize = PreviewRect_.size();

    return blockSize;
}

QRect FileSharingImagePreviewBlockLayout::evaluatePreviewRect(const FileSharingBlock &block, const int32_t blockWidth) const
{
    assert(blockWidth > 0);

    auto previewSize = Utils::scale_value(block.getOriginalPreviewSize());

    auto maxSizeWidth = std::min(blockWidth, previewSize.width());
    if (block.getMaxPreviewWidth())
        maxSizeWidth = std::min(maxSizeWidth, block.getMaxPreviewWidth());
    const QSize maxSize(maxSizeWidth, Style::Preview::getImageHeightMax());
    previewSize = limitSize(previewSize, maxSize);

    const auto minPreviewSize = Style::Preview::getMinPreviewSize();

    const auto shouldScaleUp =
        (previewSize.width() < minPreviewSize.width()) &&
        (previewSize.height() < minPreviewSize.height());
    if (shouldScaleUp)
    {
        previewSize = previewSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
    }

    QPoint leftTop;

    return QRect(leftTop, previewSize);
}

void FileSharingImagePreviewBlockLayout::setCtrlButtonGeometry(FileSharingBlock &block, const QRect &previewRect)
{
    assert(!previewRect.isEmpty());
    if (previewRect.isEmpty())
    {
        return;
    }

    const auto buttonSize = block.getCtrlButtonSize();

    assert(!buttonSize.isEmpty());
    if (buttonSize.isEmpty())
    {
        return;
    }

    QRect buttonRect(QPoint(), buttonSize);

    buttonRect.moveCenter(previewRect.center());

    block.setCtrlButtonGeometry(buttonRect);
}

UI_COMPLEX_MESSAGE_NS_END
