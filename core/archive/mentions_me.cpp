#include "stdafx.h"

#include "archive_index.h"
#include "contact_archive.h"
#include "history_message.h"
#include "messages_data.h"
#include "storage.h"

#include "mentions_me.h"


namespace
{

}

using namespace core;
using namespace archive;


mentions_me::mentions_me(const std::wstring& _file_name)
    : storage_(std::make_unique<storage>(_file_name))
{

}

mentions_me::~mentions_me()
{
}

void mentions_me::add(std::shared_ptr<archive::history_message> _message)
{

}
