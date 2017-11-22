#include "stdafx.h"
#include "SearchResults.h"

#include "NoResultsWidget.h"
#include "FoundContacts.h"

namespace Ui
{
    SearchResults::SearchResults(QWidget* _parent)
        : QWidget(_parent),
        pages_(new QStackedWidget(this)),
        rootLayout_(new QVBoxLayout()),
        noResultsWidget_(new NoResultsWidget(this)),
        contactsWidget_(new FoundContacts(this))
    {
        setLayout(rootLayout_);
        rootLayout_->setContentsMargins(0, 0, 0, 0);
        rootLayout_->setSpacing(0);

        rootLayout_->addWidget(pages_);

        pages_->addWidget(noResultsWidget_);
        pages_->addWidget(contactsWidget_);

        pages_->setCurrentWidget(contactsWidget_);

        connect(contactsWidget_, &FoundContacts::addContact, this, &SearchResults::onAddContact, Qt::QueuedConnection);
        connect(contactsWidget_, &FoundContacts::msgContact, this, &SearchResults::onMsgContact, Qt::QueuedConnection);
        connect(contactsWidget_, &FoundContacts::callContact, this, &SearchResults::onCallContact, Qt::QueuedConnection);
        connect(contactsWidget_, &FoundContacts::contactInfo, this, &SearchResults::onContactInfo, Qt::QueuedConnection);
        connect(contactsWidget_, &FoundContacts::needMore, this, &SearchResults::onNeedMore, Qt::QueuedConnection);
    }


    SearchResults::~SearchResults(void)
    {
    }


    int SearchResults::insertItems(const profiles_list& _profiles)
    {
        if (!_profiles.empty())
        {
            pages_->setCurrentWidget(contactsWidget_);
            return contactsWidget_->insertItems(_profiles);
        }
        else
        {
            if (contactsWidget_->empty())
            {
                pages_->setCurrentWidget(noResultsWidget_);
            }
        }
        return 0;
    }

    void SearchResults::contactAddResult(const QString& _contact, bool _res)
    {
        contactsWidget_->contactAddResult(_contact, _res);
    }

    void SearchResults::clear()
    {
        contactsWidget_->clear();
    }

    void SearchResults::onAddContact(const QString& _contact)
    {
        emit addContact(_contact);
    }

    void SearchResults::onMsgContact(const QString& _contact)
    {
        emit msgContact(_contact);
    }

    void SearchResults::onCallContact(const QString& _contact)
    {
        emit callContact(_contact);
    }

    void SearchResults::onNeedMore(int _skipCount)
    {
        emit needMore(_skipCount);
    }

    void SearchResults::onContactInfo(const QString& _contact)
    {
        emit contactInfo(_contact);
    }
}
