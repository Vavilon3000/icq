#include "stdafx.h"

#include "../../../../corelib/enumerations.h"
#include "../../../../common.shared/loader_errors.h"

#include "../../../core_dispatcher.h"
#include "../../../controls/CommonStyle.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../gui_settings.h"
#include "../../../main_window/MainWindow.h"
#include "../../../previewer/GalleryWidget.h"
#include "../../../themes/ResourceIds.h"
#include "../../../themes/ThemePixmap.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/LoadMovieFromFileTask.h"
#include "../../../utils/LoadPixmapFromFileTask.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../contact_list/ContactListModel.h"

#include "../ActionButtonWidget.h"
#include "../FileSizeFormatter.h"
#include "../KnownFileTypes.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "FileSharingImagePreviewBlockLayout.h"
#include "FileSharingPlainBlockLayout.h"
#include "FileSharingUtils.h"
#include "Style.h"

#include "../../sounds/SoundsManager.h"


#include "FileSharingBlock.h"
#include "QuoteBlock.h"

#ifdef __APPLE__
#include "../../../utils/macos/mac_support.h"
#endif

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingBlock::FileSharingBlock(
    ComplexMessageItem *parent,
    const QString &link,
    const core::file_sharing_content_type type)
    : FileSharingBlockBase(parent, link, type)
    , IsBodyPressed_(false)
    , IsShowInDirLinkPressed_(false)
    , OpenPreviewer_(false)
    , IsVisible_(false)
    , IsInPreloadDistance_(true)
    , PreviewRequestId_(-1)
    , CtrlButton_(nullptr)
{
    parseLink();

    if (isPreviewable())
    {
        initPreview();
    }
    else
    {
        initPlainFile();
    }
    if (getType() != core::file_sharing_content_type::undefined)
        QuoteAnimation_.setSemiTransparent();

    if (getType() == core::file_sharing_content_type::video)
    {
        /// no play quote
        QuoteAnimation_.deactivate();
    }
    setMouseTracking(true);
}

FileSharingBlock::~FileSharingBlock()
{
}

QSize FileSharingBlock::getCtrlButtonSize() const
{
    assert(CtrlButton_);

    if (CtrlButton_)
    {
        return CtrlButton_->sizeHint();
    }

    return QSize();
}

QSize FileSharingBlock::getOriginalPreviewSize() const
{
    assert(isPreviewable());
    assert(!OriginalPreviewSize_.isEmpty());

    return OriginalPreviewSize_;
}

QString FileSharingBlock::getShowInDirLinkText() const
{
    return QT_TRANSLATE_NOOP("chat_page","Show in folder");
}

bool FileSharingBlock::isPreviewReady() const
{
    return !Preview_.isNull();
}

bool FileSharingBlock::isSharingEnabled() const
{
    return true;
}

void FileSharingBlock::onVisibilityChanged(const bool isVisible)
{
    IsVisible_ = isVisible;

    FileSharingBlockBase::onVisibilityChanged(isVisible);
    if (CtrlButton_)
        CtrlButton_->onVisibilityChanged(isVisible);

    const auto isPreviewRequestInProgress = (PreviewRequestId_ != -1);
    if (isPreviewRequestInProgress)
    {
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), PreviewRequestId_);
    }

    if (isGifImage() || isVideo())
    {
        onGifImageVisibilityChanged(isVisible);
    }
}

bool FileSharingBlock::isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);

    if (intersected.height() != 0)
        return true;

    return std::min(std::abs(_viewportVisibilityAbsRect.y() - _widgetAbsGeometry.y())
        , std::abs(_viewportVisibilityAbsRect.bottom() - _widgetAbsGeometry.bottom())) < 1000;
}

void FileSharingBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    if (!isGifImage() && !isVideo())
        return;

    auto isInPreload = isInPreloadRange(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    if (IsInPreloadDistance_ == isInPreload)
    {
        return;
    }
    IsInPreloadDistance_ = isInPreload;

    qDebug() << "set distance load " << IsInPreloadDistance_ << ", file: " << getFileLocalPath();

    onChangeLoadState(IsInPreloadDistance_);
}

