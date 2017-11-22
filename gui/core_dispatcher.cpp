#include "stdafx.h"
#include "core_dispatcher.h"

#include "app_config.h"
#include "gui_settings.h"
#include "my_info.h"
#include "theme_settings.h"
#include "main_window/contact_list/SearchMembersModel.h"
#include "main_window/contact_list/SearchModelDLG.h"
#include "main_window/contact_list/RecentsModel.h"
#include "types/typing.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/LoadPixmapFromDataTask.h"
#include "utils/uid.h"
#include "utils/utils.h"
#include "cache/stickers/stickers.h"
#include "cache/themes/themes.h"
#include "utils/log/log.h"
#include "utils/macos/mac_migration.h"
#include "../common.shared/common_defs.h"
#include "../corelib/corelib.h"
#include "../corelib/core_face.h"
#include "../common.shared/loader_errors.h"

int build::is_build_icq;

#ifdef _WIN32
    #include "../common.shared/win32/crash_handler.h"
#endif

using namespace Ui;

int Ui::gui_connector::addref()
{
    return ++refCount_;
}

int Ui::gui_connector::release()
{
    if (0 == (--refCount_))
    {
        delete this;
        return 0;
    }

    return refCount_;
}

void Ui::gui_connector::link(iconnector*, const common::core_gui_settings&)
{

}

void Ui::gui_connector::unlink()
{

}

void Ui::gui_connector::receive(const char* _message, int64_t _seq, core::icollection* _messageData)
{
    if (_messageData)
        _messageData->addref();

    emit received(QString::fromUtf8(_message), _seq, _messageData);
}

core_dispatcher::core_dispatcher()
    : coreConnector_(nullptr)
    , coreFace_(nullptr)
    , voipController_(*this)
    , guiConnector_(nullptr)
    , lastTimeCallbacksCleanedUp_(QDateTime::currentMSecsSinceEpoch())
    , isStatsEnabled_(true)
    , isImCreated_(false)
    , userStateGoneAway_(false)
{
    init();
}


core_dispatcher::~core_dispatcher()
{
    uninit();
}

voip_proxy::VoipController& core_dispatcher::getVoipController()
{
    return voipController_;
}

qint64 core_dispatcher::getFileSharingPreviewSize(const QString& _url)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    return post_message_to_core(qsl("files/download/preview_size"), helper.get());
}

qint64 core_dispatcher::downloadFileSharingMetainfo(const QString& _url)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    return post_message_to_core(qsl("files/download/metainfo"), helper.get());
}

qint64 core_dispatcher::downloadSharedFile(const QString& _contactAimid, const QString& _url, bool _forceRequestMetainfo, const QString& _fileName, bool _raise_priority)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("contact", _contactAimid);
    helper.set<QString>("url", _url);
    helper.set<bool>("force_request_metainfo", _forceRequestMetainfo);
    helper.set<QString>("filename", _fileName);
    helper.set<QString>("download_dir", Utils::UserDownloadsPath());
    helper.set<bool>("raise", _raise_priority);
    return post_message_to_core(qsl("files/download"), helper.get());
}

qint64 core_dispatcher::abortSharedFileDownloading(const QString& _url)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    return post_message_to_core(qsl("files/download/abort"), helper.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QString& _localPath)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_string("contact", _contact.toUtf8().data());
    collection.set_value_as_string("file", _localPath.toUtf8().data());

    return post_message_to_core(qsl("files/upload"), collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, const QString& ext)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_string("contact", _contact.toUtf8().data());
    core::istream* stream = collection->create_stream();
    stream->write((uint8_t*)(_array.data()), _array.size());
    collection.set_value_as_stream("file_stream", stream);
    collection.set_value_as_string("ext", ext.toUtf8().data());

    return post_message_to_core(qsl("files/upload"), collection.get());
}

qint64 core_dispatcher::abortSharedFileUploading(const QString& _contact, const QString& _localPath, const QString& _uploadingProcessId)
{
    assert(!_contact.isEmpty());
    assert(!_localPath.isEmpty());
    assert(!_uploadingProcessId.isEmpty());

    core::coll_helper helper(create_collection(), true);
    helper.set_value_as_string("contact", _contact.toStdString());
    helper.set_value_as_string("local_path", _localPath.toStdString());
    helper.set_value_as_string("process_seq", _uploadingProcessId.toStdString());

    return post_message_to_core(qsl("files/upload/abort"), helper.get());
}

qint64 core_dispatcher::getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size)
{
    assert(_setId > 0);
    assert(_stickerId > 0);
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_int("sticker_id", _stickerId);
    collection.set_value_as_enum("size", _size);

    return post_message_to_core(qsl("stickers/sticker/get"), collection.get());
}

qint64 core_dispatcher::getSetIconBig(const qint32 _setId)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);

    return post_message_to_core(qsl("stickers/big_set_icon/get"), collection.get());
}

qint64 core_dispatcher::getStickersPackInfo(const qint32 _setId, const std::string& _storeId)
{
    core::coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_string("store_id", _storeId);

    return post_message_to_core(qsl("stickers/pack/info"), collection.get());
}

qint64 core_dispatcher::addStickersPack(const qint32 _setId, const QString& _storeId)
{
    core::coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_string("store_id", _storeId.toStdString());

    return post_message_to_core(qsl("stickers/pack/add"), collection.get());
}

qint64 core_dispatcher::removeStickersPack(const qint32 _setId, const QString& _storeId)
{
    core::coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_string("store_id", _storeId.toStdString());

    return post_message_to_core(qsl("stickers/pack/remove"), collection.get());
}

qint64 core_dispatcher::getStickersStore()
{
    return post_message_to_core(qsl("stickers/store/get"), nullptr);
}


qint64 core_dispatcher::getTheme(const qint32 _themeId)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("theme_id", _themeId);

    return post_message_to_core(qsl("themes/theme/get"), collection.get());
}

int64_t core_dispatcher::downloadImage(
    const QUrl& _uri,
    const QString& _contactAimid,
    const QString& _destination,
    const bool _isPreview,
    const int32_t _previewWidth,
    const int32_t _previewHeight,
    const bool _externalResource,
    const bool _raisePriority)
{
    assert(!_contactAimid.isEmpty());
    assert(_uri.isValid());
    assert(!_uri.isLocalFile());
    assert(!_uri.isRelative());
    assert(_previewWidth >= 0);
    assert(_previewHeight >= 0);

    core::coll_helper collection(create_collection(), true);
    collection.set<QUrl>("uri", _uri);
    collection.set<QString>("destination", _destination);
    collection.set<bool>("is_preview", _isPreview);
    collection.set<QString>("contact", _contactAimid);
    collection.set<bool>("ext_resource", _externalResource);
    collection.set<bool>("raise", _raisePriority);

    if (_isPreview)
    {
        collection.set<int32_t>("preview_height", _previewHeight);
        collection.set<int32_t>("preview_width", _previewWidth);
    }

    const auto seq = post_message_to_core(qsl("image/download"), collection.get());

    __INFO(
        "snippets",
        "GUI(1): requested image\n"
            __LOGP(seq, seq)
            __LOGP(uri, _uri)
            __LOGP(dst, _destination)
            __LOGP(preview, _isPreview)
            __LOGP(preview_width, _previewWidth)
            __LOGP(preview_height, _previewHeight));

    return seq;
}

