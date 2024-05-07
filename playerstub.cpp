#include "playerstub.h"

#include "option.h"
#include "player_logger.h"

namespace MM = ::v1::org::genivi::mediamanager;

namespace lge {
namespace mm {
namespace player {
int PlayerStubImpl::default_media_id = 0;

PlayerStubImpl::PlayerStubImpl(PlayerProvider *player, std::shared_ptr<command::Queue>& sp_command_queue)
  : attr_mutex_(),
    max_pe_instance_(1),
    command_queue_(sp_command_queue),
    last_retry_id_(0) {
    MMLogInfo("");

    player_ = player;
    player_->stub = this;
    map_media_id_.clear();

    playerenginemanager_ = new (std::nothrow) PlayerEngineManager();
    if (playerenginemanager_ == nullptr)
        MMLogDebug("Bad allocation while creating playerengine manager");

    try {
        initializeDefaultValues();
    } catch (...) {
        MMLogError("exception caught");
    }
}

PlayerStubImpl::~PlayerStubImpl() {
    MMLogInfo("");
    if (playerenginemanager_) {
        delete playerenginemanager_;
        playerenginemanager_ = nullptr;
    }
}

void PlayerStubImpl::initializeDefaultValues() {
  MMLogInfo("");

    trySetMuteAttribute(mute_);
    trySetShuffleAttribute(MM::PlayerTypes::Shuffle(MM::PlayerTypes::ShuffleStatus::UNSHUFFLE, 0));
    trySetRepeatOptionAttribute(MM::PlayerTypes::RepeatOption(MM::PlayerTypes::RepeatStatus::REPEAT, 0));
    trySetRandomAttribute(MM::PlayerTypes::RandomStatus::NO_RANDOM);
    trySetRateAttribute(rate_);
    trySetSpeedAttribute(speed_);
    trySetVolumeAttribute(volume_);
    trySetCanGoNextAttribute(false);
    trySetCanGoPreviousAttribute(false);
    trySetCanPauseAttribute(false);
    trySetCanPlayAttribute(false);
    trySetCanSeekAttribute(false);
    trySetCurrentTrackAttribute(track_);
    trySetPlaybackAttribute(playback_);
    trySetPositionAttribute(pos_);
    trySetDurationAttribute(dur_);

    MM::PlayerTypes::OptionMap om;
    player_->playback_option_.Get(om);
    trySetPlaybackOptionAttribute(om);

    trySetMediaTypeAttribute(MM::PlayerTypes::MediaType::AUDIO);
    trySetSortModeAttribute(MM::PlayerTypes::SortMode::URI);
    MM::PlayerTypes::ShuffleOption so(MM::PlayerTypes::ShuffleMode::VARIABLE_LIST, 500);
    trySetShuffleOptionAttribute(so);

    MM::PlayerTypes::PlaylistType playlist_type = MM::PlayerTypes::PlaylistType::ORDERED;
    std::string default_playlist_type = Option::default_playlist_type();
    if (default_playlist_type.compare("ordered") == 0)
        playlist_type = MM::PlayerTypes::PlaylistType::ORDERED;
    else if (default_playlist_type.compare("unordered") == 0)
        playlist_type = MM::PlayerTypes::PlaylistType::UNORDERED;
    trySetPlaylistTypeAttribute(playlist_type);

    map_media_id_.insert({MM::PlayerTypes::MediaType::VIDEO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::USB_AUDIO1, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::USB_AUDIO2, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::USB_VIDEO1, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::USB_VIDEO2, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::STREAM, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::NATURE_SOUND, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::KAOLAFM, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::MELON, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::QQMUSIC, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::GENIE, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::KAKAOI, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::KAKAOI2, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::KAKAOI3, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::XIMALAYA, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::MANUAL_VIDEO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::GOLF_VIDEO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::MOOD_THERAPY_AUDIO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::MOOD_THERAPY_VIDEO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::KIDS_VIDEO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::RECORDING_PLAY, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::PODBBANG, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::DVRS_FRONT, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::DVRS_REAR, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::FACE_DETECTION, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::VIBE, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::TENCENT_FUNAUDIO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::TENCENT_MINI_APP_AUDIO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::TENCENT_MINI_APP_VIDEO, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::WELAAA_AUDIO_STREAMING, 0});
    map_media_id_.insert({MM::PlayerTypes::MediaType::GENESIS_AUDIO_STREAMING, 0});
}

void PlayerStubImpl::arrangeTranscodeOutput(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _out_uri, MM::PlayerTypes::ACodecType _c_type,
        MM::PlayerTypes::FileFormatType _f_type, std::string _location, std::string _platform, arrangeTranscodeOutputReply_t _reply) {
    MMLogInfo("url=[%s]", _out_uri.c_str());
    player_->uri_transcode_output_ = _out_uri;
    player_->codec_type_ = _c_type;
    player_->file_type_ = _f_type;
    player_->file_location = _location;
    player_->platform_name = _platform;
    _reply(MM::PlayerTypes::PlayerError::NO_ERROR);
}

void PlayerStubImpl::openUri(const std::shared_ptr<CommonAPI::ClientId> _client,
                             std::string _uri, uint32_t _channels, MM::PlayerTypes::MediaType _type, openUriReply_t _reply) {
    MMLogInfo("load contents.. channels=[%u]", _channels);
    int mediaId = 0;
    int retCnt = 10;
    std::string connectionName;
    MM::PlayerTypes::MediaType currentType = _type; // GENSIX-71219 : prevent timing issue
    if (currentType == MM::PlayerTypes::MediaType::AUDIO) { // Local USB audio
        mediaId = default_media_id;
        checkValidMediaId();
        connectionName = playerenginemanager_->getDefaultConnectionName();
        if(!connectionName.empty() && default_media_id > 0) {
            MMLogInfo("connection name = %s received for default", connectionName.c_str());
            player_->clean_connection_ = playerenginemanager_->insertDefaultMediaId(default_media_id, connectionName);
            playerenginemanager_->printMap();
        } else {
            MMLogInfo("Invalid connection name(null) received for default - restoring connection[%d]", default_media_id);
            if (default_media_id <= 0) {
                playerenginemanager_->createPlayerEngine();
                default_media_id = playerenginemanager_->getPID();
                connectionName = playerenginemanager_->getConnectionName();
                while (retCnt > 0 && !playerenginemanager_->checkValidConnectionName(default_media_id, connectionName)) {
                    MMLogInfo("Duplicated connection name is detected.. retry getConnection");
                    retCnt--;
                    usleep(100 * 1000);
                    connectionName = playerenginemanager_->getConnectionName();
                }

                if (default_media_id <= 0 || connectionName.empty()) {
                    MMLogError("Fail to restore");
                    _reply(MM::PlayerTypes::PlayerError::MEDIA_MANAGER_INTERNAL_ERROR, 0);
                    return;
                }
                mediaId = default_media_id;
                player_->clean_connection_ = playerenginemanager_->insertDefaultMediaId(default_media_id, connectionName);
                playerenginemanager_->printMap();
            } else {
                MMLogError("Engine is restored by main.cpp, but not started yet");
                max_pe_instance_++;
                do {
                    retCnt--;
                    usleep(20 * 1000);
                    connectionName = playerenginemanager_->getConnectionName();
                    if (!connectionName.empty() && playerenginemanager_->checkValidConnectionName(default_media_id, connectionName)) {
                        player_->clean_connection_ = playerenginemanager_->insertDefaultMediaId(default_media_id, connectionName);
                        playerenginemanager_->printMap();
                        break;
                    }
                } while (retCnt > 0);
            }
        }
    } else {
        if (map_media_id_.find(currentType) == map_media_id_.end()) {
            MMLogInfo("New type is requested... need to check. type=[%d]", static_cast<int32_t>(currentType));
            map_media_id_[currentType] = 0;
        }
        MMLogInfo("Creating PE for multi instance, type=[%d], saved_id=[%u], pe_cnt=[%d]",
                  static_cast<int32_t>(currentType), map_media_id_[currentType], max_pe_instance_);
        checkValidMediaId();

        if (map_media_id_[currentType] > 0) {
            mediaId = map_media_id_[currentType];
            if ((playerenginemanager_->getConnectionName(mediaId)).empty()) {
                MMLogInfo("There is no connection name in map. id=[%d]", mediaId);
                connectionName = playerenginemanager_->getConnectionName();
                while (retCnt > 0 && !playerenginemanager_->checkValidConnectionName(mediaId, connectionName)) {
                    MMLogWarn("Fail to get valid ConnectionName.. Try again..");
                    retCnt--;
                    usleep(100 * 1000);
                    connectionName = playerenginemanager_->getConnectionName();
                }
                if (retCnt > 0) {
                    playerenginemanager_->insertMediaId(mediaId, connectionName);
                } else {
                    MMLogError("Fail to get connection name.");
                    _reply(MM::PlayerTypes::PlayerError::MEDIA_MANAGER_INTERNAL_ERROR, 0);
                    return;
                }
            }
        } else {
            if (max_pe_instance_ <= MAX_PLAYER_ENGINE_INSTANCE) {
                max_pe_instance_++;
                playerenginemanager_->createPlayerEngine();
                mediaId = playerenginemanager_->getPID();
                map_media_id_[currentType] = mediaId;

                connectionName = playerenginemanager_->getConnectionName();
                retCnt = 20; // Timeout 2s -> 4s
                while (retCnt > 0 && !playerenginemanager_->checkValidConnectionName(mediaId, connectionName)) {
                    MMLogInfo("Duplicated connection name is detected.. retry getConnection");
                    retCnt--;
                    usleep(100 * 1000);
                    connectionName = playerenginemanager_->getConnectionName();
                }

                if (retCnt == 0 && !playerenginemanager_->checkValidConnectionName(mediaId, connectionName)) {
                    MMLogWarn("Fail to get ConnectionName.. player-engine is not started yet.");
                    _reply(MM::PlayerTypes::PlayerError::MEDIA_MANAGER_INTERNAL_ERROR, 0);
                    return;
                }
                if (mediaId <= 0 || connectionName.empty()) {
                    MMLogInfo("Invalid values received for map: connection name = %s media ID = %d", connectionName.c_str(), mediaId);
                    _reply(MM::PlayerTypes::PlayerError::MEDIA_MANAGER_INTERNAL_ERROR, 0);
                    return;
                }
                playerenginemanager_->insertMediaId(mediaId, connectionName);
                MMLogInfo("Pass the connection name=[%s / %d] to playerprovider", connectionName.c_str(), mediaId);
            } else {
                MMLogError("Max number of PE already created...");
                _reply(MM::PlayerTypes::PlayerError::MEDIA_MANAGER_INTERNAL_ERROR, 0);
               return;
            }
        }
        playerenginemanager_->printMap();
        connectionName = playerenginemanager_->getConnectionName(mediaId);
    }
    MMLogInfo("Create openUri command=[%s / %d] to playerprovider", connectionName.c_str(), mediaId);
    switch (_type) {
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
            break;
        default:
            if (_reply) {
            _reply(MM::PlayerTypes::PlayerError::NO_ERROR, mediaId);
            _reply = NULL;
            }
            break;
    }
    command::BaseCommand* command = new (std::nothrow) command::OpenUriCommand(player_, 0, _uri, _type, true, (playerenginemanager_->connectionMap), mediaId, _channels, connectionName, _reply);
    if (!command) {
        MMLogError("failed to allocate for OpenUriCommand");
    } else {
        command->from_hmi = true;
        command_queue_->Post(command);
    }
}
#if 0
void PlayerStubImpl::openTrack(const std::shared_ptr<CommonAPI::ClientId> _client, MM::PlayerTypes::OpenTrackParam _param, uint32_t _mediaId, openTrackReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* c = new command::OpenTrackCommand(player_, _param, connectionName, _reply);
    c->from_hmi = true;
    command_queue_->Post(c);
}
#endif

void PlayerStubImpl::pause(const std::shared_ptr<CommonAPI::ClientId> _client,  uint32_t _mediaId, pauseReply_t _reply) {
    MMLogInfo("media id = %d", _mediaId);
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    MMLogInfo("connectionName = %s", connectionName.c_str());
    player_->_mutex.lock();
    player_->is_need_pause_ = true;
    player_->_mutex.unlock();
    command::BaseCommand* command = new (std::nothrow) command::PauseCommand(player_, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for PauseCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::play(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, playReply_t _reply) {
    MMLogInfo("media id = %d", _mediaId);
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    MMLogInfo("connectionName = %s", connectionName.c_str());
    command::BaseCommand* command = new (std::nothrow) command::PlayCommand(player_, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for PlayCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::playPause(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, playPauseReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::PlayPauseCommand(player_, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for PlayPauseCommand");
    else
        command_queue_->Post(command);
}
#if 0
void PlayerStubImpl::next(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, nextReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::NextCommand(player_, 1, connectionName, _reply));
}

void PlayerStubImpl::previous(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, previousReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::NextCommand(player_, -1, connectionName, _reply));
}

void PlayerStubImpl::previousForce(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, previousForceReply_t _reply) {
    MMLogInfo("");
    std::string connectionName;
    command::NextCommand* nc = new command::NextCommand(player_, -1, connectionName, _reply);
    nc->force = true;
    command_queue_->Post(nc);
}
#endif
void PlayerStubImpl::seek(const std::shared_ptr<CommonAPI::ClientId> _client, int64_t _pos,  uint32_t _mediaId, seekReply_t _reply) {
    MMLogInfo("seek=[%lld]", _pos);
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SeekCommand(player_, _pos, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for seekCommand");
    else {
        command->from_hmi = true;
        command_queue_->Post(command);
    }
}

void PlayerStubImpl::setPosition(const std::shared_ptr<CommonAPI::ClientId> _client, uint64_t _pos, uint32_t _mediaId, setPositionReply_t _reply) {
    MMLogInfo("setPosition=[%llu]", _pos);
    bool is_streaming = false;
    auto iter = map_media_id_.begin();
    for (; iter != map_media_id_.end(); iter++) {
        if (_mediaId == iter->second) {
            is_streaming = player_->IsStreamingType(iter->first);
            break;
        }
    }
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetPositionCommand(player_, is_streaming, _pos, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for setPositionCommand");
    else {
        command->from_hmi = true;
        command_queue_->Post(command);
    }
}

void PlayerStubImpl::stop(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, bool _force_kill, stopReply_t _reply) {
    MMLogInfo("mid=[%u], ins_num=[%d], aid=[%d], f=[%d]", _mediaId, max_pe_instance_, default_media_id, _force_kill);
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    if (connectionName.empty() || _mediaId == 0) {
        if (player_->last_fail_media_id_ == _mediaId) {
            MMLogInfo("reset fail_id");
            player_->last_fail_media_id_ = 0;
        }
        MMLogInfo("connection name is empty or mediaID is zero");
        _reply(MM::PlayerTypes::PlayerError::NO_ERROR);
        return;
    }

    bool is_pe_reuse = false;
    bool found_media_id = false;
    for (auto it = map_media_id_.begin(); it != map_media_id_.end(); it++) {
        if (_mediaId == it->second) {
            found_media_id = true;
            is_pe_reuse = player_->IsPlayerEngineReuse(it->first);
            break;
        }
    }

    if ((max_pe_instance_ > 0) && _force_kill == true) {
        bool needReply = true;
        max_pe_instance_--;

        if (is_pe_reuse) {
            needReply = false;
            player_->PEDestroyed(0);
            command::BaseCommand* command = new (std::nothrow) command::StopCommand(player_, true, connectionName, _mediaId, _reply);
            if (!command)
                MMLogError("failed to allocate for stopCommand");
            else {
                command_queue_->Post(command);
            }
            usleep(200000); // 200ms for stoppping player-engine (for alsa close)
        }
        // Reset proxy
        player_->resetPEProxy(connectionName);
        std::string connectionRemoved = playerenginemanager_->removeMediaId(_mediaId);
        MMLogInfo("Connection Name %s is removed from connectionMap, reuse=[%d]", connectionRemoved.c_str(), is_pe_reuse);
        if (_mediaId == default_media_id) {
            default_media_id = 0;
        } else {
            auto iter = map_media_id_.begin();
            for (; iter != map_media_id_.end(); iter++) {
                if (_mediaId == iter->second) {
                    iter->second = 0;
                    break;
                }
            }
            if (iter == map_media_id_.end()) {
                MMLogInfo("Not stored media id..[%u]", _mediaId);
            }
        }
        if (needReply) {
            _reply(MM::PlayerTypes::PlayerError::NO_ERROR);
        }
    } else if (_mediaId == default_media_id || found_media_id) {
        command::BaseCommand* command = new (std::nothrow) command::StopCommand(player_, true, connectionName, _mediaId, _reply);
        if (!command)
            MMLogError("failed to allocate for stopCommand");
        else {
            command_queue_->Post(command);
        }
    } else {
        MMLogError("Unavailable media_id..");
        _reply(MM::PlayerTypes::PlayerError::MEDIA_MANAGER_INTERNAL_ERROR);
    }
}

void PlayerStubImpl::getMediaIdByMediaType(const std::shared_ptr<CommonAPI::ClientId> _client, ::v1::org::genivi::mediamanager::PlayerTypes::MediaType _mediaType, getMediaIdByMediaTypeReply_t _reply) {
    MMLogInfo("getMediaIdByMediaType=[%d]", _mediaType.value_);
    uint32_t _mediaId = 0;
    MM::PlayerTypes::PlayerError e = ::v1::org::genivi::mediamanager::PlayerTypes::PlayerError::NO_ERROR;

    switch (_mediaType) {
        case MM::PlayerTypes::MediaType::AUDIO:
            //_mediaId = default_media_id;
            if (default_media_id > 0) {
                MMLogInfo("USB_Audio[%d] needs force reset", default_media_id);
                player_->last_fail_media_id_ = default_media_id;
                checkValidMediaId();
            }
            break;
        case MM::PlayerTypes::MediaType::NATURE_SOUND:
        {
            uint32_t nature_id_ = map_media_id_[MM::PlayerTypes::MediaType::NATURE_SOUND];
            //_mediaId = nature_id_;
            if (nature_id_ > 0) {
                MMLogInfo("NATURE_SOUND[%u] needs force reset", nature_id_);
                player_->last_fail_media_id_ = (int)nature_id_;
                checkValidMediaId();
            }
            break;
        }
        case MM::PlayerTypes::MediaType::STREAM:       // FALL-THROUGH
        case MM::PlayerTypes::MediaType::VIDEO:        // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAOLAFM:      // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MELON:        // FALL-THROUGH
        case MM::PlayerTypes::MediaType::QQMUSIC:      // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GENIE:        // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI:       // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KAKAOI2:       // FALL-THROUGH
        case MM::PlayerTypes::MediaType::XIMALAYA:     // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MANUAL_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::GOLF_VIDEO:   // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MOOD_THERAPY_AUDIO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::MOOD_THERAPY_VIDEO: // FALL-THROUGH
        case MM::PlayerTypes::MediaType::KIDS_VIDEO:
        case MM::PlayerTypes::MediaType::RECORDING_PLAY:
        case MM::PlayerTypes::MediaType::PODBBANG:
        case MM::PlayerTypes::MediaType::DVRS_FRONT:
        case MM::PlayerTypes::MediaType::DVRS_REAR:
        case MM::PlayerTypes::MediaType::FACE_DETECTION:
        case MM::PlayerTypes::MediaType::VIBE:
        case MM::PlayerTypes::MediaType::TENCENT_FUNAUDIO:
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_AUDIO:
        case MM::PlayerTypes::MediaType::TENCENT_MINI_APP_VIDEO:
        case MM::PlayerTypes::MediaType::USB_AUDIO1:
        case MM::PlayerTypes::MediaType::USB_VIDEO1:
        case MM::PlayerTypes::MediaType::USB_AUDIO2:
        case MM::PlayerTypes::MediaType::USB_VIDEO2:
        case MM::PlayerTypes::MediaType::GENESIS_AUDIO_STREAMING:
            _mediaId = map_media_id_[_mediaType];
            break;
        default:
            MMLogInfo ("Invalid media type");
            break;
    }
    MMLogInfo("return media_id=[%u], last_id=[%d]", _mediaId, last_retry_id_);
    if (player_->IsStreamingType(_mediaType)) {
        MMLogInfo("skip checking");
    } else if (last_retry_id_ > 0 && last_retry_id_ == (int)_mediaId) {
        MMLogInfo("media_id[%d] needs to be checked pending or not", last_retry_id_);
        player_->last_fail_media_id_ = last_retry_id_;
        last_retry_id_ = 0;
    } else {
        last_retry_id_ = _mediaId;
    }
    _reply(e, _mediaId);
}

void PlayerStubImpl::playEx(const std::shared_ptr<CommonAPI::ClientId> _client, MM::PlayerTypes::OpenTrackParam _param, uint64_t _pos, double _rate, uint32_t _mediaId, playExReply_t _reply) {
    MMLogInfo("playEx=[%llu]", _pos);
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::PlayExCommand(player_, _param, _pos, _rate, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for setPositionCommand");
    else {
        command->from_hmi = true;
        command_queue_->Post(command);
    }
}
#if 0
void PlayerStubImpl::openPlaylist(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _uri, uint32_t _mediaId, openPlaylistReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::OpenPlaylistCommand(player_, _uri, connectionName, _reply));
}

void PlayerStubImpl::enqueueUri(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _uri, uint32_t _mediaId, enqueueUriReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::EnqueueUriCommand(player_, _uri, connectionName, _reply));
}

void PlayerStubImpl::enqueueUriEx(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector<std::string> _uri, uint32_t _num_array, MM::PlayerTypes::MediaType _media_type, uint32_t _mediaId, enqueueUriExReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::EnqueueUriExCommand(player_, _uri, _num_array, _media_type, connectionName, _reply));
}

void PlayerStubImpl::dequeueIndex(const std::shared_ptr<CommonAPI::ClientId> _client, uint64_t _pos, uint32_t _mediaId, dequeueIndexReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::DequeueIndexCommand(player_, _pos, connectionName, _reply));
}

void PlayerStubImpl::dequeueAll(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, dequeueAllReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::DequeueAllCommand(player_, connectionName, _reply));
}

void PlayerStubImpl::dequeueDirectory(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _directory, uint32_t _mediaId, dequeueDirectoryReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::DequeueDirectoryCommand(player_, _directory, connectionName, _reply));
}

void PlayerStubImpl::dequeuePlaylist(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _uri, uint32_t _mediaId, dequeuePlaylistReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::DequeuePlaylistCommand(player_, _uri, connectionName, _reply));
}

void PlayerStubImpl::getCurrentPlayQueue(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, getCurrentPlayQueueReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::GetCurrentPlayQueueCommand(player_, connectionName, _reply));
}
#endif
void PlayerStubImpl::getAudioLanguageList(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, getAudioLanguageListReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::GetAudioLanguageListCommand(player_, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for GetAudioLanguageCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setAudioLanguage(const std::shared_ptr<CommonAPI::ClientId> _client, int32_t _index, uint32_t _mediaId, setAudioLanguageReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetAudioLanguageCommand(player_, _index, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for SetAudioLanguageCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::getSubtitleList(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, getSubtitleListReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::GetSubtitleListCommand(player_, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for GetSubtitleListCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setSubtitle(const std::shared_ptr<CommonAPI::ClientId> _client, int32_t _index,  uint32_t _mediaId, setSubtitleReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetSubtitleCommand(player_, _index, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for SetSubtitileCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setSubtitleActivate(const std::shared_ptr<CommonAPI::ClientId> _client, bool _activate, uint32_t _mediaId, setSubtitleActivateReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetSubtitleActivateCommand(player_, _activate, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for SetSubtitleActivateCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setSubtitleLanguage(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _language, uint32_t _mediaId, setSubtitleLanguageReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetSubtitleLanguageCommand(player_, _language, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for SetSubtitleLanguageCommand");
    else
        command_queue_->Post(command);
}

#if 0
void PlayerStubImpl::setSubtitleLayer(const std::shared_ptr<CommonAPI::ClientId> _client, int32_t _layer_id, int32_t _x, int32_t _y, int32_t _w, int32_t _h, uint32_t _mediaId, setSubtitleLayerReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command_queue_->Post(new command::SetSubtitleLayerCommand(player_, _layer_id, _x, _y, _w, _h, connectionName, _reply));
}
#endif

void PlayerStubImpl::setVideoWindow(const std::shared_ptr<CommonAPI::ClientId> _client, int32_t _window_id, int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _alpha, int32_t _color_key,  uint32_t _mediaId, setVideoWindowReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetVideoWindowCommand(player_, _window_id, _x, _y, _w, _h, _alpha, _color_key, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for SetVideoWindowCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setVideoBrightness(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, float _brightness, setVideoBrightnessReply_t _reply){
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetVideoBrightnessCommand(player_, connectionName, static_cast<double>(_brightness), _reply);
    if (!command)
        MMLogError("failed to allocate for SetVideoBrightnessCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setAVoffset(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, int32_t _delay, setAVoffsetReply_t _reply){
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetAVoffsetCommand(player_, connectionName, _delay, _reply);
    if (!command)
        MMLogError("failed to allocate for SetAVoffsetCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setVideoSaturation(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, float _saturation, setVideoSaturationReply_t _reply){
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetVideoSaturationCommand(player_, connectionName, static_cast<double>(_saturation),  _reply);
    if (!command)
        MMLogError("failed to allocate for SetVideoSturationCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setVideoContrast(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, float _contrast, setVideoContrastReply_t _reply){
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetVideoContrastCommand(player_, connectionName, static_cast<double>(_contrast), _reply);
    if (!command)
        MMLogError("failed to allocate for SetVideoContrastCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::switchChannel(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _mediaId, bool _useDownmix, switchChannelReply_t _reply){
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SwitchChannelCommand(player_, connectionName, _useDownmix, _reply);
    if (!command)
        MMLogError("failed to allocate for SwitchChannelCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::setVideoWindowEx(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _info, uint32_t _mediaId, setVideoWindowExReply_t _reply) {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(_mediaId);
    command::BaseCommand* command = new (std::nothrow) command::SetVideoWindowExCommand(player_, _info, connectionName, _reply);
    if (!command)
        MMLogError("failed to allocate for SetWindowExCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::checkValidMediaId() { // For checking player-engine crash
    MMLogInfo("last_fail_id=[%d]", player_->last_fail_media_id_);
    if (max_pe_instance_ >= 1) {
        int last_id = 0;
        int last_fail_id = player_->last_fail_media_id_;
        std::string connectionName = "";
        if ((default_media_id > 0 && kill(default_media_id, 0) != 0) || // Check Process valid or not
            (last_fail_id > 0 && default_media_id == last_fail_id)) { // Check last fail id (for case of not exited process)
            last_id = default_media_id;
            default_media_id = -1; // For preventing main.cpp restore logic
            max_pe_instance_++; // For compensation
        } else {
            uint32_t stored_id = 0;
            auto iter = map_media_id_.begin();
            for (; iter != map_media_id_.end(); iter++) {
                stored_id = iter->second;
                if ((stored_id > 0 && kill(stored_id, 0) != 0) || (last_fail_id > 0 && stored_id == last_fail_id)) {
                    last_id = stored_id;
                    iter->second = 0;
                }
            }
        }

        if (last_id > 0) {
            MMLogInfo("checkMediaId[%d] died", last_id);
            max_pe_instance_--;
            player_->PEDestroyed(last_id);
            usleep(10*1000); // 10ms for releasing pended command queue
            std::string connectionName = playerenginemanager_->getConnectionName(last_id);

            if (last_id == player_->last_fail_media_id_)
                player_->last_fail_media_id_ = 0;
            if (connectionName.empty()) {
                MMLogInfo("connectionName is NULL..");
                return;
            }
            command::BaseCommand* command = new (std::nothrow) command::StopCommand(player_, true, connectionName, (uint32_t)last_id, nullptr);
            if (!command) {
                MMLogError("failed to allocate for StopCommand");
                return;
            } else {
                command_queue_->Post(command);
            }
            usleep(200*1000); // 200ms for stoppping player-engine (for alsa close)
            playerenginemanager_->removeMediaId(last_id);
        } else if (default_media_id == 0 && player_->last_fail_media_id_ > 0) {
            MMLogInfo("USB_Audio exception case, PE already destroyed but not launched..");
            default_media_id = -1;
            max_pe_instance_++;
        }
    } else {
        MMLogError("Restore instance count~");
        max_pe_instance_++;
    }
}

void PlayerStubImpl::onRemoteMuteAttributeChanged() {
    MMLogInfo("");
    uint32_t media_id = 0;
    int32_t Idx = 0;
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::MuteOption> mute_t = getMuteAttribute();
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::MuteOption>::iterator itr;
    for(itr = mute_t.begin(); itr != mute_t.end(); itr++) {
        if(itr->getActive()) {
            media_id = itr->getMedia_id();
            Idx = itr - mute_t.begin();
            break;
        }
    }
    std::string connectionName = playerenginemanager_->getConnectionName(media_id);
    command::BaseCommand* command = new (std::nothrow) command::SetMuteCommand(player_, getMuteAttribute()[Idx].getStatus(), connectionName);
    if (!command)
        MMLogError("failed to allocate for SetMuteCommand");
    else
        command_queue_->Post(command);
}
#if 0
void PlayerStubImpl::onRemoteShuffleAttributeChanged() {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(getShuffleAttribute().getMedia_id());
    command_queue_->Post(new command::SetShuffleCommand(player_, getShuffleAttribute().getStatus(), connectionName));
}

void PlayerStubImpl::onRemoteRepeatAttributeChanged() {
    MMLogInfo("");
    std::string connectionName = playerenginemanager_->getConnectionName(getRepeatOptionAttribute().getMedia_id());
    command_queue_->Post(new command::SetRepeatCommand(player_, getRepeatOptionAttribute().getStatus(), connectionName));
}

void PlayerStubImpl::onRemoteRandomAttributeChanged() {
    MMLogInfo("");
    command_queue_->Post(new command::SetRandomCommand(player_, getRandomAttribute()));
}
#endif
void PlayerStubImpl::onRemoteRateAttributeChanged() {
    MMLogInfo("");
    uint32_t media_id = 0;
    int32_t Idx = 0;
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Rate> rate_t = getRateAttribute();
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Rate>::iterator itr;
    for(itr = rate_t.begin(); itr != rate_t.end(); itr++) {
        if(itr->getActive()) {
            media_id = itr->getMedia_id();
            Idx = itr - rate_t.begin();
            break;
        }
    }
    std::string connectionName = playerenginemanager_->getConnectionName(media_id);
    command::BaseCommand* command = new (std::nothrow) command::SetRateCommand(player_, getRateAttribute()[Idx].getRate(), connectionName);
    if (!command)
        MMLogError("failed to allocate for SetRateCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::onRemoteVolumeAttributeChanged() {
    MMLogInfo("");
    uint32_t media_id = 0;
    int32_t Idx = 0;
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Volume> volume_t = getVolumeAttribute();
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Volume>::iterator itr;
    for(itr = volume_t.begin(); itr != volume_t.end(); itr++) {
        if(itr->getActive()) {
            media_id = itr->getMedia_id();
            Idx = itr - volume_t.begin();
            break;
        }
    }
    std::string connectionName = playerenginemanager_->getConnectionName(media_id);
    command::BaseCommand* command = new (std::nothrow) command::SetVolumeCommand(player_, getVolumeAttribute()[Idx].getVolume(), connectionName);
    if (!command)
        MMLogError("failed to allocate for SetVolumeCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::onRemotePlaybackOptionAttributeChanged() {
    MMLogInfo("");
    command::BaseCommand* command = new (std::nothrow) command::SetPlaybackOptionCommand(player_, getPlaybackOptionAttribute());
    if (!command)
        MMLogError("Failed to allocate for SetPlaybackOptionCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::onRemoteMediaTypeAttributeChanged() {
    MMLogInfo("");
    command::BaseCommand* command = new (std::nothrow) command::SetMediaTypeCommand(player_, getMediaTypeAttribute());
    if (!command)
        MMLogError("Failed to allocate for SetMediaTypeCommand");
    else
        command_queue_->Post(command);
}

void PlayerStubImpl::onRemoteSpeedAttributeChanged() {
    MMLogInfo("");
    uint32_t media_id = 0;
    int32_t Idx = 0;
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Speed> speed_t = getSpeedAttribute();
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Speed>::iterator itr;
    for(itr = speed_t.begin(); itr != speed_t.end(); itr++) {
        if(itr->getActive()) {
            media_id = itr->getMedia_id();
            Idx = itr - speed_t.begin();
            break;
        }
    }
    std::string connectionName = playerenginemanager_->getConnectionName(media_id);
    command::BaseCommand* command = new (std::nothrow) command::SetSpeedCommand(player_, getSpeedAttribute()[Idx].getSpeed(), connectionName);
    if (!command)
        MMLogError("failed to allocate for SetSpeedCommand");
    else
        command_queue_->Post(command);
}

#if 0
void PlayerStubImpl::onRemoteSortModeAttributeChanged() {
    MMLogInfo("");
    command_queue_->Post(new command::SetSortModeCommand(player_, getSortModeAttribute()));
}

void PlayerStubImpl::onRemoteShuffleOptionAttributeChanged() {
    MMLogInfo("");
    command_queue_->Post(new command::SetShuffleOptionCommand(player_, getShuffleOptionAttribute()));
}

void PlayerStubImpl::onRemotePlaylistTypeAttributeChanged() {
    MMLogInfo("");
    command_queue_->Post(new command::SetPlaylistTypeCommand(player_, getPlaylistTypeAttribute()));
}
#endif
} // namespace player
} // namespace mm
} // namespace lge