void FileSharingBlock::setCtrlButtonGeometry(const QRect &rect)
{
    assert(!rect.isEmpty());
    assert(CtrlButton_);

    if (!CtrlButton_)
    {
        return;
    }

    const auto bias = CtrlButton_->getCenterBias();
    const auto fixedRect = rect.translated(-bias);
    CtrlButton_->setGeometry(fixedRect);

    const auto isButtonVisible = (!isGifImage() || !isGifOrVideoPlaying());
    CtrlButton_->setVisible(isButtonVisible);
}

void FileSharingBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quote_color)
{
    const auto &contentRect = getFileSharingLayout()->getContentRect();
    assert(!contentRect.isEmpty());

    if (isPreviewable())
    {
        drawPreviewableBlock(p, contentRect, quote_color);

        if (quote_color.isValid() && !isVideo())
        {
            auto clip_path = Utils::renderMessageBubble(
                _rect,
                MessageStyle::getBorderRadius(),
                isOutgoing());

            p.setClipPath(clip_path);
            p.fillRect(_rect, QBrush(quote_color));
        }

    }
    else
    {
        drawPlainFileBlock(p, contentRect, quote_color);
    }
}

void FileSharingBlock::enterEvent(QEvent* event)
{
    IsBodyPressed_ = false;
    IsShowInDirLinkPressed_ = false;

    GenericBlock::enterEvent(event);
}

void FileSharingBlock::initializeFileSharingBlock()
{
    setCursor(Qt::PointingHandCursor);

    sendGenericMetainfoRequests();
}

void FileSharingBlock::leaveEvent(QEvent*)
{
    IsBodyPressed_ = false;
    IsShowInDirLinkPressed_ = false;
}

void FileSharingBlock::mousePressEvent(QMouseEvent *event)
{
    event->ignore();

    IsShowInDirLinkPressed_ = false;
    IsBodyPressed_ = false;

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        return;
    }

    if (isPreviewable())
    {
        IsBodyPressed_ = true;
        return;
    }

    const auto &showInDirLinkRect = getFileSharingLayout()->getShowInDirLinkRect();
    const auto isOverLink = showInDirLinkRect.contains(event->pos());
    if (isOverLink && isFileDownloaded())
    {
        IsShowInDirLinkPressed_ = true;
    }
    else
    {
        IsBodyPressed_ = true;
    }
}

void FileSharingBlock::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    const auto &showInDirLinkRect = getFileSharingLayout()->getShowInDirLinkRect();
    const auto isOverLink = showInDirLinkRect.contains(event->pos());
    if (isOverLink && IsShowInDirLinkPressed_)
    {
        showFileInDir();
    }

    if (IsBodyPressed_)
    {
        onLeftMouseClick(event->globalPos());
    }

    IsBodyPressed_ = false;
    IsShowInDirLinkPressed_ = false;
}

void FileSharingBlock::applyClippingPath(QPainter &p, const QRect &previewRect)
{
    assert(isPreviewable());
    assert(!previewRect.isEmpty());

    const auto isPreviewRectChanged = (LastContentRect_ != previewRect);
    const auto shouldResetClippingPath = (PreviewClippingPath_.isEmpty() || isPreviewRectChanged);
    if (shouldResetClippingPath)
    {
        PreviewClippingPath_ = Utils::renderMessageBubble(
            previewRect,
            MessageStyle::getBorderRadius(),
            isOutgoing(),
            Utils::RenderBubble_AllRounded);

        auto relativePreviewRect = QRect(0, 0, previewRect.width(), previewRect.height());
        RelativePreviewClippingPath_ = Utils::renderMessageBubble(
            relativePreviewRect,
            MessageStyle::getBorderRadius(),
            isOutgoing(),
            Utils::RenderBubble_AllRounded);
    }

    p.setClipPath(PreviewClippingPath_);
}

void FileSharingBlock::changeGifPlaybackStatus(const bool isPlaying)
{
    assert(isGifImage() || isVideo());

    if (CtrlButton_ && isGifImage())
    {
        CtrlButton_->setVisible(!isPlaying);
        CtrlButton_->raise();
    }
}