void core_dispatcher::cancelImageDownloading(const QString& _url)
{
    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("url", _url);

    post_message_to_core(qsl("image/download/cancel"), collection.get());
}

int64_t core_dispatcher::downloadLinkMetainfo(
    const QString& _contact,
    const QString& _uri,
    const int32_t _previewWidth,
    const int32_t _previewHeight)
{
    assert(!_contact.isEmpty());
    assert(!_uri.isEmpty());
    assert(_previewWidth >= 0);
    assert(_previewHeight >= 0);

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("uri", _uri);
    collection.set<QString>("contact", _contact);
    collection.set<int32_t>("preview_height", _previewHeight);
    collection.set<int32_t>("preview_width", _previewWidth);

    return post_message_to_core(qsl("link_metainfo/download"), collection.get());
}

int64_t core_dispatcher::pttToText(const QString &_pttLink, const QString &_locale)
{
    assert(!_pttLink.isEmpty());
    assert(!_locale.isEmpty());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("url", _pttLink);
    collection.set<QString>("locale", _locale);

    return post_message_to_core(qsl("files/speech_to_text"), collection.get());
}

int64_t core_dispatcher::setUrlPlayed(const QString& _url, const bool _isPlayed)
{
    assert(!_url.isEmpty());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("url", _url);
    collection.set<bool>("played", _isPlayed);

    return post_message_to_core(qsl("files/set_url_played"), collection.get());
}

qint64 core_dispatcher::deleteMessages(const std::vector<int64_t> &_messageIds, const QString &_contactAimId, const bool _forAll)
{
    assert(!_messageIds.empty());
    assert(!_contactAimId.isEmpty());

    core::coll_helper params(create_collection(), true);

    core::ifptr<core::iarray> messages_ids(params->create_array());
    messages_ids->reserve(_messageIds.size());
    for (const auto id : _messageIds)
    {
        assert(id > 0);

        core::ifptr<core::ivalue> val(params->create_value());
        val->set_as_int64(id);
        messages_ids->push_back(val.get());
    }

    params.set_value_as_array("messages_ids", messages_ids.get());

    params.set<QString>("contact_aimid", _contactAimId);
    params.set<bool>("for_all", _forAll);

    return post_message_to_core(qsl("archive/messages/delete"), params.get());
}

qint64 core_dispatcher::deleteMessagesFrom(const QString& _contactAimId, const int64_t _fromId)
{
    assert(!_contactAimId.isEmpty());
    assert(_fromId >= 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact_aimid", _contactAimId);
    collection.set<int64_t>("from_id", _fromId);

    return post_message_to_core(qsl("archive/messages/delete_from"), collection.get());
}

qint64 core_dispatcher::raiseDownloadPriority(const QString &_contactAimid, int64_t _procId)
{
    assert(_procId > 0);
    assert(!_contactAimid.isEmpty());

    core::coll_helper collection(create_collection(), true);

    collection.set<int64_t>("proc_id", _procId);

    __TRACE(
        "prefetch",
        "requesting to raise download priority\n"
        "    contact_aimid=<" << _contactAimid << ">\n"
            "request_id=<" << _procId << ">");

    return post_message_to_core(qsl("download/raise_priority"), collection.get());
}

qint64 core_dispatcher::contactSwitched(const QString &_contactAimid)
{
    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("contact", _contactAimid);
    return post_message_to_core(qsl("contact/switched"), collection.get());
}

void core_dispatcher::sendMessageToContact(const QString& _contact, const QString& _text)
{
    assert(!_contact.isEmpty());
    assert(!_text.isEmpty());
    assert((unsigned)_text.length() <= Utils::getInputMaximumChars());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("contact", _contact);
    const auto utf8Text = _text.toUtf8();
    collection.set_value_as_string("message", utf8Text.data(), utf8Text.size());

    Ui::GetDispatcher()->post_message_to_core(qsl("send_message"), collection.get());
}

bool core_dispatcher::init()
{
#ifndef ICQ_CORELIB_STATIC_LINKING
    const QString library_path = QApplication::applicationDirPath() % ql1c('/') % ql1s(CORELIBRARY);
    QLibrary libcore(library_path);
    if (!libcore.load())
    {
        assert(false);
        return false;
    }

    typedef bool (*get_core_instance_function)(core::icore_interface**);
    get_core_instance_function get_core_instance = (get_core_instance_function) libcore.resolve("get_core_instance");

    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace))
    {
        assert(false);
        return false;
    }

    coreFace_ = coreFace;
#else
    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace) || !coreFace)
    {
        assert(false);
        return false;
    }
    coreFace_ = coreFace;
#endif //ICQ_CORELIB_STATIC_LINKING
    coreConnector_ = coreFace_->get_core_connector();
    if (!coreConnector_)
        return false;

    gui_connector* connector = new gui_connector();

    QObject::connect(connector, &gui_connector::received, this, &core_dispatcher::received, Qt::QueuedConnection);

    guiConnector_ = connector;

    common::core_gui_settings settings(
        build::is_icq(),
        Utils::scale_value(56));

    coreConnector_->link(guiConnector_, settings);

    initMessageMap();

    return true;
}

