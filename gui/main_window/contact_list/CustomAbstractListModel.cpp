#include "stdafx.h"
#include "CustomAbstractListModel.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    CustomAbstractListModel::CustomAbstractListModel(QObject *parent/* = 0*/): QAbstractListModel(parent), flags_(0)
    {
        if (platform::is_apple())
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::forceRefreshList, this, &CustomAbstractListModel::forceRefreshList);
        }
    }

    void CustomAbstractListModel::forceRefreshList(QAbstractItemModel *model, bool mouseOver)
    {
        if (model == this)
        {
            mouseOver ? setFlag(CustomAbstractListModelFlags::HasMouseOver) : unsetFlag(CustomAbstractListModelFlags::HasMouseOver);
            refreshList();
        }
    }

    void CustomAbstractListModel::refreshList()
    {
        emit dataChanged(index(0), index(rowCount()));
    }

    void CustomAbstractListModel::setFlag(int flag)
    {
        flags_ |= flag;
    }

    void CustomAbstractListModel::unsetFlag(int flag)
    {
        flags_ &= (~flag);
    }
}