void FileSharingBlock::connectImageSignals(const bool isConnected)
{
    if (isConnected)
    {
        QMetaObject::Connection connection;

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::imageDownloaded,
            this,
            &FileSharingBlock::onImageDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
        assert(connection);

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::imageDownloadingProgress,
            this,
            &FileSharingBlock::onImageDownloadingProgress,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
        assert(connection);

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::imageMetaDownloaded,
            this,
            &FileSharingBlock::onImageMetaDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
        assert(connection);

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::imageDownloadError,
            this,
            &FileSharingBlock::onImageDownloadError,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
        assert(connection);

        return;
    }

    QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::imageDownloaded,
        this,
        &FileSharingBlock::onImageDownloaded);

    QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::imageDownloadingProgress,
        this,
        &FileSharingBlock::onImageDownloadingProgress);

    QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::imageMetaDownloaded,
        this,
        &FileSharingBlock::onImageMetaDownloaded);

    QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::imageDownloadError,
        this,
        &FileSharingBlock::onImageDownloadError);
}

void FileSharingBlock::drawPlainFileBlock(QPainter &p, const QRect &frameRect, const QColor& quote_color)
{
    assert(isPlainFile());

    if (isStandalone())
    {
        drawPlainFileBubble(p, frameRect, quote_color);
    }
    else
    {
        drawPlainFileFrame(p, frameRect);
    }

    const auto layout = getFileSharingLayout();

    const auto &filenameRect = layout->getFilenameRect();
    drawPlainFileName(p, filenameRect);

    const auto text = getProgressText();
    if (!text.isEmpty())
    {
        const auto &fileSizeRect = layout->getFileSizeRect();
        drawPlainFileSizeAndProgress(p, text, fileSizeRect);
    }

    const auto &showInDirLinkRect = layout->getShowInDirLinkRect();
    drawPlainFileShowInDirLink(p, showInDirLinkRect);
}

void FileSharingBlock::drawPlainFileBubble(QPainter &p, const QRect &bubbleRect, const QColor& quote_color)
{
    assert(isStandalone());

    p.save();

    const auto& bodyBrush = MessageStyle::getBodyBrush(isOutgoing(), isSelected(), getThemeId());
    p.setBrush(bodyBrush);

    p.drawRoundedRect(
        bubbleRect,
        MessageStyle::getBorderRadius(),
        MessageStyle::getBorderRadius());

    if (quote_color.isValid())
    {
        p.setBrush(quote_color);
        p.drawRoundedRect(
            bubbleRect,
            MessageStyle::getBorderRadius(),
            MessageStyle::getBorderRadius());
    }
    p.restore();
}


void FileSharingBlock::drawPlainFileFrame(QPainter &p, const QRect &frameRect)
{
    assert(!isStandalone());

    p.save();

    QRect newRect = frameRect;
    newRect.setRect(
        newRect.x() + Style::Files::getFileSharingFramePen().width(),
        newRect.y() + Style::Files::getFileSharingFramePen().width(),
        newRect.width() - Style::Files::getFileSharingFramePen().width() * 2,
        newRect.height() - Style::Files::getFileSharingFramePen().width() * 2);

    p.setPen(Style::Files::getFileSharingFramePen());

    p.drawRoundedRect(newRect, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());

    p.restore();
}

void FileSharingBlock::drawPlainFileName(QPainter &p, const QRect &filenameRect)
{
    assert(isPlainFile());

    const auto font = Style::Files::getFilenameFont();

    QString text;

    if (!getFilename().isEmpty())
    {
        QFontMetrics fontMetrics(font);
        text = fontMetrics.elidedText(getFilename(), Qt::ElideRight, filenameRect.width());
    }
    else
    {
        text = QT_TRANSLATE_NOOP("contact_list", "File");
    }

    p.save();

    p.setFont(font);
    p.setPen(MessageStyle::getTextColor());

    p.drawText(filenameRect, text);

    p.restore();
}

void FileSharingBlock::drawPlainFileShowInDirLink(QPainter &p, const QRect &linkRect)
{
    if (linkRect.isEmpty())
    {
        return;
    }

    assert(!linkRect.isEmpty());

    if (!isFileDownloaded())
    {
        return;
    }

    p.save();

    const auto &btnText = getShowInDirLinkText();

    p.setFont(Style::Files::getShowInDirLinkFont());
    p.setPen(QPen(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT)));
    p.drawText(linkRect, btnText);

    p.restore();
}