void core_dispatcher::initMessageMap()
{
    REGISTER_IM_MESSAGE("need_login", onNeedLogin);
    REGISTER_IM_MESSAGE("im/created", onImCreated);
    REGISTER_IM_MESSAGE("login/complete", onLoginComplete);
    REGISTER_IM_MESSAGE("contactlist", onContactList);
    REGISTER_IM_MESSAGE("contactlist/diff", onContactList);
    REGISTER_IM_MESSAGE("login_get_sms_code_result", onLoginGetSmsCodeResult);
    REGISTER_IM_MESSAGE("login_result", onLoginResult);
    REGISTER_IM_MESSAGE("avatars/get/result", onAvatarsGetResult);
    REGISTER_IM_MESSAGE("avatars/presence/updated", onAvatarsPresenceUpdated);
    REGISTER_IM_MESSAGE("contact/presence", onContactPresence);
    REGISTER_IM_MESSAGE("contact/outgoing_count", onContactOutgoingMsgCount);
    REGISTER_IM_MESSAGE("gui_settings", onGuiSettings);
    REGISTER_IM_MESSAGE("core/logins", onCoreLogins);
    REGISTER_IM_MESSAGE("theme_settings", onThemeSettings);
    REGISTER_IM_MESSAGE("archive/images/get/result", onArchiveImagesGetResult);
    REGISTER_IM_MESSAGE("archive/messages/get/result", onArchiveMessagesGetResult);
    REGISTER_IM_MESSAGE("messages/received/dlg_state", onMessagesReceivedDlgState);
    REGISTER_IM_MESSAGE("messages/received/server", onMessagesReceivedServer);
    REGISTER_IM_MESSAGE("archive/messages/pending", onArchiveMessagesPending);
    REGISTER_IM_MESSAGE("messages/received/init", onMessagesReceivedInit);
    REGISTER_IM_MESSAGE("messages/received/message_status", onMessagesReceivedMessageStatus);
    REGISTER_IM_MESSAGE("messages/del_up_to", onMessagesDelUpTo);
    REGISTER_IM_MESSAGE("dlg_states", onDlgStates);

    REGISTER_IM_MESSAGE("history_search_result_msg", onHistorySearchResultMsg);
    REGISTER_IM_MESSAGE("history_search_result_contacts", onHistorySearchResultContacts);
    REGISTER_IM_MESSAGE("empty_search_results", onEmptySearchResults);
    REGISTER_IM_MESSAGE("search_need_update", onSearchNeedUpdate);
	REGISTER_IM_MESSAGE("history_update", onHistoryUpdate);

    REGISTER_IM_MESSAGE("voip_signal", onVoipSignal);
    REGISTER_IM_MESSAGE("active_dialogs_are_empty", onActiveDialogsAreEmpty);
    REGISTER_IM_MESSAGE("active_dialogs_hide", onActiveDialogsHide);
    REGISTER_IM_MESSAGE("stickers/meta/get/result", onStickersMetaGetResult);
    REGISTER_IM_MESSAGE("themes/meta/get/result", onThemesMetaGetResult);
    REGISTER_IM_MESSAGE("themes/meta/get/error", onThemesMetaGetError);
    REGISTER_IM_MESSAGE("stickers/sticker/get/result", onStickersStickerGetResult);
    REGISTER_IM_MESSAGE("stickers/big_set_icon/get/result", onStickersGetSetBigIconResult);
    REGISTER_IM_MESSAGE("stickers/pack/info/result", onStickersPackInfo);
    REGISTER_IM_MESSAGE("stickers/store/get/result", onStickersStore);
    REGISTER_IM_MESSAGE("themes/theme/get/result", onThemesThemeGetResult);
    REGISTER_IM_MESSAGE("chats/info/get/result", onChatsInfoGetResult);
    REGISTER_IM_MESSAGE("chats/blocked/result", onChatsBlockedResult);
    REGISTER_IM_MESSAGE("chats/pending/result", onChatsPendingResult);
    REGISTER_IM_MESSAGE("chats/info/get/failed", onChatsInfoGetFailed);

    REGISTER_IM_MESSAGE("files/error", fileSharingErrorResult);
    REGISTER_IM_MESSAGE("files/download/progress", fileSharingDownloadProgress);
    REGISTER_IM_MESSAGE("files/get_preview_size/result", fileSharingGetPreviewSizeResult);
    REGISTER_IM_MESSAGE("files/metainfo/result", fileSharingMetainfoResult);
    REGISTER_IM_MESSAGE("files/check_exists/result", fileSharingCheckExistsResult);
    REGISTER_IM_MESSAGE("files/download/result", fileSharingDownloadResult);
    REGISTER_IM_MESSAGE("image/download/result", imageDownloadResult);
    REGISTER_IM_MESSAGE("image/download/progress", imageDownloadProgress);
    REGISTER_IM_MESSAGE("image/download/result/meta", imageDownloadResultMeta);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/meta", linkMetainfoDownloadResultMeta);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/image", linkMetainfoDownloadResultImage);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/favicon", linkMetainfoDownloadResultFavicon);
    REGISTER_IM_MESSAGE("files/upload/progress", fileUploadingProgress);
    REGISTER_IM_MESSAGE("files/upload/result", fileUploadingResult);

    REGISTER_IM_MESSAGE("files/speech_to_text/result", onFilesSpeechToTextResult);
    REGISTER_IM_MESSAGE("contacts/remove/result", onContactsRemoveResult);
    REGISTER_IM_MESSAGE("app_config", onAppConfig);
    REGISTER_IM_MESSAGE("my_info", onMyInfo);
    REGISTER_IM_MESSAGE("signed_url", onSignedUrl);
    REGISTER_IM_MESSAGE("feedback/sent", onFeedbackSent);
    REGISTER_IM_MESSAGE("messages/received/senders", onMessagesReceivedSenders);
    REGISTER_IM_MESSAGE("typing", onTyping);
    REGISTER_IM_MESSAGE("typing/stop", onTypingStop);
    REGISTER_IM_MESSAGE("contacts/get_ignore/result", onContactsGetIgnoreResult);

    REGISTER_IM_MESSAGE("login_result_attach_uin", onLoginResultAttachUin);
    REGISTER_IM_MESSAGE("login_result_attach_phone", onLoginResultAttachPhone);
    REGISTER_IM_MESSAGE("recv_flags", onRecvFlags);
    REGISTER_IM_MESSAGE("update_profile/result", onUpdateProfileResult);
    REGISTER_IM_MESSAGE("chats/home/get/result", onChatsHomeGetResult);
    REGISTER_IM_MESSAGE("chats/home/get/failed", onChatsHomeGetFailed);
    REGISTER_IM_MESSAGE("user_proxy/result", onUserProxyResult);
    REGISTER_IM_MESSAGE("open_created_chat", onOpenCreatedChat);
    REGISTER_IM_MESSAGE("login_new_user", onLoginNewUser);
    REGISTER_IM_MESSAGE("set_avatar/result", onSetAvatarResult);
    REGISTER_IM_MESSAGE("chats/role/set/result", onChatsRoleSetResult);
    REGISTER_IM_MESSAGE("chats/block/result", onChatsBlockResult);
    REGISTER_IM_MESSAGE("chats/pending/resolve/result", onChatsPendingResolveResult);
    REGISTER_IM_MESSAGE("phoneinfo/result", onPhoneinfoResult);
    REGISTER_IM_MESSAGE("contacts/ignore/remove", onContactRemovedFromIgnore);

    REGISTER_IM_MESSAGE("masks/get_id_list/result", onMasksGetIdListResult);
    REGISTER_IM_MESSAGE("masks/preview/result", onMasksPreviewResult);
    REGISTER_IM_MESSAGE("masks/model/result", onMasksModelResult);
    REGISTER_IM_MESSAGE("masks/get/result", onMasksGetResult);
    REGISTER_IM_MESSAGE("masks/progress", onMasksProgress);
    REGISTER_IM_MESSAGE("masks/update/retry", onMasksRetryUpdate);

    REGISTER_IM_MESSAGE("mailboxes/status", onMailStatus);
    REGISTER_IM_MESSAGE("mailboxes/new", onMailNew);

    REGISTER_IM_MESSAGE("mrim/get_key/result", getMrimKeyResult);
    REGISTER_IM_MESSAGE("mentions/me/received", onMentionsMeReceived);
}

