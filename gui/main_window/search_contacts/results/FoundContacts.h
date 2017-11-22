#pragma once

namespace Logic
{
    class contact_profile;
}

namespace Ui
{
    class FlowLayout;
    class ContactWidget;

    typedef std::list<std::shared_ptr<Logic::contact_profile>> profiles_list;

    class FoundContacts : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:

        void needMore(int _skipCount);
        void addContact(const QString& _contact);
        void msgContact(const QString& _contact);
        void callContact(const QString& _contact);
        void contactInfo(const QString& _contact);

    private Q_SLOTS:

        void onAvatarLoaded(const QString& _aimid);

        void onAddContact(const QString& _contact);
        void onMsgContact(const QString& _contact);
        void onCallContact(const QString& _contact);
        void onContactInfo(const QString& _contact);

    private:

        QScrollArea* area_;
        FlowLayout* contactsLayout_;
        QVBoxLayout* rootLayout_;

        int prevHScrollValue_;
        int prevVScrollValue_;

        std::map<QString, ContactWidget*> items_;
        std::map<QString, QString> countries_;

        void hookScroll();

    public:

        int insertItems(const profiles_list& _profiles);
        void contactAddResult(const QString& _contact, bool _res);
        void clear();
        bool empty();

        explicit FoundContacts(QWidget* _parent);
        virtual ~FoundContacts(void);
    };
}
