#include "stdafx.h"

#include "contact_profile.h"

namespace Logic
{
    bool address::unserialize(Ui::gui_coll_helper _coll)
    {
        city_ = _coll.get<QString>("city");
        state_ = _coll.get<QString>("state");
        country_ = _coll.get<QString>("country");

        return true;
    }

    bool phone::unserialize(Ui::gui_coll_helper _coll)
    {
        phone_ = QString::fromUtf8(_coll.get_value_as_string("phone"));
        type_ = QString::fromUtf8(_coll.get_value_as_string("type"));

        return true;
    }


    contact_profile::contact_profile()
    {
    }

    contact_profile::~contact_profile()
    {
    }

    bool contact_profile::unserialize(Ui::gui_coll_helper _coll)
    {
        aimid_ = QString::fromUtf8(_coll.get_value_as_string("aimid"));
        first_name_ = QString::fromUtf8(_coll.get_value_as_string("firstname"));
        last_name_ = QString::fromUtf8(_coll.get_value_as_string("lastname"));
        friendly_ = QString::fromUtf8(_coll.get_value_as_string("friendly"));
        displayid_ = QString::fromUtf8(_coll.get_value_as_string("displayid"));
        relationship_ = QString::fromUtf8(_coll.get_value_as_string("relationship"));
        about_ = QString::fromUtf8(_coll.get_value_as_string("about"));
        birthdate_ = _coll.get_value_as_int64("birthdate");
        gender_ = QString::fromUtf8(_coll.get_value_as_string("gender"));

        Ui::gui_coll_helper coll_address(_coll.get_value_as_collection("homeaddress"), false);
        home_address_.unserialize(coll_address);

        if (const auto phones = _coll.get_value_as_array("phones"))
        {
            const auto size = phones->size();
            for (int i = 0; i < size; ++i)
            {
                Ui::gui_coll_helper coll_phone(phones->get_at(i)->get_as_collection(), false);
                phone p;
                p.unserialize(coll_phone);
                phones_.push_back(std::move(p));
            }
        }

        return true;
    }

    bool contact_profile::unserialize2(Ui::gui_coll_helper _coll)
    {
        {
            aimid_ = QString::fromUtf8(_coll.get_value_as_string("aimid"));
            const int pos = aimid_.indexOf("@uin.icq");
            if (pos != -1)
                aimid_ = std::move(aimid_).left(pos);
        }
        first_name_ = QString::fromUtf8(_coll.get_value_as_string("first_name"));
        last_name_ = QString::fromUtf8(_coll.get_value_as_string("last_name"));
        displayid_ = QString::fromUtf8(_coll.get_value_as_string("nick_name"));
        {
            const auto sep = ql1s(first_name_.length() ? " " : "");
            friendly_ = first_name_ % sep % last_name_;
            if (friendly_.isEmpty())
            {
                friendly_ = displayid_;
                if (friendly_.isEmpty())
                    friendly_ = aimid_;
            }
        }
        about_ = QString::fromUtf8(_coll.get_value_as_string("about"));
        {
            if (_coll.is_value_exist("birthdate"))
            {
                Ui::gui_coll_helper coll_birthdate(_coll.get_value_as_collection("birthdate"), false);
                if (coll_birthdate.is_value_exist("year") && coll_birthdate.is_value_exist("month") && coll_birthdate.is_value_exist("day"))
                {
                    QDate d;
                    d.setDate(coll_birthdate.get_value_as_int("year"), coll_birthdate.get_value_as_int("month"), coll_birthdate.get_value_as_int("day"));
                    birthdate_ = QDateTime(d).toMSecsSinceEpoch();
                }
            }
            else
            {
                birthdate_ = 0; // otherwise there'll be a garbage
            }
        }
        gender_ = QString::fromUtf8(_coll.get_value_as_string("gender"));
        home_address_ = address(QString::fromUtf8(_coll.get_value_as_string("city")),
            QString::fromUtf8(_coll.get_value_as_string("state")),
            QString::fromUtf8(_coll.get_value_as_string("country")));

        mutual_friend_count_ = _coll.get_value_as_int("mutual_count");

        return true;
    }

    const QString& contact_profile::get_aimid() const
    {
        return aimid_;
    }

    const QString& contact_profile::get_first_name() const
    {
        return first_name_;
    }

    const QString& contact_profile::get_last_name() const
    {
        return last_name_;
    }

    const QString& contact_profile::get_friendly() const
    {
        return friendly_;
    }

    const QString& contact_profile::get_displayid() const
    {
        return displayid_;
    }

    const QString& contact_profile::get_relationship() const
    {
        return relationship_;
    }

    const QString& contact_profile::get_about() const
    {
        return about_;
    }

    int64_t contact_profile::get_birthdate() const
    {
        return birthdate_;
    }

    const QString& contact_profile::get_gender() const
    {
        return gender_;
    }

    const address& contact_profile::get_home_address() const
    {
        return home_address_;
    }

    const address& contact_profile::get_origin_address() const
    {
        return origin_address_;
    }

    const QString& contact_profile::get_privatekey() const
    {
        return privatekey_;
    }

    const email_list& contact_profile::get_emails() const
    {
        return emails_;
    }

    const phone_list& contact_profile::get_phones() const
    {
        return phones_;
    }

    const interest_list& contact_profile::get_interests() const
    {
        return interests_;
    }

    int32_t contact_profile::get_children() const
    {
        return children_;
    }

    const QString& contact_profile::get_religion() const
    {
        return religion_;
    }

    const QString& contact_profile::get_sex_orientation() const
    {
        return sex_orientation_;
    }

    bool contact_profile::get_smoking() const
    {
        return smoking_;
    }

    int32_t contact_profile::get_mutual_friends_count() const
    {
        return mutual_friend_count_;
    }

    QString contact_profile::get_contact_name() const
    {
        const auto& friendly = get_friendly();
        if (!QStringRef(&friendly).trimmed().isEmpty())
            return friendly;

        const auto& displayid = get_displayid();
        if (!QStringRef(&displayid).trimmed().isEmpty())
            return displayid;

        const auto& first_name = get_first_name();
        if (!QStringRef(&first_name).trimmed().isEmpty())
            return first_name % ql1c(' ') % get_last_name();

        return get_aimid();
    }
}

