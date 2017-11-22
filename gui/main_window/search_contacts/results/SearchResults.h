#pragma once

namespace Logic
{
    class contact_profile;
}

namespace Ui
{
    class NoResultsWidget;
    class FoundContacts;

    typedef std::list<std::shared_ptr<Logic::contact_profile>> profiles_list;

    class SearchResults : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void needMore(int _skipCount);
        void addContact(const QString& _contact);
        void msgContact(const QString& _contact);
        void callContact(const QString& _contact);
        void contactInfo(const QString& _contact);

    private Q_SLOTS:

        void onAddContact(const QString& _contact);
        void onMsgContact(const QString& _contact);
        void onCallContact(const QString& _contact);
        void onContactInfo(const QString& _contact);
        void onNeedMore(int _skip_count);

    private:

        QStackedWidget* pages_;

        QVBoxLayout* rootLayout_;

        NoResultsWidget* noResultsWidget_;
        FoundContacts* contactsWidget_;

    public:

        explicit SearchResults(QWidget* _parent);
        virtual ~SearchResults();

        int insertItems(const profiles_list& _profiles);
        void contactAddResult(const QString& _contact, bool _res);
        void clear();
    };

}