void FileSharingBlock::drawPlainFileSizeAndProgress(QPainter &p, const QString &text, const QRect &fileSizeRect)
{
    assert(!fileSizeRect.isEmpty());
    assert(!text.isEmpty());

    if (getFileSize() <= 0)
    {
        return;
    }

    const auto font = Style::Files::getFileSizeFont();

    p.save();

    p.setFont(font);
    p.setPen(Style::Files::getFileSizeColor());

    p.drawText(fileSizeRect, text);

    p.restore();
}

void FileSharingBlock::drawPreview(QPainter &p, const QRect &previewRect, QPainterPath& _path, const QColor& quote_color)
{
    assert(isPreviewable());
    assert(!previewRect.isEmpty());

    if (isPreviewReady())
    {
        if (!videoplayer_)
        {
            p.drawPixmap(previewRect, Preview_);

            if (quote_color.isValid())
            {
                p.setBrush(QBrush(quote_color));
                p.drawRoundedRect(previewRect, Utils::scale_value(8), Utils::scale_value(8));
            }
        }
        else
        {
            if (!videoplayer_->isFullScreen())
            {
                videoplayer_->setClippingPath(_path);

                if (videoplayer_->state() == QMovie::MovieState::Paused)
                {
                    p.drawPixmap(previewRect, videoplayer_->getActiveImage());
                }
            }
        }
    }
    else
    {
        const auto brush = Style::Preview::getImagePlaceholderBrush();
        p.fillRect(previewRect, brush);
    }

    if (isSelected())
    {
        const QBrush brush(Utils::getSelectionColor());
        p.fillRect(previewRect, brush);
    }
}

void FileSharingBlock::drawPreviewableBlock(QPainter &p, const QRect &previewRect, const QColor& quote_color)
{
    applyClippingPath(p, previewRect);

    drawPreview(p, previewRect, RelativePreviewClippingPath_, quote_color);

    if (isFileDownloading())
    {
        p.fillRect(previewRect, Style::Preview::getImageShadeBrush());
    }
}

void FileSharingBlock::initPlainFile()
{
    auto layout = new FileSharingPlainBlockLayout();
    setBlockLayout(layout);
    setLayout(layout);

    auto resourceSet = ActionButtonWidget::ResourceSet::DownloadPlainFile_;

    assert(!CtrlButton_);
    CtrlButton_ = new ActionButtonWidget(resourceSet, this);

    QMetaObject::Connection connection;

    connection = QObject::connect(
        CtrlButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            onLeftMouseClick(globalClickCoords);
        });
    assert(connection);

    connection = QObject::connect(
        CtrlButton_,
        &ActionButtonWidget::stopClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            Q_UNUSED(globalClickCoords);

            if (isFileDownloading())
            {
                stopDownloading();
            }

            CtrlButton_->stopAnimation();

            notifyBlockContentsChanged();
        });
    assert(connection);

    auto connectionDrag = QObject::connect(
        CtrlButton_,
        &ActionButtonWidget::dragSignal,
        this,
        &FileSharingBlock::drag);
    assert(connectionDrag);
}

void FileSharingBlock::initPreview()
{
    auto layout = new FileSharingImagePreviewBlockLayout();
    setBlockLayout(layout);
    setLayout(layout);

    auto resourceSet = &ActionButtonWidget::ResourceSet::DownloadMediaFile_;

    if (isGifImage())
    {
        resourceSet = &ActionButtonWidget::ResourceSet::Gif_;
    }
    else if (isVideo())
    {
        resourceSet = &ActionButtonWidget::ResourceSet::Play_;
    }

    CtrlButton_ = new ActionButtonWidget(*resourceSet, this);

    QMetaObject::Connection connection;

    connection = QObject::connect(
        CtrlButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            onLeftMouseClick(globalClickCoords);
        });
    assert(connection);

    connection = QObject::connect(
        CtrlButton_,
        &ActionButtonWidget::stopClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            Q_UNUSED(globalClickCoords);

            if (isFileDownloading())
            {
                stopDownloading();
            }

            CtrlButton_->stopAnimation();

            notifyBlockContentsChanged();
        });
        assert(connection);

    connection = QObject::connect(
        CtrlButton_,
        &ActionButtonWidget::dragSignal,
        this,
        [this]
        {
            drag();
        });
    assert(connection);
}

bool FileSharingBlock::isGifOrVideoPlaying() const
{
    assert(isGifImage() || isVideo());

    return (videoplayer_ && (videoplayer_->state() == QMovie::Running));
}

