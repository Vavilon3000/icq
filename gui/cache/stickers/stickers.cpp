#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../main_window/smiles_menu/StickerPackInfo.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"

#include "stickers.h"

UI_STICKERS_NS_BEGIN

std::unique_ptr<Cache> g_cache;
Cache& getCache();

Sticker::Sticker()
    : id_(0)
    , failed_(false)
{

}

Sticker::Sticker(const int32_t _id)
    : id_(_id)
    , failed_(false)
{
    assert(id_ > 0);
}

void Sticker::unserialize(core::coll_helper _coll)
{
    id_ = _coll.get_value_as_int("id");
}

int32_t Sticker::getId() const
{
    return id_;
}

void Sticker::setFailed(const bool _failed)
{
    failed_ = _failed;
}

bool Sticker::isFailed() const
{
    return failed_;
}

QImage Sticker::getImage(const core::sticker_size _size, const bool _scaleIfNeed, bool& _scaled) const
{
    _scaled = false;

    const auto found = images_.find(_size);

    if (found != images_.cend())
    {
        const auto& image = std::get<0>(found->second);

        if (!image.isNull() || !_scaleIfNeed)
            return image;
    }

    if (!images_.empty() && _scaleIfNeed)
    {
        for (auto iter = images_.rbegin(); iter != images_.rend(); ++iter)
        {
            const auto& image = std::get<0>(iter->second);

            if (!image.isNull())
            {
                _scaled = true;

                return image;
            }
        }
        
        return std::get<0>(images_.rbegin()->second);
    }

    return QImage();
}

void Sticker::setImage(const core::sticker_size _size, const QImage &_image)
{
    assert(!_image.isNull());

    auto &imageData = images_[_size];

    if (std::get<0>(imageData).isNull())
    {
        imageData = std::make_tuple(_image, false);
    }
}

bool Sticker::isImageRequested(const core::sticker_size _size) const
{
    const auto found = images_.find(_size);
    if (found == images_.end())
    {
        return false;
    }

    return std::get<1>(found->second);
}

void Sticker::setImageRequested(const core::sticker_size _size, const bool _val)
{
    auto &imageData = images_[_size];

    imageData = std::make_tuple(
        std::get<0>(imageData),
        _val
        );

    assert(!_val || std::get<0>(imageData).isNull());
}

void Sticker::clearCache()
{
    for (auto &pair : images_)
    {
        auto &image = std::get<0>(pair.second);

        pair.second = std::make_tuple(image, false);

        image.loadFromData(0, 0);
    }
}

Set::Set(int32_t _maxSize)
    : id_(-1)
    , purchased_(true)
    , user_(true)
    , maxSize_(_maxSize)
{

}


QImage Set::getStickerImage(const int32_t _stickerId, const core::sticker_size _size, const bool _scaleIfNeed)
{
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        return QImage();
    }

    bool scaled = false;
    auto image = iter->second->getImage(_size, _scaleIfNeed, scaled);

    if ((scaled || image.isNull()) && !iter->second->isImageRequested(_size))
    {
        const auto setId = getId();
        assert(setId > 0);

        const auto stickerId = iter->second->getId();
        assert(stickerId);

        Ui::GetDispatcher()->getSticker(setId, stickerId, _size);

        iter->second->setImageRequested(_size, true);
    }

    return image;
}

void Set::setStickerFailed(const int32_t _stickerId)
{
    assert(_stickerId > 0);

    stickerSptr updateSticker;

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        updateSticker.reset(new Sticker(_stickerId));
        stickersTree_[_stickerId] = updateSticker;
    }
    else
    {
        updateSticker = iter->second;
    }

    updateSticker->setFailed(true);
}

void Set::setStickerImage(const int32_t _stickerId, const core::sticker_size _size, QImage _image)
{
    assert(_stickerId > 0);
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    stickerSptr updateSticker;

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        updateSticker.reset(new Sticker(_stickerId));
        stickersTree_[_stickerId] = updateSticker;
    }
    else
    {
        updateSticker = iter->second;
    }

    updateSticker->setImage(_size, _image);
}