void core_dispatcher::uninit()
{
    if (guiConnector_)
    {
        coreConnector_->unlink();
        guiConnector_->release();

        guiConnector_ = nullptr;
    }

    if (coreConnector_)
    {
        coreConnector_->release();
        coreConnector_ = nullptr;
    }

    if (coreFace_)
    {
        coreFace_->release();
        coreFace_ = nullptr;
    }

}

void core_dispatcher::executeCallback(const int64_t _seq, core::icollection* _params)
{
    assert(_seq > 0);

    const auto callbackInfoIter = callbacks_.find(_seq);
    if (callbackInfoIter == callbacks_.end())
    {
        return;
    }

    const auto &callback = callbackInfoIter->second.callback_;

    assert(callback);
    callback(_params);

    callbacks_.erase(callbackInfoIter);
}

void core_dispatcher::cleanupCallbacks()
{
    const auto now = QDateTime::currentMSecsSinceEpoch();

    const auto secsPassedFromLastCleanup = now - lastTimeCallbacksCleanedUp_;
    constexpr qint64 cleanTimeoutInMs = 30 * 1000;
    if (secsPassedFromLastCleanup < cleanTimeoutInMs)
    {
        return;
    }

    for (auto pairIter = callbacks_.begin(); pairIter != callbacks_.end();)
    {
        const auto callbackInfo = pairIter->second;

        const auto callbackRegisteredTimestamp = callbackInfo.date_;

        const auto callbackAgeInMs = now - callbackRegisteredTimestamp;

        constexpr qint64 callbackAgeInMsMax = 60 * 1000;
        if (callbackAgeInMs > callbackAgeInMsMax)
        {
            pairIter = callbacks_.erase(pairIter);
        }
        else
        {
            ++pairIter;
        }
    }

    lastTimeCallbacksCleanedUp_ = now;
}

void core_dispatcher::fileSharingErrorResult(const int64_t _seq, core::coll_helper _params)
{
    const auto rawUri = _params.get<QString>("file_url");
    const auto errorCode = _params.get<int32_t>("error", 0);

    emit fileSharingError(_seq, rawUri, errorCode);
}

void core_dispatcher::fileSharingDownloadProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto rawUri = _params.get<QString>("file_url");
    const auto size = _params.get<int64_t>("file_size");
    const auto bytesTransfer = _params.get<int64_t>("bytes_transfer", 0);

    emit fileSharingFileDownloading(_seq, rawUri, bytesTransfer, size);
}

void core_dispatcher::fileSharingGetPreviewSizeResult(const int64_t _seq, core::coll_helper _params)
{
    const auto miniPreviewUri = _params.get<QString>("mini_preview_uri");
    const auto fullPreviewUri = _params.get<QString>("full_preview_uri");

    emit fileSharingPreviewMetainfoDownloaded(_seq, miniPreviewUri, fullPreviewUri);
}

void core_dispatcher::fileSharingMetainfoResult(const int64_t _seq, core::coll_helper _params)
{
    const auto filenameShort = _params.get<QString>("file_name_short");
    const auto downloadUri = _params.get<QString>("file_dlink");
    const auto size = _params.get<int64_t>("file_size");

    emit fileSharingFileMetainfoDownloaded(_seq, filenameShort, downloadUri, size);
}

void core_dispatcher::fileSharingCheckExistsResult(const int64_t _seq, core::coll_helper _params)
{
    const auto filename = _params.get<QString>("file_name");
    const auto exists = _params.get<bool>("exists");

    emit fileSharingLocalCopyCheckCompleted(_seq, exists, filename);
}

void core_dispatcher::fileSharingDownloadResult(const int64_t _seq, core::coll_helper _params)
{
    const auto rawUri = _params.get<QString>("file_url");
    const auto filename = _params.get<QString>("file_name");

    emit fileSharingFileDownloaded(_seq, rawUri, filename);
}

void core_dispatcher::imageDownloadProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto bytesTotal = _params.get<int64_t>("bytes_total");
    const auto bytesTransferred = _params.get<int64_t>("bytes_transferred");
    const auto pctTransferred = _params.get<int32_t>("pct_transferred");

    assert(bytesTotal > 0);
    assert(bytesTransferred >= 0);
    assert(pctTransferred >= 0);
    assert(pctTransferred <= 100);

    emit imageDownloadingProgress(_seq, bytesTotal, bytesTransferred, pctTransferred);
}

void core_dispatcher::imageDownloadResult(const int64_t _seq, core::coll_helper _params)
{
    const auto isProgress = !_params.is_value_exist("error");
    if (isProgress)
    {
        return;
    }

    const auto rawUri = _params.get<QString>("url");
    const auto data = _params.get_value_as_stream("data");
    const auto local = _params.get<QString>("local");
    const auto isVideo = _params.get<bool>("is_video");

    __INFO(
        "snippets",
        "completed image downloading\n"
            __LOGP(_seq, _seq)
            __LOGP(uri, rawUri)
            __LOGP(local_path, local)
            __LOGP(success, !data->empty()));

    if (data->empty())
    {
        emit imageDownloadError(_seq, rawUri);
        return;
    }

    assert(!local.isEmpty());
    assert(!rawUri.isEmpty());

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, rawUri, local, isVideo]
        (const QPixmap& pixmap)
        {
            if (!isVideo && pixmap.isNull())
            {
                emit imageDownloadError(_seq, rawUri);
                return;
            }

            emit imageDownloaded(_seq, rawUri, pixmap, local);
        },
        Qt::QueuedConnection);
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto previewWidth = _params.get<int32_t>("preview_width");
    const auto previewHeight = _params.get<int32_t>("preview_height");
    const auto downloadUri = _params.get<QString>("download_uri");
    const auto fileSize = _params.get<int64_t>("file_size");

    assert(previewWidth >= 0);
    assert(previewHeight >= 0);

    const QSize previewSize(previewWidth, previewHeight);

    Data::LinkMetadata linkMeta(QString(), QString(), QString(), QString(), previewSize, downloadUri, fileSize);

    emit imageMetaDownloaded(_seq, linkMeta);
}

void core_dispatcher::fileUploadingProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto bytesUploaded = _params.get<int64_t>("bytes_transfer");
    const auto uploadingId = _params.get<QString>("uploading_id");

    emit fileSharingUploadingProgress(uploadingId, bytesUploaded);
}

void core_dispatcher::fileUploadingResult(const int64_t _seq, core::coll_helper _params)
{
    const auto uploadingId = _params.get<QString>("uploading_id");
    const auto localPath = _params.get<QString>("local_path");
    const auto link = _params.get<QString>("file_url");
    const auto contentType = _params.get<int32_t>("content_type");
    const auto error = _params.get<int32_t>("error");

    const auto success = (error == 0);
    const auto isFileTooBig = (error == (int32_t)loader_errors::too_large_file);
    emit fileSharingUploadingResult(uploadingId, success, localPath, link, contentType, isFileTooBig);
}

