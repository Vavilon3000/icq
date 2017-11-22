#include "stdafx.h"
#include "SmilesMenu.h"

#include "toolbar.h"
#include "../ContactDialog.h"
#include "../input_widget/InputWidget.h"
#include "../MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../cache/emoji/Emoji.h"
#include "../../cache/emoji/EmojiDb.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/CommonStyle.h"
#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    const int32_t max_stickers_count = 20;

    namespace
    {
        qint32 getEmojiItemSize();

        qint32 getStickerItemSize_();

        qint32 getStickerSize_();
    }

    using namespace Smiles;

    int getToolBarButtonSize()
    {
        return Utils::scale_value(40);
    }

    int getToolBarButtonEmojiWidth()
    {
        return Utils::scale_value(44);
    }

    int getToolBarButtonEmojiHeight()
    {
        return Utils::scale_value(40);
    }

    int getLongtapTimeout()
    {
        return 500;
    }

    QColor getHoverColor()
    {
        return QColor(0, 0, 0, (int)(255.0 * 0.1));
    }


    Emoji::EmojiSizePx getPickerEmojiSize()
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_32;
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
        case 100:
            emojiSize = Emoji::EmojiSizePx::_32;
            break;
        case 125:
            emojiSize = Emoji::EmojiSizePx::_40;
            break;
        case 150:
            emojiSize = Emoji::EmojiSizePx::_48;
            break;
        case 200:
            emojiSize = Emoji::EmojiSizePx::_64;
            break;
        default:
            assert(!"invalid scale");
        }

        return emojiSize;
    }

    int getToolbarButtonIconSize()
    {
        const int iconSize = Utils::scale_value(32);

        return (Utils::is_mac_retina() ? 2* iconSize : iconSize);
    }

    //////////////////////////////////////////////////////////////////////////
    // class ViewItemModel
    //////////////////////////////////////////////////////////////////////////
    EmojiViewItemModel::EmojiViewItemModel(QWidget* _parent, bool _singleLine)
        : QStandardItemModel(_parent),
        emojisCount_(0),
        needHeight_(0),
        singleLine_(_singleLine),
        spacing_(12)
    {
        emojiCategories_.reserve(10);
    }

    EmojiViewItemModel::~EmojiViewItemModel()
    {
    }

    int EmojiViewItemModel::spacing() const
    {
        return spacing_;
    }

    void EmojiViewItemModel::setSpacing(int _spacing)
    {
        spacing_ = _spacing;
    }

    Emoji::EmojiRecordSptr EmojiViewItemModel::getEmoji(int _col, int _row) const
    {
        int index = _row * columnCount() + _col;

        if (index < getEmojisCount())
        {
            int count = 0;
            for (const auto& category : emojiCategories_)
            {
                if ((count + (int) category.emojis_.size()) > index)
                {
                    int indexInCategory = index - count;

                    assert(indexInCategory >= 0);
                    assert(indexInCategory < (int) category.emojis_.size());

                    return category.emojis_[indexInCategory];
                }

                count += category.emojis_.size();
            }
        }

        assert(!"invalid emoji number");

        return nullptr;
    }

    QVariant EmojiViewItemModel::data(const QModelIndex& _idx, int _role) const
    {
        if (_role == Qt::DecorationRole)
        {
            int index = _idx.row() * columnCount() + _idx.column();
            if (index < getEmojisCount())
            {
                auto emoji = getEmoji(_idx.column(), _idx.row());
                if (emoji)
                {
                    auto emoji_ = Emoji::GetEmoji(emoji->Codepoint_, emoji->ExtendedCodepoint_, getPickerEmojiSize());
                    QPixmap emojiPixmap = QPixmap::fromImage(emoji_);
                    Utils::check_pixel_ratio(emojiPixmap);
                    return emojiPixmap;
                }
            }
        }

        return QVariant();
    }

    int EmojiViewItemModel::addCategory(const QString& _category)
    {
        const Emoji::EmojiRecordSptrVec& emojisVector = Emoji::GetEmojiInfoByCategory(_category);
        emojiCategories_.emplace_back(_category, emojisVector);

        emojisCount_ += emojisVector.size();

        resize(prevSize_);

        return ((int)emojiCategories_.size() - 1);
    }

    int EmojiViewItemModel::addCategory(const emoji_category& _category)
    {
        emojiCategories_.push_back(_category);

        emojisCount_ += _category.emojis_.size();

        return ((int)emojiCategories_.size() - 1);
    }

    int EmojiViewItemModel::getEmojisCount() const
    {
        return emojisCount_;
    }

    int EmojiViewItemModel::getNeedHeight() const
    {
        return needHeight_;
    }

    int EmojiViewItemModel::getCategoryPos(int _index)
    {
        const int columnWidth = getEmojiItemSize();
        int columnCount = prevSize_.width() / columnWidth;

        int emojiCountBefore = 0;

        for (int i = 0; i < _index; i++)
            emojiCountBefore += emojiCategories_[i].emojis_.size();

        if (columnCount == 0)
            return 0;

        int rowCount = (emojiCountBefore / columnCount) + (((emojiCountBefore % columnCount) > 0) ? 1 : 0);

        return (((rowCount == 0) ? 0 : (rowCount - 1)) * getEmojiItemSize());
    }

    const std::vector<emoji_category>& EmojiViewItemModel::getCategories() const
    {
        return emojiCategories_;
    }

    bool EmojiViewItemModel::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = getEmojiItemSize();
        int emojiCount = getEmojisCount();

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            int columnCount = _size.width()/ columnWidth;
            if (columnCount > emojiCount)
                columnCount = emojiCount;

            int rowCount  = 0;
            if (columnCount > 0)
            {
                rowCount = (emojiCount / columnCount) + (((emojiCount % columnCount) > 0) ? 1 : 0);
                if (singleLine_ && rowCount > 1)
                    rowCount = 1;
            }

            setColumnCount(columnCount);
            setRowCount(rowCount);

            needHeight_ = getEmojiItemSize() * rowCount;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    void EmojiViewItemModel::onEmojiAdded()
    {
        emojisCount_ = 0;
        for (uint32_t i = 0; i < emojiCategories_.size(); i++)
            emojisCount_ += emojiCategories_[i].emojis_.size();
    }

    //////////////////////////////////////////////////////////////////////////
    // TableView class
    //////////////////////////////////////////////////////////////////////////
    EmojiTableView::EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model)
        : QTableView(_parent), model_(_model), itemDelegate_(new EmojiTableItemDelegate(this))
    {
        setModel(model_);
        setItemDelegate(itemDelegate_);
        setShowGrid(false);
        verticalHeader()->hide();
        horizontalHeader()->hide();
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        verticalHeader()->setDefaultSectionSize(getEmojiItemSize());
        horizontalHeader()->setDefaultSectionSize(getEmojiItemSize());
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFocusPolicy(Qt::NoFocus);
        setSelectionMode(QAbstractItemView::NoSelection);
        setCursor(QCursor(Qt::PointingHandCursor));
    }

    EmojiTableView::~EmojiTableView()
    {
    }

    void EmojiTableView::resizeEvent(QResizeEvent * _e)
    {
        if (model_->resize(_e->size()))
            setFixedHeight(model_->getNeedHeight());

        QTableView::resizeEvent(_e);
    }

    int EmojiTableView::addCategory(const QString& _category)
    {
        return model_->addCategory(_category);
    }

    int EmojiTableView::addCategory(const emoji_category& _category)
    {
        return model_->addCategory(_category);
    }

    int EmojiTableView::getCategoryPos(int _index)
    {
        return model_->getCategoryPos(_index);
    }

    const std::vector<emoji_category>& EmojiTableView::getCategories() const
    {
        return model_->getCategories();
    }

    Emoji::EmojiRecordSptr EmojiTableView::getEmoji(int _col, int _row) const
    {
        return model_->getEmoji(_col, _row);
    }

    void EmojiTableView::onEmojiAdded()
    {
        model_->onEmojiAdded();

        QRect rect = geometry();

        if (model_->resize(QSize(rect.width(), rect.height()), true))
            setFixedHeight(model_->getNeedHeight());
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiTableItemDelegate
    //////////////////////////////////////////////////////////////////////////
    EmojiTableItemDelegate::EmojiTableItemDelegate(QObject* parent)
        : QItemDelegate(parent)
        , Prop_(0)
    {
        Animation_ = new QPropertyAnimation(this, QByteArrayLiteral("prop"), this);
    }

    void EmojiTableItemDelegate::animate(const QModelIndex& index, int start, int end, int duration)
    {
        AnimateIndex_ = index;
        Animation_->setStartValue(start);
        Animation_->setEndValue(end);
        Animation_->setDuration(duration);
        Animation_->start();
    }

    void EmojiTableItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem&, const QModelIndex& _index) const
    {
        const EmojiViewItemModel *itemModel = (EmojiViewItemModel *)_index.model();
        QPixmap data = _index.data(Qt::DecorationRole).value<QPixmap>();
        int col = _index.column();
        int row = _index.row();
        int spacing = itemModel->spacing();
        int size = (int)getPickerEmojiSize() / Utils::scale_bitmap(1);
        int smileSize = size;
        int addSize = 0;
        if (AnimateIndex_ == _index)
        {
            size = size * Prop_ / Animation_->endValue().toFloat();
            addSize = (smileSize - size) / 2;
        }

        _painter->drawPixmap(col * (smileSize + Utils::scale_value(spacing)) + addSize, row * (smileSize + Utils::scale_value(spacing)) + addSize, size, size, data);
    }

    QSize EmojiTableItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
    {
        int size = (int)getPickerEmojiSize() / Utils::scale_bitmap(1);
        return QSize(size, size);
    }

    void EmojiTableItemDelegate::setProp(int val)
    {
        Prop_ = val;
        if (AnimateIndex_.isValid())
            emit ((QAbstractItemModel*)AnimateIndex_.model())->dataChanged(AnimateIndex_, AnimateIndex_);
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiViewWidget
    //////////////////////////////////////////////////////////////////////////
    EmojisWidget::EmojisWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        QVBoxLayout* vLayout = Utils::emptyVLayout();
        setLayout(vLayout);

        LabelEx* setHeader = new LabelEx(this);
        setHeader->setText(QT_TRANSLATE_NOOP("input_widget", "EMOJI"));
        setHeader->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
        setHeader->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));

        vLayout->addWidget(setHeader);

        view_ = new EmojiTableView(this, new EmojiViewItemModel(this));
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        Utils::grabTouchWidget(view_);

        toolbar_ = new Toolbar(this, buttons_align::center);
        toolbar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        toolbar_->setFixedHeight(Utils::scale_value(48));
        toolbar_->setObjectName(qsl("smiles_cat_selector"));

        TabButton* firstButton = nullptr;

        const QStringList emojiCategories = Emoji::GetEmojiCategories();

        buttons_.reserve(emojiCategories.size());

        for (const auto& category : emojiCategories)
        {
            const QString resourceString = ql1s(":/smiles_menu/emoji_") % category % ql1s("_100");
            TabButton* button = toolbar_->addButton(resourceString);
            button->setFixedSize(getToolBarButtonEmojiWidth(), getToolBarButtonEmojiHeight());
            button->setProperty("underline", false);
            buttons_.push_back(button);

            int categoryIndex = view_->addCategory(category);

            if (!firstButton)
                firstButton = button;

            connect(button, &TabButton::clicked, this, [this, categoryIndex]()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_cathegory_click);
                emit scrollToGroup(view_->geometry().top() + view_->getCategoryPos(categoryIndex));
            });
        }

        vLayout->addWidget(view_);

        QWidget* toolbarPlace = new QWidget(this);
        toolbarPlace->setObjectName(qsl("red_marker"));
        toolbarPlace->setFixedHeight(toolbar_->height());
        vLayout->addWidget(toolbarPlace);

        if (firstButton)
            firstButton->toggle();

        connect(view_, &EmojiTableView::clicked, this, [this](const QModelIndex & _index)
        {
            auto emoji = view_->getEmoji(_index.column(), _index.row());
            if (emoji)
            {
                emit emojiSelected(emoji);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_picker);
            }
        });

        toolbar_->raise();
    }

    EmojisWidget::~EmojisWidget()
    {
    }

    void EmojisWidget::onViewportChanged(const QRect& _viewRect, bool _blockToolbarSwitch)
    {
        if (!_blockToolbarSwitch)
        {
            auto categories = view_->getCategories();
            for (uint32_t i = 0; i < categories.size(); i++)
            {
                int pos = view_->getCategoryPos(i);

                if (_viewRect.top() < pos && pos < (_viewRect.top() + _viewRect.width()/2))
                {
                    buttons_[i]->setChecked(true);
                    break;
                }
            }
        }

        placeToolbar(_viewRect);
    }

    void EmojisWidget::placeToolbar(const QRect& _viewRect)
    {
        int h = toolbar_->height();

        QRect thisRect = rect();

        int y = _viewRect.bottom() - h;
        if (_viewRect.bottom() > thisRect.bottom() || ((-1 *(_viewRect.top()) > (_viewRect.height() / 2))))
            y = thisRect.bottom() - h;

        toolbar_->setGeometry(0, y, thisRect.width(), h);
    }

    void EmojisWidget::selectFirstButton()
    {
        if (buttons_.size() > 0)
        {
            buttons_[0]->setChecked(true);
        }
    }









    StickersTable::StickersTable(
        QWidget* _parent,
        const int32_t _stickersSetId,
        const int32_t _stickerSize,
        const int32_t _itemSize)
        : QWidget(_parent),
        longtapTimer_(new QTimer(this)),
        stickersSetId_(_stickersSetId),
        needHeight_(0),
        columnCount_(0),
        rowCount_(0),
        stickerSize_(_stickerSize),
        itemSize_(_itemSize),
        hoveredSticker_(-1, -1)
    {
        preloader_ = Themes::GetPixmap(Themes::PixmapResourceId::StickerPickerPlaceholder);

        assert(preloader_);

        QObject::connect(longtapTimer_, &QTimer::timeout, this, &StickersTable::longtapTimeout);

        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    StickersTable::~StickersTable()
    {
    }

    void StickersTable::resizeEvent(QResizeEvent * _e)
    {
        if (resize(_e->size()))
            setFixedHeight(getNeedHeight());

        QWidget::resizeEvent(_e);
    }

    void StickersTable::onStickerAdded()
    {
        QRect rect = geometry();

        if (resize(QSize(rect.width(), rect.height()), true))
            setFixedHeight(getNeedHeight());
    }

    bool StickersTable::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = itemSize_;

        int stickersCount = (int) Stickers::getSetStickersCount(stickersSetId_);

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            columnCount_ = _size.width()/ columnWidth;
            if (columnCount_ > stickersCount)
                columnCount_ = stickersCount;

            rowCount_ = 0;
            if (stickersCount > 0 && columnCount_ > 0)
            {
                rowCount_ = (stickersCount / columnCount_) + (((stickersCount % columnCount_) > 0) ? 1 : 0);
            }

            needHeight_ = itemSize_ * rowCount_;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    int StickersTable::getNeedHeight() const
    {
        return needHeight_;
    }

    void StickersTable::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        auto stickerPosInSet = Stickers::getStickerPosInSet(_setId, _stickerId);
        if (stickerPosInSet >= 0)
            update(getStickerRect(stickerPosInSet));
    }

    QRect& StickersTable::getStickerRect(int _index) const
    {
        static QRect rect;

        if (columnCount_ > 0 && rowCount_ > 0)
        {
            int itemY = _index / columnCount_;
            int itemX = _index % columnCount_;

            int x = itemSize_ * itemX;
            int y = itemSize_ * itemY;

            rect.setRect(x, y, itemSize_, itemSize_);
        }
        else
        {
            rect.setRect(0, 0, 0, 0);
        }

        return rect;
    }

    void StickersTable::drawSticker(QPainter& _painter, const int32_t _setId, const int32_t _stickerId, const QRect& _rect)
    {
        static auto hoveredBrush = QBrush(getHoverColor());

        if (hoveredSticker_ == std::make_pair(_setId, _stickerId))
        {
            _painter.save();
            _painter.setBrush(hoveredBrush);
            _painter.setPen(Qt::NoPen);

            const qreal radius = qreal(Utils::scale_value(8));
            _painter.drawRoundedRect(_rect, radius, radius);

            _painter.restore();
        }

        auto image = Stickers::getStickerImage(_setId, _stickerId, core::sticker_size::small);
        if (image.isNull())
        {
            assert(preloader_);
            preloader_->Draw(_painter, _rect);
            return;
        }

        const int sticker_margin = ((_rect.width() - stickerSize_) / 2);

        const QSize imageSize(Utils::scale_bitmap(stickerSize_), Utils::scale_bitmap(stickerSize_));

        image = image.scaled(imageSize.width(), imageSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        const QRect imageRect(_rect.left() + sticker_margin, _rect.top() + sticker_margin, stickerSize_, stickerSize_);

        Utils::check_pixel_ratio(image);

        QRect targetRect(imageRect.left(), imageRect.top(), Utils::unscale_bitmap(image.width()), Utils::unscale_bitmap(image.height()));

        targetRect.moveCenter(imageRect.center());

        _painter.drawImage(targetRect, image, image.rect());
    }

    void StickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        const auto& stickersList = Stickers::getStickers(stickersSetId_);

        for (uint32_t i = 0; i < stickersList.size(); ++i)
        {
            const auto& stickerRect = getStickerRect(i);
            if (!_e->rect().intersects(stickerRect))
            {
                continue;
            }

            drawSticker(p, stickersSetId_, stickersList[i], stickerRect);
        }

        return QWidget::paintEvent(_e);
    }

    std::pair<int32_t, int32_t> StickersTable::getStickerFromPos(const QPoint& _pos) const
    {
        const auto& stickersList = Stickers::getStickers(stickersSetId_);

        for (uint32_t i = 0; i < stickersList.size(); ++i)
        {
            if (getStickerRect(i).contains(_pos))
            {
                return std::make_pair(stickersSetId_, stickersList[i]);
            }
        }

        return std::make_pair(stickersSetId_, -1);
    }

    void StickersTable::mouseReleaseEvent(const QPoint& _pos)
    {
        longtapTimer_->stop();

        const auto sticker = getStickerFromPos(_pos);
        if (sticker.second >= 0)
            emit stickerSelected(sticker.first, sticker.second);
    }

    void StickersTable::redrawSticker(const int32_t _setId, const int32_t _stickerId)
    {
        if (_setId >= 0 && _stickerId >= 0)
        {
            const auto stickerPosInSet = Stickers::getStickerPosInSet(_setId, _stickerId);
            if (stickerPosInSet >= 0)
                repaint(getStickerRect(stickerPosInSet));
        }
    }

    void StickersTable::mouseMoveEvent(const QPoint& _pos)
    {
        const auto sticker = getStickerFromPos(_pos);

        if (hoveredSticker_ != sticker)
        {
            const auto prevSticker = hoveredSticker_;

            hoveredSticker_ = sticker;

            redrawSticker(sticker.first, sticker.second);
            redrawSticker(prevSticker.first, prevSticker.second);

            if (sticker.first >= 0 && sticker.second >= 0)
                emit stickerHovered(sticker.first, sticker.second);
        }
    }

    void StickersTable::leaveEvent()
    {
        const auto newHoveredSticker = std::make_pair(-1, -1);

        if (hoveredSticker_ != newHoveredSticker)
        {
            const auto prevHoveredSticker = hoveredSticker_;

            hoveredSticker_ = newHoveredSticker;

            redrawSticker(hoveredSticker_.first, hoveredSticker_.second);
            redrawSticker(prevHoveredSticker.first, prevHoveredSticker.second);
        }
    }

    void StickersTable::longtapTimeout()
    {
        longtapTimer_->stop();

        const auto pos = mapFromGlobal(QCursor::pos());
        const auto sticker = getStickerFromPos(pos);
        if (sticker.second >= 0)
            emit stickerPreview(sticker.first, sticker.second);
    }

    void StickersTable::mousePressEvent(const QPoint& _pos)
    {
        const auto sticker = getStickerFromPos(_pos);
        if (sticker.second >= 0)
        {
            longtapTimer_->stop();
            longtapTimer_->start(getLongtapTimeout());
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentsStickersTable
    //////////////////////////////////////////////////////////////////////////
    RecentsStickersTable::RecentsStickersTable(QWidget* _parent, const qint32 _stickerSize, const qint32 _itemSize)
        :   StickersTable(_parent, -1, _stickerSize, _itemSize),
            maxRowCount_(-1)
    {

    }

    RecentsStickersTable::~RecentsStickersTable()
    {

    }

    void RecentsStickersTable::setMaxRowCount(int _val)
    {
        maxRowCount_ = _val;
    }

    bool RecentsStickersTable::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = itemSize_;

        int stickersCount = (int) recentStickersArray_.size();

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            columnCount_ = _size.width()/ columnWidth;
            if (columnCount_ > stickersCount)
                columnCount_ = stickersCount;

            rowCount_ = 0;
            if (stickersCount > 0 && columnCount_ > 0)
            {
                rowCount_ = (stickersCount / columnCount_) + (((stickersCount % columnCount_) > 0) ? 1 : 0);
                if (maxRowCount_ > 0 && rowCount_ > maxRowCount_)
                    rowCount_ = maxRowCount_;
            }

            needHeight_ = itemSize_ * rowCount_;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    void RecentsStickersTable::redrawSticker(const int32_t _setId, const int32_t _stickerId)
    {
        if (_setId >= 0 && _stickerId >= 0)
        {
            for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
            {
                if (_setId == recentStickersArray_[i].first && recentStickersArray_[i].second == _stickerId)
                {
                    repaint(getStickerRect(i));
                }
            }
        }
    }

    void RecentsStickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
        {
            const auto& stickerRect = getStickerRect(i);
            if (!_e->rect().intersects(stickerRect))
            {
                continue;
            }

            drawSticker(p, recentStickersArray_[i].first, recentStickersArray_[i].second, stickerRect);
        }

        return QWidget::paintEvent(_e);
    }

    void RecentsStickersTable::clear()
    {
        recentStickersArray_.clear();
    }

    bool RecentsStickersTable::addSticker(int32_t _setId, int32_t _stickerId)
    {
        for (auto iter = recentStickersArray_.begin(); iter != recentStickersArray_.end(); ++iter)
        {
            if (iter->first == _setId && iter->second == _stickerId)
            {
                if (iter == recentStickersArray_.begin())
                    return false;

                recentStickersArray_.erase(iter);
                break;
            }
        }

        recentStickersArray_.emplace(recentStickersArray_.begin(), _setId, _stickerId);

        if ((int32_t)recentStickersArray_.size() > max_stickers_count)
            recentStickersArray_.pop_back();

        return true;
    }

    void RecentsStickersTable::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
        {
            if (recentStickersArray_[i].first == _setId && recentStickersArray_[i].second == _stickerId)
            {
                update(getStickerRect(i));
                break;
            }
        }
    }

    std::pair<int32_t, int32_t> RecentsStickersTable::getStickerFromPos(const QPoint& _pos) const
    {
        for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
        {
            if (getStickerRect(i).contains(_pos))
            {
                return std::make_pair(recentStickersArray_[i].first, recentStickersArray_[i].second);
            }
        }

        return std::make_pair(-1, -1);
    }

    const recentStickersArray& RecentsStickersTable::getStickers() const
    {
        return recentStickersArray_;
    }

    //////////////////////////////////////////////////////////////////////////
    // class StickersWidget
    //////////////////////////////////////////////////////////////////////////


    StickersWidget::StickersWidget(QWidget* _parent, Toolbar* _toolbar)
        : QWidget(_parent)
        , toolbar_(_toolbar)
        , initialized_(false)
        , previewActive_(false)
    {
    }

    StickersWidget::~StickersWidget()
    {
    }

    void StickersWidget::insertNextSet(int32_t _setId)
    {
        if (setTables_.find(_setId) != setTables_.end())
            return;

        auto stickersView = new StickersTable(this, _setId, getStickerSize_(), getStickerItemSize_());


        auto button = toolbar_->addButton(
            Stickers::getSetIcon(_setId).scaled(
                getToolbarButtonIconSize(),
                getToolbarButtonIconSize(),
                Qt::AspectRatioMode::KeepAspectRatio,
                Qt::TransformationMode::SmoothTransformation));

        button->setFixedSize(getToolBarButtonSize(), getToolBarButtonSize());


        button->AttachView(AttachedView(stickersView, this));

        connect(button, &TabButton::clicked, this, [this, _setId]()
        {
            emit scrollToSet(setTables_[_setId]->geometry().top());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        LabelEx* setHeader = new LabelEx(this);
        setHeader->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
        setHeader->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        setHeader->setText(Stickers::getSetName(_setId));
        setHeader->setFixedHeight(Utils::scale_value(24));
        vLayout_->addWidget(setHeader);


        vLayout_->addWidget(stickersView);
        setTables_[_setId] = stickersView;
        Utils::grabTouchWidget(stickersView);

        connect(stickersView, &StickersTable::stickerSelected, this, [this, stickersView](int32_t _setId, int32_t _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_picker);
        });

        connect(stickersView, &StickersTable::stickerHovered, this, [this, stickersView](int32_t _setId, int32_t _stickerId)
        {
            emit stickerHovered(_setId, _stickerId);
        });

        connect(stickersView, &StickersTable::stickerPreview, this, [this, stickersView](int32_t _setId, int32_t _stickerId)
        {
            previewActive_ = true;
            emit stickerPreview(_setId, _stickerId);
        });

        setCursor(QCursor(Qt::PointingHandCursor));

        setMouseTracking(true);
    }

    void StickersWidget::init()
    {
        if (!initialized_)
        {
            vLayout_ = Utils::emptyVLayout();
        }

        clear();

        for (const auto _stickersSetId : Stickers::getStickersSets())
        {
            if (Stickers::isPurchasedSet(_stickersSetId))
                insertNextSet(_stickersSetId);
        }

        if (!initialized_)
        {
            setLayout(vLayout_);
        }

        initialized_ = true;
    }

    void StickersWidget::clear()
    {
        while (vLayout_->count())
        {
            QLayoutItem* childItem = vLayout_->itemAt(0);

            QWidget* childWidget = childItem->widget();

            vLayout_->removeItem(childItem);

            delete childWidget;
        }

        setTables_.clear();
        toolbar_->Clear();
    }

    void StickersWidget::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        auto iterSet = setTables_.find(_setId);
        if (iterSet == setTables_.end())
        {
            return;
        }

        iterSet->second->onStickerUpdated(_setId, _stickerId);
    }

    void StickersWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::MouseButton::LeftButton)
        {
            if (previewActive_)
            {
                previewActive_ = false;

                emit stickerPreviewClose();
            }
            else
            {
                for (auto iter = setTables_.begin(); iter != setTables_.end(); ++iter)
                {
                    auto table = iter->second;

                    const auto rc = table->geometry();

                    if (rc.contains(_e->pos()))
                    {
                        table->mouseReleaseEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
                    }
                }
            }
        }

        QWidget::mouseReleaseEvent(_e);
    }

    void StickersWidget::mousePressEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::MouseButton::LeftButton)
        {
            for (auto iter = setTables_.begin(); iter != setTables_.end(); ++iter)
            {
                auto table = iter->second;

                const auto rc = table->geometry();

                if (rc.contains(_e->pos()))
                {
                    table->mousePressEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
                }
            }
        }

        QWidget::mousePressEvent(_e);
    }

    void StickersWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        for (auto iter = setTables_.begin(); iter != setTables_.end(); ++iter)
        {
            auto table = iter->second;

            const auto rc = table->geometry();

            table->mouseMoveEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
        }

        QWidget::mouseMoveEvent(_e);
    }

    void StickersWidget::leaveEvent(QEvent* _e)
    {
        for (auto iter = setTables_.begin(); iter != setTables_.end(); ++iter)
        {
            auto table = iter->second;

            table->leaveEvent();
        }

        QWidget::leaveEvent(_e);
    }

    //////////////////////////////////////////////////////////////////////////
    // Recents widget
    //////////////////////////////////////////////////////////////////////////
    RecentsWidget::RecentsWidget(QWidget* _parent)
        : QWidget(_parent)
        , vLayout_(nullptr)
        , emojiView_(nullptr)
        , stickersView_(nullptr)
        , previewActive_(false)
    {
        initEmojisFromSettings();

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::onStickers, this, &RecentsWidget::stickers_event);

        setMouseTracking(true);

        setCursor(QCursor(Qt::PointingHandCursor));
    }

    RecentsWidget::~RecentsWidget()
    {
    }


    void RecentsWidget::stickers_event()
    {
        if (stickersView_)
        {
            stickersView_->clear();
        }

        initStickersFromSettings();
    }

    void RecentsWidget::init()
    {
        if (vLayout_)
            return;

        vLayout_ = Utils::emptyVLayout();

        LabelEx* setHeader = new LabelEx(this);
        setHeader->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
        setHeader->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        setHeader->setText(QT_TRANSLATE_NOOP("input_widget", "RECENTS"));
        vLayout_->addWidget(setHeader);

        stickersView_ = new RecentsStickersTable(this, getStickerSize_(), getStickerItemSize_());
        stickersView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        stickersView_->setMaxRowCount(2);
        vLayout_->addWidget(stickersView_);
        Utils::grabTouchWidget(stickersView_);

        emojiView_ = new EmojiTableView(this, new EmojiViewItemModel(this, true));
        emojiView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emojiView_->addCategory(emoji_category(qsl("recents"), emojis_));
        vLayout_->addWidget(emojiView_);
        Utils::grabTouchWidget(emojiView_);

        setLayout(vLayout_);

        connect(emojiView_, &EmojiTableView::clicked, this, [this](const QModelIndex & _index)
        {
            auto emoji = emojiView_->getEmoji(_index.column(), _index.row());
            if (emoji)
            {
                emit emojiSelected(emoji);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_from_recents);
            }
        });

        connect(stickersView_, &RecentsStickersTable::stickerSelected, this, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_recents);
        });

        connect(stickersView_, &RecentsStickersTable::stickerHovered, this, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerHovered(_setId, _stickerId);
        });

        connect(stickersView_, &RecentsStickersTable::stickerPreview, this, [this](qint32 _setId, qint32 _stickerId)
        {
            previewActive_ = true;
            emit stickerPreview(_setId, _stickerId);
        });
    }

    void RecentsWidget::addSticker(int32_t _setId, int32_t _stickerId)
    {
        init();

        if (stickersView_->addSticker(_setId, _stickerId))
        {
            stickersView_->onStickerAdded();

            storeStickers();
        }
    }

    void RecentsWidget::storeStickers()
    {
        const auto& recentsStickers = stickersView_->getStickers();

        std::vector<int32_t> vStickers;

        vStickers.reserve(recentsStickers.size());

        for (uint32_t i = 0; i < recentsStickers.size(); ++i)
        {
            vStickers.push_back(recentsStickers[i].first);
            vStickers.push_back(recentsStickers[i].second);
        }

        get_gui_settings()->set_value<std::vector<int32_t>>(settings_recents_stickers, vStickers);
    }

    void RecentsWidget::initStickersFromSettings()
    {
        static int count = 0;

        ++count;

        auto sticks = get_gui_settings()->get_value<std::vector<int32_t>>(settings_recents_stickers, std::vector<int32_t>());
        if (sticks.empty() || (sticks.size() % 2 != 0))
            return;

        init();

        bool changed = false;

        for (uint32_t i = 0; i < sticks.size();)
        {
            int32_t setId = sticks[i];
            int32_t stickerId = sticks[++i];

            ++i;

            if (!Stickers::isConfigHasSticker(setId, stickerId))
            {
                changed = true;

                continue;
            }

            stickersView_->addSticker(setId, stickerId);
        }

        if (changed)
        {
            storeStickers();
        }

        stickersView_->onStickerAdded();
    }

    void RecentsWidget::addEmoji(Emoji::EmojiRecordSptr _emoji)
    {
        init();

        for (auto iter = emojis_.begin(); iter != emojis_.end(); iter++)
        {
            if (_emoji->Codepoint_ == (*iter)->Codepoint_ && _emoji->ExtendedCodepoint_ == (*iter)->ExtendedCodepoint_)
            {
                if (iter == emojis_.begin())
                    return;

                emojis_.erase(iter);
                break;
            }
        }

        emojis_.insert(emojis_.begin(), _emoji);
        if (emojis_.size() > 20)
            emojis_.pop_back();

        emojiView_->onEmojiAdded();

        std::vector<int32_t> vEmojis;
        vEmojis.reserve(emojis_.size() * 2);
        for (uint32_t i = 0; i < emojis_.size(); i++)
        {
            vEmojis.push_back(emojis_[i]->Codepoint_);
            vEmojis.push_back(emojis_[i]->ExtendedCodepoint_);
        }

        get_gui_settings()->set_value<std::vector<int32_t>>(settings_recents_emojis, vEmojis);
    }

    void RecentsWidget::initEmojisFromSettings()
    {
        auto emojis = get_gui_settings()->get_value<std::vector<int32_t>>(settings_recents_emojis, std::vector<int32_t>());
        if (emojis.empty() || (emojis.size() % 2 != 0))
            return;

        init();

        emojis_.reserve(20);

        for (uint32_t i = 0; i < emojis.size();)
        {
            int32_t codepoint = emojis[i];
            int32_t extCodepoint = emojis[++i];

            ++i;

            auto emoji = Emoji::GetEmojiInfoByCodepoint(codepoint, extCodepoint);
            if (!emoji)
            {
                assert(false);
                continue;
            }

            emojis_.push_back(emoji);
        }

        emojiView_->onEmojiAdded();
    }

    void RecentsWidget::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        if (stickersView_)
        {
            stickersView_->onStickerUpdated(_setId, _stickerId);
        }
    }

    void RecentsWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::MouseButton::LeftButton)
        {
            if (previewActive_)
            {
                previewActive_ = false;

                emit stickerPreviewClose();
            }
            else
            {
                const auto rc = stickersView_->geometry();

                if (rc.contains(_e->pos()))
                {
                    stickersView_->mouseReleaseEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
                }
            }
        }

        QWidget::mouseReleaseEvent(_e);
    }

    void RecentsWidget::mousePressEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::MouseButton::LeftButton)
        {
            const auto rc = stickersView_->geometry();

            if (rc.contains(_e->pos()))
            {
                stickersView_->mousePressEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
            }
        }

        QWidget::mousePressEvent(_e);
    }

    void RecentsWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        const auto rc = stickersView_->geometry();

        if (rc.contains(_e->pos()))
        {
            stickersView_->mouseMoveEvent(QPoint(_e->pos().x() - rc.left(), _e->pos().y() - rc.top()));
        }

        QWidget::mouseMoveEvent(_e);
    }

    void RecentsWidget::leaveEvent(QEvent* _e)
    {
        stickersView_->leaveEvent();

        QWidget::leaveEvent(_e);
    }





    //////////////////////////////////////////////////////////////////////////
    // StickerPreview
    //////////////////////////////////////////////////////////////////////////
    StickerPreview::StickerPreview(
        QWidget* _parent,
        const int32_t _setId,
        const int32_t _stickerId)

        : QWidget(_parent)
        , setId_(_setId)
        , stickerId_(_stickerId)
    {
        QObject::connect(GetDispatcher(), &core_dispatcher::onSticker, this, [this](
            const qint32 _error,
            const qint32 _setId,
            const qint32 _stickerId)
        {
            if (_setId == setId_ && _stickerId == stickerId_)
            {
                sticker_ = QPixmap();

                repaint();
            }
        });
    }

    void StickerPreview::showSticker(
        const int32_t _setId,
        const int32_t _stickerId)
    {
        stickerId_ = _stickerId;
        setId_ = _setId;

        sticker_ = QPixmap();

        repaint();
    }

    void StickerPreview::paintEvent(QPaintEvent* _e)
    {
        QWidget::paintEvent(_e);

        const auto clientRect = rect();

        const auto imageMargin = Utils::scale_value(16);

        QRect imageRect(
            clientRect.left() + imageMargin,
            clientRect.top() + imageMargin,
            clientRect.width() - 2 * imageMargin,
            clientRect.height() - 2 * imageMargin);

        if (sticker_.isNull())
        {
            sticker_ = QPixmap::fromImage(Stickers::getStickerImage(setId_, stickerId_, core::sticker_size::xxlarge, true));

            if (!sticker_.isNull())
            {
                sticker_ = sticker_.scaled(Utils::scale_bitmap(imageRect.size()), Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

                Utils::check_pixel_ratio(sticker_);
            }
        }

        QPainter p(this);

        p.fillRect(clientRect, QColor(0xff, 0xff, 0xff, int(255.0 * 0.9)));

        if (!sticker_.isNull())
        {
            const QSize stickerSize = Utils::unscale_bitmap(sticker_.size());

            const int x = imageRect.left() + ((imageRect.width() - stickerSize.width()) / 2);
            const int y = imageRect.top() + ((imageRect.height() - stickerSize.height()) / 2);

            p.drawPixmap(x, y, stickerSize.width(), stickerSize.height(), sticker_);
        }
    }







    //////////////////////////////////////////////////////////////////////////
    // SmilesMenu class
    //////////////////////////////////////////////////////////////////////////
    SmilesMenu::SmilesMenu(QWidget* _parent)
        : QFrame(_parent)
        , topToolbar_(nullptr)
        , bottomToolbar_(nullptr)
        , viewArea_(nullptr)
        , recentsView_(nullptr)
        , emojiView_(nullptr)
        , stickersView_(nullptr)
        , stickerPreview_(nullptr)
        , animHeight_(nullptr)
        , animScroll_(nullptr)
        , isVisible_(false)
        , blockToolbarSwitch_(false)
        , stickerMetaRequested_(false)
        , currentHeight_(0)
    {
        rootVerticalLayout_ = Utils::emptyVLayout(this);
        setLayout(rootVerticalLayout_);

        setStyleSheet(Utils::LoadStyle(qsl(":/qss/smiles_menu")));

        InitSelector();
        InitStickers();
        InitResents();

        if (Ui::GetDispatcher()->isImCreated())
        {
            im_created();
        }

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, &SmilesMenu::im_created, Qt::QueuedConnection);
    }

    SmilesMenu::~SmilesMenu()
    {
    }

    void SmilesMenu::im_created()
    {
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        std::string size = "small";

        switch (scale)
        {
        case 150:
            size = "medium";
            break;
        case 200:
            size = "large";
            break;
        }

        stickerMetaRequested_ = true;

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("size", size);
        Ui::GetDispatcher()->post_message_to_core(qsl("stickers/meta/get"), collection.get());
    }

    void SmilesMenu::touchScrollStateChanged(QScroller::State state)
    {
        recentsView_->blockSignals(state != QScroller::Inactive);
        emojiView_->blockSignals(state != QScroller::Inactive);
        stickersView_->blockSignals(state != QScroller::Inactive);
    }

    void SmilesMenu::paintEvent(QPaintEvent* _e)
    {
        /*QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive (QStyle::PE_Widget, &opt, &p, this);*/

       // QWidget::paintEvent(_e);

        QPainter p(this);

        QRect rc = rect();

        p.fillRect(rc, QColor(0xff, 0xff, 0xff, int(255.0*0.1)));
    }

    void SmilesMenu::focusOutEvent(QFocusEvent* _event)
    {
        Qt::FocusReason reason = _event->reason();

        if (reason != Qt::FocusReason::ActiveWindowFocusReason)
            Hide();

        QFrame::focusOutEvent(_event);
    }

    void SmilesMenu::focusInEvent(QFocusEvent* _event)
    {
        QFrame::focusInEvent(_event);
    }


    void SmilesMenu::Show()
    {
        if (isVisible_)
            return;

        ShowHide();
    }

    void SmilesMenu::Hide()
    {
        if (!isVisible_)
            return;

        ShowHide();
        emit menuHidden();
    }

    void SmilesMenu::ShowHide()
    {
        isVisible_ = !isVisible_;

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        int pickerDefaultHeight = Utils::scale_value(320);
        int minTopMargin = Utils::scale_value(160);
        InputWidget* input = Utils::InterConnector::instance().getContactDialog()->getInputWidget();
        int inputWidgetHeight = input->get_current_height();
        auto pickerHeight =
            ((mainWindow->height() - pickerDefaultHeight - inputWidgetHeight) > minTopMargin) ?
            pickerDefaultHeight : (mainWindow->height() - minTopMargin - inputWidgetHeight);
        int start_value = isVisible_ ? 0 : pickerHeight;
        int end_value = isVisible_ ? pickerHeight : 0;

        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 200;

        if (!animHeight_)
        {
            animHeight_ = new QPropertyAnimation(this, QByteArrayLiteral("currentHeight"), this);

            connect(animHeight_, &QPropertyAnimation::finished, this, [this]()
            {
                if (!isVisible_)
                    Ui::Stickers::clearCache();
            });
        }

        animHeight_->stop();
        animHeight_->setDuration(duration);
        animHeight_->setStartValue(start_value);
        animHeight_->setEndValue(end_value);
        animHeight_->setEasingCurve(easing_curve);
        animHeight_->start();
    }

    bool SmilesMenu::IsHidden() const
    {
        assert(currentHeight_ >= 0);
        return (currentHeight_ == 0);
    }

    void SmilesMenu::ScrollTo(int _pos)
    {
        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 200;

        if (!animScroll_)
        {
            animScroll_ = new QPropertyAnimation(viewArea_->verticalScrollBar(), QByteArrayLiteral("value"), this);
            connect(animScroll_, &QPropertyAnimation::finished, this, [this]()
            {
                blockToolbarSwitch_ = false;
            });
        }

        blockToolbarSwitch_ = true;
        animScroll_->stop();
        animScroll_->setDuration(duration);
        animScroll_->setStartValue(viewArea_->verticalScrollBar()->value());
        animScroll_->setEndValue(_pos);
        animScroll_->setEasingCurve(easing_curve);
        animScroll_->start();
    }

    void SmilesMenu::InitResents()
    {
        connect(recentsView_, &RecentsWidget::emojiSelected, this, [this](Emoji::EmojiRecordSptr _emoji)
        {
            emit emojiSelected(_emoji->Codepoint_, _emoji->ExtendedCodepoint_);
        });

        connect(recentsView_, &RecentsWidget::stickerSelected, this, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);
        });

        connect(recentsView_, &RecentsWidget::stickerHovered, this, [this](qint32 _setId, qint32 _stickerId)
        {
            updateStickerPreview(_setId, _stickerId);
        });

        connect(recentsView_, &RecentsWidget::stickerPreview, this, [this](qint32 _setId, qint32 _stickerId)
        {
            showStickerPreview(_setId, _stickerId);
        });

        connect(recentsView_, &RecentsWidget::stickerPreviewClose, this, [this]()
        {
            hideStickerPreview();
        });
    }

    void SmilesMenu::stickersMetaEvent()
    {
        stickerMetaRequested_ = false;
        stickersView_->init();
    }

    void SmilesMenu::stickerEvent(const qint32 _error, const qint32 _setId, const qint32 _stickerId)
    {
        stickersView_->onStickerUpdated(_setId, _stickerId);
        recentsView_->onStickerUpdated(_setId, _stickerId);
    }

    void SmilesMenu::InitStickers()
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::onStickers, this, &SmilesMenu::stickersMetaEvent);
        connect(Ui::GetDispatcher(), &core_dispatcher::onSticker, this, &SmilesMenu::stickerEvent);

        connect(stickersView_, &StickersWidget::stickerSelected, this, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);

            recentsView_->addSticker(_setId, _stickerId);
        });

        connect(stickersView_, &StickersWidget::stickerHovered, this, [this](qint32 _setId, qint32 _stickerId)
        {
            updateStickerPreview(_setId, _stickerId);
        });

        connect(stickersView_, &StickersWidget::stickerPreview, this, [this](qint32 _setId, qint32 _stickerId)
        {
            showStickerPreview(_setId, _stickerId);
        });

        connect(stickersView_, &StickersWidget::stickerPreviewClose, this, [this]()
        {
            hideStickerPreview();
        });

        connect(stickersView_, &StickersWidget::scrollToSet, this, [this](int _pos)
        {
            ScrollTo(stickersView_->geometry().top() + _pos);
        });
    }


    void SmilesMenu::InitSelector()
    {
        topToolbar_ = new Toolbar(this, buttons_align::left);
        topToolbar_->addButtonStore();
        recentsView_ = new RecentsWidget(this);
        emojiView_ = new EmojisWidget(this);

        topToolbar_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        topToolbar_->setFixedHeight(getToolBarButtonSize() + Utils::scale_value(1));

        auto resents_button = topToolbar_->addButton(qsl(":/smiles_menu/i_recents_a_100"));
        resents_button->setFixedSize(getToolBarButtonSize(), getToolBarButtonSize());
        resents_button->AttachView(recentsView_);
        resents_button->setFixed(true);
        connect(resents_button, &TabButton::clicked, this, [this]()
        {
            ScrollTo(recentsView_->geometry().top());
            emojiView_->selectFirstButton();
        });

        auto emojiButton = topToolbar_->addButton(qsl(":/smiles_menu/i_emoji_100"));
        emojiButton->setFixedSize(getToolBarButtonSize(), getToolBarButtonSize());
        emojiButton->AttachView(AttachedView(emojiView_));
        emojiButton->setFixed(true);
        connect(emojiButton, &TabButton::clicked, this, [this]()
        {
            ScrollTo(emojiView_->geometry().top());
            emojiView_->selectFirstButton();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        rootVerticalLayout_->addWidget(topToolbar_);

        viewArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        viewArea_->setFocusPolicy(Qt::NoFocus);
        Utils::grabTouchWidget(viewArea_->viewport(), true);
        connect(QScroller::scroller(viewArea_->viewport()), &QScroller::stateChanged, this, &SmilesMenu::touchScrollStateChanged, Qt::QueuedConnection);

        QWidget* scroll_area_widget = new QWidget(viewArea_);
        scroll_area_widget->setObjectName(qsl("scroll_area_widget"));
        viewArea_->setWidget(scroll_area_widget);
        viewArea_->setWidgetResizable(true);
        rootVerticalLayout_->addWidget(viewArea_);


        QVBoxLayout* sa_widgetLayout = new QVBoxLayout();
        sa_widgetLayout->setContentsMargins(Utils::scale_value(10), 0, Utils::scale_value(10), 0);
        scroll_area_widget->setLayout(sa_widgetLayout);

        recentsView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sa_widgetLayout->addWidget(recentsView_);
        Utils::grabTouchWidget(recentsView_);

        emojiView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sa_widgetLayout->addWidget(emojiView_);
        Utils::grabTouchWidget(emojiView_);

        stickersView_ = new StickersWidget(this, topToolbar_);
        stickersView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sa_widgetLayout->addWidget(stickersView_);
        Utils::grabTouchWidget(stickersView_);

        emojiButton->toggle();

        connect(emojiView_, &EmojisWidget::emojiSelected, this, [this](Emoji::EmojiRecordSptr _emoji)
        {
            emit emojiSelected(_emoji->Codepoint_, _emoji->ExtendedCodepoint_);

            recentsView_->addEmoji(_emoji);
        });

        connect(emojiView_, &EmojisWidget::scrollToGroup, this, [this](int _pos)
        {
            ScrollTo(emojiView_->geometry().top() + _pos);
        });

        HookScroll();
    }

    void SmilesMenu::HookScroll()
    {
        connect(viewArea_->verticalScrollBar(), &QAbstractSlider::valueChanged, this, [this](int _value)
        {
            QRect viewPortRect = viewArea_->viewport()->geometry();
            QRect emojiRect = emojiView_->geometry();
            QRect viewPortRectForEmoji(0, _value - emojiRect.top(), viewPortRect.width(), viewPortRect.height() + 1);

            emojiView_->onViewportChanged(viewPortRectForEmoji, blockToolbarSwitch_);

            viewPortRect.adjust(0, _value, 0, _value);

            if (blockToolbarSwitch_)
                return;

            for (TabButton* button : topToolbar_->GetButtons())
            {
                auto view = button->GetAttachedView();
                if (view.getView())
                {
                    QRect rcView = view.getView()->geometry();

                    if (view.getViewParent())
                    {
                        QRect rcViewParent = view.getViewParent()->geometry();
                        rcView.adjust(0, rcViewParent.top(), 0, rcViewParent.top());
                    }

                    QRect intersectedRect = rcView.intersected(viewPortRect);

                    if (intersectedRect.height() > (viewPortRect.height() / 2))
                    {
                        button->setChecked(true);
                        topToolbar_->scrollToButton(button);
                        break;
                    }
                }
            }
        });
    }


    void SmilesMenu::showStickerPreview(const int32_t _setId, const int32_t _stickerId)
    {
        if (!stickerPreview_)
        {
            stickerPreview_ = new StickerPreview(this, _setId, _stickerId);
            stickerPreview_->setGeometry(rect());
            stickerPreview_->show();
            stickerPreview_->raise();
        }
    }

    void SmilesMenu::updateStickerPreview(const int32_t _setId, const int32_t _stickerId)
    {
        if (stickerPreview_)
        {
            stickerPreview_->showSticker(_setId, _stickerId);
        }
    }

    void SmilesMenu::hideStickerPreview()
    {
        if (!stickerPreview_)
            return;

        stickerPreview_->hide();
        delete stickerPreview_;
        stickerPreview_ = nullptr;
    }

    void SmilesMenu::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        if (!viewArea_ || !emojiView_)
            return;

        QRect viewPortRect = viewArea_->viewport()->geometry();
        QRect emojiRect = emojiView_->geometry();

        emojiView_->onViewportChanged(QRect(0, viewArea_->verticalScrollBar()->value() - emojiRect.top(), viewPortRect.width(), viewPortRect.height() + 1),
            blockToolbarSwitch_);


    }

    void SmilesMenu::setCurrentHeight(int _val)
    {
        setMaximumHeight(_val);
        setMinimumHeight(_val);

        currentHeight_ = _val;
    }

    int SmilesMenu::getCurrentHeight() const
    {
        return currentHeight_;
    }

    namespace
    {
        qint32 getEmojiItemSize()
        {
            const auto EMOJI_ITEM_SIZE = 44;
            return Utils::scale_value(EMOJI_ITEM_SIZE);
        }

        qint32 getStickerItemSize_()
        {
            const auto STICKER_ITEM_SIZE = 68;
            return Utils::scale_value(STICKER_ITEM_SIZE);
        }

        qint32 getStickerSize_()
        {
            const auto STICKER_SIZE = 60;
            return Utils::scale_value(STICKER_SIZE);
        }
    }
}