void Set::setBigIcon(const QImage _image)
{
    bigIcon_ = QPixmap::fromImage(_image);
}

void Set::setId(int32_t _id)
{
    id_= _id;
}

int32_t Set::getId() const
{
    return id_;
}

bool Set::empty() const
{
    return stickers_.empty();
}

void Set::setName(const QString& _name)
{
    name_ = _name;
}

QString Set::getName() const
{
    return name_;
}

void Set::setStoreId(const QString& _storeId)
{
    storeId_ = _storeId;
}

QString Set::getStoreId() const
{
    return storeId_;
}

void Set::setDescription(const QString& _description)
{
    description_ = _description;
}

QString Set::getDescription() const
{
    return description_;
}

void Set::setPurchased(const bool _purchased)
{
    purchased_ = _purchased;
}

bool Set::isPurchased() const
{
    return purchased_;
}

void Set::setUser(const bool _user)
{
    user_ = _user;
}

bool Set::isUser() const
{
    return user_;
}

void Set::loadIcon(char* _data, int32_t _size)
{
    icon_.loadFromData((const uchar*)_data, _size);

    Utils::check_pixel_ratio(icon_);
}

QPixmap Set::getIcon() const
{
    return icon_;
}

QPixmap Set::getBigIcon() const
{
    if (bigIcon_.isNull())
    {
        Ui::GetDispatcher()->getSetIconBig(getId());
    }

    return bigIcon_;
}

void Set::loadBigIcon(char* _data, int32_t _size)
{
    icon_.loadFromData((const uchar*)_data, _size);
}

stickerSptr Set::getSticker(int32_t _stickerId) const
{
    auto iter = stickersTree_.find(_stickerId);
    if (iter != stickersTree_.end())
    {
        return iter->second;
    }

    return nullptr;
}

int32_t Set::getCount() const
{
    return (int32_t) stickers_.size();
}

int32_t Set::getStickerPos(int32_t _stickerId) const
{
    for (int32_t i = 0; i < (int32_t) stickers_.size(); ++i)
    {
        if (stickers_[i] == _stickerId)
            return i;
    }

    return -1;
}

void Set::unserialize(core::coll_helper _coll)
{
    setId(_coll.get_value_as_int("id"));
    setName(_coll.get_value_as_string("name"));
    setStoreId(_coll.get_value_as_string("store_id"));
    setPurchased(_coll.get_value_as_bool("purchased"));
    setUser(_coll.get_value_as_bool("usersticker"));
    setName(_coll.get_value_as_string("name"));
    setDescription(_coll.get_value_as_string("description"));

    if (_coll.is_value_exist("icon"))
    {
        core::istream* iconStream = _coll.get_value_as_stream("icon");
        if (iconStream)
        {
            int32_t iconSize = iconStream->size();
            if (iconSize > 0)
            {
                loadIcon((char*)iconStream->read(iconSize), iconSize);
            }
        }
    }

    core::iarray* sticks = _coll.get_value_as_array("stickers");

    stickers_.reserve(sticks->size());

    for (int32_t i = 0; i < sticks->size(); i++)
    {
        core::coll_helper coll_sticker(sticks->get_at(i)->get_as_collection(), false);

        auto insertedSticker = std::make_shared<Sticker>();
        insertedSticker->unserialize(coll_sticker);

        stickersTree_[insertedSticker->getId()] = insertedSticker;
        stickers_.push_back(insertedSticker->getId());
    }
}

void Set::resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size)
{
    auto iter = stickersTree_.find(_stickerId);
    if (iter != stickersTree_.end())
    {
        iter->second->setImageRequested(_size, false);
    }
}

void Set::clearCache()
{
    for (auto iter = stickersTree_.begin(); iter != stickersTree_.end(); ++iter)
    {
        iter->second->clearCache();
    }
}

const stickersArray& Set::getStickers() const
{
    return stickers_;
}

void unserialize(core::coll_helper _coll)
{
    getCache().unserialize(_coll);
}

void unserialize_store(core::coll_helper _coll)
{
    getCache().unserialize_store(_coll);
}

void setStickerData(core::coll_helper _coll)
{
    getCache().setStickerData(_coll);
}