void core_dispatcher::linkMetainfoDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");
    const auto title = _params.get<QString>("title", "");
    const auto annotation = _params.get<QString>("annotation", "");
    const auto siteName = _params.get<QString>("site_name", "");
    const auto contentType = _params.get<QString>("content_type", "");
    const auto previewWidth = _params.get<int32_t>("preview_width", 0);
    const auto previewHeight = _params.get<int32_t>("preview_height", 0);
    const auto downloadUri = _params.get<QString>("download_uri", "");
    const auto fileSize = _params.get<int64_t>("file_size", -1);

    assert(previewWidth >= 0);
    assert(previewHeight >= 0);
    const QSize previewSize(previewWidth, previewHeight);

    const Data::LinkMetadata linkMetadata(title, annotation, siteName, contentType, previewSize, downloadUri, fileSize);

    emit linkMetainfoMetaDownloaded(_seq, success, linkMetadata);
}

void core_dispatcher::linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        emit linkMetainfoImageDownloaded(_seq, false, QPixmap());
        return;
    }

    const auto data = _params.get_value_as_stream("data");

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, success]
        (const QPixmap& pixmap)
        {
            emit linkMetainfoImageDownloaded(_seq, success, pixmap);
        },
        Qt::QueuedConnection);
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        emit linkMetainfoFaviconDownloaded(_seq, false, QPixmap());
        return;
    }

    const auto data = _params.get_value_as_stream("data");

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, success]
        (const QPixmap& pixmap)
        {
            emit linkMetainfoFaviconDownloaded(_seq, success, pixmap);
        },
        Qt::QueuedConnection);
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::onFilesSpeechToTextResult(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");
    int comeback = _params.get_value_as_int("comeback");
    const QString text = QString::fromUtf8(_params.get_value_as_string("text"));

    emit speechToText(_seq, error, text, comeback);
}

void core_dispatcher::onContactsRemoveResult(const int64_t _seq, core::coll_helper _params)
{
    const QString contact = QString::fromUtf8(_params.get_value_as_string("contact"));

    emit contactRemoved(contact);
}

template<typename T>
static T toContainerOfString(const core::iarray* array)
{
    T container;
    if (array)
    {
        const auto size = array->size();
        container.reserve(size);
        for (int i = 0; i < size; ++i)
            container.push_back(QString::fromUtf8(array->get_at(i)->get_as_string()));
    }
    return container;
}

void core_dispatcher::onMasksGetIdListResult(const int64_t _seq, core::coll_helper _params)
{
    const auto array = _params.get_value_as_array("mask_id_list");
    const auto maskList = toContainerOfString<QList<QString>>(array);

    emit maskListLoaded(maskList);
}

void core_dispatcher::onMasksPreviewResult(const int64_t _seq, core::coll_helper _params)
{
    emit maskPreviewLoaded(_seq, QString::fromUtf8(_params.get_value_as_string("local_path")));
}

void core_dispatcher::onMasksModelResult(const int64_t _seq, core::coll_helper _params)
{
    emit maskModelLoaded();
}

void core_dispatcher::onMasksGetResult(const int64_t _seq, core::coll_helper _params)
{
    emit maskLoaded(_seq, QString::fromUtf8(_params.get_value_as_string("local_path")));
}

void core_dispatcher::onMasksProgress(const int64_t _seq, core::coll_helper _params)
{
    emit maskLoadingProgress(_seq, _params.get_value_as_uint("percent"));
}

void core_dispatcher::onMasksRetryUpdate(const int64_t _seq, core::coll_helper _params)
{
    emit maskRetryUpdate();
}

void core_dispatcher::onMailStatus(const int64_t _seq, core::coll_helper _params)
{
    auto array = _params.get_value_as_array("mailboxes");
    if (!array->empty()) //only one email
    {
        bool init = _params->is_value_exist("init") ? _params.get_value_as_bool("init") : false;
        core::coll_helper helper(array->get_at(0)->get_as_collection(), false);
        emit mailStatus(QString::fromUtf8(helper.get_value_as_string("email")), helper.get_value_as_uint("unreads"), init);
    }
}

void core_dispatcher::onMailNew(const int64_t _seq, core::coll_helper _params)
{
    emit newMail(
        QString::fromUtf8(_params.get_value_as_string("email")),
        QString::fromUtf8(_params.get_value_as_string("from")),
        QString::fromUtf8(_params.get_value_as_string("subj")),
        QString::fromUtf8(_params.get_value_as_string("uidl")));
}

void core_dispatcher::getMrimKeyResult(const int64_t _seq, core::coll_helper _params)
{
    emit mrimKey(_seq, QString::fromUtf8(_params.get_value_as_string("key")));
}

void core_dispatcher::onAppConfig(const int64_t _seq, core::coll_helper _params)
{
    Ui::AppConfigUptr config(new AppConfig(_params));

    Ui::SetAppConfig(config);

    emit appConfig();
}

void core_dispatcher::onMyInfo(const int64_t _seq, core::coll_helper _params)
{
    Ui::MyInfo()->unserialize(&_params);
    Ui::MyInfo()->CheckForUpdate();

    emit myInfo();
}

void core_dispatcher::onSignedUrl(const int64_t _seq, core::coll_helper _params)
{
    emit signedUrl(QString::fromUtf8(_params.get_value_as_string("url")));
}

void core_dispatcher::onFeedbackSent(const int64_t _seq, core::coll_helper _params)
{
    emit feedbackSent(_params.get_value_as_bool("succeeded"));
}

void core_dispatcher::onMessagesReceivedSenders(const int64_t _seq, core::coll_helper _params)
{
    const QString aimId = QString::fromUtf8(_params.get_value_as_string("aimid"));
    QVector<QString> sendersAimIds;
    if (_params.is_value_exist("senders"))
    {
        const auto array = _params.get_value_as_array("senders");
        sendersAimIds = toContainerOfString<QVector<QString>>(array);
    }

    emit messagesReceived(aimId, sendersAimIds);
}

void core_dispatcher::onTyping(const int64_t _seq, core::coll_helper _params)
{
    onEventTyping(_params, true);
}

void core_dispatcher::onTypingStop(const int64_t _seq, core::coll_helper _params)
{
    onEventTyping(_params, false);
}

void core_dispatcher::onContactsGetIgnoreResult(const int64_t _seq, core::coll_helper _params)
{
    const auto array = _params.get_value_as_array("aimids");
    const auto ignoredAimIds = toContainerOfString<QVector<QString>>(array);
    Logic::updateIgnoredModel(ignoredAimIds);
    emit recvPermitDeny(ignoredAimIds.isEmpty());
}

core::icollection* core_dispatcher::create_collection() const
{
    core::ifptr<core::icore_factory> factory(coreFace_->get_factory());
    return factory->create_collection();
}