bool FileSharingBlock::drag()
{
    auto fileTypeIcon = History::GetIconByFilename(getFilename());
    return Utils::dragUrl(this, isPreviewable() ? Preview_ : fileTypeIcon->GetPixmap(), getLink());
}

void FileSharingBlock::onDownloadingFailed(const int64_t requestId)
{
    if (CtrlButton_)
    {
        CtrlButton_->stopAnimation();
    }
}

void FileSharingBlock::onDownloadingStarted()
{
    assert(CtrlButton_);

    if (shouldDisplayProgressAnimation())
    {
        CtrlButton_->startAnimation(500);
    }

    notifyBlockContentsChanged();
}

void FileSharingBlock::onDownloadingStopped()
{
    assert(CtrlButton_);

    if (CtrlButton_)
    {
        CtrlButton_->stopAnimation();
    }

    notifyBlockContentsChanged();
}

void FileSharingBlock::onDownloaded()
{
    CtrlButton_->stopAnimation();

    notifyBlockContentsChanged();
}

void FileSharingBlock::onDownloadedAction()
{
    if (getChatAimid() != Logic::getContactListModel()->selectedContact())
        return;

    if (OpenPreviewer_ && !isVideo())
    {
        OpenPreviewer_ = false;
        showImagePreviewer(getFileLocalPath());
    }

    /*if (isVideo() && !ffmpeg::is_enable_streaming())
    {
        Utils::InterConnector::instance().getMainWindow()->playVideo(getFileLocalPath());
    }*/

    if (isGifImage() || isVideo())
    {
        playMedia(getFileLocalPath());
    }
}

void FileSharingBlock::onLeftMouseClick(const QPoint &globalPos)
{
    Q_UNUSED(globalPos);

    const auto isGifImageLoaded = (isGifImage() && isFileDownloaded());
    if (isGifImageLoaded)
    {
        if (isGifOrVideoPlaying())
        {
            pauseMedia(true);
        }
        else
        {
            playMedia(getFileLocalPath());
        }

        return;
    }

    if (isImage())
    {
        if (isFileDownloading())
        {
            stopDownloading();
        }
        else
        {
            OpenPreviewer_ = true;
            startDownloading(true);
        }

        return;
    }

    if (isVideo() || isGifImage())
    {
        startDownloading(true);
    }

    if (isPlainFile())
    {
        if (isFileDownloaded())
        {
            showFileInDir();
        }
        else
        {
            startDownloading(true, false, true);
        }
    }
}

void FileSharingBlock::onRestoreResources()
{
    if (isGifImage() || isVideo())
    {
        if (isFileDownloaded() && ((!videoplayer_) || (videoplayer_ && !videoplayer_->isPausedByUser())))
        {
            playMedia(getFileLocalPath());
        }
    }

    FileSharingBlockBase::onRestoreResources();
}

void FileSharingBlock::onUnloadResources()
{
    if (videoplayer_ && videoplayer_->getAttachedPlayer())
        return;

    if (videoplayer_)
    {
        __TRACE(
            "resman",
            "unloading gif fs block\n"
            __LOGP(local, getFileLocalPath())
            __LOGP(uri, getLink()));

        qDebug() << "reset onUnloadResources";
        videoplayer_.reset();
    }

    FileSharingBlockBase::onUnloadResources();
}

void FileSharingBlock::parseLink()
{
    Id_ = extractIdFromFileSharingUri(getLink());
    assert(!Id_.isEmpty());

    if (isPreviewable())
    {
        OriginalPreviewSize_ = extractSizeFromFileSharingId(Id_);

        if (OriginalPreviewSize_.isEmpty())
        {
            OriginalPreviewSize_ = Utils::scale_value(QSize(64, 64));
        }
    }
}

void FileSharingBlock::pauseMedia(const bool _byUser)
{
    assert(isGifImage() || isVideo());
    assert(videoplayer_);

    videoplayer_->setPaused(true, _byUser);

    changeGifPlaybackStatus(false);
}

void FileSharingBlock::onPaused()
{
    repaint();
}


