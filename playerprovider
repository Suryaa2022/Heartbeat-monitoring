#include "playerprovider.h"

#include "glib_helper.h"
#include "lang_convert.h"
#include "option.h"
#include "player_logger.h"
#include "player/player_export.h"

#include <math.h>

#define WAIT_ON_ERROR_SECONDS 6 // ToDo: need to export to configure file

#define CACHE_SERVICE_NAME   "com.lge.stream-caching-service"
#define CACHE_OBJECT_PATH    "/com/lge/caching"
#define CACHE_INTERFACE_NAME "com.lge.StreamCaching"

#define CACHE_SETCACHEDMUSIC  "SetCachedMusic"
#define CACHE_GETCACHEDMOVIE  "GetCachedMovie"
#define CACHE_SETCACHEDMOVIE  "SetCachedMovie"

#define GOLF_SUBTITLE_PATH  "/rw_data/app/golf/subtitle/"

namespace MM = ::v1::org::genivi::mediamanager;

using boost::property_tree::ptree;
using namespace genivimedia;

static const std::string kFilePrefix("file://");

namespace lge {
namespace mm {
namespace player {
std::string PlayerProvider::sender_name_;

PlayerProvider::PlayerProvider(std::shared_ptr<command::Queue>& sp_command_queue)
    : ServiceProvider("com.lge.PlayerEngine"),
        stub(nullptr),
        playerengine_proxy_{nullptr},
        sig_id_state_change_{0},
        gbus_sync_connection_{nullptr},
        command_queue_(sp_command_queue),
        event_system_(sp_command_queue),
        sc_notifier_(),
        /* playlist_mgr_(), */
        audio_tracks_(),
        subtitle_tracks_(),
        pe_proxy_map_(),
        uri_last_open_(),
        uri_transcode_output_(),
        codec_type_(MM::PlayerTypes::ACodecType::AAC),
        file_type_(MM::PlayerTypes::FileFormatType::MP4),
        file_location(),
        platform_name(),
        need_to_open_when_play_{false},
        track_opened_(),
        media_type_(MM::PlayerTypes::MediaType::AUDIO),
        playback_option_(),
        video_window_backup_{11000, 0, 0, 1280, 720, 0, 7, "", ""},
        state_{State::Stopped},
        is_playing_(false),
        is_need_new_index_(false),
        is_Error_state(false),
        is_EOS_state_(false),
        is_need_pause_(false),
        seeking_position_idx(-1),
        last_fail_media_id_(0),
        multi_channel_media_id_(0),
        multi_channel_media_type_(""),
        clean_connection_(""),
        single_connection_name_(""),
        playback_blocked_(false),
        play_direction_(Direction::Forward),
        updated_current_time_since_trickplay_(false),
        saturation_(0xdeadbeef),
        brightness_(0xdeadbeef),
        contrast_(0xdeadbeef),
        saturation_2nd_(0xdeadbeef),
        brightness_2nd_(0xdeadbeef),
        contrast_2nd_(0xdeadbeef),
        cache_path_buffer_(""),
        pe_audio_slot_map_(),
        audio_slot_num_(""),
        audio_slot_num_2ch_(""),
        audio_slot_num_6ch_(""),
        speed_ignore_buffering_map_() {
    MMLogInfo("");

    try{
        event_system_.Init();
    }catch(...) {
        MMLogInfo("Failed to initialize event_system");
    }

    //playlist::PlaylistOption::set_platform(Option::platform());
    if (!Option::platform().compare("B_AVN")) {
        playback_option_.set_potion_on_stop_state = false;
    }
}

PlayerProvider::~PlayerProvider() {
    MMLogInfo("");
}

void PlayerProvider::ServiceRegistered() {
    MMLogDebug("");

    sc_notifier_.SetStub(stub);

    GlibHelper::CallAsync(event_system_.GetGMainContext(), [this]() -> gboolean {
        MMLogInfo("ServiceRegistered Callback");
        //resetPlaylistManager();
        return FALSE;
    });
}

void PlayerProvider::PEDestroyed(int destroy_mid) {
    MMLogInfo("id=[%d]", destroy_mid);
    last_fail_media_id_ = destroy_mid;

    GlibHelper::CallAsync(event_system_.GetGMainContext(), [this]() -> gboolean {
        MMLogError("PEDestroyed Callback[%d]", last_fail_media_id_);
        event_system_.SetEvent(command::EventType::ErrorOccured, nullptr);

        ptree pt;
        pt.put("ErrorCode", ERROR_GST_INTERNAL_ERROR);
        if (last_fail_media_id_ > 0)
            onPEDestroyed(pt, last_fail_media_id_);
        return FALSE;
    });
}

uint32_t PlayerProvider::getMediaID(std::string connectionName) {
    std::map<int, std::string>::iterator it;
    for (it = connection_map_.begin(); it != connection_map_.end(); it++) {
        if (it->second == connectionName) {
            return it->first;
        }
    }
    return -1;
}

int32_t PlayerProvider::getPositionAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::Position> pos_t = stub->getPositionAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Position>::iterator itr;

    for(itr = pos_t.begin(); itr != pos_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - pos_t.begin());
        }
    }
    return -1;
}

int32_t PlayerProvider::getDurationAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::Duration> dur_t = stub->getDurationAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Duration>::iterator itr;

    for(itr = dur_t.begin(); itr != dur_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - dur_t.begin());
        }
    }
    return -1;
}

int32_t PlayerProvider::getBufferingAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::Buffering> buf_t = stub->getBufferingAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Buffering>::iterator itr;

    for(itr = buf_t.begin(); itr != buf_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - buf_t.begin());
        }
    }
    return -1;
}

int32_t PlayerProvider::getCurrentTrackAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::Track> track_t = stub->getCurrentTrackAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Track>::iterator itr;

    for(itr = track_t.begin(); itr != track_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - track_t.begin());
        }
    }
    return -1;
}

int32_t PlayerProvider::getMuteAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::MuteOption> mute_t = stub->getMuteAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::MuteOption>::iterator itr;

    for(itr = mute_t.begin(); itr != mute_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - mute_t.begin());
        }
    }
    return -1;
}

int32_t PlayerProvider::getRateAttrIdx(std::string connectionName) {

    _attribute_mutex.lock();
    std::vector<MM::PlayerTypes::Rate> rate_t = stub->getRateAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Rate>::iterator itr;

    MMLogError("[getRateAttrIdx] Rate list size[%lu]", rate_t.size());
    for(itr = rate_t.begin(); itr != rate_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             _attribute_mutex.unlock();
             return (itr - rate_t.begin());
        }
    }
    _attribute_mutex.unlock();
    return -1;
}

int32_t PlayerProvider::getSpeedAttrIdx(std::string connectionName) {

    _attribute_mutex.lock();
    std::vector<MM::PlayerTypes::Speed> speed_t = stub->getSpeedAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Speed>::iterator itr;

    MMLogError("[getSpeedAttrIdx] Speed list size[%lu]", speed_t.size());
    for(itr = speed_t.begin(); itr != speed_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             _attribute_mutex.unlock();
             return (itr - speed_t.begin());
        }
    }
    _attribute_mutex.unlock();
    return -1;
}

int32_t PlayerProvider::getPlaybackAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::Playback> playback_t = stub->getPlaybackAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Playback>::iterator itr;

    for(itr = playback_t.begin(); itr != playback_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - playback_t.begin());
        }
    }
    return -1;
}

int32_t PlayerProvider::getVolumeAttrIdx(std::string connectionName) {
    std::vector<MM::PlayerTypes::Volume> volume_t = stub->getVolumeAttribute();
    uint32_t media_id = getMediaID(connectionName);
    std::vector<MM::PlayerTypes::Volume>::iterator itr;

    for(itr = volume_t.begin(); itr != volume_t.end(); itr++) {
        if(itr->getMedia_id() == media_id) {
             return (itr - volume_t.begin());
        }
    }
    return -1;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::OpenUriCommand* command) {
    uint32_t index = command->track.getIndex();
    std::string uri = command->track.getUri();
    bool is_dsd = false;
    //bool skip_wait = false;
    MM::PlayerTypes::MediaType current_media_type = command->media_type;
    media_type_ = current_media_type;

    if (last_fail_media_id_ > 0 && last_fail_media_id_ == (int)command->media_id) {
        MMLogError("[OpenUriCommand] Already destroyed PE - skip command");
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        return false;
    }
    std::string connectionName = command->connectionName;
    connection_map_ = command->connectionMap;
    if (current_media_type == MM::PlayerTypes::MediaType::AUDIO || current_media_type == MM::PlayerTypes::MediaType::VIDEO ||
        current_media_type == MM::PlayerTypes::MediaType::USB_AUDIO1 || current_media_type == MM::PlayerTypes::MediaType::USB_VIDEO1 ||
        current_media_type == MM::PlayerTypes::MediaType::USB_AUDIO2 || current_media_type == MM::PlayerTypes::MediaType::USB_VIDEO2 ||
        current_media_type == MM::PlayerTypes::MediaType::MOOD_THERAPY_AUDIO ||
        current_media_type == MM::PlayerTypes::MediaType::MOOD_THERAPY_VIDEO ||
        current_media_type == MM::PlayerTypes::MediaType::RECORDING_PLAY) {
        single_connection_name_ = connectionName;
    }
    int proxyId = preparePEProxy(connectionName, true);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }
#if 0
    static std::vector<command::CommandType> cv = { command::CommandType::Stop };
    if (command_queue_->Exist(cv, command->connectionName) && IsStreamingType(current_media_type)) {
        MMLogInfo("[OpenUriCommand] " "skip wait event for processing stop early.");
        //skip_wait = true;
    }
#endif
    MMLogInfo("[OpenUriCommand] " "index: %d, uri: %s, pos_us: %llu, channel: %u", index , uri.c_str(), command->pos_us, command->channel_num);
    if ((uri.rfind(".dff") != std::string::npos) || (uri.rfind(".dsf") != std::string::npos)) { // To Do: XXX
        is_dsd = true;
    }
#if 0
    // set this track to be loaded
    if (command->from_hmi) {
        if (stub->getRepeatOptionAttribute().getStatus() == MM::PlayerTypes::RepeatStatus::REPEAT_DIRECTORY) {
            std::string directory;
            std::string filename;
            playlist::ParseUtils::ParseUri(uri, directory, filename);
            playlist_mgr_->SelectDirectory(directory);
        }
        playlist::Track t(index, uri, "");
        boost::optional<playlist::Track> track = playlist_mgr_->SelectTrack(t);

        uri_last_open_ = uri;
        if (!track) {
            MMLogWarn("[OpenUriCommand] " "selected track is not in playlist");
            is_need_new_index_ = true;
        } else {
            // update index when do openUri with uri. INVALID_INDEX -> index or 0.
            MMLogInfo("[OpenTrackCommand] " "selected track index is updated[%u]", (*track).index());
            command->track.setIndex((*track).index());
            is_need_new_index_ = false;
        }
    } else {
        is_need_new_index_ = false; // Default value, already has own index.
        uri_last_open_.clear();
    }
#endif
    need_to_open_when_play_[proxyId] = false;
    int32_t Idx = getCurrentTrackAttrIdx(connectionName);
    if (Idx < stub->getCurrentTrackAttribute().size() && Idx != -1) {
        track_opened_.erase(track_opened_.begin()+Idx);
        track_opened_.insert(track_opened_.begin()+Idx, command->track);
    } else {
        track_opened_.push_back(command->track);
    }
    stub->setCurrentTrackAttribute(track_opened_);

    Idx = getCurrentTrackAttrIdx(connectionName);
    if (Idx > -1)
        sc_notifier_.NotifyCurrentTrack(track_opened_[Idx], getMediaID(connectionName));
    std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
    Idx = getPositionAttrIdx(connectionName);
    if (Idx <= -1) {
        MM::PlayerTypes::Position pos_t(command->pos_us, getMediaID(connectionName), true);
        pos_list.push_back(pos_t);
        /* Now the index will be the last element */
        Idx = pos_list.size() - 1;
    } else {
        pos_list[Idx].setMedia_id(getMediaID(connectionName));
        pos_list[Idx].setPosition(command->pos_us);
    }
    for (auto itr = pos_list.begin(); itr != pos_list.end(); itr++) {
        if ((itr - pos_list.begin()) != Idx) {
            itr->setActive(false);
        } else {
            if (Idx != (pos_list.size() - 1)) // For ignoring sending 0 position
                itr->setActive(true);
            else
                itr->setActive(false);
        }
    }
    stub->setPositionAttribute(pos_list);

    // set_uri option : json string
    boost::property_tree::ptree tree;
    boost::property_tree::ptree sub_tree;
    std::stringstream ss;
    std::string mediatype = MediaTypeToString(current_media_type);
    bool cache = IsCacheNeeded(current_media_type);

    if (mediatype.size() > 0)
        sub_tree.put("mediatype", mediatype);
    if (mediatype.compare("video") == 0) { //store B/S/C data for USB Video case only
        if (fabs(saturation_ - 0xdeadbeef) > DBL_EPSILON)
            sub_tree.put("saturation", saturation_);
        if (fabs(brightness_ - 0xdeadbeef) > DBL_EPSILON)
            sub_tree.put("brightness", brightness_);
        if (fabs(contrast_ - 0xdeadbeef) > DBL_EPSILON)
            sub_tree.put("contrast", contrast_);
    } else if (mediatype.compare("video_2nd") == 0) { //store B/S/C data for USB Video2nd case only
        if (fabs(saturation_2nd_ - 0xdeadbeef) > DBL_EPSILON)
            sub_tree.put("saturation", saturation_2nd_);
        if (fabs(brightness_2nd_ - 0xdeadbeef) > DBL_EPSILON)
            sub_tree.put("brightness", brightness_2nd_);
        if (fabs(contrast_2nd_ - 0xdeadbeef) > DBL_EPSILON)
            sub_tree.put("contrast", contrast_2nd_);
    }
    if (current_media_type == MM::PlayerTypes::MediaType::STREAM)
        addRecorderOption(sub_tree);

    sub_tree.put("cache", cache);
    sub_tree.put("show-preroll-frame", command->show_preroll_frame);
    sub_tree.put("provide-global-clock", command->provide_global_clock);
    sub_tree.put("is-dsd", is_dsd);
    /*
    if ((multi_channel_media_id_ != getMediaID(connectionName)) &&(multi_channel_media_id_ > 0)) {
        MMLogWarn("[%d] already occupied multi channel alsa. Try to open 2ch alsa slot", multi_channel_media_id_);
        command->channel_num = 2;
    }*/
    sub_tree.put("channel", command->channel_num);

    getSubtitlePath(uri, current_media_type, sub_tree);
    if (current_media_type == MM::PlayerTypes::MediaType::GOLF_VIDEO) {
        MMLogInfo("get cached golf video path..");
        sendDbusCaching(Caching::GetCachedMovie, uri);
    }

    audio_slot_num_6ch_ = "_";
    audio_slot_num_2ch_ = "_";
    audio_slot_num_ = "_";
    if (uri.length() > 2 && uri[0] == '<' && uri[2] == '>') { // ccRC project needs this logic
      MMLogInfo("6ch_slot=[%c]", uri[1]);
      sub_tree.put("6ch_slot", uri[1]);
      audio_slot_num_6ch_ = "6ch_";
      audio_slot_num_6ch_.push_back(uri[1]);
      uri.erase(0, 3);
    }
    if (uri.length() > 2 && uri[0] == '[' && uri[2] == ']') {
      MMLogInfo("2ch_slot=[%c]", uri[1]);
      sub_tree.put("2ch_slot", uri[1]);
      audio_slot_num_2ch_ = "2ch_";
      audio_slot_num_2ch_.push_back(uri[1]);
      uri.erase(0, 3);
    }

    tree.add_child("Option", sub_tree);
    boost::property_tree::json_parser::write_json(ss, tree, false);

    GError *dbus_error = NULL;
    GError *error = NULL;
#ifdef PLATFORM_CCIC 
//ccIC의 경우 6ch은 무조건 0번 슬롯을 사용하기때문에 2ch slot 정보만 보내고 있음.
    audio_slot_num_6ch_ = "6ch_0";
    gint result_channel = 0;
    MMLogInfo("call getChannelInfo in sync");
    com_lge_player_engine_call_get_channel_info_sync(
        playerengine_proxy_[proxyId],
        uri.c_str(),
        ss.str().c_str(),
        &result_channel,
        NULL,
        &dbus_error
    );
    MMLogInfo("get result from getChannelInfo in sync");
    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        MMLogError("GError found - [%s]", dbus_error->message);
        g_error_free(dbus_error);
        state_[proxyId] = State::Stopped;

        return false;
    }

    audio_slot_num_ = result_channel > 5 ? audio_slot_num_6ch_ : result_channel > 0 ? audio_slot_num_2ch_ : "_";
    if(audio_slot_num_[0] == '_') {
        MMLogError("Failed to get slot");
    } else if (result_channel > 5){
        MMLogInfo("if 6ch slot is being used, it will be downmixed");
        auto pe_audio_slot_iter = pe_audio_slot_map_.find(audio_slot_num_);
        if(pe_audio_slot_iter != pe_audio_slot_map_.end()) {
            if(pe_audio_slot_iter->second != connectionName) {
                MMLogInfo("Force downmix 6ch to 2ch");
                command::SwitchChannelCommand sc(this, pe_audio_slot_iter->second, true, nullptr);
                sc.Execute(in);
            }
        }
    } else {
        auto pe_audio_slot_iter = pe_audio_slot_map_.find(audio_slot_num_);
        if(pe_audio_slot_iter != pe_audio_slot_map_.end()) {
            if(pe_audio_slot_iter->second != connectionName) {
                int slot_proxyId = preparePEProxy(pe_audio_slot_iter->second, false);
                if (slot_proxyId <= -1) {
                    MMLogInfo("there is no pe proxy Id for audio slot %s", audio_slot_num_.c_str());
                } else {
                    command::StopCommand sc(this, false, pe_audio_slot_iter->second, getMediaID(pe_audio_slot_iter->second), nullptr);
                    sc.Execute(in);
                }
            }
        }
    }