qint64 core_dispatcher::post_message_to_core(const QString& _message, core::icollection* _collection, const QObject* _object, const message_processed_callback _callback)
{
    const auto seq = Utils::get_uid();

    coreConnector_->receive(_message.toUtf8(), seq, _collection);

    if (_callback)
    {
        const auto result = callbacks_.emplace(seq, callback_info(_callback, QDateTime::currentMSecsSinceEpoch(), _object));
        assert(result.second);
    }

    if (_object)
    {
        QObject::connect(_object, &QObject::destroyed, this, [this](QObject* _obj)
        {
            for (auto iter = callbacks_.cbegin(), end = callbacks_.cend(); iter != end; ++iter)
            {
                if (iter->second.object_ == _obj)
                {
                    callbacks_.erase(iter);
                    return;
                }
            }
        });
    }


    return seq;
}

void core_dispatcher::set_enabled_stats_post(bool _isStatsEnabled)
{
    isStatsEnabled_ = _isStatsEnabled;
}

bool core_dispatcher::get_enabled_stats_post() const
{
    return isStatsEnabled_;
}

qint64 core_dispatcher::post_stats_to_core(core::stats::stats_event_names _eventName)
{
    core::stats::event_props_type props;
    return post_stats_to_core(_eventName, props);
}

qint64 core_dispatcher::post_stats_to_core(core::stats::stats_event_names _eventName, const core::stats::event_props_type& _props)
{
    if (!get_enabled_stats_post())
        return -1;

    assert(_eventName > core::stats::stats_event_names::min);
    assert(_eventName < core::stats::stats_event_names::max);

    core::coll_helper coll(create_collection(), true);
    coll.set_value_as_enum("event", _eventName);

    core::ifptr<core::iarray> propsArray(coll->create_array());
    propsArray->reserve(_props.size());
    for (const auto &prop : _props)
    {
        assert(!prop.first.empty() && !prop.second.empty());

        core::coll_helper collProp(coll->create_collection(), true);

        collProp.set_value_as_string("name", prop.first);
        collProp.set_value_as_string("value", prop.second);
        core::ifptr<core::ivalue> val(coll->create_value());
        val->set_as_collection(collProp.get());
        propsArray->push_back(val.get());
    }

    coll.set_value_as_array("props", propsArray.get());
    return post_message_to_core(qsl("stats"), coll.get());
}

void core_dispatcher::received(const QString& _receivedMessage, const qint64 _seq, core::icollection* _params)
{
    if (_seq > 0)
    {
        executeCallback(_seq, _params);
    }

    cleanupCallbacks();

    core::coll_helper collParams(_params, true);

    auto iter_handler = messages_map_.find(_receivedMessage.toStdString());
    if (iter_handler == messages_map_.end())
    {
        return;
    }

    iter_handler->second(_seq, collParams);
}

bool core_dispatcher::isImCreated() const
{
    return isImCreated_;
}

void core_dispatcher::onEventTyping(core::coll_helper _params, bool _isTyping)
{
    Logic::TypingFires currentTypingStatus(
        QString::fromUtf8(_params.get_value_as_string("aimId")),
        QString::fromUtf8(_params.get_value_as_string("chatterAimId")),
        QString::fromUtf8(_params.get_value_as_string("chatterName")));

    static std::list<Logic::TypingFires> typingFires;

    auto iterFires = std::find(typingFires.begin(), typingFires.end(), currentTypingStatus);

    if (_isTyping)
    {
        if (iterFires == typingFires.end())
        {
            typingFires.push_back(currentTypingStatus);
        }
        else
        {
            ++iterFires->counter_;
        }

        QTimer::singleShot(8000, this, [this, currentTypingStatus]()
        {
            auto iterFires = std::find(typingFires.begin(), typingFires.end(), currentTypingStatus);

            if (iterFires != typingFires.end())
            {
                --iterFires->counter_;
                if (iterFires->counter_ <= 0)
                {
                    typingFires.erase(iterFires);
                    emit typingStatus(currentTypingStatus, false);
                }
            }
        });
    }
    else
    {
        if (iterFires != typingFires.end())
        {
            typingFires.erase(iterFires);
            /*
            --iterFires->counter_;

            if (iterFires->counter_ <= 0)
            {
                typingFires.erase(iterFires);
            }
            */
        }
    }

    emit typingStatus(currentTypingStatus, _isTyping);
}



void core_dispatcher::setUserState(const core::profile_state state)
{
    assert(state > core::profile_state::min);
    assert(state < core::profile_state::max);

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    std::stringstream stream;
    stream << state;

    collection.set_value_as_string("state", stream.str());
    collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
    Ui::GetDispatcher()->post_message_to_core(qsl("set_state"), collection.get());
}

void core_dispatcher::invokeStateAway()
{
    // dnd or invisible can't swap to away
    if (MyInfo()->state().compare(ql1s("online"), Qt::CaseInsensitive) == 0)
    {
        userStateGoneAway_ = true;
        setUserState(core::profile_state::away);
    }
}

void core_dispatcher::invokePreviousState()
{
    if (userStateGoneAway_ || MyInfo()->state().compare(ql1s("away"), Qt::CaseInsensitive) == 0)
    {
        userStateGoneAway_ = false;
        setUserState(core::profile_state::online);
    }
}

void core_dispatcher::onNeedLogin(const int64_t _seq, core::coll_helper _params)
{
    userStateGoneAway_ = false;

    bool is_auth_error = false;

    if (_params.get())
    {
        is_auth_error = _params.get_value_as_bool("is_auth_error", false);
    }

    emit needLogin(is_auth_error);
}

void core_dispatcher::onImCreated(const int64_t _seq, core::coll_helper _params)
{
    isImCreated_ = true;

    emit im_created();
}

void core_dispatcher::onLoginComplete(const int64_t _seq, core::coll_helper _params)
{
    emit loginComplete();
}

void core_dispatcher::onContactList(const int64_t _seq, core::coll_helper _params)
{
    auto cl = std::make_shared<Data::ContactList>();

    QString type;

    Data::UnserializeContactList(&_params, *cl, type);

    emit contactList(cl, type);
}

void core_dispatcher::onLoginGetSmsCodeResult(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int code_length = _params.get_value_as_int("code_length", 0);
    int error = result ? 0 : _params.get_value_as_int("error");

    emit getSmsResult(_seq, error, code_length);
}

void core_dispatcher::onLoginResult(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    emit loginResult(_seq, error);
}

void core_dispatcher::onAvatarsGetResult(const int64_t _seq, core::coll_helper _params)
{
    std::unique_ptr<QPixmap> avatar(Data::UnserializeAvatar(&_params));

    const QString contact = QString::fromUtf8(_params.get_value_as_string("contact"));

    const int size = _params.get_value_as_int("size");

    emit avatarLoaded(contact, avatar.release(), size);
}

void core_dispatcher::onAvatarsPresenceUpdated(const int64_t _seq, core::coll_helper _params)
{
    emit avatarUpdated(QString::fromUtf8(_params.get_value_as_string("aimid")));
}

void core_dispatcher::onContactPresence(const int64_t _seq, core::coll_helper _params)
{
    emit presense(Data::UnserializePresence(&_params));
}

void Ui::core_dispatcher::onContactOutgoingMsgCount(const int64_t _seq, core::coll_helper _params)
{
    const auto aimid = QString::fromUtf8(_params.get_value_as_string("aimid"));
    const auto count = _params.get_value_as_int("outgoing_count");
    emit outgoingMsgCount(aimid, count);
}