void FileSharingBlock::playMedia(const QString &localPath)
{
    auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    if (!mainWindow || !mainWindow->isActive() || Logic::getContactListModel()->selectedContact() != getChatAimid())
        return;

    const auto isFileExists = QFile::exists(localPath);
    assert(isFileExists);

    if (isGifOrVideoPlaying() || !isFileExists)
    {
        return;
    }

    if (videoplayer_)
    {
        const auto &previewRect = getFileSharingLayout()->getContentRect();
        if (previewRect.isEmpty())
        {
            return;
        }

        changeGifPlaybackStatus(true);

        videoplayer_->setLoadingState(true);
        videoplayer_->start(IsVisible_);

        return;
    }

    bool player_exist = videoplayer_ != nullptr;

    if (!IsInPreloadDistance_)
        return;

    load_task_ = std::make_unique<Utils::LoadMovieToFFMpegPlayerFromFileTask>(localPath, isGifImage(), this);

    QObject::connect(
        load_task_.get(),
        &Utils::LoadMovieToFFMpegPlayerFromFileTask::loadedSignal,
        this,
        [this, player_exist, localPath](QSharedPointer<Ui::DialogPlayer> _movie)
    {
        if (!IsInPreloadDistance_)
            return;

        assert(_movie);

        qDebug() << "create player " << getFilename();

        _movie->setPreview(Preview_);
        videoplayer_ = _movie;

        QObject::connect(
            videoplayer_.data(),
            &Ui::DialogPlayer::paused,
            this, &FileSharingBlock::onPaused,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        Ui::GetSoundsManager();
        bool mute = true;
        int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

        videoplayer_->setParent(this);

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        bool play = (IsVisible_ && mainWindow->isActive());
        videoplayer_->start(play);

        if (play && CtrlButton_)
            CtrlButton_->setVisible(false);

        videoplayer_->setVolume(volume);
        videoplayer_->setMute(mute);

        const auto &contentRect = getFileSharingLayout()->getContentRect();

        videoplayer_->updateSize(contentRect);
        videoplayer_->show();

        QObject::connect(videoplayer_.data(), &DialogPlayer::mouseClicked, this, [this, localPath]()
        {
            Utils::InterConnector::instance().getMainWindow()->openGallery(
                getChatAimid(), Data::Image(getParentComplexMessage()->getId(), getLink(), true), localPath);
        });

        update();

        load_task_.reset();
    });

    load_task_->run();
}

void FileSharingBlock::resizeEvent(QResizeEvent * _event)
{
    FileSharingBlockBase::resizeEvent(_event);

    if (videoplayer_)
    {
        const auto &contentRect = getFileSharingLayout()->getContentRect();

        videoplayer_->updateSize(contentRect);
    }
}


void FileSharingBlock::requestPreview(const QString &uri)
{
    assert(isPreviewable());
    assert(PreviewRequestId_ == -1);

    PreviewRequestId_ = GetDispatcher()->downloadImage(uri, getChatAimid(), QString(), false, 0, 0, false);
}

void FileSharingBlock::sendGenericMetainfoRequests()
{
    if (isPreviewable())
    {
        connectImageSignals(true);

        requestMetainfo(true);
    }

    requestMetainfo(false);
}

void FileSharingBlock::showFileInDir()
{
#ifdef _WIN32
    assert(QFile::exists(getFileLocalPath()));

    const QString param = ql1s("/select,") + QDir::toNativeSeparators(getFileLocalPath());
    const QString command = ql1s("explorer ") + param;
    QProcess::startDetached(command);
#else
#ifdef __APPLE__
    MacSupport::openFinder(getFileLocalPath());
#else
    QDir dir(getFileLocalPath());
    dir.cdUp();
    QDesktopServices::openUrl(dir.absolutePath());
#endif // __APPLE__
#endif //_WIN32
}

void FileSharingBlock::showImagePreviewer(const QString &localPath)
{
    assert(isImage());
    assert(!localPath.isEmpty());

    Utils::InterConnector::instance().getMainWindow()->openGallery(
        getChatAimid(), Data::Image(getParentComplexMessage()->getId(), getLink(), true), localPath);
}

bool FileSharingBlock::shouldDisplayProgressAnimation() const
{
    return true;
}

void FileSharingBlock::unloadGif()
{
    assert(isGifImage() || isVideo());
    assert(videoplayer_);

    qDebug() << "reset unloadGif";

    videoplayer_.reset();
}

void FileSharingBlock::updateFileTypeIcon()
{
    assert(isPlainFile());

    const auto &filename = getFilename();

    assert(!filename.isEmpty());
    if (filename.isEmpty())
    {
        return;
    }

    const auto iconId = History::GetIconIdByFilename(filename);

    auto resourceSet = CtrlButton_->getResourceSet();

    resourceSet.StartButtonImage_ = iconId;
    resourceSet.StartButtonImageActive_ = iconId;
    resourceSet.StartButtonImageHover_ = iconId;

    CtrlButton_->setResourceSet(resourceSet);
}

void FileSharingBlock::onDownloading(const int64_t _bytesTransferred, const int64_t _bytesTotal)
{
    assert(_bytesTotal > 0);
    if (_bytesTotal <= 0)
    {
        return;
    }

    if (!shouldDisplayProgressAnimation())
    {
        return;
    }

    CtrlButton_->startAnimation();

    const auto progress = ((double)_bytesTransferred / (double)_bytesTotal);
    assert(progress >= 0);

    CtrlButton_->setProgress(progress);

    if (isPreviewable())
    {
        const auto text = HistoryControl::formatProgressText(_bytesTotal, _bytesTransferred);
        assert(!text.isEmpty());

        CtrlButton_->setProgressText(text);
    }

    notifyBlockContentsChanged();
}

void FileSharingBlock::onMetainfoDownloaded()
{
    if (isGifImage() || (isVideo() && get_gui_settings()->get_value<bool>(settings_autoplay_video, true)))
    {
        startDownloading(false);
    }

    if (isPlainFile())
    {
        updateFileTypeIcon();

        notifyBlockContentsChanged();
    }
}

void FileSharingBlock::onImageDownloadError(qint64 seq, QString /*rawUri*/)
{
    assert(seq > 0);

    if (PreviewRequestId_ != seq)
    {
        return;
    }

    PreviewRequestId_ = -1;

    onDownloadingFailed(seq);
}

void FileSharingBlock::onImageDownloaded(int64_t seq, QString uri, QPixmap image, QString /*localPath*/)
{
    assert(!image.isNull());

    if (PreviewRequestId_ != seq)
    {
        return;
    }

    PreviewRequestId_ = -1;

    Preview_ = image;

    update();
}

void FileSharingBlock::onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred)
{
    Q_UNUSED(seq);
    Q_UNUSED(bytesTotal);
    Q_UNUSED(bytesTransferred);
}

