#pragma once

namespace core
{

    enum class profile_state
    {
        min = 0,

        online,
        dnd,
        invisible,
        away,
        offline,

        max,
    };

    // don't change texts in this func - it uses for sending to server
    inline std::ostream& operator<<(std::ostream &oss, const profile_state arg)
    {
        assert(arg > profile_state::min);
        assert(arg < profile_state::max);

        switch(arg)
        {
            case profile_state::online:    oss << "online"; break;
            case profile_state::dnd:       oss << "dnd"; break;
            case profile_state::invisible: oss << "invisible"; break;
            case profile_state::away:      oss << "away"; break;

        default:
            assert(!"unknown core::profile_state value");
            break;
        }

        return oss;
    }

    enum class message_type
    {
        min,


        undefined,
        base,
        file_sharing,
        sms,
        sticker,
        chat_event,
        voip_event,

        max
    };

    enum class file_sharing_function
    {
        min,

        download_file,
        download_file_metainfo,
        download_preview_metainfo,
        check_local_copy_exists,

        max
    };

    enum class file_sharing_content_type
    {
        min,

        undefined,
        image,
        gif,
        video,
        ptt,

        max
    };

    enum class typing_status
    {
        min,

        none,
        typing,
        typed,
        looking,

        max,
    };

    enum class sticker_size
    {
        min,

        small,
        medium,
        large,
        xlarge,
        xxlarge,

        max
    };

    enum class chat_event_type
    {
        // the codes are stored in a db thus do not reorder the items below

        invalid,

        min,

        added_to_buddy_list,

        mchat_add_members,
        mchat_invite,
        mchat_leave,
        mchat_del_members,
        mchat_kicked,

        chat_name_modified,

        buddy_reg,
        buddy_found,

        birthday,

        avatar_modified,

        generic,

        chat_description_modified,

        message_deleted,

        chat_rules_modified,

        max
    };

    enum class voip_event_type
    {
        invalid,

        min,

        missed_call,
        call_ended,
        accept,

        max
    };