void core_dispatcher::onGuiSettings(const int64_t _seq, core::coll_helper _params)
{
#ifdef _WIN32
    core::dump::set_os_version(_params.get_value_as_string("os_version"));
#endif

    get_gui_settings()->unserialize(_params);

    emit guiSettings();
}

void core_dispatcher::onCoreLogins(const int64_t _seq, core::coll_helper _params)
{
    const bool has_valid_login = _params.get_value_as_bool("has_valid_login");

    emit coreLogins(has_valid_login);
}

void core_dispatcher::onThemeSettings(const int64_t _seq, core::coll_helper _params)
{
    get_qt_theme_settings()->unserialize(_params);

    emit themeSettings();
}

void core_dispatcher::onArchiveImagesGetResult(const int64_t _seq, core::coll_helper _params)
{
    emit getImagesResult(Data::UnserializeImages(_params));
}

void core_dispatcher::onArchiveMessages(Ui::MessagesBuddiesOpt _type, const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    const auto result = Data::UnserializeMessageBuddies(&_params, myAimid);

    if (!result.introMessages.isEmpty())
        emit messageBuddies(result.introMessages, result.aimId, Ui::MessagesBuddiesOpt::Intro, result.havePending, _seq, result.lastMsgId);

    emit messageBuddies(result.messages, result.aimId, _type, result.havePending, _seq, result.lastMsgId);

    if (_params.is_value_exist("deleted"))
    {
        QVector<int64_t> deletedIds;

        const auto deletedIdsArray = _params.get_value_as_array("deleted");
        assert(!deletedIdsArray->empty());
        const auto size = deletedIdsArray->size();
        deletedIds.reserve(size);
        for (auto i = 0; i < size; ++i)
            deletedIds.push_back(deletedIdsArray->get_at(i)->get_as_int64());

        emit messagesDeleted(result.aimId, deletedIds);
    }

    if (!result.modifications.isEmpty())
        emit messagesModified(result.aimId, result.modifications);
}

void core_dispatcher::onArchiveMessagesGetResult(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Requested, _seq, _params);
}

void core_dispatcher::onMessagesReceivedDlgState(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::DlgState, _seq, _params);
}

void core_dispatcher::onMessagesReceivedServer(const int64_t _seq, core::coll_helper _params)
{
    const auto result = Data::UnserializeServerMessagesIds(_params);
    if (!result.AllIds_.isEmpty())
        emit messageIdsFromServer(result.AllIds_, result.AimId_, _seq);

    if (!result.DeletedIds_.isEmpty())
        emit messagesDeleted(result.AimId_, result.DeletedIds_);
    if (!result.UpdatedMessages_.isEmpty())
        emit messagesModified(result.AimId_, result.UpdatedMessages_);
}

void core_dispatcher::onArchiveMessagesPending(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Pending, _seq, _params);
}

void core_dispatcher::onMessagesReceivedInit(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Init, _seq, _params);
}

void core_dispatcher::onMessagesReceivedMessageStatus(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::MessageStatus, _seq, _params);
}

void core_dispatcher::onMessagesDelUpTo(const int64_t _seq, core::coll_helper _params)
{
    const auto id = _params.get<int64_t>("id");
    assert(id > -1);

    const auto contact = _params.get<QString>("contact");
    assert(!contact.isEmpty());

    emit messagesDeletedUpTo(contact, id);
}

void core_dispatcher::onDlgStates(const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    QVector<Data::DlgState> dlgStatesList;

    auto arrayDlgStates = _params.get_value_as_array("dlg_states");
    if (!arrayDlgStates)
    {
        assert(false);
        return;
    }

    const auto size = arrayDlgStates->size();
    dlgStatesList.reserve(size);
    for (int32_t i = 0; i < size; ++i)
    {
        const auto val = arrayDlgStates->get_at(i);

        core::coll_helper helper(val->get_as_collection(), false);

        dlgStatesList.push_back(Data::UnserializeDlgState(&helper, myAimid, false));
    }

    emit dlgStates(dlgStatesList);
}

void core_dispatcher::onMentionsMeReceived(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");
    const auto myaimid = _params.get<QString>("my_aimid");

    core::coll_helper coll_message(_params.get_value_as_collection("message"), false);

    auto message = Data::unserializeMessage(coll_message, contact, myaimid, -1, -1);

    emit mentionMe(contact, message);
}


void core_dispatcher::onHistorySearchResultMsg(const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    Data::DlgState state = Data::UnserializeDlgState(&_params, myAimid, true /* from_search */);

    if (_params.is_value_exist("from_search"))
    {
        state.IsFromSearch_ = _params.get<bool>("from_search");
    }

    if (_params.is_value_exist("term"))
    {
        state.SearchTerm_ = _params.get<QString>("term");
    }

    if (_params.is_value_exist("request_id"))
    {
        state.RequestId_ = _params.get<int64_t>("request_id");
    }
    if (_params.is_value_exist("priority"))
    {
        state.SearchPriority_ = _params.get<int>("priority");
    }

    emit Utils::InterConnector::instance().hideNoSearchResults();
    emit Utils::InterConnector::instance().hideSearchSpinner();
    emit searchedMessage(state);
}

void core_dispatcher::onHistorySearchResultContacts(const int64_t _seq, core::coll_helper _params)
{
    QVector<Data::DlgState> states;
    const auto msgArray = _params.get_value_as_array("results");
    const auto size = msgArray->size();
    states.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        const auto val = msgArray->get_at(i);
        core::coll_helper helper(val->get_as_collection(), false);
        auto aimId = helper.get<QString>("contact");
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(aimId);
        if (state.AimId_.isEmpty())
            state.AimId_ = std::move(aimId);

        if (helper.is_value_exist("is_contact"))
        {
            state.IsContact_ = helper.get<bool>("is_contact");
        }

        if (helper.is_value_exist("is_chat"))
        {
            state.Chat_ = helper.get<bool>("is_chat");
        }

        if (helper.is_value_exist("from_search"))
        {
            state.IsFromSearch_ = helper.get<bool>("from_search");
        }

        if (helper.is_value_exist("term"))
        {
            state.SearchTerm_ = helper.get<QString>("term");
        }

        if (helper.is_value_exist("request_id"))
        {
            state.RequestId_ = helper.get<int64_t>("request_id");
        }
        if (helper.is_value_exist("priority"))
        {
            state.SearchPriority_ = helper.get<int>("priority");
        }
        states.push_back(std::move(state));
    }

    if (!states.isEmpty())
    {
        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();
    }
    emit searchedContacts(states, _params.get_value_as_int64("reqId"));
}

void core_dispatcher::onEmptySearchResults(const int64_t _seq, core::coll_helper _params)
{
    emit emptySearchResults(_params.get_value_as_int64("req_id"));
}

void core_dispatcher::onSearchNeedUpdate(const int64_t _seq, core::coll_helper _params)
{
    emit Utils::InterConnector::instance().repeatSearch();
}

