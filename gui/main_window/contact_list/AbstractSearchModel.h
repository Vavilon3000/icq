#pragma once

#include "CustomAbstractListModel.h"

namespace Logic
{
    class AbstractSearchModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void results();

    public:
        AbstractSearchModel(QObject* _parent = nullptr)
            : CustomAbstractListModel(_parent)
        {
        }

        virtual void searchPatternChanged(QString _p) = 0;

        virtual void setFocus() = 0;
        virtual void emitChanged(int _first, int _last) = 0;
        virtual void setSelectEnabled(bool) { };
        virtual bool isServiceItem(int i) const = 0;

        virtual QString getCurrentPattern() const = 0;
    };
}