    inline std::ostream& operator<<(std::ostream &oss, const message_type arg)
    {
        switch(arg)
        {
        case message_type::base:
            oss << "base";
            break;

        case message_type::file_sharing:
            oss << "file_sharing";
            break;

        case message_type::sticker:
            oss << "sticker";
            break;

        case message_type::sms:
            oss << "sms";
            break;

        default:
            assert(!"unknown core::message_type value");
            break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const file_sharing_function arg)
    {
        assert(arg > file_sharing_function::min);
        assert(arg < file_sharing_function::max);

        switch(arg)
        {
        case file_sharing_function::check_local_copy_exists:
            oss << "check_local_copy_exists";
            break;

        case file_sharing_function::download_file:
            oss << "download_file";
            break;

        case file_sharing_function::download_file_metainfo:
            oss << "download_file_metainfo";
            break;

        case file_sharing_function::download_preview_metainfo:
            oss << "download_preview_metainfo";
            break;

        default:
            assert(!"unknown core::file_sharing_function value");
            break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << "small";
            break;

        case sticker_size::medium:
            oss << "medium";
            break;

        case sticker_size::large:
            oss << "large";
            break;

        case sticker_size::xlarge:
            oss << "xlarge";
            break;

        case sticker_size::xxlarge:
            oss << "xxlarge";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    inline std::wostream& operator<<(std::wostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << L"small";
            break;

        case sticker_size::medium:
            oss << L"medium";
            break;

        case sticker_size::large:
            oss << L"large";
            break;

        case sticker_size::xlarge:
            oss << L"xlarge";
            break;

        case sticker_size::xxlarge:
            oss << L"xxlarge";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    enum class group_chat_info_errors
    {
        min = 1,

        network_error = 2,
        not_in_chat = 3,
        blocked = 4,

        max,
    };

    namespace stats
    {
        // NOTE: do not change old numbers, add new one if necessary
        enum class stats_event_names
        {
            min = 0,

            service_session_start = 1001,
            start_session = 1002,
            start_session_params_loaded = 1003,

            reg_page_phone = 2001,
            reg_login_phone = 2002,
            reg_page_uin = 2003,
            reg_login_uin = 2004,
            reg_edit_country = 2005,
            reg_error_phone = 2006,
            reg_sms_send = 2007,
            reg_sms_resend = 2008,
            reg_error_code = 2009,
            reg_edit_phone = 2010,
            reg_error_uin = 2011,
            reg_error_other = 2012,
            login_forgot_password = 2013,

            main_window_fullscreen = 3001,
            main_window_resize = 3002,

            groupchat_from_create_button = 4001,
            groupchat_from_sidebar = 4002,
            groupchat_created = 4003,
            groupchat_create_members_count = 4005,
            groupchat_members_count = 4006,
            groupchat_leave = 4007,
            groupchat_rename = 4009,
            groupchat_avatar_changed = 4010,
            groupchat_add_member_sidebar = 4011,
            groupchat_add_member_dialog = 4012,

            filesharing_sent_count = 5001,
            filesharing_sent = 5002,
            filesharing_sent_success = 5003,
            filesharing_count = 5004,
            filesharing_filesize = 5005,
            filesharing_dnd_recents = 5006,
            filesharing_dnd_dialog = 5007,
            filesharing_dialog = 5008,
            filesharing_cancel = 5009,
            filesharing_download_file = 5010,
            filesharing_download_cancel = 5011,
            filesharing_download_success = 5012,

            smile_sent_picker = 6001,
            smile_sent_from_recents = 6002,
            sticker_sent_from_picker = 6003,
            sticker_sent_from_recents = 6004,
            picker_cathegory_click = 6005,
            picker_tab_click = 6006,

            alert_click = 7001,
            alert_viewall = 7002,
            alert_close = 7003,
            alert_mail_common = 7004,
            alert_mail_letter = 7005,
            tray_mail = 7006,
            titlebar_message = 7007,
            titlebar_mail = 7008,

            spam_cl_menu = 8001,
            spam_auth_widget = 8002,
            spam_sidebar = 8003,
            spam_members_list = 8004,
            spam_chat_avatar = 8005,

            recents_close = 9001,
            recents_read = 9002,
            recents_readall = 9003,
            mute_recents_menu = 9004,
            mute_sidebar = 9005,
            unmute = 9006,

            ignore_recents_menu = 10001,
            ignore_cl_menu = 10002,
            ignore_auth_widget = 10003,
            ignore_sidebar = 10004,
            ignorelist_open = 10006,
            ignorelist_remove = 10007,

            cl_empty_write_msg = 11001,
            cl_empty_android = 11002,
            cl_empty_ios = 11003,
            cl_empty_find_friends = 11004,
            cl_search = 11005,
            cl_load = 11006,
            cl_search_openmessage = 11007,
            cl_search_dialog = 11008,
            cl_search_dialog_openmessage = 11009,
            cl_search_nohistory = 11010,

            myprofile_edit = 12001,
            myprofile_open = 12002,

            profile_cl = 13001,
            profile_auth_widget = 13002,
            profile_avatar = 13003,
            profile_search_results = 13004,
            profile_members_list = 13005,
            profile_call = 13006,
            profile_video_call = 13007,
            profile_sidebar = 13008,
            profile_write_message = 13009,

            add_user_profile_page = 14001,
            add_user_auth_widget = 14002,
            add_user_sidebar = 14004,
            delete_auth_widget = 14005,
            delete_sidebar = 14006,
            delete_cl_menu = 14007,
            delete_profile_page = 14008,
            rename_contact = 14009,

            search_open_page = 15001,
            search_no_results = 15002,
            add_user_search_results = 15003,
            search = 15004,

            message_send_button = 16001,
            message_enter_button = 16002,

            open_chat_recents = 17001,
            open_chat_search_recents = 17002,
            open_chat_cl = 17003,

            message_pending = 18001,
            message_delete_my = 18002,
            message_delete_all = 18003,

            history_preload = 19002,
            history_delete = 19003,

            open_popup_livechat = 20001,

            gui_load = 21001,

            feedback_show = 22001,
            feedback_sent = 22002,
            feedback_error = 22003,

            call_from_chat = 23001,
            videocall_from_chat = 23002,
            call_from_cl_menu = 23005,
            call_from_search_results = 23008,
            voip_callback = 23009,
            voip_incoming_call = 23010,
            voip_accept = 23011,
            voip_accept_video = 23012,
            voip_declined = 23013,
            voip_started = 23014,
            voip_finished = 23015,

            voip_chat = 24001,
            voip_camera_off = 24002,
            voip_camera_on = 24003,
            voip_microphone_off = 24004,
            voip_microphone_on = 24005,
            voip_dynamic_off = 24006,
            voip_dynamic_on = 24007,
            voip_sound_off = 24008,
            voip_settings = 24009,
            voip_fullscreen = 24010,
            voip_camera_change = 24011,
            voip_microphone_change = 24012,
            voip_dynamic_change = 24013,
            voip_aspectratio_change = 24014,

            settings_about_show = 25001,
            client_settings = 25002,

            proxy_open = 26001,
            proxy_set = 26002,

            strange_event = 27001,

            favorites_set = 29001,
            favorites_unset = 29002,
            favorites_load = 29003,

            livechats_page_open = 30001,
            livechat_profile_open = 30002,
            livechat_join_fromprofile = 30003,
            livechat_join_frompopup = 30004,
            livechat_admins = 30005,
            livechat_blocked = 30006,

            profile_avatar_changed = 31001,
            introduce_name_set = 31002,
            introduce_avatar_changed = 31003,
            introduce_avatar_fail = 31004,

            masks_open = 32001,
            masks_select = 32002,

            chats_create_open = 33001,
            chats_create_public = 33002,
            chats_create_approval = 33003,
            chats_create_readonly = 33004,
            chats_create_rename = 33005,
            chats_create_avatar = 33006,
            chats_created = 33007,

            quotes_send_alone = 34001,
            quotes_send_answer = 34002,
            quotes_messagescount = 34003,

            forward_send_frommenu = 35001,
            forward_send_frombutton = 35002,
            forward_send_preview = 35003,
            forward_messagescount = 35004,

            merge_accounts = 35100,

            unknowns_add_user = 36000,
            unknowns_close = 36001,
            unknowns_closeall = 36002,

            chat_down_button = 36003,

            max = 36004,
        };

        inline std::ostream& operator<<(std::ostream &oss, const stats_event_names arg)
        {
            assert(arg > stats_event_names::min);
            assert(arg < stats_event_names::max);

            switch(arg)
            {
            case stats_event_names::start_session : oss << "Start_Session [session enable]"; break;
            case stats_event_names::start_session_params_loaded : oss << "Start_Session_Params_Loaded"; break;
            case stats_event_names::service_session_start : assert(false); break;

            // registration
            case stats_event_names::reg_page_phone : oss << "Reg_Page_Phone"; break;
            case stats_event_names::reg_login_phone : oss << "Reg_Login_Phone"; break;
            case stats_event_names::reg_page_uin : oss << "Reg_Page_Uin"; break;
            case stats_event_names::reg_login_uin : oss << "Reg_Login_UIN"; break;
            case stats_event_names::reg_edit_country : oss << "Reg_Edit_Country"; break;
            case stats_event_names::reg_error_phone : oss << "Reg_Error_Phone"; break;
            case stats_event_names::reg_sms_send : oss << "Reg_Sms_Send"; break;
            case stats_event_names::reg_sms_resend : oss << "Reg_Sms_Resend"; break;
            case stats_event_names::reg_error_code : oss << "Reg_Error_Code"; break;
            case stats_event_names::reg_edit_phone : oss << "Reg_Edit_Phone"; break;
            case stats_event_names::reg_error_uin : oss << "Reg_Error_UIN"; break;
            case stats_event_names::reg_error_other : oss << "Reg_Error_Other"; break;
            case stats_event_names::login_forgot_password : oss << "Login_Forgot_Password"; break;

            // main window
            case stats_event_names::main_window_fullscreen : oss << "Mainwindow_Fullscreen"; break;
            case stats_event_names::main_window_resize : oss << "Mainwindow_Resize"; break;

            // group chat
            case stats_event_names::groupchat_from_create_button : oss << "Groupchat_FromCreateButton"; break;
            case stats_event_names::groupchat_from_sidebar : oss << "Groupchat_FromSidebar"; break;
            case stats_event_names::groupchat_created : oss << "Groupchat_Created"; break;
            case stats_event_names::groupchat_members_count : oss << "Groupchat_MembersCount"; break;
            case stats_event_names::groupchat_leave : oss << "Groupchat_Leave"; break;
            case stats_event_names::groupchat_rename : oss << "Groupchat_Rename"; break;
            case stats_event_names::groupchat_avatar_changed : oss << "Groupchat_Avatar_Changed"; break;
            case stats_event_names::groupchat_add_member_sidebar : oss << "Groupchat_Add_member_Sidebar"; break;
            case stats_event_names::groupchat_add_member_dialog : oss << "Groupchat_Add_member_Dialog"; break;

            // filesharing
            case stats_event_names::filesharing_sent : oss << "Filesharing_Sent"; break;
            case stats_event_names::filesharing_sent_count : oss << "Filesharing_Sent_Count"; break;
            case stats_event_names::filesharing_sent_success : oss << "Filesharing_Sent_Success"; break;
            case stats_event_names::filesharing_count : oss << "Filesharing_Count"; break;
            case stats_event_names::filesharing_filesize : oss << "Filesharing_Filesize"; break;
            case stats_event_names::filesharing_dnd_recents : oss << "Filesharing_DNDRecents"; break;
            case stats_event_names::filesharing_dnd_dialog : oss << "Filesharing_DNDDialog"; break;
            case stats_event_names::filesharing_dialog : oss << "Filesharing_Dialog"; break;
            case stats_event_names::filesharing_cancel : oss << "Filesharing_Cancel"; break;
            case stats_event_names::filesharing_download_file : oss << "Filesharing_Download_File"; break;
            case stats_event_names::filesharing_download_cancel : oss << "Filesharing_Download_Cancel"; break;
            case stats_event_names::filesharing_download_success : oss << "Filesharing_Download_Success"; break;

            // smiles and stickers
            case stats_event_names::smile_sent_picker : oss << "Smile_Sent_Picker"; break;
            case stats_event_names::smile_sent_from_recents : oss << "Smile_Sent_From_Recents"; break;
            case stats_event_names::sticker_sent_from_picker : oss << "Sticker_Sent_From_Picker"; break;
            case stats_event_names::sticker_sent_from_recents : oss << "Sticker_Sent_From_Recents"; break;
            case stats_event_names::picker_cathegory_click : oss << "Picker_Cathegory_Click"; break;
            case stats_event_names::picker_tab_click : oss << "Picker_Tab_Click"; break;

            // alerts
            case stats_event_names::alert_click : oss << "Alert_Click"; break;
            case stats_event_names::alert_viewall : oss << "Alert_ViewAll"; break;
            case stats_event_names::alert_close : oss << "Alert_Close"; break;
            case stats_event_names::alert_mail_common:  oss << "Alert_Mail_Common"; break;
            case stats_event_names::alert_mail_letter: oss << "Alert_Mail_Letter"; break;
            case stats_event_names::tray_mail: oss << "Tray_Mail"; break;
            case stats_event_names::titlebar_message: oss << "Titlebar_Message"; break;
            case stats_event_names::titlebar_mail: oss << "Titlebar_Mail"; break;

            // spam
            case stats_event_names::spam_cl_menu : oss << "Spam_CL_Menu"; break;
            case stats_event_names::spam_auth_widget : oss << "Spam_Auth_Widget"; break;
            case stats_event_names::spam_sidebar : oss << "Spam_Sidebar"; break;
            case stats_event_names::spam_members_list: oss << "Spam_Members_List"; break;
            case stats_event_names::spam_chat_avatar: oss << "Spam_Chat_Avatar"; break;

            // cl
            case stats_event_names::recents_close : oss << "Recents_Close"; break;
            case stats_event_names::recents_read : oss << "Recents_Read"; break;
            case stats_event_names::recents_readall : oss << "Recents_Readall"; break;

            case stats_event_names::mute_recents_menu : oss << "Mute_Recents_Menu"; break;
            case stats_event_names::mute_sidebar : oss << "Mute_Sidebar"; break;
            case stats_event_names::unmute : oss << "Unmute"; break;

            case stats_event_names::ignore_recents_menu : oss << "Ignore_Recents_Menu"; break;
            case stats_event_names::ignore_cl_menu : oss << "Ignore_CL_Menu"; break;
            case stats_event_names::ignore_auth_widget : oss << "Ignore_Auth_Widget"; break;
            case stats_event_names::ignore_sidebar : oss << "Ignore_Sidebar"; break;
            case stats_event_names::ignorelist_open : oss << "Ignorelist_Open"; break;
            case stats_event_names::ignorelist_remove : oss << "Ignorelist_Remove"; break;


            case stats_event_names::cl_empty_write_msg : oss << "CL_Empty_Write_Msg"; break;
            case stats_event_names::cl_empty_android : oss << "CL_Empty_Android"; break;
            case stats_event_names::cl_empty_ios : oss << "CL_Empty_IOS"; break;
            case stats_event_names::cl_empty_find_friends : oss << "CL_Empty_Find_Friends"; break;

            case stats_event_names::rename_contact : oss << "rename_contact"; break;

            case stats_event_names::cl_search : oss << "CL_Search"; break;
            case stats_event_names::cl_load : oss << "CL_Load"; break;
            case stats_event_names::cl_search_openmessage : oss << "CL_Search_OpenMessage"; break;
            case stats_event_names::cl_search_dialog : oss << "CL_Search_Dialog"; break;
            case stats_event_names::cl_search_dialog_openmessage : oss << "CL_Search_Dialog_OpenMessage"; break;
            case stats_event_names::cl_search_nohistory : oss << "CL_Search_NoHistory"; break;

            // profile
            case stats_event_names::myprofile_open : oss << "Myprofile_Open"; break;

            case stats_event_names::profile_cl : oss << "Profile_CL"; break;
            case stats_event_names::profile_auth_widget : oss << "Profile_Auth_Widget"; break;
            case stats_event_names::profile_avatar : oss << "Profile_Avatar"; break;
            case stats_event_names::profile_search_results : oss << "Profile_Search_Results"; break;
            case stats_event_names::profile_members_list : oss << "Profile_Members_List"; break;
            case stats_event_names::profile_call : oss << "Profile_Call"; break;
            case stats_event_names::profile_video_call : oss << "Profile_Video_Call"; break;
            case stats_event_names::profile_sidebar : oss << "Profile_Sidebar"; break;
            case stats_event_names::profile_write_message: oss << "Profile_Write_Message"; break;

            case stats_event_names::add_user_profile_page : oss << "Add_User_Profile_Page"; break;
            case stats_event_names::myprofile_edit : oss << "Myprofile_Edit"; break;


            // search, adding and auth
            case stats_event_names::add_user_auth_widget : oss << "Add_User_Auth_Widget"; break;
            case stats_event_names::add_user_sidebar : oss << "Add_User_Sidebar"; break;
            case stats_event_names::unknowns_add_user: oss << "Unknowns_Add_User"; break;


            case stats_event_names::delete_auth_widget : oss << "Delete_Auth_Widget"; break;
            case stats_event_names::delete_sidebar : oss << "Delete_Sidebar"; break;
            case stats_event_names::delete_cl_menu : oss << "Delete_CL_Menu"; break;
            case stats_event_names::delete_profile_page : oss << "Delete_Profile_Page"; break;
            case stats_event_names::unknowns_close: oss << "Unknowns_Close"; break;
            case stats_event_names::unknowns_closeall: oss << "Unknowns_CloseAll"; break;

            case stats_event_names::search_open_page : oss << "Search_Open_Page"; break;
            case stats_event_names::search_no_results : oss << "Search_No_Results"; break;
            case stats_event_names::add_user_search_results : oss << "Add_User_Search_Results"; break;
            case stats_event_names::search : oss << "Search"; break;


            // messaging
            case stats_event_names::message_send_button : oss << "Message_Send_Button"; break;
            case stats_event_names::message_enter_button : oss << "Message_Enter_Button"; break;
            case stats_event_names::open_chat_recents : oss << "Open_Chat_Recents"; break;
            case stats_event_names::open_chat_search_recents : oss << "Open_Chat_Search_Recents"; break;
            case stats_event_names::open_chat_cl : oss << "Open_Chat_CL"; break;

            case stats_event_names::message_pending : oss << "Message_Pending"; break;
            case stats_event_names::message_delete_my : oss << "message_delete_my"; break;
            case stats_event_names::message_delete_all : oss << "message_delete_all"; break;
            case stats_event_names::open_popup_livechat : oss << "open_popup_livechat"; break;

            case stats_event_names::history_preload : oss << "History_Preload"; break;
            case stats_event_names::history_delete : oss << "history_delete"; break;

            case stats_event_names::feedback_show : oss << "Feedback_Show"; break;
            case stats_event_names::feedback_sent : oss << "Feedback_Sent"; break;
            case stats_event_names::feedback_error : oss << "Feedback_Error"; break;

            case stats_event_names::call_from_chat : oss << "Call_From_Chat"; break;
            case stats_event_names::videocall_from_chat : oss << "Videocall_From_Chat"; break;
            case stats_event_names::call_from_cl_menu : oss << "Call_From_CL_Meu"; break;
            case stats_event_names::call_from_search_results : oss << "Call_From_Search_Results"; break;
            case stats_event_names::voip_callback : oss << "Voip_Callback"; break;
            case stats_event_names::voip_incoming_call : oss << "Voip_Incoming_Call"; break;
            case stats_event_names::voip_accept : oss << "Voip_Accept"; break;
            case stats_event_names::voip_accept_video : oss << "Voip_Accept_Video"; break;
            case stats_event_names::voip_declined : oss << "Voip_Declined"; break;
            case stats_event_names::voip_started : oss << "Voip_Started"; break;
            case stats_event_names::voip_finished : oss << "Voip_Finished"; break;

            case stats_event_names::voip_chat : oss << "Voip_Chat"; break;
            case stats_event_names::voip_camera_off : oss << "Voip_Camera_Off"; break;
            case stats_event_names::voip_camera_on : oss << "Voip_Camera_On"; break;
            case stats_event_names::voip_microphone_off : oss << "Voip_Microphone_Off"; break;
            case stats_event_names::voip_microphone_on : oss << "Voip_Microphone_On"; break;
            case stats_event_names::voip_dynamic_off : oss << "Voip_Dynamic_Off"; break;
            case stats_event_names::voip_dynamic_on : oss << "Voip_Dynamic_On"; break;
            case stats_event_names::voip_sound_off : oss << "Voip_Sound_Off"; break;
            case stats_event_names::voip_settings : oss << "Voip_Settings"; break;
            case stats_event_names::voip_fullscreen : oss << "Voip_Fullscreen"; break;
            case stats_event_names::voip_camera_change : oss << "Voip_Camera_Change"; break;
            case stats_event_names::voip_microphone_change : oss << "Voip_Microphone_Change"; break;
            case stats_event_names::voip_dynamic_change : oss << "Voip_Dynamic_Change"; break;
            case stats_event_names::voip_aspectratio_change : oss << "Voip_AspectRatio_Change"; break;

            case stats_event_names::gui_load : oss << "Gui_Load"; break;

            case stats_event_names::settings_about_show : oss << "Settings_About_Show"; break;
            case stats_event_names::client_settings : oss << "Client_Settings"; break;

            case stats_event_names::proxy_open : oss << "proxy_open"; break;
            case stats_event_names::proxy_set : oss << "proxy_set"; break;

            case stats_event_names::favorites_set : oss << "favorites_set"; break;
            case stats_event_names::favorites_unset : oss << "favorites_unset"; break;
            case stats_event_names::favorites_load : oss << "favorites_load"; break;

            case stats_event_names::livechats_page_open : oss << "Livechats_Page_Open"; break;
            case stats_event_names::livechat_profile_open : oss << "Livechat_Profile_Open"; break;
            case stats_event_names::livechat_join_fromprofile : oss << "Livechat_Join_FromProfile"; break;
            case stats_event_names::livechat_join_frompopup : oss << "Livechat_Join_FromPopup"; break;
            case stats_event_names::livechat_admins : oss << "Livechat_Admins"; break;
            case stats_event_names::livechat_blocked : oss << "Livechat_Blocked"; break;


            case stats_event_names::profile_avatar_changed : oss << "Profile_Avatar_Changed"; break;
            case stats_event_names::introduce_name_set : oss << "Introduce_Name_Set"; break;
            case stats_event_names::introduce_avatar_changed : oss << "Introduce_Avatar_Changed"; break;
            case stats_event_names::introduce_avatar_fail : oss << "Introduce_Avatar_Fail"; break;

            // Masks
            case stats_event_names::masks_open : oss << "Masks_Open"; break;
            case stats_event_names::masks_select : oss << "Masks_Select"; break;

            // Create chat
            case stats_event_names::chats_create_open : oss << "Chats_Create_Open"; break;
            case stats_event_names::chats_create_public : oss << "Chats_Create_Public"; break;
            case stats_event_names::chats_create_approval : oss << "Chats_Create_Approval"; break;
            case stats_event_names::chats_create_readonly : oss << "Chats_Create_Readonly"; break;
            case stats_event_names::chats_create_rename : oss << "Chats_Create_Rename"; break;
            case stats_event_names::chats_create_avatar : oss << "Chats_Create_Avatar"; break;
            case stats_event_names::chats_created : oss << "Chats_Created"; break;

            case stats_event_names::quotes_send_alone : oss << "Quotes_Send_Alone"; break;
            case stats_event_names::quotes_send_answer : oss << "Quotes_Send_Answer"; break;
            case stats_event_names::quotes_messagescount : oss << "Quotes_MessagesCount"; break;

            case stats_event_names::forward_send_frommenu : oss << "Forward_Send_FromMenu"; break;
            case stats_event_names::forward_send_frombutton : oss << "Forward_Send_FromButton"; break;
            case stats_event_names::forward_send_preview : oss << "Forward_Send_Preview"; break;
            case stats_event_names::forward_messagescount : oss << "Forward_MessagesCount"; break;

            case stats_event_names::merge_accounts: oss << "Merge_Accounts"; break;

            case stats_event_names::chat_down_button: oss << "Chat_DownButton"; break;

            default:
                assert(!"unknown core::stats_event_names ");
                oss << "Unknown event " << (int)arg; break;
            }

            return oss;
        }
    }

    enum class proxy_types
    {
        min = -100,
        auto_proxy = -1,

        // positive values are reserved for curl_proxytype
        http_proxy = 0,
        socks4 = 4,
        socks5 = 5,

        max
    };

    inline std::ostream& operator<<(std::ostream &oss, const proxy_types arg)
    {
        assert(arg > proxy_types::min);
        assert(arg < proxy_types::max);

        switch(arg)
        {
            case proxy_types::auto_proxy: oss << "Auto"; break;
            case proxy_types::http_proxy: oss << "HTTP Proxy"; break;
            case proxy_types::socks4: oss << "SOCKS4"; break;
            case proxy_types::socks5: oss << "SOCKS5"; break;

            default:
                assert(!"unknown core::proxy_types value");
                break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const file_sharing_content_type arg)
    {
        assert(arg > file_sharing_content_type::min);
        assert(arg < file_sharing_content_type::max);

        switch (arg)
        {
            case file_sharing_content_type::gif: return (oss << "gif");
            case file_sharing_content_type::image: return (oss << "image");
            case file_sharing_content_type::ptt: return (oss << "ptt");
            case file_sharing_content_type::undefined: return (oss << "undefined");
            case file_sharing_content_type::video: return (oss << "video");
            default:
                assert(!"unexpected file sharing content type");
        }

        return oss;
    }

}