void core_dispatcher::onVoipSignal(const int64_t _seq, core::coll_helper _params)
{
    voipController_.handlePacket(_params);
}

void core_dispatcher::onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params)
{
    emit Utils::InterConnector::instance().showNoRecentsYet();
}

void core_dispatcher::onActiveDialogsHide(const int64_t _seq, core::coll_helper _params)
{
    QString aimId = Data::UnserializeActiveDialogHide(&_params);

    emit activeDialogHide(aimId);
}

void core_dispatcher::onStickersMetaGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::unserialize(_params);

    emit onStickers();
}

void core_dispatcher::onStickersStore(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::unserialize_store(_params);

    emit onStore();
}

void core_dispatcher::onThemesMetaGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::themes::unserialize(_params);

    get_qt_theme_settings()->flushThemesToLoad();   // here we call delayed themes requests

    emit onThemesMeta();
}

void core_dispatcher::onThemesMetaGetError(const int64_t _seq, core::coll_helper _params)
{
    emit onThemesMetaError();
}

void core_dispatcher::onStickersStickerGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::setStickerData(_params);

    emit onSticker(
        _params.get_value_as_int("error"),
        _params.get_value_as_int("set_id"),
        _params.get_value_as_int("sticker_id"));
}

void core_dispatcher::onStickersGetSetBigIconResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::setSetBigIcon(_params);

    emit onSetBigIcon(
        _params.get_value_as_int("error"),
        _params.get_value_as_int("set_id"));
}

void core_dispatcher::onStickersPackInfo(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");

    std::string store_id = _params.get_value_as_string("store_id");

    std::shared_ptr<Ui::Stickers::Set> info;

    if (result)
    {
        core::coll_helper coll_set(_params.get_value_as_collection("info"), false);

        info = Stickers::parseSet(coll_set);

        Stickers::addSet(info);
    }

    emit onStickerpackInfo(result, info);
}

void core_dispatcher::onThemesThemeGetResult(const int64_t _seq, core::coll_helper _params)
{
    bool failed = _params.get_value_as_int("failed", 0) != 0;
    if (!failed)
    {
        Ui::themes::setThemeData(_params);
    }
    emit onTheme(_params.get_value_as_int("theme_id"), failed);
}

void core_dispatcher::onChatsInfoGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto info = std::make_shared<Data::ChatInfo>(Data::UnserializeChatInfo(&_params));
    if (!info->AimId_.isEmpty())
        emit chatInfo(_seq, info);
}

void core_dispatcher::onChatsBlockedResult(const int64_t _seq, core::coll_helper _params)
{
    emit chatBlocked(Data::UnserializeChatMembers(&_params));
}

void core_dispatcher::onChatsPendingResult(const int64_t _seq, core::coll_helper _params)
{
    emit chatPending(Data::UnserializeChatMembers(&_params));
}

void core_dispatcher::onChatsInfoGetFailed(const int64_t _seq, core::coll_helper _params)
{
    auto errorCode = _params.get_value_as_int("error");

    emit chatInfoFailed(_seq, (core::group_chat_info_errors) errorCode);
}

void core_dispatcher::onLoginResultAttachUin(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    emit loginResultAttachUin(_seq, error);
}

void core_dispatcher::onLoginResultAttachPhone(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    emit loginResultAttachPhone(_seq, error);
}

void core_dispatcher::onRecvFlags(const int64_t _seq, core::coll_helper _params)
{
    emit recvFlags(_params.get_value_as_int("flags"));
}

void core_dispatcher::onUpdateProfileResult(const int64_t _seq, core::coll_helper _params)
{
    emit updateProfile(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsHomeGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto result = Data::UnserializeChatHome(&_params);

    emit chatsHome(result.chats, result.newTag, result.restart, result.finished);
}

void core_dispatcher::onChatsHomeGetFailed(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");

    emit chatsHomeError(error);
}

void core_dispatcher::onUserProxyResult(const int64_t _seq, core::coll_helper _params)
{
    Utils::ProxySettings userProxy;

    userProxy.type_ = (core::proxy_types)_params.get_value_as_int("settings_proxy_type", (int32_t)core::proxy_types::auto_proxy);
    userProxy.proxyServer_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_server", ""));
    userProxy.port_ = _params.get_value_as_int("settings_proxy_port", Utils::ProxySettings::invalidPort);
    userProxy.username_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_username", ""));
    userProxy.password_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_password", ""));
    userProxy.needAuth_ = _params.get_value_as_bool("settings_proxy_need_auth", false);

    *Utils::get_proxy_settings() = userProxy;

    emit getUserProxy();
}

void core_dispatcher::onOpenCreatedChat(const int64_t _seq, core::coll_helper _params)
{
    emit openChat(QString::fromUtf8(_params.get_value_as_string("aimId")));
}

void core_dispatcher::onLoginNewUser(const int64_t _seq, core::coll_helper _params)
{
    emit login_new_user();
}

void core_dispatcher::onSetAvatarResult(const int64_t _seq, core::coll_helper _params)
{
    emit Utils::InterConnector::instance().setAvatar(_params.get_value_as_int64("seq"), _params.get_value_as_int("error"));
    emit Utils::InterConnector::instance().setAvatarId(QString::fromUtf8(_params.get_value_as_string("id")));
}

void core_dispatcher::onChatsRoleSetResult(const int64_t _seq, core::coll_helper _params)
{
    emit setChatRoleResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsBlockResult(const int64_t _seq, core::coll_helper _params)
{
    emit blockMemberResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsPendingResolveResult(const int64_t _seq, core::coll_helper _params)
{
    emit pendingListResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onPhoneinfoResult(const int64_t _seq, core::coll_helper _params)
{
    Data::PhoneInfo data;
    data.deserialize(&_params);

    emit phoneInfoResult(_seq, data);
}

void core_dispatcher::onContactRemovedFromIgnore(const int64_t _seq, core::coll_helper _params)
{
    auto cl = std::make_shared<Data::ContactList>();

    QString type;

    Data::UnserializeContactList(&_params, *cl, type);

    emit contactList(cl, type);
}

void Ui::core_dispatcher::onHistoryUpdate(const int64_t _seq, core::coll_helper _params)
{
	qint64 last_read_msg = _params.get_value_as_int64("last_read_msg");

	emit historyUpdate(QString::fromUtf8(_params.get_value_as_string("contact")), last_read_msg);
}

namespace { std::unique_ptr<core_dispatcher> gDispatcher; }

core_dispatcher* Ui::GetDispatcher()
{
    if (!gDispatcher)
    {
        assert(false);
    }

    return gDispatcher.get();
}

void Ui::createDispatcher()
{
    if (gDispatcher)
    {
        assert(false);
    }

    gDispatcher = std::make_unique<core_dispatcher>();
}

void Ui::destroyDispatcher()
{
    if (!gDispatcher)
    {
        assert(false);
    }

    gDispatcher.reset();
}