void FileSharingBlock::onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta)
{
    assert(isPreviewable());

    Q_UNUSED(seq);
    Q_UNUSED(meta);
}

void FileSharingBlock::onGifImageVisibilityChanged(const bool isVisible)
{
    assert(isGifImage() || isVideo());

    if (isVisible)
    {
        if (isFileDownloaded() && ((!videoplayer_) || (videoplayer_ && !videoplayer_->isPausedByUser())))
        {
            playMedia(getFileLocalPath());
        }

        return;
    }

    if (isGifOrVideoPlaying() && !videoplayer_->getAttachedPlayer())
    {
        pauseMedia(false);
    }
}

void FileSharingBlock::onChangeLoadState(const bool _isLoad)
{
    qDebug() << "onChangeLoadState " << _isLoad << ", file: " << getFileLocalPath();


    assert(isGifImage() || isVideo());

    if (!videoplayer_)
        return;

    videoplayer_->setLoadingState(_isLoad);
}

void FileSharingBlock::onLocalCopyInfoReady(const bool isCopyExists)
{
    if (isCopyExists)
    {
        update();
    }
}

void FileSharingBlock::onPreviewMetainfoDownloaded(const QString &miniPreviewUri, const QString &fullPreviewUri)
{
    const auto &previewUri = (
        fullPreviewUri.isEmpty() ?
            miniPreviewUri :
            fullPreviewUri);

    requestPreview(previewUri);
}

void FileSharingBlock::connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover)
{
    if (CtrlButton_)
    {
        connectButtonToHover(CtrlButton_, hover);
        GenericBlock::connectToHover(hover);
    }
}

void FileSharingBlock::setQuoteSelection()
{
    GenericBlock::setQuoteSelection();
    emit setQuoteAnimation();
}

UI_COMPLEX_MESSAGE_NS_END