void setSetBigIcon(core::coll_helper _coll)
{
    getCache().setSetBigIcon(_coll);
}

const setsIdsArray& getStickersSets()
{
    return getCache().getSets();
}

const setsIdsArray& getStoreStickersSets()
{
    return getCache().getStoreSets();
}

void clearCache()
{
    getCache().clearCache();
}

std::shared_ptr<Sticker> getSticker(uint32_t _setId, uint32_t _stickerId)
{
    return getCache().getSticker(_setId, _stickerId);
}

Cache::Cache()
{

}

void Cache::setStickerData(const core::coll_helper& _coll)
{
    const qint32 setId = _coll.get_value_as_int("set_id");

    setSptr stickerSet;

    auto iterSet = setsTree_.find(setId);
    if (iterSet == setsTree_.end())
    {
        stickerSet = std::make_shared<Set>();
        stickerSet->setId(setId);
        setsTree_[setId] = stickerSet;
    }
    else
    {
        stickerSet = iterSet->second;
    }

    const qint32 stickerId = _coll.get_value_as_int("sticker_id");
    const qint32 error = _coll.get_value_as_int("error");

    if (error == 0)
    {
        const auto loadData = [&_coll, stickerSet, stickerId](const char *_id, const core::sticker_size _size)
        {
            if (!_coll->is_value_exist(_id))
            {
                return;
            }

            auto data = _coll.get_value_as_stream(_id);
            const auto dataSize = data->size();

            QImage image;

            if (image.loadFromData(data->read(dataSize), dataSize))
            {
                stickerSet->setStickerImage(stickerId, _size, std::move(image));
            }
        };

        loadData("data/small", core::sticker_size::small);
        loadData("data/medium", core::sticker_size::medium);
        loadData("data/large", core::sticker_size::large);
        loadData("data/xlarge", core::sticker_size::xlarge);
        loadData("data/xxlarge", core::sticker_size::xxlarge);
    }
    else
    {
        stickerSet->setStickerFailed(stickerId);
    }

    stickerSet->resetFlagRequested(stickerId, core::sticker_size::small);
    stickerSet->resetFlagRequested(stickerId, core::sticker_size::medium);
    stickerSet->resetFlagRequested(stickerId, core::sticker_size::large);
    stickerSet->resetFlagRequested(stickerId, core::sticker_size::xlarge);
    stickerSet->resetFlagRequested(stickerId, core::sticker_size::xxlarge);
}

void Cache::setSetBigIcon(const core::coll_helper& _coll)
{
    const qint32 setId = _coll.get_value_as_int("set_id");

    setSptr stickerSet;

    auto iterSet = storeTree_.find(setId);
    if (iterSet == storeTree_.end())
    {
        stickerSet = std::make_shared<Set>();
        stickerSet->setId(setId);
        storeTree_[setId] = stickerSet;
    }
    else
    {
        stickerSet = iterSet->second;
    }

    const qint32 error = _coll.get_value_as_int("error");

    if (error == 0)
    {
        if (!_coll->is_value_exist("icon"))
        {
            return;
        }

        auto data = _coll.get_value_as_stream("icon");

        const auto dataSize = data->size();

        QImage image;

        if (image.loadFromData(data->read(dataSize), dataSize))
        {
            stickerSet->setBigIcon(std::move(image));
        }
    }
}


void Cache::unserialize(const core::coll_helper &_coll)
{
    core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return;

    sets_.clear();

    sets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); i++)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        auto insertedSet = std::make_shared<Set>();

        insertedSet->unserialize(collSet);

        setsTree_[insertedSet->getId()] = insertedSet;

        sets_.push_back(insertedSet->getId());
    }
}

void Cache::unserialize_store(const core::coll_helper &_coll)
{
    core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return;

    storeSets_.clear();

    storeSets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); i++)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        auto insertedSet = std::make_shared<Set>();

        insertedSet->unserialize(collSet);

        storeTree_[insertedSet->getId()] = insertedSet;

        storeSets_.push_back(insertedSet->getId());
    }
}

const setsIdsArray& Cache::getSets() const
{
    return sets_;
}

