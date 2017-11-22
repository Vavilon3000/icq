#include "stdafx.h"
#include "ContactItem.h"
#include "contact_profile.h"

namespace
{
    const int maxDaysForActive = 7;
}

namespace Logic
{
	ContactItem::ContactItem(Data::ContactPtr _contact)
		: contact_(_contact)
		, visible_(true)
	{
	}

	bool ContactItem::operator== (const ContactItem& _other) const
	{
		return contact_->AimId_ == _other.Get()->AimId_;
	}

	bool ContactItem::is_visible() const
	{
		return visible_;
	}

	void ContactItem::set_visible(bool _visible)
	{
		visible_ = _visible;
	}

	bool ContactItem::is_not_auth() const
	{
		return contact_->NotAuth_;
	}

	void ContactItem::reset_not_auth()
	{
		contact_->NotAuth_ = false;
	}

	Data::Contact* ContactItem::Get() const
	{
		return contact_.get();
	}

	bool ContactItem::is_group() const
	{
		return contact_->GetType() == Data::GROUP;
	}

	bool ContactItem::is_online() const
	{
		return contact_->HasLastSeen_ && !contact_->LastSeen_.isValid();
	}

	bool ContactItem::is_phone() const
	{
		return contact_->UserType_ == ql1s("sms") || contact_->UserType_ == ql1s("phone");
	}

    bool ContactItem::recently() const
	{
        return recently(QDateTime::currentDateTime());
	}

    bool ContactItem::recently(const QDateTime& _current) const
    {
        return contact_->LastSeen_.isValid() && contact_->LastSeen_.daysTo(_current) <= maxDaysForActive;
    }

    bool ContactItem::is_active(const QDateTime& _current) const
    {
        return (is_online() || recently(_current) || is_chat() || is_live_chat()) && !is_group();
    }

	bool ContactItem::is_chat() const
	{
		return contact_->Is_chat_;
	}

	bool ContactItem::is_muted() const
	{
		return contact_->Muted_;
	}

    bool ContactItem::is_live_chat() const
    {
        return contact_->IsLiveChat_;
	}
    bool ContactItem::is_official() const
    {
        return contact_->IsOfficial_;
    }

	bool ContactItem::is_checked() const
	{
		return contact_->IsChecked_;
	}

	void ContactItem::set_checked(bool _isChecked)
	{
		contact_->IsChecked_ = _isChecked;
	}

    void ContactItem::set_chat_role(const QString& role)
    {
        chat_role_ = role;
    }

    QString ContactItem::get_chat_role() const
    {
        return chat_role_;
    }

	void ContactItem::set_input_text(const QString& _inputText)
	{
		input_text_ = _inputText;
	}

	QString ContactItem::get_input_text() const
	{
		return input_text_;
	}

	void ContactItem::set_contact_profile(profile_ptr _profile)
	{
		profile_ = _profile;
	}

	profile_ptr ContactItem::getContactProfile() const
	{
		return profile_;
	}

	QString ContactItem::get_aimid() const
	{
		return contact_->AimId_;
	}

    int ContactItem::get_outgoing_msg_count() const
    {
        return contact_->OutgoingMsgCount_;
    }

    void ContactItem::set_outgoing_msg_count(const int _count)
    {
        contact_->OutgoingMsgCount_ = _count;
    }

    void ContactItem::set_stamp(const QString & _stamp)
    {
        stamp_ = _stamp;
    }

    QString ContactItem::get_stamp() const
    {
        return stamp_;
    }
}