#endif
    // async call
    com_lge_player_engine_call_set_uri(
        playerengine_proxy_[proxyId],
        uri.c_str(),
        ss.str().c_str(),
        NULL,
        (GAsyncReadyCallback)PlayerProvider::callAsyncCallback,
        (gpointer*)new std::pair<PlayerProvider*, command::BaseCommand*>(this, command)
    );

    //if (!skip_wait)
        event_system_.WaitEvent(in, (uint32_t)command::EventType::CallAsync, 0);
    // async call result
    dbus_error = NULL;
    gboolean succeed = FALSE;
    com_lge_player_engine_call_set_uri_finish(
        playerengine_proxy_[proxyId],
        &succeed,
        command->async_result,
        &dbus_error
    );
    g_object_unref(command->async_result);

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        MMLogError("GError found - [%s]", dbus_error->message);
        g_error_free(dbus_error);
        goto EXIT_ERROR;
    }

    if (succeed == FALSE) {
        goto EXIT_ERROR;
    } else {
#ifdef PLATFORM_CCIC
        /* mapping audio slot with pe connection */
        if(audio_slot_num_[0] != '_') {
            pe_audio_slot_map_.insert({audio_slot_num_, connectionName});
            if(result_channel > 5)
                pe_audio_slot_map_.insert({audio_slot_num_2ch_, connectionName});
        }
#endif
        /* Create vector index for duration attribute */
        std::vector<MM::PlayerTypes::Duration> dur_list = stub->getDurationAttribute();
        Idx = getDurationAttrIdx(connectionName);
        if (Idx == -1 || dur_list.size() == 0) {
            MM::PlayerTypes::Duration dur_t(0, getMediaID(connectionName), true);
            dur_list.push_back(dur_t);
            /* Now the index will be the last element */
            Idx = dur_list.size() - 1;
        } else {
            dur_list[Idx].setMedia_id(getMediaID(connectionName));
            dur_list[Idx].setDuration(0);
        }

        for (auto itr = dur_list.begin(); itr != dur_list.end(); itr++) {
            if ((itr - dur_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setDurationAttribute(dur_list);

        /* Create vector index for buffering attribute */
        std::vector<MM::PlayerTypes::Buffering> buf_list = stub->getBufferingAttribute();
        Idx = getBufferingAttrIdx(connectionName);
        if (Idx == -1 || buf_list.size() == 0) {
            MM::PlayerTypes::Buffering buf_t(100/*progress*/, getMediaID(connectionName), false);
            buf_list.push_back(buf_t);
            /* Now the index will be the last element */
            //Idx = buf_list.size() - 1;
        } else {
            buf_list[Idx].setMedia_id(getMediaID(connectionName));
            buf_list[Idx].setProgress(100);
        }

        for (auto itr = buf_list.begin(); itr != buf_list.end(); itr++) {
            itr->setActive(false); // We don't need to notify this here
        }
        stub->setBufferingAttribute(buf_list);

        /* Create vector index for Mute attribute */
        std::vector<MM::PlayerTypes::MuteOption> mute_list = stub->getMuteAttribute();
        Idx = getMuteAttrIdx(connectionName);
        if (Idx == -1 || mute_list.size() == 0) {
            MM::PlayerTypes::MuteOption mute_t(MM::PlayerTypes::MuteStatus::UNMUTED, getMediaID(connectionName), true);
            mute_list.push_back(mute_t);
            /* Now the index will be the last element */
            Idx = mute_list.size() - 1;
        } else {
            mute_list[Idx].setMedia_id(getMediaID(connectionName));
            // For preserving previous mute status, we don't need to set up
            //mute_list[Idx].setStatus(MM::PlayerTypes::MuteStatus::UNMUTED);
        }

        for (auto itr = mute_list.begin(); itr != mute_list.end(); itr++) {
            if ((itr - mute_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setMuteAttribute(mute_list);

        /* Create vector index for Rate attribute */
        MMLogInfo("Mutex lock() in rate_list before inserting it");
        std::vector<MM::PlayerTypes::Rate> rate_list = stub->getRateAttribute();
        Idx = getRateAttrIdx(connectionName);
        if (Idx == -1 || rate_list.size() == 0) {
            MMLogError("Index -1 or Size = 0, Insert rate_list[0] rate=1  to vector");
            MM::PlayerTypes::Rate rate_t(1.0, getMediaID(connectionName), true);
            rate_list.push_back(rate_t);
            /* Now the index will be the last element */
            Idx = rate_list.size() - 1;
        } else {
            MMLogError("Insert rate_list[%d] rate=1 to vector", Idx);
            rate_list[Idx].setMedia_id(getMediaID(connectionName));
            rate_list[Idx].setRate(1.0);
        }

        for (auto itr = rate_list.begin(); itr != rate_list.end(); itr++) {
            if ((itr - rate_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setRateAttribute(rate_list);

        /* Create vector index for Volume attribute */
        std::vector<MM::PlayerTypes::Volume> volume_list = stub->getVolumeAttribute();
        Idx = getVolumeAttrIdx(connectionName);
        if (Idx == -1 || volume_list.size() == 0) {
            MM::PlayerTypes::Volume volume_t(10.0, getMediaID(connectionName), true);
            volume_list.push_back(volume_t);
            /* Now the index will be the last element */
            Idx = volume_list.size() - 1;
        } else {
            volume_list[Idx].setMedia_id(getMediaID(connectionName));
            volume_list[Idx].setVolume(1.0);
        }

        for (auto itr = volume_list.begin(); itr != volume_list.end(); itr++) {
            if ((itr - volume_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setVolumeAttribute(volume_list);

        /* Create vector index for Playback attribute */
        std::vector<MM::PlayerTypes::Playback> playback_list = stub->getPlaybackAttribute();
        Idx = getPlaybackAttrIdx(connectionName);
        if (Idx == -1 || playback_list.size() == 0) {
            MM::PlayerTypes::Playback playback_t(MM::PlayerTypes::PlaybackStatus::UNINIT, getMediaID(connectionName), true);
            playback_list.push_back(playback_t);
            /* Now the index will be the last element */
            Idx = playback_list.size() - 1;
        } else {
            playback_list[Idx].setMedia_id(getMediaID(connectionName));
            playback_list[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::UNINIT);
        }
        MMLogInfo("getPlaybackAttribute Idx = [%d]", Idx);
        for (auto itr = playback_list.begin(); itr != playback_list.end(); itr++) {
            if ((itr - playback_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setPlaybackAttribute(playback_list);

        /*
        if (command->channel_num > 2 && multi_channel_media_id_ == 0) {
            MMLogInfo("multi channel source is requested.. set to [%u]", command->media_id);
            multi_channel_media_id_ = (int)command->media_id;
            multi_channel_media_type_ = mediatype;
        }*/

        /* Create vector index for Speed attribute */
        MMLogInfo("Mutex lock() in speed_list before inserting it");
        std::vector<MM::PlayerTypes::Speed> speed_list = stub->getSpeedAttribute();
        Idx = getSpeedAttrIdx(connectionName);
        if (Idx == -1 || speed_list.size() == 0) {
            MMLogError("Index -1 or Size = 0, Insert speed_list[0] rate=1  to vector");
            MM::PlayerTypes::Speed speed_t(1.0, getMediaID(connectionName), true);
            speed_list.push_back(speed_t);
            /* Now the index will be the last element */
            Idx = speed_list.size() - 1;
        } else {
            MMLogError("Insert speed_list[%d] rate=1 to vector", Idx);
            speed_list[Idx].setMedia_id(getMediaID(connectionName));
            speed_list[Idx].setSpeed(1.0);
        }

        for (auto itr = speed_list.begin(); itr != speed_list.end(); itr++) {
            if ((itr - speed_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setSpeedAttribute(speed_list);
    }/* if end */

    if (!IsStreamingType(current_media_type)) {
        if (command->reply) {
            command->reply(MM::PlayerTypes::PlayerError::NO_ERROR, command->media_id);
            command->reply = NULL;
        }
        //if (event_system_.WaitEvent(in, (uint32_t)command::EventType::SourceInfo, (uint32_t)command::EventType::ErrorOccured) == false)
        //    goto EXIT_ERROR;
    }
    state_[proxyId] = State::Paused;

    return true;

EXIT_ERROR:
    state_[proxyId] = State::Stopped;

    return false;
}

#if 0
bool PlayerProvider::process(command::Coro::pull_type& in, command::OpenTrackCommand* command) {
    uint32_t index;
    std::string uri;
    std::string connectionName = command->connectionName;

    if (command->open_track_param.isType<std::string>()) {
        index = playlist::Track::INVALID_INDEX;
        uri = command->open_track_param.get<std::string>();
        MMLogInfo("[OpenTrackCommand] " "index: %d, uri: %s", index, uri.c_str());
    } else {
        index = command->open_track_param.get<uint64_t>();

        playlist::Track t(index, "", "");
        boost::optional<playlist::Track> track = playlist_mgr_->SelectTrack(t);

        if (!track) {
            MMLogWarn("[OpenTrackCommand] " "invalid track");
            // TODO: need to set command->e
            return false;
        }

        index = (*track).index();
        uri = (*track).GetInfo(playlist::Track::InfoType::Uri);
        MMLogInfo("[OpenTrackCommand] " "index: %d, uri: %s", index, uri.c_str());
    }
    command::OpenUriCommand ouc(this, index, uri, true, connection_map_, getMediaID(connectionName), 0, connectionName, nullptr);
    ouc.from_hmi = command->from_hmi;
    ouc.show_preroll_frame = command->show_preroll_frame;
    ouc.pos_us = command->pos_us;
    if (ouc.Execute(in) == false) {
        command->e = ouc.e;
        return false;
    }

    return true;
}
#endif
bool PlayerProvider::process(command::Coro::pull_type& in, command::PauseCommand* command) {
    MMLogInfo("[PauseCommand] " "");

    _mutex.lock();
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        _mutex.unlock();
        return false;
    }

    MMLogInfo("[PauseCommand] " "state : %d", state_[proxyId]);
    State prev_state = state_[proxyId];     //prev_state need to be checked for multiple playback
    int32_t rate_index = getRateAttrIdx(connectionName);
    if (rate_index < 0) {
        MMLogInfo("Invalid rate_index");
        _mutex.unlock();
        return false;
    }
    double prev_rate = stub->getRateAttribute()[rate_index].getRate();
    is_need_pause_ = (is_EOS_state_ || is_Error_state) ? true : false;

    if ((fabs(prev_rate - 0.0) > DBL_EPSILON) && (fabs(prev_rate - 1.0) > DBL_EPSILON)) {
        stopTrickPlay(in, IsVideoType(media_type_), connectionName);
    }

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;

    com_lge_player_engine_call_pause_sync(
        playerengine_proxy_[proxyId],
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    if (succeed == TRUE) {
        // check need to set PlaybackStatus attribute
        if ((fabs(prev_rate - 1.0) > DBL_EPSILON) || prev_state == State::Paused) {
            MMLogInfo("[PauseCommand] " "PlaybackStatus is Paused");
            std::vector<MM::PlayerTypes::Playback> pb_t = stub->getPlaybackAttribute();
            std::vector<MM::PlayerTypes::Playback>::iterator itr;
            int32_t Idx = getPlaybackAttrIdx(connectionName);

            if (Idx > -1 && pb_t.size() > 0) {
                pb_t[Idx].setMedia_id(getMediaID(connectionName));
                pb_t[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::PAUSED);
                for(itr = pb_t.begin(); itr != pb_t.end(); itr++) {
                    if((itr - pb_t.begin()) == Idx) {
                        itr->setActive(true);
                    } else {
                        itr->setActive(false);
                    }
                }
                stub->setPlaybackAttribute(pb_t);
            }
        }

        state_[proxyId] = State::Paused;
    } else if (prev_state == State::Stopped) {
        // may be already released or error handling case -> app needs status changed event
        std::vector<MM::PlayerTypes::Playback> pb_t = stub->getPlaybackAttribute();
        std::vector<MM::PlayerTypes::Playback>::iterator itr;
        int32_t Idx = getPlaybackAttrIdx(connectionName);

        if (Idx > -1 && pb_t.size() > 0) {
            pb_t[Idx].setMedia_id(getMediaID(connectionName));
            pb_t[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::PAUSED);
            for(itr = pb_t.begin(); itr != pb_t.end(); itr++) {
                if((itr - pb_t.begin()) == Idx) {
                    itr->setActive(true);
                } else {
                    itr->setActive(false);
                }
            }
            stub->setPlaybackAttribute(pb_t);
        }
    } else {
        // may be null pipeline.
        state_[proxyId] = State::Stopped;
    }

    is_playing_ = false;
    _mutex.unlock();

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::PlayCommand* command) {
    // skip commands
    static std::vector<command::CommandType> cv = { command::CommandType::Play };
    std::string connectionName = command->connectionName;

    if (command_queue_->Exist(cv, connectionName)) {
        MMLogInfo("[PlayCommand] " "skip command");
        return true;
    }

    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }
#if 0
    // check if it is needed to open uri.
    if (need_to_open_when_play_[proxyId]) {
        MMLogInfo("[PlayCommand] " "track is changed or stopped");

        boost::optional<playlist::Track> track = playlist_mgr_->SeekTrack(0);
        if (!track) {
            MMLogWarn("[PlayCommand] " "invalid track");
            return false;
        }

        uint32_t index = (*track).index();
        std::string uri = (*track).GetInfo(playlist::Track::InfoType::Uri);

        is_playing_ = true;
        command::OpenUriCommand ouc(this, index, uri, true, connection_map_, getMediaID(connectionName), 0, connectionName, nullptr);
        if (ouc.Execute(in) == false) {
            command->e = ouc.e;
            return false;
        }
    } else if (state_[proxyId] == State::Stopped) {
        MMLogInfo("[PlayCommand] " "Stopped. skip command");
        return true;
    }
#endif
    MMLogInfo("[PlayCommand] " "");

    // block playback if needed.
    if (blockPlayback()) {
        MMLogInfo("[PlayCommand] " "blockPlayback(), so skip");
        return true;
    }

    playback_blocked_ = false;

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    com_lge_player_engine_call_play_sync(
        playerengine_proxy_[proxyId],
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    if (succeed == TRUE) {
        is_playing_ = true;
        state_[proxyId] = State::Playing;
    }
    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::PlayPauseCommand* command) {
    bool succeed;
    std::string connectionName = command->connectionName;
    MMLogInfo("[PlayPauseCommand] " "");

    if (is_playing_ == true) {
        command::PauseCommand pc(this, connectionName, nullptr);
        succeed = pc.Execute(in);
    } else {
        command::PlayCommand pc(this, connectionName, nullptr);
        succeed = pc.Execute(in);
    }

    return succeed;
}
#if 0
bool PlayerProvider::process(command::Coro::pull_type& in, command::NextCommand* command) {
    MMLogInfo("[NextCommand] " "pos: %d, need_index=[%d]", command->pos, is_need_new_index_);
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    bool pause_after_next = false; // For EOS pause case (BAVN-6023)
    std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
    int32_t Idx = getPositionAttrIdx(connectionName);
    if (Idx > -1 && pos_list.size() > 0) {
        uint64_t last_position = pos_list[Idx].getPosition();
        MMLogInfo("Last position = %d", last_position);
        pos_list[Idx].setPosition(0);
        pos_list[Idx].setMedia_id(getMediaID(connectionName));
        for(auto itr : pos_list) {
            itr.setActive(false);
        }
        pos_list[Idx].setActive(true);
        stub->setPositionAttribute(pos_list);

        if (playback_option_.set_position_on_prev) {
            bool goto_begin_of_track =
                command->pos == -1
                && command->force == false
                && last_position >= playback_option_.set_position_on_prev_threshold_us;

            if (goto_begin_of_track) {
                // go to begin of stream.
                MMLogInfo("[NextCommand] " "pos == -1 and position >= %lld us", playback_option_.set_position_on_prev_threshold_us);

                command::SetPositionCommand spc(this, false, 0, connectionName, nullptr);
                spc.Execute(in);

                if (playback_option_.auto_resume_on_next_or_prev) {
                    command::PlayCommand pc(this, connectionName, nullptr);
                    pc.Execute(in);
                }

                return true;
            }
        }
    }

    if (command->pos == 2 || command->pos == -2) { // Need pause after next command (BAVN-6023,6776)
        pause_after_next = true;
        is_playing_ = false; // For safety
        if (command->pos == 2) command->pos = 1;
        if (command->pos == -2) command->pos = -1;
    }

    if (command->pos > 0 && playback_option_.auto_stop_command_on_last_list && playlist_mgr_->IsLastOfTracks()) {
        MMLogInfo("[NextCommand] " "last track on stop mode enabled");
        command::StopCommand sc(this, false, connectionName, nullptr);
        sc.Execute(in);

        onPlaybackStopped();
        need_to_open_when_play_[proxyId] = false;
        state_[proxyId] = State::Stopped;
        return true;
    }

    if (IsVideoType(media_type_)) {
        MMLogInfo("[NextCommand] " "stop video before next/prev command");
        command::StopCommand sc(this, false, connectionName, nullptr);
        sc.Execute(in);
        state_[proxyId] = State::Stopped;
    }

    // normal next track
    boost::optional<playlist::Track> track;
    if (is_need_new_index_ && !uri_last_open_.empty()) {
        MMLogInfo("[NextCommand] " "update new index - [%s]", uri_last_open_.c_str());
        track = playlist_mgr_->SelectTrackByUri(uri_last_open_);
    }
    track = playlist_mgr_->SeekTrack(command->pos);

    need_to_open_when_play_[proxyId] = true;

    // skip duplicate commands
    static std::vector<command::CommandType> cv = { command::CommandType::Next,
                                                    command::CommandType::Previous };
    if (command_queue_->Exist(cv, command->connectionName)) {
        MMLogInfo("[NextCommand] " "skip command");
        return true;
    }

    if (!track) {
        MMLogWarn("[NextCommand] " "Invalid Track");
        return false;
    }

    uint32_t index = (*track).index();
    std::string uri = (*track).GetInfo(playlist::Track::InfoType::Uri);
    MMLogInfo("[NextCommand] " "Next Track-> index: %d, uri: %s", index, uri.c_str());

    if (playback_option_.auto_resume_on_next_or_prev && !pause_after_next) {
        is_playing_ = true;
    }

    if (is_playing_ == false) {
        MMLogInfo("[NextCommand] " "paused, Current Track-> index: %d, uri: %s", index, uri.c_str());

        command::OpenUriCommand ouc(this, index, uri, true, connection_map_, getMediaID(connectionName), 0, connectionName, nullptr);
        if (ouc.Execute(in) == false) {
            command->e = ouc.e;
            return false;
        }
        return true;
    }

    if (command->pos >= 0)
        play_direction_ = Direction::Forward;
    else
        play_direction_ = Direction::Backward;

    // play
    command::PlayCommand pc(this, connectionName, nullptr);
    if (pc.Execute(in) == false)
        return false;

    play_direction_ = Direction::Forward;

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::NextAndTrickCommand* command) {
    MMLogInfo("[NextAndTrickCommand] " "pos: %d, rate: %f", command->pos, command->rate);
    int proxyId = preparePEProxy(sender_name_);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    // normal next track
    boost::optional<playlist::Track> track = playlist_mgr_->SeekTrack(command->pos);
    need_to_open_when_play_[proxyId] = true;

    if (!track) {
        MMLogWarn("[NextAndTrickCommand] " "Invalid Track");
        return false;
    }

    uint32_t index = (*track).index();
    std::string uri = (*track).GetInfo(playlist::Track::InfoType::Uri);

    if (command->pos >= 0)
        play_direction_ = Direction::Forward;
    else
        play_direction_ = Direction::Backward;

    // open uri
    command::OpenUriCommand ouc(this, index, uri, true, connection_map_, getMediaID(sender_name_), 0, sender_name_,  nullptr);
    if (ouc.Execute(in) == false) {
        return false;
    }

    int32_t Idx = getDurationAttrIdx(sender_name_);
    if (command->pos < 0 && Idx > -1) {
        // seek to the end of stream
        uint64_t new_position = stub->getDurationAttribute()[Idx].getDuration();
        if (new_position < TimeConvert::SecToUs(3)) {
            MMLogWarn("[NextAndTrickCommand] " "duration is smaller than 3 seconds");
            return false;
        }
        new_position -= TimeConvert::SecToUs(3);
        command::SetPositionCommand spc(this, false, new_position, sender_name_, nullptr);
        if (spc.Execute(in) == false)
            return false;
    }

    // play with trick mode
    command::SetRateCommand src(this, command->rate, sender_name_);
    if (src.Execute(in) == false)
        return false;

    play_direction_ = Direction::Forward;

    return true;
}
#endif

bool PlayerProvider::process(command::Coro::pull_type& in, command::SeekCommand* command) {
    static std::vector<command::CommandType> cv = { command::CommandType::Seek,
                                                    command::CommandType::SetPosition };
    std::string connectionName = command->connectionName;
    if (command_queue_->Exist(cv, connectionName)) {
        MMLogInfo("[SeekCommand] " "skip command");
        return true;
    }

    MMLogInfo("[SeekCommand] " "pos_us : %lld", command->pos_us);

    int32_t rate_index = getRateAttrIdx(connectionName);
    if (rate_index < 0) {
        MMLogInfo("Invalid rate_index");
        return false;
    }
    double rate = stub->getRateAttribute()[rate_index].getRate();
    if (command->from_hmi && (fabs(rate - 1.0) > DBL_EPSILON)) {
        MMLogInfo("[SeekCommand] " "rate != 1.0");
        return false;
    }

    int32_t Idx = getPositionAttrIdx(connectionName);
    if (Idx < 0) {
        MMLogInfo("Invalid position index");
        return false;
    }
    uint64_t new_pos_us = stub->getPositionAttribute()[Idx].getPosition();

    Idx = getDurationAttrIdx(connectionName);
    if (Idx < 0) {
        MMLogInfo("Invalid duration index");
        return false;
    }
    uint64_t duration = stub->getDurationAttribute()[Idx].getDuration();

    if (command->pos_us < 0 && (uint64_t)abs(command->pos_us) > new_pos_us) {
        MMLogWarn("[SeekCommand] " "out of range ( pos_us : %lld is bigger than position %llu )", command->pos_us, new_pos_us);
        return false;
    }

    new_pos_us += command->pos_us;

    if (new_pos_us > duration) {
        MMLogWarn("[SeekCommand] " "out of range ( new_pos_us : %llu is bigger than duration %llu )", new_pos_us, duration);
        return false;
    }

    std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
    Idx = getPositionAttrIdx(connectionName);

    if (Idx > -1 && pos_list.size() > 0) {
        pos_list[Idx].setPosition(new_pos_us);
        pos_list[Idx].setMedia_id(getMediaID(connectionName));
        for(auto itr : pos_list) {
            itr.setActive(false);
        }
        pos_list[Idx].setActive(true);
        stub->setPositionAttribute(pos_list);
    }

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_position_sync(
        playerengine_proxy_[proxyId],
        (gint64)TimeConvert::UsToMs(new_pos_us),
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    } else if (!succeed) {
        MMLogInfo("[SeekCommand] " "failed to set position");
    } else {
        event_system_.WaitEvent(in, (uint32_t)command::EventType::AsyncDone, (uint32_t)command::EventType::ErrorOccured);
    }

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetPositionCommand* command) {
    static std::vector<command::CommandType> cv = { command::CommandType::Seek, command::CommandType::SetPosition };
    static std::vector<command::CommandType> cv_rate = { command::CommandType::SetRate };
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    if (command_queue_->Exist(cv, connectionName)) {
        MMLogInfo("[SetPositionCommand] " "skip command");
        return true;
    }

    MMLogInfo("[SetPositionCommand] " "pos_us : %llu, skip_wait=[%d]", command->pos_us, command->is_streaming);

    int32_t rate_index = getRateAttrIdx(connectionName);
    if (rate_index < 0) {
        MMLogInfo("Invalid rate_index");
        return false;
    }
    double rate = stub->getRateAttribute()[rate_index].getRate();
    MMLogInfo("[SetPositionCommand] " "cur rate : %f", rate);
    if (command->from_hmi && (fabs(rate - 0.0) > DBL_EPSILON) && (fabs(rate - 1.0) > DBL_EPSILON) && !(command_queue_->Exist(cv_rate, connectionName)) ) {
        MMLogInfo("[SetPositionCommand] " "rate != 1.0");
        return false;
    }

    int32_t Idx = getDurationAttrIdx(connectionName);
    if (Idx < 0) {
        MMLogInfo("Invalid duration index");
        return false;
    }
    uint64_t duration = stub->getDurationAttribute()[Idx].getDuration();

    if ((uint64_t)command->pos_us > duration) {
        MMLogError("[SetPositionCommand] " "out of range ( pos_us : %llu is bigger than duration %llu )", command->pos_us, duration);
        return false;
    } else if ((uint64_t)command->pos_us == duration && command->pos_us > 1000LL) { // For preventing infinite playEx EOS loop, margine 1ms
        command->pos_us -= 1000LL;
    }

    std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
    Idx = getPositionAttrIdx(connectionName);

    if (Idx > -1 && pos_list.size() > 0) {
        MMLogInfo("[SetPositionCommand] " "The position[%llu] of Index[%d][%s] changed to [%llu], pos_list size[%ld]",
            stub->getPositionAttribute()[Idx].getPosition(), Idx, connectionName.c_str(), command->pos_us, pos_list.size());
        seeking_position_idx = Idx;
        pos_list[Idx].setPosition(command->pos_us);
        pos_list[Idx].setMedia_id(getMediaID(connectionName));
        for(auto itr : pos_list) {
            itr.setActive(false);
        }
        pos_list[Idx].setActive(true);
        stub->setPositionAttribute(pos_list);
    }

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;

    com_lge_player_engine_call_set_position_sync(
        playerengine_proxy_[proxyId],
        (gint64)TimeConvert::UsToMs(command->pos_us),
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    } else if (!succeed) {
        MMLogError("[SetPositionCommand] " "failed to set position");
    } else {
        event_system_.WaitEvent(in, (uint32_t)command::EventType::AsyncDone, (uint32_t)command::EventType::ErrorOccured);
    }
    MMLogError("[SetPositionCommand] " "return value :%d", succeed);
    seeking_position_idx = -1;

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::StopCommand* command) {
    MMLogInfo("[StopCommand] " "mid=[%u]", command->media_id);
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    MMLogInfo("[StopCommand] " "proxy=[%u]" " state=[%u]", proxyId, state_[proxyId]);
    /* rate index check need to be done if state is > Stopped */
    if (state_[proxyId] > State::Stopped) {
        int32_t rate_index = getRateAttrIdx(connectionName);
        if (rate_index < 0) {
            MMLogInfo("Invalid rate_index");
            return false;
        }
        if ((fabs(stub->getRateAttribute()[rate_index].getRate() - 0.0) > DBL_EPSILON) && (fabs(stub->getRateAttribute()[rate_index].getRate() - 1.0) > DBL_EPSILON)) {
            stopTrickPlay(in, false, connectionName);
        }
    }

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    com_lge_player_engine_call_stop(
        playerengine_proxy_[proxyId],
        NULL,
        (GAsyncReadyCallback)PlayerProvider::callAsyncCallback,
        (gpointer*)new std::pair<PlayerProvider*, command::BaseCommand*>(this, command)
    );

    event_system_.WaitEvent(in, (uint32_t)command::EventType::CallAsync, 0);

    com_lge_player_engine_call_stop_finish(
        playerengine_proxy_[proxyId],
        &succeed,
        command->async_result,
        &dbus_error
    );
    g_object_unref(command->async_result);

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        last_fail_media_id_ = getMediaID(connectionName);
        MMLogError("StopCommand dbus error - [%d]", last_fail_media_id_);
        g_error_free(dbus_error);
    }

    /* Erase respective attribute index from the vector */
    int32_t Idx = getDurationAttrIdx(connectionName);
    if(Idx < stub->getDurationAttribute().size() && Idx > -1) {
        MMLogInfo("Erasing attribute from vector wit media-id : %d", stub->getDurationAttribute()[Idx].getMedia_id());
        std::vector<MM::PlayerTypes::Duration> dur_list = stub->getDurationAttribute();
        dur_list.erase(dur_list.begin()+Idx);
        stub->setDurationAttribute(dur_list);
    }
    Idx = getPositionAttrIdx(connectionName);
    if(Idx < stub->getPositionAttribute().size() && Idx > -1) {
        std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
        pos_list.erase(pos_list.begin()+Idx);
        stub->setPositionAttribute(pos_list);
    }
    Idx = getBufferingAttrIdx(connectionName);
    if(Idx < stub->getBufferingAttribute().size() && Idx > -1) {
        std::vector<MM::PlayerTypes::Buffering> buf_list = stub->getBufferingAttribute();
        buf_list.erase(buf_list.begin()+Idx);
        stub->setBufferingAttribute(buf_list);
    }
    Idx = getMuteAttrIdx(connectionName);
    if(Idx < stub->getMuteAttribute().size() && Idx > -1) {
        std::vector<MM::PlayerTypes::MuteOption> mute_list = stub->getMuteAttribute();
        mute_list.erase(mute_list.begin()+Idx);
        stub->setMuteAttribute(mute_list);
    }
    Idx = getRateAttrIdx(connectionName);
    if(Idx < stub->getRateAttribute().size() && Idx > -1) {
        MMLogInfo("Mutex lock() in rate_list before removing it");
        _attribute_mutex.lock();
        std::vector<MM::PlayerTypes::Rate> rate_list = stub->getRateAttribute();
        rate_list.erase(rate_list.begin()+Idx);
        stub->setRateAttribute(rate_list);
        _attribute_mutex.unlock();
    }
    Idx = getVolumeAttrIdx(connectionName);
    if(Idx < stub->getVolumeAttribute().size() && Idx > -1) {
        std::vector<MM::PlayerTypes::Volume> volume_list = stub->getVolumeAttribute();
        volume_list.erase(volume_list.begin()+Idx);
        stub->setVolumeAttribute(volume_list);
    }
    Idx = getCurrentTrackAttrIdx(connectionName);
    if(Idx < stub->getCurrentTrackAttribute().size() && Idx > -1) {
        track_opened_.erase(track_opened_.begin()+Idx);
        stub->setCurrentTrackAttribute(track_opened_);
    }
    Idx = getPlaybackAttrIdx(connectionName);
    if(Idx < stub->getPlaybackAttribute().size() && Idx > -1) {
        std::vector<MM::PlayerTypes::Playback> playback_list = stub->getPlaybackAttribute();
        playback_list.erase(playback_list.begin()+Idx);
        stub->setPlaybackAttribute(playback_list);
    }
    Idx = getSpeedAttrIdx(connectionName);
    if(Idx < stub->getSpeedAttribute().size() && Idx > -1) {
        MMLogInfo("Mutex lock() in speed_list before removing it");
        _attribute_mutex.lock();
        std::vector<MM::PlayerTypes::Speed> speed_list = stub->getSpeedAttribute();
        speed_list.erase(speed_list.begin()+Idx);
        stub->setSpeedAttribute(speed_list);
        _attribute_mutex.unlock();
    }
#ifdef PLATFORM_CCIC
    auto audio_slot_iter = pe_audio_slot_map_.begin();
    while (audio_slot_iter != pe_audio_slot_map_.end()) {
        if (connectionName == audio_slot_iter->second) {
            audio_slot_iter = pe_audio_slot_map_.erase(audio_slot_iter);
        } else {
            if (pe_audio_slot_map_.size() == 0) {
                break;
            }
            audio_slot_iter++;
        }
    }
#endif
    is_playing_ = false;
    state_[proxyId] = State::Stopped;
    need_to_open_when_play_[proxyId] = true;
    /*
    if (multi_channel_media_id_ > 0 && multi_channel_media_id_ == command->media_id) {
        MMLogInfo("multi channel source is released.. set to 0");
        multi_channel_media_id_ = 0;
        multi_channel_media_type_ = "";
    }*/

    //reset proxy map
    resetPEProxy(connectionName);
    sender_name_ = "";
    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::PlayExCommand* command) {
    // skip commands
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    MMLogInfo("[PlayExCommand] " "state=[%d]", state_[proxyId]);
    // skip commands

    static std::vector<command::CommandType> cv = { command::CommandType::PlayEx };
    if (command_queue_->Exist(cv, connectionName)) {
        MMLogInfo("[PlayExCommand] " "skip command");
        return true;
    }

    if (state_[proxyId] != State::Stopped) { // Needs normal play
        gboolean succeed = FALSE;
        command::PlayCommand pc(this, connectionName, nullptr);
        succeed = pc.Execute(in);
        return succeed;
    }

    // stop - ensure state status
    command::StopCommand sc(this, false, connectionName, getMediaID(connectionName), nullptr);
    sc.Execute(in);

    if (fabs(command->rate - 0) > DBL_EPSILON) {
        is_playing_ = true;
    }

    // open track
#if 0
    command::OpenTrackCommand otc(this, command->open_track_param, connectionName, nullptr);
    otc.from_hmi = command->from_hmi;
    otc.show_preroll_frame = (command->rate == 0) ? true : false;
    otc.pos_us = command->pos_us;
    if (otc.Execute(in) == false) {
        command->e = otc.e;
        return false;
    }
#endif
    if (command->pos_us != 0) {
        command::SetPositionCommand spc(this, false, command->pos_us, connectionName, nullptr);
        spc.Execute(in);
    }

    // pause or play normal or trick
    if (fabs(command->rate - 0) <= DBL_EPSILON) {
        command::PauseCommand pc(this, connectionName, nullptr);
        if (pc.Execute(in) == false)
            return false;
    } else if (fabs(command->rate - 1.0) <= DBL_EPSILON) {
        command::PlayCommand pc(this, connectionName, nullptr);
        if (pc.Execute(in) == false)
            return false;
    } else {
        std::vector<MM::PlayerTypes::Rate> rate_t = stub->getRateAttribute();
        std::vector<MM::PlayerTypes::Rate>::iterator itr;
        int32_t Idx = getRateAttrIdx(connectionName);

        if (Idx > -1 && rate_t.size() > 0) {
            rate_t[Idx].setMedia_id(getMediaID(connectionName));
            rate_t[Idx].setRate(command->rate);
            for(itr = rate_t.begin(); itr != rate_t.end(); itr++) {
                if((itr - rate_t.begin()) == Idx) {
                    itr->setActive(true);
                } else {
                    itr->setActive(false);
                }
            }
            stub->setRateAttribute(rate_t);
        }

        command::SetRateCommand src(this, command->rate, connectionName);
        if (src.Execute(in) == false)
            return false;
    }

    return true;
}
#if 0
bool PlayerProvider::process(command::Coro::pull_type& in, command::OpenPlaylistCommand* command) {
    MMLogInfo("[OpenPlaylistCommand] " "uri: %s", command->uri.c_str());
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    bool result = playlist_mgr_->OpenPlaylist(command->uri);
    if (result == false) {
        MMLogWarn("[OpenPlaylistCommand] " "failed to open playlist");
        command->e = MM::PlayerTypes::PlayerError::BAD_PLAYLIST;
        return result;
    }

    if (state_[proxyId] == State::Playing || state_[proxyId] == State::Paused) {
        int32_t Idx = -1;
        Idx = getCurrentTrackAttrIdx(connectionName);
        if (Idx > -1) {
            playlist::Track t(playlist::Track::INVALID_INDEX, track_opened_[Idx].getUri(), "");
            boost::optional<playlist::Track> track = playlist_mgr_->SelectTrack(t);

            if (!track) {
                MMLogWarn("[OpenPlaylistCommand] " "current loaded track is not in playlist, so do not update track info");
            } else {
                uint32_t index = (*track).index();
                std::string uri = (*track).GetInfo(playlist::Track::InfoType::Uri);
                MMLogInfo("[OpenPlaylistCommand] " "updated track info. index: %d, uri: %s", index, uri.c_str());
            }
        }
    } else {
        need_to_open_when_play_[proxyId] = true;
    }

    return result;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::EnqueueUriCommand* command) {
    MMLogDebug("[EnqueueUriCommand] " "uri: %s", command->uri.c_str());

    return playlist_mgr_->EnqueueUri(command->uri, "", media_type_);
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::EnqueueUriExCommand* command) {
    MMLogInfo("[EnqueueUriExCommand] " "uri_vector_size: %u, num: %u", command->uri.size(), command->num_array);

    if (command->uri.size() == 0) {
        MMLogInfo("[EnqueueUriExCommand] There is no url information.");
        return false;
    }
    std::vector<std::string>::iterator it;
    for (it = command->uri.begin(); it != command->uri.end(); ++it) {
        playlist_mgr_->EnqueueUri(*it, "", command->media_type);
    }

    MMLogInfo("[EnqueueUriExCommand] Done - [%d]", command->media_type);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::DequeueAllCommand* command) {
    MMLogInfo("[DequeueAllCommand] " "");
    std::string connectionName = command->connectionName;

    playlist_mgr_->DequeueAll();
    MM::PlayerTypes::Shuffle shuffle_t;
    shuffle_t.setMedia_id(getMediaID(connectionName));
    shuffle_t.setStatus(MM::PlayerTypes::ShuffleStatus::UNSHUFFLE);
    stub->setShuffleAttribute(shuffle_t);
    stub->setRandomAttribute(MM::PlayerTypes::RandomStatus::NO_RANDOM);

    playlist_mgr_->SetPlayMode(playlist::PlayMode::Normal);
    is_need_new_index_ = true;

#if 0 // dangerous... so do not do below routine...
    command::StopCommand sc(this, false, nullptr);
    sc.Execute(in);
#endif

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::DequeueIndexCommand* command) {
    MMLogInfo("[DequeueIndexCommand] " "pos: %llu", command->pos);
    playlist_mgr_->DequeueIndex(command->pos);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::DequeueDirectoryCommand* command) {
    MMLogInfo("[DequeueDirectoryCommand] " "directory: %s", command->directory.c_str());
    playlist_mgr_->DequeueDirectory(command->directory);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::DequeuePlaylistCommand* command) {
    MMLogInfo("[DequeuePlaylistCommand] " "playlist: %s", command->uri.c_str());
    playlist_mgr_->DequeuePlaylist(command->uri);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::GetCurrentPlayQueueCommand* command) {
    MMLogInfo("[GetCurrentPlayQueueCommand] " "");

    playlist_mgr_->ForEachTracks([&](const playlist::Track& track) -> bool {
        MM::MediaTypes::ResultMap rm;
        std::pair<std::string, MM::MediaTypes::ResultUnion> elem;
        elem.first = "URI";
        elem.second = MM::MediaTypes::ResultUnion(track.GetInfo(playlist::Track::InfoType::Uri));
        rm.insert(elem);
        command->play_queue.push_back(rm);
        return true;
    });

    return true;
}
#endif
bool PlayerProvider::process(command::Coro::pull_type& in, command::GetAudioLanguageListCommand* command) {
    MMLogInfo("[GetAudioLanguageListCommand] " "");

    for (auto& t : audio_tracks_)
        command->list.push_back(t);

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetAudioLanguageCommand* command) {
    MMLogInfo("[SetAudioLanguageCommand] " "index: %d", command->index);
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_audio_language_sync(
        playerengine_proxy_[proxyId],
        (gint)command->index,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::GetSubtitleListCommand* command) {
    MMLogInfo("[GetSubtitleListCommand] " "");

    for (auto& t : subtitle_tracks_)
        command->list.push_back(t);

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetSubtitleCommand* command) {
    MMLogInfo("[SetSubtitleCommand] " "index: %d", command->index);
    std::string connectionName = command->connectionName;
    if ((uint32_t)command->index >= subtitle_tracks_.size()) {
        MMLogError("[SetSubtitleCommand] " "index is bigger than subtitle_tracks_ size");
        return false;
    }

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_subtitle_language_index_sync(
        playerengine_proxy_[proxyId],
        (gint)command->index,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetSubtitleActivateCommand* command) {
    MMLogInfo("[SetSubtitleActivateCommand] " "activate: %d", command->activate);
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_subtitle_enable_sync(
        playerengine_proxy_[proxyId],
        (gboolean)command->activate,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetSubtitleLanguageCommand* command) {
    MMLogInfo("[SetSubtitleLanguageCommand] " "language: %s", command->language.c_str());

    auto it = std::find(subtitle_tracks_.begin(), subtitle_tracks_.end(), command->language);
    if (it == subtitle_tracks_.end()) {
        MMLogError("[SetSubtitleLanguageCommand] " "cannnot find language");
        return false;
    }
    std::string connectionName = command->connectionName;
    uint32_t index = std::distance(subtitle_tracks_.begin(), it);
    command::SetSubtitleCommand ssc(this, index, connectionName, nullptr);
    bool succeed = ssc.Execute(in);
    if (succeed == false) {
        command->e = ssc.e;
    }

    return succeed;
}

#if 0
bool PlayerProvider::process(command::Coro::pull_type& in, command::SetSubtitleLayerCommand* command) {
    std::string info = command->info.ToString();
    MMLogInfo("[SetSubtitleLayerCommand] " "%s", info.c_str());
    std::string connectionName = command->connectionName;
    MMLogInfo("[SetSubtitleLayerCommand] Media Id %d", getMediaID(connectionName));
    GError *dbus_error= NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_subtitle_layer_sync(
        (proxyId < 0) ? playerengine_proxy_[0] : playerengine_proxy_[proxyId],
        info.c_str(),
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    return succeed;
}
#endif

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetVideoWindowCommand* command) {
    std::string info = command->info.ToString();
    MMLogInfo("[SetVideoWindowCommand] " "%s", info.c_str());
    std::string connectionName = command->connectionName;
    MMLogInfo("[SetVideoWindowCommand] Media Id %d", getMediaID(connectionName));
    GError *dbus_error= NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_video_window_sync(
        playerengine_proxy_[proxyId],
        info.c_str(),
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    video_window_backup_ = command->info;

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetVideoWindowExCommand* command) {
    MMLogInfo("[SetVideoWindowExCommand] " "%s", command->info.c_str());
    std::string connectionName = command->connectionName;
    GError *dbus_error= NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_video_window_sync(
        playerengine_proxy_[proxyId],
        command->info.c_str(),
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error) {
        MMLogError("SetVideoWindowExCommand dbus error");
        command->e = MM::PlayerTypes::PlayerError::BACKEND_UNREACHABLE;
        g_error_free(dbus_error);
    }

    video_window_backup_.Update(command->info);

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetMuteCommand* command) {
    MMLogInfo("[SetMuteCommand] " "mute : %d", (int)command->mute);
    std::string connectionName = command->connectionName;
    GError *dbus_error= NULL;
    gboolean succeed = FALSE;
    gboolean mute = (command->mute == MM::PlayerTypes::MuteStatus::MUTED) ? TRUE : FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_audio_mute_sync(
        playerengine_proxy_[proxyId],
        mute,
        &succeed,
        NULL,
        &dbus_error
    );

    if (succeed == TRUE) {
        MMLogInfo("Mute=[%d] return OK", mute);
        std::vector<MM::PlayerTypes::MuteOption> mute_list = stub->getMuteAttribute();
        int32_t Idx = getMuteAttrIdx(connectionName);

        if (Idx <= -1 || mute_list.size() == 0) {
            MMLogInfo("There is no mute list");
        } else {
            mute_list[Idx].setMedia_id(getMediaID(connectionName));
            mute_list[Idx].setStatus((mute == TRUE) ? MM::PlayerTypes::MuteStatus::MUTED : MM::PlayerTypes::MuteStatus::UNMUTED);
            stub->setMuteAttribute(mute_list);
        }
    } else {
        MMLogError("Mute return Not OK");
        std::vector<MM::PlayerTypes::MuteOption> mute_list = stub->getMuteAttribute();
        int32_t Idx = getMuteAttrIdx(connectionName);

        if (Idx <= -1 || mute_list.size() == 0) {
            MMLogInfo("There is no mute list");
        } else {
            mute_list[Idx].setMedia_id(getMediaID(connectionName));
            mute_list[Idx].setStatus((mute == TRUE) ? MM::PlayerTypes::MuteStatus::UNMUTED : MM::PlayerTypes::MuteStatus::MUTED);
            stub->setMuteAttribute(mute_list);
        }
        if (dbus_error)
            g_error_free(dbus_error);
    }

    return true;
}
#if 0
bool PlayerProvider::process(command::Coro::pull_type& in, command::SetShuffleCommand* command) {
    MMLogInfo("[SetShuffleCommand] " "shuffle : %d", (int)command->shuffle);
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    playlist::PlayMode mode;
    if (command->shuffle == MM::PlayerTypes::ShuffleStatus::SHUFFLE) {
        stub->setRandomAttribute(MM::PlayerTypes::RandomStatus::NO_RANDOM);
        mode = playlist::PlayMode::Shuffle;
    }
    else {
        mode = playlist::PlayMode::Normal;
    }

    playlist_mgr_->SetPlayMode(mode);

    if (playback_option_.auto_resume_on_user_action) {
        if (state_[proxyId] == State::Paused && command->shuffle != MM::PlayerTypes::ShuffleStatus::UNSHUFFLE) {
            MMLogInfo("[SetShuffleCommand] " "auto resume");
            command_queue_->Post(new command::PlayCommand(this, connectionName, nullptr));
        }
    }

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetRepeatCommand* command) {
    MMLogInfo("[SetRepeatCommand] " "repeat : %d", (int)command->repeat);
    std::string connectionName = command->connectionName;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    if (command->repeat == MM::PlayerTypes::RepeatStatus::REPEAT_DIRECTORY) {
        int32_t Idx = getCurrentTrackAttrIdx(connectionName);
        if (Idx < 0) {
            MMLogInfo("Invalid Track index");
            return false;
        }
        std::string uri = track_opened_[Idx].getUri();
        std::string filename;
        std::string directory;
        if (uri.empty() == false) {
            playlist::ParseUtils::ParseUri(uri, directory, filename);
        } else {
            boost::optional<playlist::Track> track = playlist_mgr_->SeekTrack(0);
            if (track)
                directory = (*track).GetInfo(playlist::Track::InfoType::Directory);
        }
        playlist_mgr_->SelectDirectory(directory);
    } else {
        playlist_mgr_->SelectDirectory("");
    }

    if (playback_option_.auto_resume_on_user_action) {
        if (state_[proxyId] == State::Paused && command->repeat != MM::PlayerTypes::RepeatStatus::NO_REPEAT) {
            MMLogInfo("[SetRepeatCommand] " "auto resume");
            command_queue_->Post(new command::PlayCommand(this, connectionName, nullptr));
        }
    }

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetRandomCommand* command) {
    MMLogInfo("[SetRandomCommand] " "random : %d", (int)command->random);
    int proxyId = preparePEProxy(sender_name_);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    playlist::PlayMode mode;
    if (command->random == MM::PlayerTypes::RandomStatus::RANDOM) {
        MM::PlayerTypes::Shuffle shuffle_t;
        shuffle_t.setMedia_id(getMediaID(sender_name_));
        shuffle_t.setStatus(MM::PlayerTypes::ShuffleStatus::UNSHUFFLE);
        stub->setShuffleAttribute(shuffle_t);
        mode = playlist::PlayMode::Random;
    } else {
        mode = playlist::PlayMode::Normal;
    }

    playlist_mgr_->SetPlayMode(mode);

    if (playback_option_.auto_resume_on_user_action) {
        if (state_[proxyId] == State::Paused && command->random != MM::PlayerTypes::RandomStatus::NO_RANDOM) {
            MMLogInfo("[SetRandomCommand] " "auto resume");
            command_queue_->Post(new command::PlayCommand(this, sender_name_, nullptr));
        }
    }

    return true;
}
#endif

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetRateCommand* command) {
    MMLogInfo("[SetRateCommand] " "rate : %f", command->rate);
    std::string connectionName = command->connectionName;

    if (fabs(command->rate - 0.0) <= DBL_EPSILON) {
        MMLogInfo("[SetRateCommand] ignore zero rate");
        return true;
    }

    if (fabs(command->rate - 1.0) <= DBL_EPSILON) {
        bool need_to_seek = false;
        if (IsVideoType(media_type_) && updated_current_time_since_trickplay_)
            need_to_seek = true;
        stopTrickPlay(in, need_to_seek, connectionName);

        if (is_playing_ == true) {
            command::PlayCommand pc(this, connectionName, nullptr);
            pc.Execute(in);
        }
        return true;
    }

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    if (command->rate >= 0) {
        com_lge_player_engine_call_fast_forward_sync(
            playerengine_proxy_[proxyId],
            (gdouble)command->rate,
            &succeed,
            NULL,
            &dbus_error
        );
    } else {
        com_lge_player_engine_call_rewind_sync(
            playerengine_proxy_[proxyId],
            (gdouble)command->rate,
            &succeed,
            NULL,
            &dbus_error
        );
    }

    if (succeed == TRUE) {
        controlMuteForTrickPlay(in, command->rate, connectionName);
        is_playing_ = true;
        state_[proxyId] = State::Playing;

        updated_current_time_since_trickplay_ = false;
    } else if (dbus_error) {
        g_error_free(dbus_error);
    }

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetSpeedCommand* command) {
    MMLogInfo("[SetSpeedCommand] " "speed : %f for connection name : %s", command->speed, command->connectionName.c_str());
    std::string connectionName = command->connectionName;

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;

    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    //set the ignore buffering flag
    speed_ignore_buffering_map_[connectionName] = true;

    if (command->speed > 0) {
        com_lge_player_engine_call_set_playback_speed_sync(
         playerengine_proxy_[proxyId],
         (gdouble)command->speed,
         &succeed,
         NULL,
         &dbus_error
        );
    }

    if (dbus_error) {
        g_error_free(dbus_error);
    }

    if (!succeed) {
        MMLogInfo("[SetSpeedCommand] " "failed to set speed");
    } else {
        event_system_.WaitEvent(in, (uint32_t)command::EventType::AsyncDone, (uint32_t)command::EventType::ErrorOccured);
        MMLogInfo("[SetSpeedCommand] " "Set speed complete for value : %f", (gdouble)command->speed);
    }
    //reset the ignore buffering flag
    speed_ignore_buffering_map_[connectionName] = false;

    return succeed;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetVolumeCommand* command) {
    MMLogInfo("[SetVolumeCommand] " "volume : %f", command->volume);
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_audio_volume_sync(
        playerengine_proxy_[proxyId],
        (gdouble)command->volume,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error)
        g_error_free(dbus_error);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetVideoContrastCommand* command){
    MMLogInfo("[SetVideoContrastCommand] " "contrast : %f", command->contrast);
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }
    // Adjust proper value (GENGIX-54084)
    command->contrast = (command->contrast < 0.2f) ? 0.5f :
                        (command->contrast < 0.4f) ? 0.6f :
                        (command->contrast < 0.6f) ? 0.7f :
                        (command->contrast < 0.8f) ? 0.8f :
                        (command->contrast < 1.0f) ? 0.9f : command->contrast;


    if (media_type_ == MM::PlayerTypes::MediaType::USB_VIDEO2) {
        contrast_2nd_ = command->contrast;
    } else {
        contrast_ = command->contrast;
    }

    com_lge_player_engine_call_set_video_contrast_sync(
        playerengine_proxy_[proxyId],
        (gdouble)command->contrast,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error)
        g_error_free(dbus_error);
    return true;
}


bool PlayerProvider::process(command::Coro::pull_type& in, command::SetVideoBrightnessCommand* command){
    MMLogInfo("[SetVideoBrightnessCommand] " "brightness : %f", command->brightness);
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }
    // Adjust proper value (GENGIX-54084)
    command->brightness = (command->brightness < -0.4f) ? -0.4f :
                          (command->brightness < -0.3f) ? -0.3f :
                          (command->brightness < -0.2f) ? -0.25f: command->brightness;

    if (media_type_ == MM::PlayerTypes::MediaType::USB_VIDEO2) {
        brightness_2nd_ = command->brightness;
    } else {
        brightness_ = command->brightness;
    }

    com_lge_player_engine_call_set_video_brightness_sync(
        playerengine_proxy_[proxyId],
        (gdouble)command->brightness,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error)
        g_error_free(dbus_error);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetAVoffsetCommand* command){
    MMLogInfo("[SetAVoffsetCommand] " "AVoffset : %d", static_cast<int32_t>(command->delay));
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_set_avoffset_sync(
        playerengine_proxy_[proxyId],
        (gint)command->delay,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error)
        g_error_free(dbus_error);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetVideoSaturationCommand* command){
    MMLogInfo("[SetVideoSaturationCommand] " "saturation : %f", command->saturation);
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    if (media_type_ == MM::PlayerTypes::MediaType::USB_VIDEO2) {
        saturation_2nd_ = command->saturation;
    } else {
        saturation_ = command->saturation;
    }

    com_lge_player_engine_call_set_video_saturation_sync(
        playerengine_proxy_[proxyId],
        (gdouble)command->saturation,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error)
        g_error_free(dbus_error);
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SwitchChannelCommand* command){
    MMLogInfo("[SwitchChannelCommand] " "useDownmix : %s", command->useDownmix ? "true" : "false");
    std::string connectionName = command->connectionName;
    GError *dbus_error = NULL;
    gboolean succeed = FALSE;
    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }
#ifdef PLATFORM_CCIC
    if (!(gboolean)command->useDownmix) {
        //check if 6ch slot is being used
        MMLogInfo("[SwitchChannelCommand] check 6ch slot available before upmix");
        auto pe_audio_slot_iter = pe_audio_slot_map_.find("6ch_0");
        if(pe_audio_slot_iter != pe_audio_slot_map_.end()) {
            if(pe_audio_slot_iter->second != connectionName) {
                int prev_proxyID = preparePEProxy(pe_audio_slot_iter->second);
                if(prev_proxyID >= 0) {
                    com_lge_player_engine_call_switch_channel_sync(
                    playerengine_proxy_[prev_proxyID],
                    TRUE,
                    &succeed,
                    NULL,
                    &dbus_error);

                    if(succeed) {
                        pe_audio_slot_map_.erase("6ch_0");
                        succeed = FALSE;
                    }
                    if (dbus_error)
                        g_error_free(dbus_error);
                }
            }
        }
    }
#endif
    com_lge_player_engine_call_switch_channel_sync(
        playerengine_proxy_[proxyId],
        (gboolean)command->useDownmix,
        &succeed,
        NULL,
        &dbus_error
    );

    if (dbus_error)
        g_error_free(dbus_error);
#ifdef PLATFORM_CCIC
    if(succeed)
    {
        if ((gboolean)command->useDownmix) {
            pe_audio_slot_map_.erase("6ch_0");
        } else {
            pe_audio_slot_map_.insert({"6ch_0", connectionName});
        }
    }
#endif
    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetPlaybackOptionCommand* command) {
    playback_option_.Update(command->option);
    playback_option_.Get(command->option);
    //stub->setPlaybackOptionAttribute(command->option);

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetMediaTypeCommand* command) {
    MMLogInfo("[SetMediaTypeCommand] " "prev: %d, now: %d", static_cast<int32_t>(media_type_), static_cast<int32_t>(command->media_type));

    media_type_ = command->media_type;
    //playlist_mgr_->SetPlaylistQueue(media_type_);

    return true;
}

#if 0
bool PlayerProvider::process(command::Coro::pull_type& in, command::SetSortModeCommand* command) {
    playlist_mgr_->SetSortMode(convSortMode(command->sort_mode));

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetShuffleOptionCommand* command) {
    playlist::ShuffleOption option = convShuffleOption(command->option);
    playlist_mgr_->SetShuffleOption(option);

    return true;
}

bool PlayerProvider::process(command::Coro::pull_type& in, command::SetPlaylistTypeCommand* command) {
    MMLogInfo("[SetPlaylistTypeCommand] " "type: %d", command->type);

    resetPlaylistManager();

    return true;
}
#endif

void PlayerProvider::callAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    auto ppair = (std::pair<PlayerProvider*, command::BaseCommand*>*)user_data;
    PlayerProvider* that = ppair->first;
    command::BaseCommand* command = ppair->second;
    delete ppair;

    command->async_result = res;
    g_object_ref(command->async_result);

    that->onCallAsync(ptree());
}

void PlayerProvider::handleStateChange(GDBusConnection *connection,
                                                                             const gchar *sender_name,
                                                                             const gchar *object_path,
                                                                             const gchar *interface_name,
                                                                             const gchar *signal_name,
                                                                             GVariant *parameters,
                                                                             gpointer user_data) {

    sender_name_ = sender_name ? sender_name : "";
    static std::map<std::string, void (PlayerProvider::*)(const ptree&)> handler = {
        { "Duration",             &PlayerProvider::onDuration },
        { "CurrentTime",       &PlayerProvider::onCurrentTime },
        { "PlaybackStatus", &PlayerProvider::onPlaybackStatus },
        { "BeginOfStream",   &PlayerProvider::onBeginOfStream },
        { "EndOfStream",       &PlayerProvider::onEndOfStream },
        { "AsyncDone",           &PlayerProvider::onAsyncDone },
        { "SourceInfo",         &PlayerProvider::onSourceInfo },
        { "Subtitle",             &PlayerProvider::onSubtitle },
        { "ChannelInfo",      &PlayerProvider::onChannelEvent },
        { "AddressInfo",      &PlayerProvider::onAddressEvent },
        { "StreamingEvent", &PlayerProvider::onStreamingEvent },
        { "Warning",               &PlayerProvider::onWarning },
        { "Error",                   &PlayerProvider::onError },
        { "ContentType",       &PlayerProvider::onContentType },
    };

    GVariant *inner = g_variant_get_child_value(parameters, 0);
    if (!g_variant_is_of_type(inner, G_VARIANT_TYPE_STRING)) {
        MMLogError("Invalid type");
        if (inner)
            g_variant_unref(inner);
        return;
    }

    ptree pt;
    const char *jsonCStr = g_variant_get_string(inner, NULL);
    std::string s(jsonCStr ? jsonCStr : "");
    std::stringstream ss(s);
    boost::property_tree::json_parser::read_json(ss, pt);
    auto iter = pt.begin();
    std::string state = iter->first ;
    ptree sub_pt = iter->second;

    auto it = handler.find(state);
    if (it == handler.end()) {
        if (inner)
            g_variant_unref(inner);
        return;
    }

    PlayerProvider* that = (PlayerProvider*)user_data;
    (that->*(handler[state]))(sub_pt);

    if (inner)
        g_variant_unref(inner);
}

void PlayerProvider::onContentType(const ptree& pt) {
    auto content = pt.get_value_optional<std::string>();
    if(!content) {
        MMLogError("Invalid content type event~!!");
        return;
    }
    std::string name = *content;
    MMLogInfo("Send Content Type=[%s]", name.c_str());
    sc_notifier_.NotifyContentType(name, getMediaID(sender_name_));
}

void PlayerProvider::onPEDestroyed(const ptree& pt, int mediaId) {
    if (mediaId > 0) {
        MMLogInfo("id=[%d], sender_name=[%s]", mediaId, sender_name_.c_str());
        std::map<int, std::string>::iterator it;
        for (it = connection_map_.begin(); it != connection_map_.end(); it++) {
            if (it->first == mediaId) {
                sender_name_ = it->second;
                break;
            }
        }
    }
    MMLogInfo("updated sender_name=[%s]", sender_name_.c_str());
    command_queue_->PostFront(new command::SetVideoWindowExCommand(this, this->video_window_backup_.ToString(), sender_name_, nullptr));

    onError(pt);
}

void PlayerProvider::onCallAsync(const ptree& pt) {
    event_system_.SetEvent(command::EventType::CallAsync, nullptr);
}
#if 0
void PlayerProvider::onVMSourceNotification(const ptree& pt) {
    event_system_.SetEvent(command::EventType::VMSourceNotification, nullptr);
}
#endif
void PlayerProvider::onDuration(const ptree& pt) {
    auto vduration_ms = pt.get_value_optional<gint64>();
    if (!vduration_ms)
        return;

    gint64 duration_ms = *vduration_ms;
    int32_t Idx = getDurationAttrIdx(sender_name_);

    std::vector<MM::PlayerTypes::Duration> dur_list = stub->getDurationAttribute();
    std::vector<MM::PlayerTypes::Duration>::iterator itr;

    MMLogInfo("Duration [%lld], list size=[%lu], index=[%d]", duration_ms, dur_list.size(), Idx);
    if (Idx > -1 && dur_list.size() > 0) {
        dur_list[Idx].setDuration(TimeConvert::MsToUs((uint64_t)duration_ms));
        dur_list[Idx].setMedia_id(getMediaID(sender_name_));
        for (itr = dur_list.begin(); itr != dur_list.end(); itr++) {
            if ((itr - dur_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setDurationAttribute(dur_list);
        MMLogInfo("Duration aft set %lld", dur_list[Idx].getDuration());
    }
}

void PlayerProvider::onCurrentTime(const ptree& pt) {
    auto vcurrent_time_ms = pt.get_value_optional<gint64>();
    if (!vcurrent_time_ms)
        return;

    gint64 current_time_ms = *vcurrent_time_ms;
    int32_t Idx = getPositionAttrIdx(sender_name_);
    if (seeking_position_idx > -1 && Idx == seeking_position_idx) {
        MMLogWarn("CurrentTime[%lld] event is skipped (Seek in progress)", current_time_ms);
        return;
    }
    std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
    std::vector<MM::PlayerTypes::Position>::iterator itr;

    if (Idx > -1 && pos_list.size() > 0) {
        pos_list[Idx].setPosition(TimeConvert::MsToUs((uint64_t)current_time_ms));
        pos_list[Idx].setMedia_id(getMediaID(sender_name_));
        for (itr=pos_list.begin(); itr != pos_list.end(); itr++) {
            if ((itr - pos_list.begin()) != Idx) {
                itr->setActive(false);
            } else {
                itr->setActive(true);
            }
        }
        stub->setPositionAttribute(pos_list);
        MMLogInfo("get position = %lld", stub->getPositionAttribute()[Idx].getPosition());
    }
    updated_current_time_since_trickplay_ = true;
}

void PlayerProvider::onPlaybackStatus(const ptree& pt) {
    auto vstatus = pt.get_value_optional<std::string>();
    if (!vstatus)
        return;

    std::string status = *vstatus;
    int proxyId = preparePEProxy(sender_name_);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return;
    }

    if (status.compare("Playing") == 0) {
        MMLogInfo("Playing");
        std::vector<MM::PlayerTypes::Playback> pb_t = stub->getPlaybackAttribute();
        std::vector<MM::PlayerTypes::Playback>::iterator itr;
        int32_t Idx = getPlaybackAttrIdx(sender_name_);

        if (Idx > -1 && pb_t.size() > 0) {
            pb_t[Idx].setMedia_id(getMediaID(sender_name_));
            pb_t[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::PLAYING);
            for (itr = pb_t.begin(); itr != pb_t.end(); itr++) {
                if ((itr - pb_t.begin()) == Idx) {
                    itr->setActive(true);
                } else {
                    itr->setActive(false);
                }
            }
            stub->setPlaybackAttribute(pb_t);
        }
        //if (need_to_open_when_play_[proxyId] == true) {
        //    command_queue_->Post(new command::PlayCommand(this, sender_name_, nullptr));
        //}
    } else if (status.compare("Paused") == 0) {
        MMLogInfo("Paused");
        std::vector<MM::PlayerTypes::Playback> pb_t = stub->getPlaybackAttribute();
        std::vector<MM::PlayerTypes::Playback>::iterator itr;
        int32_t Idx = getPlaybackAttrIdx(sender_name_);

        if (Idx > -1 && pb_t.size() > 0) {
            pb_t[Idx].setMedia_id(getMediaID(sender_name_));
            pb_t[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::PAUSED);
            for (itr = pb_t.begin(); itr != pb_t.end(); itr++) {
                if ((itr - pb_t.begin()) == Idx) {
                    itr->setActive(true);
                } else {
                    itr->setActive(false);
                }
            }
            stub->setPlaybackAttribute(pb_t);
        }
    } else if (status.compare("Ready") == 0) {
        MMLogInfo("Ready");
        std::vector<MM::PlayerTypes::Playback> pb_t = stub->getPlaybackAttribute();
        std::vector<MM::PlayerTypes::Playback>::iterator itr;
        int32_t Idx = getPlaybackAttrIdx(sender_name_);

        if (Idx > -1 && pb_t.size() > 0) {
            pb_t[Idx].setMedia_id(getMediaID(sender_name_));
            pb_t[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::READY);
            for (itr = pb_t.begin(); itr != pb_t.end(); itr++) {
                if ((itr - pb_t.begin()) == Idx) {
                    itr->setActive(true);
                } else {
                    itr->setActive(false);
                }
            }
            stub->setPlaybackAttribute(pb_t);
        }
    } else if (status.compare("Stopped") == 0) {
        MMLogInfo("Stopped");
        onPlaybackStopped();
    }
}

void PlayerProvider::onTrickAndBOS(double rate) {
    if (playback_option_.what_to_do_on_trick_and_BOS == "stop") {
        MMLogInfo("change to paused and seek 0");

        command_queue_->Post(new command::PauseCommand(this, sender_name_, nullptr));
        command_queue_->Post(new command::SetMuteCommand(this, MM::PlayerTypes::MuteStatus::MUTED, sender_name_)); // For preventing noise(GENSIX-55553)
        command_queue_->Post(new command::SetPositionCommand(this, false, 0, sender_name_, nullptr));
    } else if (playback_option_.what_to_do_on_trick_and_BOS == "normal_play") {
        MMLogInfo("change to normal play");

        command_queue_->Post(new command::PauseCommand(this, sender_name_, nullptr));
        command_queue_->Post(new command::SetPositionCommand(this, false, 0, sender_name_, nullptr));
        command_queue_->Post(new command::PlayCommand(this, sender_name_, nullptr));
    }
    /*
    else if (playback_option_.what_to_do_on_trick_and_BOS == "trick_play") {
        MMLogInfo("continue trick play");

        if (playback_option_.set_position_on_trick_and_repeat_single &&
                stub->getRepeatOptionAttribute().getStatus() == MM::PlayerTypes::RepeatStatus::REPEAT_SINGLE) {
            MMLogInfo("set_position_on_trick_and_repeat_single");

            uint64_t new_position = 0;
            command_queue_->Post(new command::SetPositionCommand(this, false, new_position, sender_name_, nullptr));
            command_queue_->Post(new command::SetRateCommand(this, stub->getRateAttribute()[getRateAttrIdx(sender_name_)].getRate(), sender_name_));
        } else {
            MMLogInfo("NextAndTrick");

            uint32_t pos = (rate > 0) ? 1 : -1;
            command_queue_->Post(new command::NextAndTrickCommand(this, pos, rate));
        }
    }
    */
}

void PlayerProvider::onTrickAndEOS(double rate) {
    if (playback_option_.what_to_do_on_trick_and_EOS == "stop") {
        MMLogInfo("change to paused and seek 0");

        command_queue_->Post(new command::PauseCommand(this, sender_name_, nullptr));
        command_queue_->Post(new command::SetPositionCommand(this, false, 0, sender_name_, nullptr));
    }
    /* else if (playback_option_.what_to_do_on_trick_and_EOS == "normal_play") {
        MMLogInfo("change to normal play");

        command_queue_->Post(new command::StopCommand(this, false, sender_name_, nullptr));
        command_queue_->Post(new command::NextCommand(this, 1, sender_name_, nullptr));
        command_queue_->Post(new command::PlayCommand(this, sender_name_, nullptr));
    } else if (playback_option_.what_to_do_on_trick_and_EOS == "trick_play") {
        MMLogInfo("continue trick play");

        if (playback_option_.set_position_on_trick_and_repeat_single &&
                stub->getRepeatOptionAttribute().getStatus() == MM::PlayerTypes::RepeatStatus::REPEAT_SINGLE) {
            MMLogInfo("set_position_on_trick_and_repeat_single");

            uint64_t new_position = stub->getDurationAttribute()[getDurationAttrIdx(sender_name_)].getDuration();
            if (new_position < TimeConvert::SecToUs(3)) {
                MMLogWarn("duration is smaller than 3 seconds");
                onPlaybackStopped();
                return;
            }
            new_position -= TimeConvert::SecToUs(3);

            command_queue_->Post(new command::SetPositionCommand(this, new_position, sender_name_, nullptr));
            command_queue_->Post(new command::SetRateCommand(this, stub->getRateAttribute()[getRateAttrIdx(sender_name_)].getRate(), sender_name_));
        } else {
            MMLogInfo("NextAndTrick");

            uint32_t pos = (rate > 0) ? 1 : -1;
            command_queue_->Post(new command::NextAndTrickCommand(this, pos, rate));
        }
    }
    */
}

void PlayerProvider::onBeginOfStream(const ptree& pt) {
    MMLogInfo("");

    int32_t Idx_t = getCurrentTrackAttrIdx(sender_name_);
    if (Idx_t < 0) {
        MMLogInfo("Invalid Track index");
        return;
    }
    std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
    int32_t Idx = getPositionAttrIdx(sender_name_);

    if (Idx > -1 && pos_list.size() > 0) {
        pos_list[Idx].setPosition(0);
        pos_list[Idx].setMedia_id(getMediaID(sender_name_));
        for(auto itr : pos_list) {
            itr.setActive(false);
        }
        pos_list[Idx].setActive(true);
        stub->setPositionAttribute(pos_list);
    }

    int32_t rate_index = getRateAttrIdx(sender_name_);
    if (rate_index < 0) {
        MMLogInfo("Invalid rate_index");
        return;
    }
    double rate = stub->getRateAttribute()[rate_index].getRate();
    onTrickAndBOS(rate);

/*
 * [HMCCCIC-6804] 
 * When receiving BOS, it requests mute to gstreamer. As a result, gstreamer is muted.
 * So, We send NotifyBOS to application after performing onTrickAndBOS();
 */
    sc_notifier_.NotifyBOS(track_opened_[Idx_t], getMediaID(sender_name_));
}

void PlayerProvider::onEndOfStream(const ptree& pt) {
    MMLogInfo("");

    int32_t Idx = getCurrentTrackAttrIdx(sender_name_);
    if (Idx < 0) {
        MMLogInfo("Invalid Track index");
        return;
    }
    sc_notifier_.NotifyEOS(track_opened_[Idx], getMediaID(sender_name_));
/*
    int proxyId = preparePEProxy(sender_name_);
    if (proxyId == -1) {
        MMLogInfo("Invalid Proxy Id");
        return;
    }
*/
    int32_t rate_index = getRateAttrIdx(sender_name_);
    if (rate_index < 0) {
        MMLogInfo("Invalid rate_index");
        return;
    }
    double rate = stub->getRateAttribute()[rate_index].getRate();
    if (fabs(rate - 1.0) > DBL_EPSILON) {
        onTrickAndEOS(rate);
        return;
    }

    MM::PlayerTypes::RepeatStatus repeat_status = stub->getRepeatOptionAttribute().getStatus();
    MMLogInfo("repeat_status: %d", static_cast<int32_t>(repeat_status));

    //if (repeat_status != MM::PlayerTypes::RepeatStatus::REPEAT_SINGLE) {
        /* Erase respective attribute index from the vector */
        Idx = getDurationAttrIdx(sender_name_);
        if(Idx < stub->getDurationAttribute().size() && Idx != -1) {
            MMLogInfo("Erasing duration attribute from vector wit media-id : %d", stub->getDurationAttribute()[Idx].getMedia_id());
            std::vector<MM::PlayerTypes::Duration> dur_list = stub->getDurationAttribute();
            dur_list.erase(dur_list.begin()+Idx);
            stub->setDurationAttribute(dur_list);
        }
        Idx = getPositionAttrIdx(sender_name_);
        if(Idx < stub->getPositionAttribute().size() && Idx != -1) {
            MMLogInfo("Erasing position attribute from vector wit media-id : %d", stub->getPositionAttribute()[Idx].getMedia_id());
            std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
            pos_list.erase(pos_list.begin()+Idx);
            stub->setPositionAttribute(pos_list);
        }
    //}
#if 0
    is_EOS_state_ = true;
    switch (repeat_status) {
        case MM::PlayerTypes::RepeatStatus::REPEAT: // FALL-THROUGH
        case MM::PlayerTypes::RepeatStatus::REPEAT_DIRECTORY: {
            usleep(TimeConvert::SecToUs(1));

            _mutex.lock();
            if (is_need_pause_) {
                MMLogInfo("need pause after EOS process");
                is_need_pause_ = false;
                command_queue_->Post(new command::NextCommand(this, 2, sender_name_, nullptr));
            } else {
                MMLogInfo("don't need pause after EOS process");
                command_queue_->Post(new command::NextCommand(this, 1, sender_name_, nullptr));
            }
            _mutex.unlock();
            break;
        }
        case MM::PlayerTypes::RepeatStatus::REPEAT_SINGLE: {
            command_queue_->Post(new command::SetPositionCommand(this, false, 0, sender_name_, nullptr));
            break;
        }
        case MM::PlayerTypes::RepeatStatus::NO_REPEAT: {
            if (playlist_mgr_->IsLastOfTracks()) {
                MMLogInfo("last of tracks, so stop");
                onPlaybackStopped();
                sc_notifier_.NotifyAllTrackPlayed("LastOfTrack", getMediaID(sender_name_));
                state_[proxyId] = State::Stopped;
                is_EOS_state_ = false;
                return;
            }
            usleep(TimeConvert::SecToUs(1));

            _mutex.lock();
            if (is_need_pause_) {
                MMLogInfo("need pause after EOS process");
                is_need_pause_ = false;
                command_queue_->Post(new command::NextCommand(this, 2, sender_name_, nullptr));
            } else {
                MMLogInfo("don't need pause after EOS process");
                command_queue_->Post(new command::NextCommand(this, 1, sender_name_, nullptr));
            }
            _mutex.unlock();
            break;
        }
        case MM::PlayerTypes::RepeatStatus::NO_REPEAT_AND_SINGLE: {
            onPlaybackStopped();
            break;
        default:
            break;
        }
    }
    is_EOS_state_ = false;
#endif
}

void PlayerProvider::onAsyncDone(const ptree& pt) {
    event_system_.SetEvent(command::EventType::AsyncDone, nullptr);
}

void PlayerProvider::onSourceInfo(const ptree& pt) {
    MMLogInfo("");

    auto vcan_seek = pt.get_optional<bool>("CanSeek");
    if (vcan_seek) {
        bool can_seek = *vcan_seek;
        MMLogInfo("CanSeek : %d", can_seek);
        stub->setCanSeekAttribute(can_seek);
    }

    fillAudioLanguageList(pt);

    fillSubtitleList(pt);

    if (IsVideoType(media_type_) == true) {
        sc_notifier_.NotifyAudioLanguageList(audio_tracks_, getMediaID(sender_name_));

        sc_notifier_.NotifySubtitleList(subtitle_tracks_, getMediaID(sender_name_));

        int32_t width, height;
        if (getVideoResolution(pt, width, height))
            sc_notifier_.NotifyVideoResolution(width, height, getMediaID(sender_name_));
    }

    event_system_.SetEvent(command::EventType::SourceInfo, nullptr);
}

void PlayerProvider::onSubtitle(const ptree& pt) {
    sc_notifier_.NotifySubtitleData(getMediaID(sender_name_));
}

void PlayerProvider::onChannelEvent(const boost::property_tree::ptree& pt) {
    MMLogInfo("");
    auto channel = pt.get_optional<int>("CHANNEL");
    auto media_type = pt.get_optional<int>("TYPE");

    if (channel) {
        MMLogInfo("acquired channel=[%d], curr_multi_mid=[%d]", *channel, multi_channel_media_id_);
        /*
        if (multi_channel_media_id_ == 0 && *channel >= 6) {
            multi_channel_media_id_ = (int)getMediaID(sender_name_);
            if (media_type) {
                multi_channel_media_type_ = *media_type;
            }
            MMLogInfo("multi channel source is requested(by event) set to [%d]", multi_channel_media_id_);
        }*/
        sc_notifier_.NotifyChannelInfo(*channel, getMediaID(sender_name_));
    }
}

void PlayerProvider::onAddressEvent(const boost::property_tree::ptree& pt) {
    MMLogInfo("");
    auto address = pt.get_optional<std::string>("ADDRESS");
    if (address) {
        std::string address_ = *address;
        MMLogInfo("acquired address=[%s]", address_.c_str());
        sc_notifier_.NotifyAddressInfo(*address, getMediaID(sender_name_));
    }
}

void PlayerProvider::onStreamingEvent(const ptree& pt) {
    MMLogInfo("");
    auto cache_location = pt.get_optional<std::string>("CACHELOCATION");
    if (cache_location) {
        cache_path_buffer_ = *cache_location;
    }

    auto buff_value = pt.get_optional<int>("BUFFERING");
    auto speed_ignore_buffering_iter = speed_ignore_buffering_map_.find(sender_name_);
    if (buff_value) {
        //check for speed change request for poddbang
        if (media_type_ == MM::PlayerTypes::MediaType::PODBBANG) {
            if (speed_ignore_buffering_iter != speed_ignore_buffering_map_.end() && speed_ignore_buffering_iter->second) {
                MMLogInfo("Ignore buffering events during speed change");
                return;
            }
        }
        MMLogInfo("Streaming buffering=[%d]", *buff_value);
        int32_t buf_Idx = getBufferingAttrIdx(sender_name_);
        if (buf_Idx < 0) {
            MMLogError("Invalid Buffering Track index");
            return;
        }

        std::vector<MM::PlayerTypes::Buffering> buf_list = stub->getBufferingAttribute();
        std::vector<MM::PlayerTypes::Buffering>::iterator itr;
        if (buf_Idx > -1 && buf_list.size() > 0) {
            buf_list[buf_Idx].setProgress(*buff_value);
            buf_list[buf_Idx].setMedia_id(getMediaID(sender_name_));
            for (itr = buf_list.begin(); itr != buf_list.end(); itr++) {
                if ((itr - buf_list.begin()) != buf_Idx) {
                    itr->setActive(false);
                } else {
                    itr->setActive(true);
                }
            }
            stub->setBufferingAttribute(buf_list);
        }
        return;
    }

    auto cache_value = pt.get_optional<int>("CACHING");
    if (cache_value) {
        MMLogInfo("Streaming caching OK=[%d]", *cache_value);

        std::string path_buff("/Streaming/Partial_Temp_File");
        if ((CACHING_COMPLETED_STREAMING_PIPELINE == (*cache_value)) && (cache_path_buffer_.size() > 0)) {
            // Streaming_Pipeline(Golf) caching
            path_buff = cache_path_buffer_;
            sendDbusCaching(Caching::SetCachedMovie, path_buff);
        } else if (CACHING_COMPLETED_VO_PD == (*cache_value)) {
            // VisualOn PD caching
            path_buff.append("_PD");
            sendDbusCaching(Caching::SetCachedMusic, path_buff);
        } else if (CACHING_COMPLETED_VO_HLS == (*cache_value)) {
            // VisualOn HLS caching
            path_buff.append("_HLS");
            sendDbusCaching(Caching::SetCachedMusic, path_buff);
        }
        MMLogInfo("Streaming cache location=[%s]", path_buff.c_str());
    }
}

void PlayerProvider::onWarning(const ptree& pt) {
    auto vwarn_code = pt.get_optional<int>("WarnCode");
    if (!vwarn_code)
        return;

    int warn_code = *vwarn_code;
    switch (warn_code) {
        case WARNING_FRAMERATE_OVER_RANGE:
            sc_notifier_.NotifyWarning("FramerateOverRange", getMediaID(sender_name_));
            break;
        case WARNING_BITRATE_OVER_RANGE:
            sc_notifier_.NotifyWarning("BitrateOverRange", getMediaID(sender_name_));
            break;
        case WARNING_AUDIO_CODEC_NOT_SUPPORTED:
            sc_notifier_.NotifyWarning("AudioCodecNotSupported", getMediaID(sender_name_));
            break;
        case WARNING_LOADING_START:
            sc_notifier_.NotifyWarning("LoadingStart", getMediaID(sender_name_));
            break;
        case WARNING_LOADING_COMPLETE:
            sc_notifier_.NotifyWarning("LoadingComplete", getMediaID(sender_name_));
            break;
        default:
            MMLogInfo("[%d] ignored", warn_code);
            break;
    }
}

void PlayerProvider::onError(const ptree& pt) {
    auto verror_code = pt.get_optional<int>("ErrorCode");
    auto opt = pt.get_optional<std::string>("ErrorText");
    std::string err_msg = "";

    if (!verror_code)
        return;

    int32_t Idx = getCurrentTrackAttrIdx(sender_name_);
    if (Idx < 0) {
        MMLogInfo("Invalid Track index");
        return;
    }

    int error_code = *verror_code;
    switch (error_code) {
        case ERROR_OPERATION_NOT_SUPPORTED: // FALL-THROUGH
        case ERROR_PIPELINE_NOT_CREATED: {
            MMLogInfo("[%d] ignored", error_code);
            return;
        }
        case ERROR_FILE_NOT_SUPPORTED:{
            MMLogError("[%d] notify FILE_NOT_SUPPORTED", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::FILE_NOT_SUPPORTED, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_FILE_NOT_FOUND: {
            MMLogError("[%d] notify BAD_URI", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::BAD_URI, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_VIDEO_RESOLUTION_NOT_SUPPORTED: {
            MMLogError("[%d] notify VIDEO_RESOLUTION_NOT_SUPPORTED", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::VIDEO_RESOLUTION_NOT_SUPPORTED, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_STREAM_AUDIO_SHORT: {
            MMLogError("[%d] ERROR_STREAM_AUDIO_SHORT - Try using non provide clock option", error_code);
            command_queue_->PostFront(new command::OpenUriCommand(this, track_opened_[Idx].getIndex(),
                                                                  track_opened_[Idx].getUri(), media_type_, false,
                                                                  connection_map_, getMediaID(sender_name_), 0, sender_name_,  nullptr));
            break;
        }
        case ERROR_GST_NO_RESPONSE: {
            MMLogError("[%d] ERROR_GST_NO_RESPONSE - Try forced stop for restore", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::NO_RESPONSE_ERROR, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_AUDIO_CODEC_NOT_SUPPORTED:    // FALL-THROUGH
        case ERROR_VIDEO_CODEC_NOT_SUPPORTED:    // FALL-THROUGH
        case ERROR_RESOURCE_AUDIO_NOT_AVAILABLE: // FALL-THROUGH
        case ERROR_RESOURCE_VIDEO_NOT_AVAILABLE: // FALL-THROUGH
        case ERROR_GST_INTERNAL_ERROR: {
            MMLogError("[%d] notify PLAYBACK_INTERNAL_ERROR", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::PLAYBACK_INTERNAL_ERROR, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_GST_RESOURCE_ERROR_READ: {
            MMLogError("[%d] notify RESOURCE_READ_ERROR", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::RESOURCE_READ_ERROR, track_opened_[Idx], getMediaID(sender_name_));
            command_queue_->Post(new command::StopCommand(this, true, sender_name_, getMediaID(sender_name_), nullptr)); // pipeline could be playing.
            onPlaybackStopped();    // should not play next track.
            break;
        }
        case ERROR_RESOURCE_NOT_AUTHORIZED: {
             if (!opt) {
                 MMLogError("Error message not passed");
                 return;
             } else {
                 err_msg = *opt;
             }
             if (err_msg.compare("Forbidden") == 0) {
                 MMLogInfo("403 Resource forbidden Server Error (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_403, track_opened_[Idx], getMediaID(sender_name_));
             }
             break;
        }
        case ERROR_GST_RESOURCE_ERROR_OPEN_READ: {
             MMLogError("[%d] notify RESOURCE_OPEN_READ_ERROR", error_code);
             if (!opt) {
                 MMLogError("Error message not passed");
                 return;
             } else {
                 err_msg = *opt;
             }
             if (err_msg.compare("Internal Server Error") == 0) {
                 MMLogInfo("500 Internal Server Error (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_500, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("Not Implemented") == 0) {
                 MMLogInfo("501 Not Implemented (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_501, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("Bad Gateway") == 0) {
                 MMLogInfo("502 Bad Gateway (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_502, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("Service Unavailable") == 0) {
                 MMLogInfo("503 Service Unavailable (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_503, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("Gateway Timeout") == 0) {
                 MMLogInfo("504 Gateway Timeout (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_504, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("HTTP Version Not Supported") == 0) {
                 MMLogInfo("505 HTTP Version Not Supported (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_505, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("Internal Server Configuration Error") == 0) {
                 MMLogInfo("506 Internal Server Configuration Error (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_506, track_opened_[Idx], getMediaID(sender_name_));
             } else if (err_msg.compare("Insufficient Storage") == 0) {
                 MMLogInfo("507 Insufficient Storage (HTTP)");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::HTTP_ERROR_RESPONSE_507, track_opened_[Idx], getMediaID(sender_name_));
             } else {
                 MMLogInfo("RESOURCE_OPEN_READ_ERROR");
                 stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::RESOURCE_READ_ERROR, track_opened_[Idx], getMediaID(sender_name_));
             }

            break;
        }
        case ERROR_DIVX_AUDIO_CODEC_NOT_SUPPORTED:    // FALL-THROUGH
        case ERROR_DIVX_VIDEO_CODEC_NOT_SUPPORTED:{
            MMLogError("[%d] notify DIVX_CODEC_NOT_SUPPORTED", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::DIVX_CODEC_NOT_SUPPORTED, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_DIVX_VIDEO_RESOLUTION_NOT_SUPPORTED: {
            MMLogError("[%d] notify DIVX_RESOLUTION_NOT_SUPPORTED", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::DIVX_RESOLUTION_NOT_SUPPORTED, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        case ERROR_DIVX_UNAUTHORIZED: {
            MMLogError("[%d] notify DIVX_UNAUTHORIZED", error_code);
            stub->fireErrorOccuredEvent(MM::PlayerTypes::PlaybackError::DIVX_UNAUTHORIZED, track_opened_[Idx], getMediaID(sender_name_));
            break;
        }
        default: {
            MMLogError("[%d] Undefined error", error_code);
            return;
        }
    }

    onPlaybackError(pt);
}

void PlayerProvider::onPlaybackError(const ptree& pt) {
    MMLogInfo("");
    int proxyId = preparePEProxy(sender_name_);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return;
    }

    event_system_.SetEvent(command::EventType::ErrorOccured, nullptr);
    std::vector<MM::PlayerTypes::Duration> dur_t = stub->getDurationAttribute();
    int32_t Idx = getDurationAttrIdx(sender_name_);

    if (Idx > -1 && dur_t.size() > 0) {
        dur_t[Idx].setDuration(0);
        dur_t[Idx].setMedia_id(getMediaID(sender_name_));
        for(auto itr : dur_t) {
            itr.setActive(false);
        }
        dur_t[Idx].setActive(true);
        stub->setDurationAttribute(dur_t);
    }

    bool exist_play_command = false;
    static std::vector<command::CommandType> cv = { command::CommandType::Play };
    if (command_queue_->Exist(cv, sender_name_))
        exist_play_command = true;

    if (is_playing_ == false &&
            exist_play_command == false) {
        MMLogInfo("is_playing_ is false, so no need_post_command");
        return;
    }

    if (playback_option_.set_playback_error_and_stop) {
        onPlaybackStopped();
        //need_to_open_when_play_[proxyId] = false;
        return;
    }
#if 0
    auto need_post_command = [this]() -> bool {
        static std::vector<command::CommandType> cv = { command::CommandType::OpenPlaylist,
                                                        command::CommandType::OpenUri,
                                                        command::CommandType::OpenTrack,
                                                        command::CommandType::DequeueAll,
                                                        command::CommandType::Stop,
                                                        command::CommandType::Next,
                                                        command::CommandType::Previous,
                                                        command::CommandType::NextAndTrick,
                                                        command::CommandType::EnqueueUri };
        if (command_queue_->Exist(cv)) {
            MMLogInfo("dup commands, no need_post_command");
            return false;
        }

        MMLogInfo("need_post_command");
        return true;
    };

    int32_t optionLoop = WAIT_ON_ERROR_SECONDS;
    int32_t pos = (play_direction_ == Direction::Forward) ? 1 : -1;

    int32_t rate_index = getRateAttrIdx(sender_name_);
    if (rate_index < 0) {
        MMLogInfo("Invalid rate_index");
        return;
    }
    double rate = stub->getRateAttribute()[rate_index].getRate();
    if (rate != 1.0) {
        MMLogInfo("rate != 1.0");
        usleep(TimeConvert::SecToUs(1));
        if (need_post_command()) {
            command_queue_->Post(new command::NextAndTrickCommand(this, pos, rate));
        }
        return;
    }

    is_Error_state = true;
    MM::PlayerTypes::RepeatStatus repeat_status = stub->getRepeatOptionAttribute().getStatus();
    MMLogInfo("repeat_status: %d", repeat_status);
    switch (repeat_status) {
        case MM::PlayerTypes::RepeatStatus::REPEAT: // FALL-THROUGH
        case MM::PlayerTypes::RepeatStatus::REPEAT_DIRECTORY: {
            while (optionLoop-- > 0 && need_post_command())
                usleep(TimeConvert::SecToUs(1));

            if (need_post_command()) {
                if (playlist_mgr_->IsFirstOfTracks() && playlist_mgr_->IsLastOfTracks()) { // Only 1 file remain
                    MMLogInfo("Only 1 file remain.. stop");
                    onPlaybackStopped();
                    break;
                }

                if (!playback_option_.auto_next_command_on_error) {
                    MMLogInfo("disabled next on error");
                    onPlaybackStopped();
                    need_to_open_when_play_[proxyId] = false;
                    state_[proxyId] = State::Stopped;
                    break;
                }

                _mutex.lock();
                if (is_need_pause_) {
                    MMLogInfo("need pause after error handling");
                    is_need_pause_ = false;
                    pos = (play_direction_ == Direction::Forward) ? 2 : -2;
                } else {
                    MMLogInfo("don't need pause after error handling");
                }
                command_queue_->Post(new command::NextCommand(this, pos, sender_name_, nullptr));
                _mutex.unlock();

                if (is_playing_ == false && exist_play_command == true) {
                    MMLogInfo("add play command");
                    command_queue_->Post(new command::PlayCommand(this, sender_name_, nullptr));
                }
            }
            break;
        }
        case MM::PlayerTypes::RepeatStatus::REPEAT_SINGLE: {
            onPlaybackStopped();

            // Prevent continous play command
            need_to_open_when_play_[proxyId] = false;
            state_[proxyId] = State::Stopped;
            break;
        }
        case MM::PlayerTypes::RepeatStatus::NO_REPEAT: {
            if (play_direction_ == Direction::Forward && playlist_mgr_->IsLastOfTracks()) {
                MMLogInfo("last of track, so no need_post_command");
                sc_notifier_.NotifyAllTrackPlayed("LastOfTrack", getMediaID(sender_name_));
                onPlaybackStopped();
            } else if (play_direction_ == Direction::Backward && playlist_mgr_->IsFirstOfTracks()) {
                MMLogInfo("first of track, so no need_post_command");
                sc_notifier_.NotifyAllTrackPlayed("FirstOfTrack", getMediaID(sender_name_));
                onPlaybackStopped();
            } else if (!playback_option_.auto_next_command_on_error) {
                MMLogInfo("disabled next on error(no repeat)");
                onPlaybackStopped();
                need_to_open_when_play_[proxyId] = false;
                state_[proxyId] = State::Stopped;
            } else {
                while (optionLoop-- > 0 && need_post_command())
                    usleep(TimeConvert::SecToUs(1));
                if (need_post_command()) {
                    _mutex.lock();
                    if (is_need_pause_) {
                        MMLogInfo("need pause after error handling");
                        is_need_pause_ = false;
                        pos = (play_direction_ == Direction::Forward) ? 2 : -2;
                    } else {
                        MMLogInfo("don't need pause after error handling");
                    }
                    command_queue_->Post(new command::NextCommand(this, pos, sender_name_, nullptr));
                    _mutex.unlock();

                    if (is_playing_ == false && exist_play_command == true) {
                        MMLogInfo("add play command");
                        command_queue_->Post(new command::PlayCommand(this, sender_name_, nullptr));
                    }
                }
            }
            break;
        }
        case MM::PlayerTypes::RepeatStatus::NO_REPEAT_AND_SINGLE: {
            onPlaybackStopped();
            break;
        default:
            break;
        }
    }
    is_Error_state = false;
#endif
}

void PlayerProvider::onPlaybackStopped() {
    is_playing_ = false;
    int proxyId = preparePEProxy(sender_name_);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return;
    }

    need_to_open_when_play_[proxyId] = true;
    std::vector<MM::PlayerTypes::Playback> pb_t = stub->getPlaybackAttribute();
    std::vector<MM::PlayerTypes::Playback>::iterator itr;
    int32_t Idx = getPlaybackAttrIdx(sender_name_);

    if (Idx > -1 && pb_t.size() > 0) {
        pb_t[Idx].setMedia_id(getMediaID(sender_name_));
        pb_t[Idx].setStatus(MM::PlayerTypes::PlaybackStatus::STOPPED);
        for(itr = pb_t.begin(); itr != pb_t.end(); itr++) {
            if((itr - pb_t.begin()) == Idx) {
                itr->setActive(true);
            } else {
                itr->setActive(false);
            }
        }
        stub->setPlaybackAttribute(pb_t);
    }
    if (playback_option_.set_potion_on_stop_state) {
        std::vector<MM::PlayerTypes::Position> pos_list = stub->getPositionAttribute();
        std::vector<MM::PlayerTypes::Position>::iterator itr;
        int32_t Idx = getPositionAttrIdx(sender_name_);

        if (Idx > -1 && pos_list.size() > 0 ) {
            pos_list[Idx].setPosition(0);
            pos_list[Idx].setMedia_id(getMediaID(sender_name_));
            for(itr = pos_list.begin(); itr != pos_list.end(); itr++) {
                if((itr - pos_list.begin()) == Idx) {
                    itr->setActive(true);
                } else {
                    itr->setActive(false);
                }
            }
            stub->setPositionAttribute(pos_list);
        }
    }
    std::vector<MM::PlayerTypes::Rate> rate_t = stub->getRateAttribute();
    std::vector<MM::PlayerTypes::Rate>::iterator itr_rate;
    Idx = getRateAttrIdx(sender_name_);

    if (Idx > -1 && rate_t.size() > 0) {
        rate_t[Idx].setMedia_id(getMediaID(sender_name_));
        rate_t[Idx].setRate(1.0);
        for(itr_rate = rate_t.begin(); itr_rate != rate_t.end(); itr_rate++) {
            if((itr_rate - rate_t.begin()) == Idx) {
                itr_rate->setActive(true);
            } else {
                itr_rate->setActive(false);
            }
        }
        stub->setRateAttribute(rate_t);
    }

    std::vector<MM::PlayerTypes::Speed> speed_t = stub->getSpeedAttribute();
    std::vector<MM::PlayerTypes::Speed>::iterator itr_speed;
    Idx = getSpeedAttrIdx(sender_name_);

    if (Idx > -1 && speed_t.size() > 0) {
        speed_t[Idx].setMedia_id(getMediaID(sender_name_));
        speed_t[Idx].setSpeed(1.0);
        for(itr_speed = speed_t.begin(); itr_speed != speed_t.end(); itr_speed++) {
            if((itr_speed - speed_t.begin()) == Idx) {
                itr_speed->setActive(true);
            } else {
                itr_speed->setActive(false);
            }
        }
        stub->setSpeedAttribute(speed_t);
    }
}

void PlayerProvider::resetPEProxy(std::string connectionName) {
    MMLogInfo("");
    int proxyId = 0;
    std::map<std::string, int>::iterator it;
    it = pe_proxy_map_.find(connectionName);
    if (it != pe_proxy_map_.end()) {
        proxyId = it->second;

        if ( (proxyId > -1) && (proxyId < MAX_PLAYER_ENGINE_INSTANCE) ) {
            g_dbus_connection_signal_unsubscribe(gbus_sync_connection_[proxyId], sig_id_state_change_[proxyId]);
            g_object_unref(gbus_sync_connection_[proxyId]);
            pe_proxy_map_.erase(connectionName);
            sig_id_state_change_[proxyId] = 0;
            g_object_unref(playerengine_proxy_[proxyId]);
            playerengine_proxy_[proxyId] = nullptr;
            MMLogInfo("Connection Name %s is erased", connectionName.c_str());
        }
    }
}

int PlayerProvider::preparePEProxy(std::string connectionName, bool addMode) {

    int i = 0, proxyId = 0;
    if (connectionName.empty()) {
        MMLogInfo("Invalid Connection name %s ", connectionName.c_str());
        return -1;
    }
    std::map<std::string, int>::iterator it;
    it = pe_proxy_map_.find(connectionName);
    if (it != pe_proxy_map_.end()) {
        proxyId = it->second;
        return proxyId;
    } else if (addMode == false) {
        MMLogInfo("There is no connection name(%s) in map", connectionName.c_str());
        return -1;
    } else {
        std::map<std::string, int>::iterator it1;
        if (!clean_connection_.empty()) {
            MMLogInfo("clean_connection_ is needed: %s",clean_connection_.c_str());
            it1 = pe_proxy_map_.find(clean_connection_);
            if ((it1 != pe_proxy_map_.end()) &&
                (it1->second > -1) &&
                (it1->second < MAX_PLAYER_ENGINE_INSTANCE)){
                state_[it1->second] = State::Stopped;
                need_to_open_when_play_[it1->second] = true;
                sig_id_state_change_[it1->second] = 0;
                pe_proxy_map_.erase(clean_connection_);
            }
            clean_connection_ = "";
        }
        for (i=1; i<MAX_PLAYER_ENGINE_INSTANCE; i++) {
            bool bFoundProxy = false;
            for (it = pe_proxy_map_.begin(); it != pe_proxy_map_.end(); ++it) {
                if (it->second == i) {
                    bFoundProxy = true;
                    break;
                }
            }
            if (!bFoundProxy){
                proxyId = i;
                MMLogInfo("Proxy id inserted as %d, [%s]", proxyId, connectionName.c_str());
                pe_proxy_map_.insert(std::pair<std::string, int>(connectionName, proxyId));
                break;
            }
        }
        if (i >= MAX_PLAYER_ENGINE_INSTANCE) {
            MMLogInfo("Proxy queue is full... can not create more than %d proxy objects", MAX_PLAYER_ENGINE_INSTANCE);
            return -1;
        }
    }

    MMLogInfo("preparePEProxy connectionName =%s, PEProxyID = %d", connectionName.c_str(), proxyId);
    if (proxyId <= -1) {
      MMLogError("Invalid proxy ID");
      return -1;
    }
    GError *error = NULL;
    playerengine_proxy_[proxyId] = com_lge_player_engine_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        connectionName.c_str(),
        "/com/lge/PlayerEngine",
        NULL,
        &error
    );
    if (error) {
        MMLogError("Failed to set up playerengine_proxy : %s", error->message);
        g_error_free(error);
        return -1;
    }
    MMLogInfo("playerengine_proxy_[%d] is created", proxyId);
    gbus_sync_connection_[proxyId] = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        MMLogError("Failed to set up connection for signal listener : %s", error->message);
        g_error_free(error);
        return -1;
    }

    if (!sig_id_state_change_[proxyId]) {
        MMLogInfo("proxyId = %d::connectionName = %s", proxyId, connectionName.c_str());
        sig_id_state_change_[proxyId] = g_dbus_connection_signal_subscribe(
          gbus_sync_connection_[proxyId],
          connectionName.c_str(),
          "com.lge.PlayerEngine",
          "StateChange",
          NULL,
          NULL,
          G_DBUS_SIGNAL_FLAGS_NONE,
          PlayerProvider::handleStateChange,
          this,
          NULL
        );
    }

    return proxyId;
}

bool PlayerProvider::stopTrickPlay(command::Coro::pull_type& in, bool include_seek, std::string connectionName) {
    MMLogInfo("");

    GError *dbus_error = NULL;
    gboolean succeed = FALSE;

    int proxyId = preparePEProxy(connectionName);
    if (proxyId <= -1) {
        MMLogInfo("Invalid Proxy Id");
        return false;
    }

    com_lge_player_engine_call_stop_rate_change_sync(
        playerengine_proxy_[proxyId],
        &succeed,
        NULL,
        &dbus_error
    );

    if (succeed == TRUE) {
        event_system_.WaitEvent(in, (uint32_t)command::EventType::AsyncDone, (uint32_t)command::EventType::ErrorOccured);
    } else if (dbus_error) {
        g_error_free(dbus_error);
    }

    int32_t Idx = getPositionAttrIdx(sender_name_);
    if (include_seek == true && Idx > -1) {
        command::SetPositionCommand spc(this, false, stub->getPositionAttribute()[Idx].getPosition(), sender_name_, nullptr);
        spc.Execute(in);
    }
    std::vector<MM::PlayerTypes::Rate> rate_t = stub->getRateAttribute();
    std::vector<MM::PlayerTypes::Rate>::iterator itr;
    Idx = getRateAttrIdx(sender_name_);

    if (Idx > -1 && rate_t.size() > 0) {
        rate_t[Idx].setMedia_id(getMediaID(sender_name_));
        rate_t[Idx].setRate(1.0);
        for(itr = rate_t.begin(); itr != rate_t.end(); itr++) {
            if((itr - rate_t.begin()) == Idx) {
                itr->setActive(true);
            } else {
                itr->setActive(false);
            }
        }
        stub->setRateAttribute(rate_t);
    }

    controlMuteForTrickPlay(in, 1.0f, connectionName);

    return true;
}

bool PlayerProvider::controlMuteForTrickPlay(command::Coro::pull_type& in, double rate, std::string connectionName) {
    if (playback_option_.audio_mute_on_trick == false)
        return true;

    MM::PlayerTypes::MuteStatus mute;
    if (fabs(rate - 1.0) > DBL_EPSILON) {
        mute = MM::PlayerTypes::MuteStatus::MUTED;
        MMLogInfo("MUTED~!!, rate=[%lf]", rate);
    } else {
        static std::vector<command::CommandType> cv = { command::CommandType::SetMute };
        if (command_queue_->Exist(cv, connectionName)) {
            MMLogWarn("BOS case, unmute will be handled by HMI request");
            return true;
        }
        mute = MM::PlayerTypes::MuteStatus::UNMUTED;
        MMLogInfo("UNMUTED~!!, rate=[%lf]", rate);
    }
    command::SetMuteCommand smc(this, mute, connectionName);
    smc.Execute(in);

    return true;
}

void PlayerProvider::fillAudioLanguageList(const ptree& pt) {
    audio_tracks_.clear();
    try {
        char buff[256];
        uint32_t index = 1;
        ptree audio_track = pt.get_child("TrackInfo.AudioTrack");
        std::for_each(std::begin(audio_track), std::end(audio_track),
            [&](ptree::value_type& kv) {
                std::string iso639 = kv.second.get<std::string>("Format");
                std::string english_name = LangConvert::ISO639ToEnglishName(iso639);
                if (english_name == "")
                    english_name = "Unknown";
                snprintf(buff, 256, "%s %d", english_name.c_str(), index);
                audio_tracks_.push_back(buff);
                index++;
            }
        );
    } catch (const boost::property_tree::ptree_error& exception) {
        MMLogInfo("No AudioTrack : %s", exception.what());
    }
}

void PlayerProvider::fillSubtitleList(const ptree& pt) {
    subtitle_tracks_.clear();
    try {
        ptree subtitle_track = pt.get_child("TrackInfo.TextTrack");
        std::for_each(std::begin(subtitle_track), std::end(subtitle_track),
            [&](ptree::value_type& kv) {
                std::string iso639 = kv.second.get<std::string>("Format");
                std::string english_name = LangConvert::ISO639ToEnglishName(iso639);
                if (english_name == "")
                    english_name = "Subtitle1";
                subtitle_tracks_.push_back(english_name);
            }
        );
    } catch (const boost::property_tree::ptree_error& exception) {
        MMLogInfo("No TextTrack : %s", exception.what());
    }
}

void PlayerProvider::getSubtitlePath(const std::string& url, MM::PlayerTypes::MediaType media_type, boost::property_tree::ptree& pt) {
    if (media_type == MM::PlayerTypes::MediaType::VIDEO ||
        media_type == MM::PlayerTypes::MediaType::USB_VIDEO1 ||
        media_type == MM::PlayerTypes::MediaType::USB_VIDEO2) {
        std::string subtitle_uri(url);
        std::size_t found = subtitle_uri.find(kFilePrefix);
        if (found != std::string::npos) {
            subtitle_uri = subtitle_uri.substr(found+kFilePrefix.size());
            pt.put("subtitle-path", subtitle_uri);
        } else {
            MMLogInfo("Invalid file URI in USB_VIDEO : %s", subtitle_uri.c_str());
        }
    } else if (media_type == MM::PlayerTypes::MediaType::GOLF_VIDEO) {
        std::string filename_path(url);
        std::string golf_subtitle_uri(GOLF_SUBTITLE_PATH);

        std::size_t found = filename_path.find(".mp4");
        if (found != std::string::npos) {
            filename_path.erase(filename_path.begin()+found+4, filename_path.end());
        }
        found = filename_path.find("/external/");
        if (found != std::string::npos) {
            filename_path = filename_path.substr(found+10);
        } else {
            found = filename_path.find_last_of("/");
            filename_path = filename_path.substr(found+1);
        }
        golf_subtitle_uri.append(filename_path);
        MMLogInfo("Golf video subtitle path=[%s]", golf_subtitle_uri.c_str());
        pt.put("subtitle-path", golf_subtitle_uri);
    }
}

bool PlayerProvider::getVideoResolution(const ptree& pt, int32_t& out_width, int32_t& out_height) {
    out_width = 0;
    out_height = 0;

    try {
        ptree video_track = pt.get_child("TrackInfo.VideoTrack");

        std::for_each(std::begin(video_track), std::end(video_track),
            [&](ptree::value_type& kv) {
                out_width = kv.second.get<int32_t>("Width");
                out_height = kv.second.get<int32_t>("Height");
            }
        );
    } catch (const boost::property_tree::ptree_error& exception) {
        MMLogInfo("No Resolution Info : %s", exception.what());
        out_width = 0;
        out_height = 0;
    }

    if (out_width > 0 && out_height > 0)
        return true;
    else
        return false;
}

playlist::SortMode PlayerProvider::convSortMode(const MM::PlayerTypes::SortMode& mode) {
    switch (mode) {
        case MM::PlayerTypes::SortMode::TITLE:
            return playlist::SortMode::Title;
        case MM::PlayerTypes::SortMode::URI:
            return playlist::SortMode::Uri;
    }
    return playlist::SortMode::Title;
}

playlist::ShuffleOption PlayerProvider::convShuffleOption(const MM::PlayerTypes::ShuffleOption& option) {
    playlist::ShuffleOption result;

    if (option.getMode() == MM::PlayerTypes::ShuffleMode::FIXED_ARRAY)
        result.use_fixed_array = true;
    else
        result.use_fixed_array = false;

    result.fixed_array_size = option.getCount();

    return result;
}

void PlayerProvider::addRecorderOption(boost::property_tree::ptree& pt) {
    std::string sCodec = "";
    std::string sFileType = "";

    switch (codec_type_) {
        case MM::PlayerTypes::ACodecType::AAC:
            sCodec = "AAC";
            break;
        case MM::PlayerTypes::ACodecType::OPUS:
            sCodec = "OPUS";
            break;
        case MM::PlayerTypes::ACodecType::WAV_PCM:
            sCodec = "WAV_PCM";
            break;
        default:
            sCodec = "WAV_PCM";
            break;
    }
    switch (file_type_) {
        case MM::PlayerTypes::FileFormatType::MP4:
            sFileType = "MP4";
            break;
        case MM::PlayerTypes::FileFormatType::OGG:
            sFileType = "OGG";
            break;
        case MM::PlayerTypes::FileFormatType::WAV:
            sFileType = "WAV";
            break;
        default:
            sFileType = "WAV";
            break;
    }

    pt.put("transcode_codec", sCodec);
    pt.put("transcode_fileformat", sFileType);
    if (uri_transcode_output_.size() > 0) {
        pt.put("transcode_output", uri_transcode_output_);
    }
    if (file_location.size() > 0) {
        pt.put("transcode_location", file_location);
    }
    if (platform_name.size() > 0) {
        pt.put("transcode_platform", platform_name);
    }
}

void PlayerProvider::sendDbusCaching(Caching mode, std::string& cache_url) {
    GError *error = NULL;
    GVariant *ret = NULL;
    GVariant *params = NULL;
    const gchar* check_utf8 = NULL;
    const gchar* method_name = NULL;
    const gchar* subtitle_path = NULL;
    if (cache_url.size() == 0) {
        MMLogError("Invalid cache url");
        return;
    }

    GDBusConnection *conn = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
    if (conn == NULL || error) {
        MMLogError("get dbus connection fail");
        if (error)
            g_error_free(error);
        return;
    }

    if (mode == Caching::SetCachedMusic || mode == Caching::SetCachedMovie) {
        std::string cdn_url = "";
        std::string golf_full_path(GOLF_SUBTITLE_PATH);
        int32_t Idx = getCurrentTrackAttrIdx(sender_name_);
        if (Idx < 0) {
            MMLogError("Invalid Track index");
            g_object_unref(conn);
            return;
        }

        cdn_url = track_opened_[Idx].getUri();
        if (cdn_url.size() < 3) {
            MMLogError("Invalid cdn url[%s]", cdn_url.c_str());
            g_object_unref(conn);
            return;
        }

        if (cdn_url.at(0) == '[' && cdn_url.at(2) == ']')
            cdn_url.erase(0, 3);
        check_utf8 = (const gchar*)cdn_url.c_str();
        if (g_utf8_validate(check_utf8, -1, NULL)) {
            MMLogInfo("cdn_url[%s] is UTF-8", check_utf8);
        } else {
            MMLogWarn("cdn_url[%s] is not UTF-8", check_utf8);
        }

        if (mode == Caching::SetCachedMovie) {
            std::string filename(cdn_url);

            std::size_t found = filename.find(".mp4");
            if (found != std::string::npos) {
                filename.erase(filename.begin()+found+1, filename.end());
                filename.append("smi");
            }

            found = filename.find_last_of("/");
            if (found != std::string::npos) {
                filename = filename.substr(found+1);
            }
            golf_full_path.append(filename);
            MMLogInfo("golf_subtitle_url[%s]", golf_full_path.c_str());
            subtitle_path = (const gchar*)golf_full_path.c_str();
        }

        if (mode == Caching::SetCachedMusic) {
            method_name = CACHE_SETCACHEDMUSIC;
            params = g_variant_new ("(ss)", check_utf8, cache_url.c_str());
        } else if (mode == Caching::SetCachedMovie) {
            method_name = CACHE_SETCACHEDMOVIE;
            params = g_variant_new ("(sss)", check_utf8, cache_url.c_str(), subtitle_path);
        }
    } else if (mode == Caching::GetCachedMovie) {
        std::string cdn_url = cache_url;
        if (cdn_url.at(0) == '[' && cdn_url.at(2) == ']')
            cdn_url.erase(0, 3);
        method_name = CACHE_GETCACHEDMOVIE;
        params = g_variant_new ("(s)", cdn_url.c_str());
    } else {
        // SHOULD NOT BE HERE
        MMLogError("Invalid argument");
        g_object_unref(conn);
        return;
    }

    ret = g_dbus_connection_call_sync(conn, CACHE_SERVICE_NAME, CACHE_OBJECT_PATH, CACHE_INTERFACE_NAME, method_name,
                                      params, /* No need to unref*/
                                      NULL,
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      &error);
    if (error) {
        MMLogError("caching gdbus call fail");
        g_error_free(error);
        g_object_unref(conn);
        return;
    }

    if (ret) {
        gchar* ret_1 = NULL;
        gchar* ret_2 = NULL;
        gchar* ret_3 = NULL;
        if (mode == Caching::SetCachedMusic)
            g_variant_get(ret, "(ss)", &ret_1, &ret_2); // "OK" / "NOT_FOUND" / "FAIL"
        else
            g_variant_get(ret, "(sss)", &ret_1, &ret_2, &ret_3); // "OK" / "NOT_FOUND" / "FAIL"
        MMLogInfo("gdbus return OK, [%s], [%s], [%s]", ret_1, ret_2, ret_3);

        if (mode == Caching::GetCachedMovie && g_strcmp0(ret_1, "OK") == 0) {
            std::string saved_path(ret_2 ? ret_2 : "");
            saved_path.insert(0, "file://");
            if (cache_url.at(0) == '[' && cache_url.at(2) == ']')
                saved_path.insert(0, cache_url, 0, 3);
            cache_url = saved_path;
            MMLogInfo("updated url=[%s]", cache_url.c_str());
        }
        g_free(ret_1);
        g_free(ret_2);
        if (ret_3)
            g_free(ret_3);
        g_variant_unref(ret);
        ret = NULL;
    }
    if (conn) {
        g_object_unref(conn);
        conn = NULL;
    }
}

std::string PlayerProvider::MediaTypeToString(MM::PlayerTypes::MediaType media_type) {
    switch (media_type) {
        case MM::PlayerTypes::MediaType::AUDIO:      // FALL-THROUGH
        case MM::PlayerTypes::MediaType::USB_AUDIO1: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::RTSP_AUDIO:
            return "audio";
        case MM::PlayerTypes::MediaType::USB_AUDIO2: // FALL-THROUGH
            return "audio_2nd";
        case MM::PlayerTypes::MediaType::VIDEO:      // FALL-THROUGH
        case MM::PlayerTypes::MediaType::USB_VIDEO1: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::RTSP_VIDEO:
            return "video";
        case MM::PlayerTypes::MediaType::USB_VIDEO2: // FALL-THROUGH
            return "video_2nd";
        case MM::PlayerTypes::MediaType::STREAM:
            return "transcode";
        case MM::PlayerTypes::MediaType::NATURE_SOUND:
            return "nature_sound";
        case MM::PlayerTypes::MediaType::KAOLAFM:
            return "kaola_fm";
        case MM::PlayerTypes::MediaType::MELON:
            return "melon";
        case MM::PlayerTypes::MediaType::QQMUSIC:
            return "qq_music";
        case MM::PlayerTypes::MediaType::KAKAOI:
            return "kakao_i";
        case MM::PlayerTypes::MediaType::KAKAOI2:
            return "kakao_i2";
        case MM::PlayerTypes::MediaType::KAKAOI3:
            return "kakao_i3";
        case MM::PlayerTypes::MediaType::GENIE:
            return "genie";
        case MM::PlayerTypes::MediaType::XIMALAYA:
            return "ximalaya";
        case MM::PlayerTypes::MediaType::MANUAL_VIDEO:
            return "manual_video";
        case MM::PlayerTypes::MediaType::GOLF_VIDEO:
            return "golf_video";
        case MM::PlayerTypes::MediaType::MOOD_THERAPY_AUDIO:
            return "mood_therapy_audio";
        case MM::PlayerTypes::MediaType::MOOD_THERAPY_VIDEO:
            return "mood_therapy_video";
        case MM::PlayerTypes::MediaType::KIDS_VIDEO:
            return "kids_video";
        case MM::PlayerTypes::MediaType::RECORDING_PLAY:
            return "recording_play";
        case MM::PlayerTypes::MediaType::PODBBANG:
            return "podbbang";
        case MM::PlayerTypes::MediaType::DVRS_FRONT:
            return "dvrs_front";
        case MM::PlayerTypes::MediaType::DVRS_REAR:
            return "dvrs_rear";
        case MM::PlayerTypes::MediaType::FACE_DETECTION:
            return "face_detection";
        case MM::PlayerTypes::MediaType::VIBE:
            return "vibe";
        case MM::PlayerTypes::MediaType::TENCENT_FUNAUDIO:
            return "tencent_funaudio";
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_AUDIO:
            return "tencent_mini_app_audio";
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_VIDEO:
            return "tencent_mini_app_video";
        case MM::PlayerTypes::MediaType::WELAAA_AUDIO_STREAMING:
            return "welaaa_audio_streaming";
        case MM::PlayerTypes::MediaType::GENESIS_AUDIO_STREAMING:
            return "genesis_audio_streaming";
        default:
            return std::string();
    }
}

bool PlayerProvider::IsVideoType(MM::PlayerTypes::MediaType media_type) {
    switch (media_type) {
        case MM::PlayerTypes::MediaType::VIDEO:      // FALL-THROUGH
        case MM::PlayerTypes::MediaType::USB_VIDEO1: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::USB_VIDEO2: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::RTSP_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GOLF_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KIDS_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MOOD_THERAPY_VIDEO:
        case MM::PlayerTypes::MediaType::RECORDING_PLAY:
        case MM::PlayerTypes::MediaType::DVRS_FRONT:
        case MM::PlayerTypes::MediaType::DVRS_REAR:
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_VIDEO:
            return true;
        default:
            return false;
    }
}

bool PlayerProvider::IsStreamingType(MM::PlayerTypes::MediaType media_type) {
    switch (media_type) {
        case MM::PlayerTypes::MediaType::STREAM:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAOLAFM:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MELON:    // FALL-THROUGH
        case MM::PlayerTypes::MediaType::QQMUSIC:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI2:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI3:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GENIE:    // FALL-THROUGH
        case MM::PlayerTypes::MediaType::XIMALAYA: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GOLF_VIDEO:
        case MM::PlayerTypes::MediaType::PODBBANG:
        case MM::PlayerTypes::MediaType::VIBE:
        case MM::PlayerTypes::MediaType::TENCENT_FUNAUDIO:
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_AUDIO:
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_VIDEO:
        case MM::PlayerTypes::MediaType::WELAAA_AUDIO_STREAMING:
        case MM::PlayerTypes::MediaType::GENESIS_AUDIO_STREAMING:
            return true;
        default:
            return false;
    }
}

bool PlayerProvider::IsCacheNeeded(MM::PlayerTypes::MediaType media_type) {
    switch (media_type) {
        case MM::PlayerTypes::MediaType::STREAM:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAOLAFM:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::QQMUSIC:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI2:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI3:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::XIMALAYA:
            return false;
        case MM::PlayerTypes::MediaType::GOLF_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MELON:      // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GENIE:
            return true;
        default:
            return false;
    }
}

bool PlayerProvider::IsPlayerEngineReuse(MM::PlayerTypes::MediaType media_type) {
    switch (media_type) {
        case MM::PlayerTypes::MediaType::STREAM: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAOLAFM:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::QQMUSIC:  // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI2:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI3:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::XIMALAYA:
        case MM::PlayerTypes::MediaType::MANUAL_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GOLF_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KIDS_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::RECORDING_PLAY: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::DVRS_FRONT:
        case MM::PlayerTypes::MediaType::DVRS_REAR:
        case MM::PlayerTypes::MediaType::FACE_DETECTION:
            return false;
        default:
            return true;
    }
}
  
#if 0
void PlayerProvider::resetPlaylistManager() {
    playlist::PlaylistType playlist_type;
    if (stub->getPlaylistTypeAttribute() == MM::PlayerTypes::PlaylistType::ORDERED)
        playlist_type = playlist::PlaylistType::Ordered;
    else {
        // FIXME:
        MMLogInfo("Unordered play-list ");
        playlist_type = playlist::PlaylistType::Unordered;
    }

    playlist::PlayMode play_mode;
    if (stub->getShuffleAttribute().getStatus() == MM::PlayerTypes::ShuffleStatus::SHUFFLE)
        play_mode = playlist::PlayMode::Shuffle;
    else if (stub->getRandomAttribute() == MM::PlayerTypes::RandomStatus::RANDOM)
        play_mode = playlist::PlayMode::Random;
    else
        play_mode = playlist::PlayMode::Normal;

    playlist::SortMode sort_mode = convSortMode(stub->getSortModeAttribute());

    playlist::ShuffleOption so = convShuffleOption(stub->getShuffleOptionAttribute());

    playlist_mgr_.reset(
        playlist::PlaylistManagerBuilder()
            .SetPlaylistType(playlist_type)
            .SetPlayMode(play_mode)
            .SetSortMode(sort_mode)
            .SetLangCode(playlist::GetGlobalLangCode())
            .SetShuffleMode(Option::directory_shuffle_mode())
            .SetShuffleOption(so)
            .Build()
    );
    MMLogInfo("reset list_mgr OK. pl_type=[%d], p_mode=[%d], s_mode=[%d]", playlist_type, play_mode, sort_mode);
}
#endif
bool PlayerProvider::blockPlayback() {
    if (playback_option_.entertainment_mode == false) {
        MMLogInfo("Entertainment Off");
        return true;
    }
    return false;
}

} // namespace player
} // namespace mm
} // namespace lge