const setsIdsArray& Cache::getStoreSets() const
{
    return storeSets_;
}

setSptr Cache::getSet(int32_t _setId) const
{
    auto iter = setsTree_.find(_setId);
    if (iter == setsTree_.end())
        return nullptr;

    return iter->second;
}

setSptr Cache::getStoreSet(int32_t _setId) const
{
    auto iter = storeTree_.find(_setId);
    if (iter == storeTree_.end())
        return nullptr;

    return iter->second;
}

void Cache::addSet(std::shared_ptr<Set> _set)
{
    setsTree_[_set->getId()] = _set;
}

setSptr Cache::insertSet(int32_t _setId)
{
    auto iter = setsTree_.find(_setId);
    if (iter != setsTree_.end())
    {
        assert(false);
        return iter->second;
    }

    auto insertedSet = std::make_shared<Set>();
    insertedSet->setId(_setId);
    setsTree_[_setId] = insertedSet;

    return insertedSet;
}

std::shared_ptr<Sticker> Cache::getSticker(uint32_t _setId, uint32_t _stickerId) const
{
    auto iterSet = setsTree_.find(_setId);
    if (iterSet == setsTree_.end())
        return nullptr;

    return iterSet->second->getSticker(_stickerId);
}

void Cache::clearCache()
{
    for (const auto& set : setsTree_)
    {
        set.second->clearCache();
    }
}

const int32_t getSetStickersCount(int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    assert(searchSet);
    if (!searchSet)
        return 0;

    return searchSet->getCount();
}

const int32_t getStickerPosInSet(int32_t _setId, int32_t _stickerId)
{
    auto searchSet = getCache().getSet(_setId);
    assert(searchSet);
    if (!searchSet)
        return -1;

    return searchSet->getStickerPos(_stickerId);
}

const stickersArray& getStickers(int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return getCache().insertSet(_setId)->getStickers();
    }

    return searchSet->getStickers();
}

QImage getStickerImage(int32_t _setId, int32_t _stickerId, const core::sticker_size _size, const bool _scaleIfNeed)
{
    auto searchSet = getCache().getSet(_setId);
    if (!searchSet)
    {
        return QImage();
    }

    return searchSet->getStickerImage(_stickerId, _size, _scaleIfNeed);
}

QPixmap getSetIcon(int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return QPixmap();
    }

    return searchSet->getIcon();
}

QString getSetName(int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return QString();
    }

    return searchSet->getName();
}

bool isConfigHasSticker(int32_t _setId, int32_t _stickerId)
{
    bool found = false;

    const auto& sets = getStickersSets();

    for (auto _id : sets)
    {
        if (_id == _setId)
        {
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    return !!getSticker(_setId, _stickerId);
}

void showStickersPack(const int32_t _set_id)
{
    QTimer::singleShot(0, [_set_id]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), _set_id, QString());

        dlg.show();
    });
}

void showStickersPack(const QString& _store_id)
{
    QTimer::singleShot(0, [_store_id]
    {
        Ui::StickerPackInfo dlg(Utils::InterConnector::instance().getMainWindow(), -1, _store_id);

        dlg.show();
    });
}

std::shared_ptr<Set> parseSet(core::coll_helper _coll_set)
{
    auto newset = std::make_shared<Set>();

    newset->unserialize(_coll_set);

    return newset;
}

void addSet(std::shared_ptr<Set> _set)
{
    getCache().addSet(_set);
}


setSptr getSet(const int32_t _setId)
{
    return getCache().getSet(_setId);
}

setSptr getStoreSet(const int32_t _setId)
{
    return getCache().getStoreSet(_setId);
}

bool isUserSet(const int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    if (!searchSet)
    {
        return true;
    }

    return searchSet->isUser();
}

bool isPurchasedSet(const int32_t _setId)
{
    auto searchSet = getCache().getSet(_setId);
    if (!searchSet)
    {
        return true;
    }

    return searchSet->isPurchased();
}

void resetCache()
{
    if (g_cache)
        g_cache.reset();
}

Cache& getCache()
{
    if (!g_cache)
        g_cache.reset(new Cache());

    return (*g_cache);
}

UI_STICKERS_NS_END
