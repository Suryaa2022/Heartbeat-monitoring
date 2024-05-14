/**
* @file playerprovider.h
* @author Sunyup, Kim sunyup.kim@lge.com
* @version 1.0
* Header for the class lge::mm::player::PlayerProvider
*/

#ifndef PLAYERPROVIDER_H_
#define PLAYERPROVIDER_H_

#include <stdlib.h>
#include <unistd.h>
#include <gio/gio.h>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/coroutine/all.hpp>
#include <atomic>
#include <mutex>
#include <map>
#include <iterator>

#include <CommonAPI/CommonAPI.hpp>
#include "v1/org/genivi/mediamanager/PlayerStubDefault.hpp"
#include "dbus_player_interface.h"

#include "common.h"
#include "command_queue.h"
#include "commands.h"
#include "event_system.h"
#include "playback_option.h"
#include "player_receiver_interface.h"
#include "serviceprovider.h"
#include "state_change_notifier.h"
#include "playlist/playlist.h"

namespace lge {
namespace mm {
namespace player {

#define MAX_PLAYER_ENGINE_INSTANCE 14 

/**
* @class lge::mm::player::PlayerProvider
* @brief This is the main class for handling commands and events from other modules.
* @details This module manages media playback by using PlayerEngine module.<BR>
*          This module manages playlist by using playlist module.
* @see ServiceProvider, IPlayerReceiver, PlayerStubImpl
*/
class PlayerProvider : public ServiceProvider, public IPlayerReceiver {
    friend class PlayerStubImpl;

public:
    /**
    * @defgroup PlayerProviderTypes
    * @{
    */

    enum class State : uint8_t {
      Stopped,  /**< Stopped state */
      Paused,   /**< Paused state */
      Playing   /**< Playing state */
    };

    enum class Direction : uint8_t {
      Forward,  /**< Forward direction */
      Backward  /**< Backward direction */
    };

    enum class Caching : uint8_t {
      SetCachedMusic, /**< Set caching music */
      GetCachedMovie, /**< Get cached movie */
      SetCachedMovie  /**< Set caching movie */
    };

    enum class PlayerEngineType : uint8_t {
      SingleInstance = 1,
      MultiInstance
    };

    /**@}*/

    /**
    * @fn PlayerProvider
    * @brief Constructor.
    * @section function Function Flow
    * - Registers callbacks for handling events from Audio/Video Manager.
    * - Initializes EventSystem.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    PlayerProvider(std::shared_ptr<command::Queue>& sp_command_queue);

    /**
    * @fn ~PlayerProvider
    * @brief Destructor.
    * @section function_none Function Flow : None
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    ~PlayerProvider();

    /**
    * @fn ServiceRegistered
    * @brief Informs that player CommonAPI service is registered.
    * @section function Function Flow
    * - Registers PlayerStubImpl to StateChangeNotifier.
    * - Resets playlist manager. This process should be done in command handling thread.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void ServiceRegistered();

    /**
    * @fn PEDestroyed
    * @brief Informs that PlayerEngine is recovered from exit.
    * @section function Function Flow
    * - Invokes error handler. This process should be done in command handling thread.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void PEDestroyed(int mediaId = 0);

    /**
    * @fn getMediaID
    * @brief gets the media id mapped to connection name.
    * @section function Function Flow
    * - Searches the media id mapped to connection name in the map.
    *
    * @param[in] connectionName : connection name
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : media id
    */
    uint32_t getMediaID(std::string connectionName);

    int32_t getDurationAttrIdx(std::string connectionName);

    int32_t getPositionAttrIdx(std::string connectionName);

    int32_t getBufferingAttrIdx(std::string connectionName);

    int32_t getCurrentTrackAttrIdx(std::string connectionName);

    int32_t getMuteAttrIdx(std::string connectionName);

    int32_t getRateAttrIdx(std::string connectionName);

    int32_t getSpeedAttrIdx(std::string connectionName);

    int32_t getPlaybackAttrIdx(std::string connectionName);

    int32_t getVolumeAttrIdx(std::string connectionName);

    /**
    * @fn bool process(command::Coro::pull_type&, command::OpenUriCommand*);
    * @brief Handles OpenUriCommand.
    * @section function Function Flow
    * - Acquires Audio/Video resource in accordance with media type.
    * - Checks which tract to play.
    * - Updates CommonAPI attributes - CurrentTrack, Position.
    * - Generates option parameters.<BR>
    *   Invokes SetUri API of PlayerEngine asynchronously.
    * - Waits event of completion of API call.
    * - Handles completion of asynchronous API call.
    * - Waits event of SourceInfo from PlayerEngine.
    * - If Mute CommonAPI attribute is ON, mutes audio.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::OpenUriCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::OpenTrackCommand*);
    * @brief Handles OpenTrackCommand.
    * @section function Function Flow
    * - If track parameter is index type, gets URI that matches with the given index from playlist manager.
    * - Opens media track with the URI.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
//    bool process(command::Coro::pull_type&, command::OpenTrackCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::PauseCommand*);
    * @brief Handles PauseCommand.
    * @section function Function Flow
    * - Stops trick playback.
    * - Invokes Pause API of PlayerEngine.
    * - For the case paused event not comming from PlayerEngine, updates PlaybackStatus attribute to paused.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::PauseCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::PlayCommand*);
    * @brief Handles PlayCommand.
    * @section function Function Flow
    * - Skips duplicated commands.
    * - If track is changed, opens track with the changed URI.
    * - If playback should be blocked, skips playback.
    * - Invokes Play API of PlayerEngine.
    * - If it was trick playback mode, changes playback rate as previous value.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::PlayCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::PlayPauseCommand*);
    * @brief Handles PlayPauseCommand.
    * @section function Function Flow
    * - If media track was being played. pauses playback.
    * - If media track was paused, resumes playback.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::PlayPauseCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::NextCommand*);
    * @brief Handles NextCommand.
    * @section function Function Flow
    * - Set current position to 0.
    * - If playback direction is backword and current position is bigger than threshhold,<BR>
    *   then jumps to the begin of the track.
    * - Skips duplicated commands.
    * - Moves to next track and gets URI and index of the track.
    * - Opens media track with new URI.
    * - Starts playback, if needed.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
   // bool process(command::Coro::pull_type&, command::NextCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::NextAndTrickCommand*);
    * @brief Handles NextAndTrickCommand.
    * @section function Function Flow
    * - Moves to next track and gets URI and index of the track.
    * - Opens media track with changed URI.
    * - If playback direction is backward, then jumps to the last position of the track.
    * - Changes playback rate.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::NextAndTrickCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SeekCommand*);
    * @brief Handles SeekCommand.
    * @section function Function Flow
    * - Calculates position to move.
    * - Verifies the position is valid.
    * - Invokes SetPosition API of PlayerEngine.
    * - Waits event of AsyncDone from PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SeekCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetPositionCommand*);
    * @brief Handles SetPositionCommand.
    * @section function Function Flow
    * - Verifies the position is valid.
    * - Invokes SetPosition API of PlayerEngine.
    * - Waits event of AsyncDone from PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetPositionCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::StopCommand*);
    * @brief Handles StopCommand.
    * @section function Function Flow
    * - If it was trick playback, stops trick playback.
    * - Invokes Stop API of PlayerEngine asynchronously.
    * - Waits event of completion of API call.
    * - Handles completion of asynchronous API call.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::StopCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::PlayExCommand*);
    * @brief Handles PlayExCommand.
    * @section function Function Flow
    * - Stops playback.
    * - Opens media track with the given track parameter.
    * - Jumps to the the given position.
    * - Start playback with the given playback rate.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::PlayExCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::OpenPlaylistCommand*);
    * @brief Handles OpenPlaylistCommand.
    * @section function Function Flow
    * - Opens playlist with the given m3u path.
    * - If playback is playing or paused, refresh current track index.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::OpenPlaylistCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::EnqueueUriCommand*);
    * @brief Handles EnqueueUriCommand.
    * @section function Function Flow
    * - Appends track with the given URI to the playlist.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::EnqueueUriCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::EnqueueUriExCommand*);
    * @brief Handles EnqueueUriExCommand.
    * @section function Function Flow
    * - Appends track with the given URI and title to the playlist.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::EnqueueUriExCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::DequeueAllCommand*);
    * @brief Handles DequeueAllCommand.
    * - Removes all tracks in the playlist.
    *
    * @section function Function Flow
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::DequeueAllCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::DequeueIndexCommand*);
    * @brief Handles DequeueIndexCommand.
    * @section function Function Flow
    * - Removes a track of the given index in the playlist.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::DequeueIndexCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::DequeueDirectoryCommand*);
    * @brief Handles DequeueDirectoryCommand.
    * @section function Function Flow
    * - Removes tracks of the given directory path.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::DequeueDirectoryCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::DequeuePlaylistCommand*);
    * @brief Handles DequeuePlaylistCommand.
    * @section function Function Flow
    * - Removes tracks of the given m3u file from the playlist.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::DequeuePlaylistCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::GetCurrentPlayQueueCommand*);
    * @brief Handles GetCurrentPlayQueueCommand.
    * @section function Function Flow
    * - Generates track list from the playlist and returns it.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::GetCurrentPlayQueueCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::GetAudioLanguageListCommand*);
    * @brief Handles GetAudioLanguageListCommand.
    * @section function Function Flow
    * - Generates audio track list and returns it.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::GetAudioLanguageListCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetAudioLanguageCommand*);
    * @brief Handles SetAudioLanguageCommand.
    * @section function Function Flow
    * - Invokes SetAudioLanguage API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetAudioLanguageCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::GetSubtitleListCommand*);
    * @brief Handles GetSubtitleListCommand.
    * @section function Function Flow
    * - Generates subtitle track list and returns it.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::GetSubtitleListCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetSubtitleCommand*);
    * @brief Handles SetSubtitleCommand.
    * @section function Function Flow
    * - Verifies subtitle index is valid.
    * - Invokes SetSubtitleLanguageIndex API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetSubtitleCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetSubtitleActivateCommand*);
    * @brief Handles SetSubtitleActivateCommand.
    * @section function Function Flow
    * - Invokes SetSubtitleEnable API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetSubtitleActivateCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetSubtitleLanguageCommand*);
    * @brief Handles SetSubtitleLanguageCommand.
    * @section function Function Flow
    * - Verifies subtitle language is valid.
    * - Gets subtitle index from the given subtitle language.
    * - Sets subtitle language with the subtitile index.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetSubtitleLanguageCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetSubtitleLayerCommand*);
    * @brief Handles SetSubtitleLayerCommand.
    * @section function Function Flow
    * - Invokes SetSubtitleLayer API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetSubtitleLayerCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetVideoWindowCommand*);
    * @brief Handles SetVideoWindowCommand.
    * @section function Function Flow
    * - Invokes SetVideoWindow API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetVideoWindowCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetVideoWindowExCommand*);
    * @brief Handles SetVideoWindowExCommand.
    * @section function Function Flow
    * - Invokes SetVideoWindow API of PlayerEngine.
    * - Saves backup of video window info.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetVideoWindowExCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetMuteCommand*);
    * @brief Handles SetMuteCommand.
    * @section function Function Flow
    * - Invokes SetAudioMute API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetMuteCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetShuffleCommand*);
    * @brief Handles SetShuffleCommand.
    * @section function Function Flow
    * - Changes playback mode.
    * - If playback is paused and auto_resume_on_user_action option is ON, then resumes playback.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetShuffleCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetRepeatCommand*);
    * @brief Handles SetRepeatCommand.
    * @section function Function Flow
    * - If repeat mode is for directory, re-arranges playlist.
    * - If playback is paused and auto_resume_on_user_action option is ON, then resumes playback.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetRepeatCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetRandomCommand*);
    * @brief Handles SetRandomCommand.
    * @section function Function Flow
    * - Changes playback mode.
    * - If playback is paused and auto_resume_on_user_action option is ON, then resumes playback.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetRandomCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetRateCommand*);
    * @brief Handles SetRateCommand.
    * @section function Function Flow
    * - If rate is 1.0x, stops trick play and starts playback normal.<BR>
    *   Else, Starts trick playback with given rate.
    * - If it should be muted during trick playback, mutes audio.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetRateCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetSpeedCommand*);
    * @brief Handles SetSpeedCommand.
    * @section function Function Flow
    * - If speed is greater than 0x, starts playback with speed option.<BR>
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetSpeedCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetVolumeCommand*);
    * @brief Handles SetVolumeCommand.
    * @section function Function Flow
    * - Invokes SetAudioVolume API of PlayerEngine.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetVolumeCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetPlaybackOptionCommand*);
    * @brief Handles SetPlaybackOptionCommand.
    * @section function Function Flow
    * - Updates playback options.
    * - Removes invalid playback options from PlaybackOption attribute.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetPlaybackOptionCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetMediaTypeCommand*);
    * @brief Handles SetMediaTypeCommand.
    * @section function Function Flow
    * - Changes media type.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool process(command::Coro::pull_type&, command::SetMediaTypeCommand*);

    bool process(command::Coro::pull_type& in, command::SetVideoContrastCommand* command);

    bool process(command::Coro::pull_type& in, command::SetVideoSaturationCommand* command);

    bool process(command::Coro::pull_type& in, command::SetVideoBrightnessCommand* command);
    
    bool process(command::Coro::pull_type& in, command::SetAVoffsetCommand* command);

    bool process(command::Coro::pull_type& in, command::SwitchChannelCommand* command);


    /**
    * @fn bool process(command::Coro::pull_type&, command::SetSortModeCommand*);
    * @brief Handles SetSortModeCommand.
    * @section function Function Flow
    * - Changes sort mode of the playlist.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetSortModeCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetShuffleOptionCommand*);
    * @brief Handles SetShuffleOptionCommand.
    * @section function Function Flow
    * - Changes shuffle option of the playlist.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetShuffleOptionCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::SetPlaylistTypeCommand*);
    * @brief Handles SetPlaylistTypeCommand.
    * @section function Function Flow
    * - Resets playlist manager.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::SetPlaylistTypeCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::AMRemovedEventCommand*);
    * @brief Handles AMRemovedEventCommand.
    * @section function Function Flow
    * - Verifies that the event is valid.
    * - Stops playback.
    * - Notifies that playback was stopped by Audio Manager.
    * - Clears connection info.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::AMRemovedEventCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::AMSourceNotificationEventCommand*);
    * @brief Handles AMSourceNotificationEventCommand.
    * @section function Function Flow
    * - Verifies that the event is valid.
    * - For resume command, resumes playback and notifies that playback is resumed by Audio Manager.
    * - For pause command, pauses playback and notifies that playback is paused by Audio Manager.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::AMSourceNotificationEventCommand*);

    /**
    * @fn bool process(command::Coro::pull_type&, command::VMRemovedEventCommand*);
    * @brief Handles VMRemovedEventCommand.
    * @section function Function Flow
    * - Verifies that the event is valid.
    * - Stops playback.
    * - Notifies that playback was stopped by Video Manager.
    * - Clears connection info.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] command : command to handle
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    //bool process(command::Coro::pull_type&, command::VMRemovedEventCommand*);

    ::v1::org::genivi::mediamanager::PlayerStubDefault *stub;

private:
    // <<- proxy
    /**
    * @fn preparePEProxy
    * @brief Initializes PlayerEngine proxy.
    * @section function Function Flow
    * - Creates proxy for PlayerEngine.
    * - Subscribes state change event from PlayerEngine.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    int preparePEProxy(std::string connectionName, bool addMode = false);

    /**
    * @fn resetProxy
    * @brief resets player-engine proxy of the index instance.
    * @section function Function Flow
    * - resets player-engine proxy of the index instance to null to handle PE crashes
    *
    * @param : connectionName : connection name of the PE instance
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return None
    */
    void resetPEProxy(std::string connectionName);

    ComLgePlayerEngine* playerengine_proxy_[MAX_PLAYER_ENGINE_INSTANCE];
    guint sig_id_state_change_[MAX_PLAYER_ENGINE_INSTANCE];
    GDBusConnection *gbus_sync_connection_[MAX_PLAYER_ENGINE_INSTANCE];
    // proxy ->>

    // <<- event handling
    /**
    * @fn callAsyncCallback
    * @brief Handles completion of asynchronous gdbus call to PlayerEngine.
    * @section function Function Flow
    * - Invokes handler for event that indicates completion of the asynchronous gdbus call.
    *
    * @param[in] source_object : PlayerEngine proxy.
    * @param[in] res : result of the gdbus call.
    * @param[in] user_data : context of the gdbus call.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    static void callAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);

    /**
    * @fn handleStateChange
    * @brief Callback function for handling StateChanged event from PlayerEngine
    * @section function Function Flow
    * @section function Function Flow
    * - Parses parameters from PlayerEngine.
    * - Looks for event handler.
    * - Executes the event handler.
    *
    * @param[in] connection : gdbus connection
    * @param[in] sender_name : sender name in dbus
    * @param[in] object_path : object path in dbus
    * @param[in] interface_name : interface name in dbus
    * @param[in] signal_name : signal name in dbus
    * @param[in] parameters : event in GVariant type.
    * @param[in] user_data : userdata set when PlayerEngine initialized.
    * @section global_variable_none Global Variables : None
    * @section dependencies Dependencies
    * - preparePEProxy()
    *
    * @return : None
    */
    static void handleStateChange(GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data);
    /**
    * @fn onContentType
    */
    void onContentType(const boost::property_tree::ptree& pt);

    /**
    * @fn onPEDestroyed
    * @brief Handles event that PlayerEngine is destroyed.
    * @section function Function Flow
    * - Recovers video window size.
    * - Handles the error.
    *
    * @param[in] pt : boost::property_tree object has error code.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */

    void onPEDestroyed(const boost::property_tree::ptree& pt, int mediaId);

    /**
    * @fn onCallAsync
    * @brief Handles event that asynchronous gdbus call is finished.
    * @section function Function Flow
    * - Informs CallAsync event to EventSystem.
    *
    * @param[in] pt : boost::property_tree object
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onCallAsync(const boost::property_tree::ptree& pt);

    /**
    * @fn onVMSourceNotification
    * @brief Handles VideoSourceNotification event.
    * @section function Function Flow
    * - Informs VideoSourceNotification event to EventSystem.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    //void onVMSourceNotification(const boost::property_tree::ptree& pt);

    /**
    * @fn onDuration
    * @brief Sub function for handleStateChange(). This handles Duration event.
    * @section function Function Flow
    * - Extracts duration from boost::property_tree object.
    * - Updates Duration attribute.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onDuration(const boost::property_tree::ptree& pt);

    /**
    * @fn onCurrentTime
    * @brief Sub function for handleStateChange(). This handles CurrentTime event.
    * @section function Function Flow
    * - Extracts current time from boost::property_tree object.
    * - Updates current time attribute.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onCurrentTime(const boost::property_tree::ptree& pt);

    /**
    * @fn onPlaybackStatus
    * @brief Sub function for handleStateChange(). This handles PlaybackStatus event.
    * @section function Function Flow
    * - Extracts playback status from boost::property_tree object.
    * - Updates playback status.
    * - If current track is changed, plays new track.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onPlaybackStatus(const boost::property_tree::ptree& pt);

    /**
    * @fn onTrickAndBOS
    * @brief Executes common tasks if it is trick playback mode and met BOS.
    * @section function Function Flow
    * - If what_to_do_on_trick_and_BOS option is normal play, then pauses, seeks to 0, and plays.
    * - If set_position_on_trick_and_repeat_single option is ON, jumps to the end of the current track.<BR>
    *   Else, plays next track.
    *
    * @param[in] rate : playback rate
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onTrickAndBOS(double rate);

    /**
    * @fn onTrickAndEOS
    * @brief Executes common tasks if it is trick playback mode and met EOS.
    * @section function Function Flow
    * - If what_to_do_on_trick_and_EOS option is normal play, then stops, changes to next, and plays.
    * - If set_position_on_trick_and_repeat_single option is ON, jumps to the end of the current track.<BR>
    *   Else, plays next track.
    *
    * @param[in] rate : playback rate
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onTrickAndEOS(double rate);

    /**
    * @fn onBeginOfStream
    * @brief Sub function for handleStateChange(). This handles BeginOfStream event.
    * @section function Function Flow
    * - Notifies Begin-Of-Stream event occured.
    * - If it was trick playback mode, invokes onTrickAndBOS().
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onBeginOfStream(const boost::property_tree::ptree& pt);

    /**
    * @fn onEndOfStream
    * @brief Sub function for handleStateChange(). This handles EndOfStream event.
    * @section function Function Flow
    * - Notifies End-Of-Stream event occured.
    * - If it was trick playback mode, invokes onTrickAndEOS().
    * - If it needed to change track, plays next track.
    * - If it needed to repeat current track, jumps to the begin of the current track.
    * - If it needed to stop, stops playback.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onEndOfStream(const boost::property_tree::ptree& pt);

    /**
    * @fn onAsyncDone
    * @brief Sub function for handleStateChange(). This handles AsyncDone event.
    * @section function Function Flow
    * - Informs AsyncDone event to EventSystem.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onAsyncDone(const boost::property_tree::ptree& pt);

    /**
    * @fn onSourceInfo
    * @brief Sub function for handleStateChange(). This handles SourceInfo event.
    * @section function Function Flow
    * - Extracts CanSeek from boost::property_tree object.
    * - Updates CanSeek attributes.
    * - Gets audio track list.
    * - Gets subtitle track list.
    * - If media type is video, notifies subtitle track list and video resolution.
    * - Informs SourceInfo event to EventSystem.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onSourceInfo(const boost::property_tree::ptree& pt);

    /**
    * @fn onSubtitle
    * @brief Sub function for handleStateChange(). This handles Subtitle event.
    * @section function Function Flow
    * - Notifies subtitle updated.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onSubtitle(const boost::property_tree::ptree& pt);

    /**
    * @fn onChannelEvent
    * @brief Sub function for handleStateChange(). This handles channel event.
    * @section function Function Flow
    * - Notifies channel info updated.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onChannelEvent(const boost::property_tree::ptree& pt);

    /**
    * @fn onAddressEvent
    * @brief Sub function for handleStateChange(). This handles address event.
    * @section function Function Flow
    * - Notifies address info updated.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onAddressEvent(const boost::property_tree::ptree& pt);

    /**
    * @fn onStreamingEvent
    * @brief Sub function for handleStateChange(). This handles Streaming event.
    * @section function Function Flow
    * - Notifies streaming event updated.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onStreamingEvent(const boost::property_tree::ptree& pt);

    /**
    * @fn onWarning
    * @brief Sub function for handleStateChange(). This handles Warning event.
    * @section function Function Flow
    * - Extracts warning code from boost::property_tree object.
    * - Notifies warning message corresponding the warning code.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onWarning(const boost::property_tree::ptree& pt);

    /**
    * @fn onError
    * @brief Sub function for handleStateChange(). This handles Error event.
    * @section function Function Flow
    * - Extracts error code from boost::property_tree object.
    * - Notifies error occured event corresponding error code.
    * - Handles playback error event.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onError(const boost::property_tree::ptree& pt);

    /**
    * @fn onPlaybackError
    * @brief Sub function for onError(). This executes common tasks on playback error.
    * @section function Function Flow
    * - Informs ErrorOccured event to EventSystem.
    * - Updates Duration attribute.
    * - If it needed to change track, plays new track.
    * - If it needed to stop, stops playback.
    *
    * @param[in] pt : parameter about event.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onPlaybackError(const boost::property_tree::ptree& pt);

    /**
    * @fn onPlaybackStopped
    * @brief Executes common tasks that should be done on playback stopped.
    * @section function Function Flow
    * - Updates CommonAPI attributes - CurrentTrack, Position, Rate.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void onPlaybackStopped();

    std::shared_ptr<command::Queue> command_queue_;
    command::EventSystem event_system_;
    StateChangeNotifier sc_notifier_;
    // event handling ->>

    /**
    * @fn stopTrickPlay
    * @brief Stops trick playback.
    * @section function Function Flow
    * - Invokes StopRateChange API of PlayerEngine.
    * - Waits event of AsyncDone from PlayerEngine.
    * - If include_seek is ON, jumps current position.
    * - Changes playback rate as 1.0.
    * - Controls audio mute.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] include_seek : whether including seek or not.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool stopTrickPlay(command::Coro::pull_type& in, bool include_seek, std::string connectionName);

    /**
    * @fn controlMuteForTrickPlay
    * @brief Controls audio mute in accordance with playback rate.
    * @section function Function Flow
    * - If audio_mute_on_trick option is ON and it is trick playback mode, then mutes audio.
    *
    * @param[in] in : context of boost::coroutine
    * @param[in] rate : playback rate
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool controlMuteForTrickPlay(command::Coro::pull_type& in, double rate, std::string connectionName);

    /**
    * @fn fillAudioLanguageList
    * @brief Gets audio track list gathered during openUri.
    * @section function Function Flow
    * - Extracts audio track list from boost::property_tree object.
    *
    * @param[inout] pt : boost::property_tree object.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void fillAudioLanguageList(const boost::property_tree::ptree& pt);

    /**
    * @fn fillSubtitleList
    * @brief Gets subtitle track list gathered during openUri.
    * @section function Function Flow
    * - Extracts subtitle track list from boost::property_tree object.
    *
    * @param[inout] pt : boost::property_tree object.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void fillSubtitleList(const boost::property_tree::ptree& pt);

    /**
    * @fn getSubtitlePath
    * @brief Gets subtitle path based on mediatype.
    * @section function Function Flow
    * - return subtitle path based on mediatype.
    *
    * @param[in] url : video Url to open.
    * @param[in] type : media type.<BR>
    *                   refer /git/src/interfaces/fidl/mm/PlayerTypes.fidl
    * @param[inout] pt : boost::property_tree object.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    void getSubtitlePath(const std::string& url, ::v1::org::genivi::mediamanager::PlayerTypes::MediaType type, boost::property_tree::ptree& pt);

    /**
    * @fn getVideoResolution
    * @brief Gets video resolution from the given boost::property_tree.
    * @section function Function Flow
    * - Extracts video resolution from boost::property_tree object.
    *
    * @param[in] pt : boost::property_tree object.
    * @param[out] out_width : width of video.
    * @param[out] out_height : height of video.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - SUCCESS, false - FAIL)
    */
    bool getVideoResolution(const boost::property_tree::ptree& pt, int32_t& out_width, int32_t& out_height);

    /**
    * @fn convSortMode
    * @brief Converts PlayerTypes::SortMode to playlist::SortMode.
    * @section function Function Flow
    * - Converts PlayerTypes::SortMode to playlist::SortMode.
    *
    * @param[in] mode : mode to be converted.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return playlist::SortMode
    */
    playlist::SortMode convSortMode(const MM::PlayerTypes::SortMode& mode);

    /**
    * @fn convShuffleOption
    * @brief Converts PlayerTypes::ShuffleOption to playlist::ShuffleOption.
    * @section function Function Flow
    * - Converts PlayerTypes::ShuffleOption to playlist::ShuffleOption.
    *
    * @param[in] option : option to be converted.
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return playlist::ShuffleOption
    */
    playlist::ShuffleOption convShuffleOption(const MM::PlayerTypes::ShuffleOption& option);

    /**
    * @fn addRecorderOption
    * @brief Add recorder option into ptree.
    * @section function Function Flow
    * - Add recorder option into ptree.
    *
    * @param[in] pt : base ptree of openUri
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return None
    */
    void addRecorderOption(boost::property_tree::ptree& pt);

    /**
    * @fn sendDbusCaching
    * @brief Send dbus call to caching service.
    * @section function Function Flow
    * - Get/Set cached/caching url information.
    *
    * @param[in] is_set : get / set request
    * @param[in] cache_url : url path of cached/caching file
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return None
    */
    void sendDbusCaching(Caching mode, std::string& cache_url);

    /**
    * @fn MediaTypeToString
    * @brief Returns the mediatype as string.
    * @section function Function Flow
    * - Returns the media type as string.
    *
    * @param[in] type : media type.<BR>
    *                   refer /git/src/interfaces/fidl/mm/PlayerTypes.fidl
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return std::string
    */
    std::string MediaTypeToString(::v1::org::genivi::mediamanager::PlayerTypes::MediaType type);

    /**
    * @fn IsVideoType
    * @brief Checks whether media type is video or not.
    * @section function Function Flow
    * - Checks whether media type is video or not.
    *
    * @param[in] type : media type.<BR>
    *                   refer /git/src/interfaces/fidl/mm/PlayerTypes.fidl
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - YES, false - NO)
    */
    bool IsVideoType(::v1::org::genivi::mediamanager::PlayerTypes::MediaType type);

    /**
    * @fn IsStreamingType
    * @brief Checks whether media type is streaming or not.
    * @section function Function Flow
    * - Checks whether media type is streaming or not.
    *
    * @param[in] type : media type.<BR>
    *                   refer /git/src/interfaces/fidl/mm/PlayerTypes.fidl
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - YES, false - NO)
    */
    bool IsStreamingType(::v1::org::genivi::mediamanager::PlayerTypes::MediaType type);

    /**
    * @fn IsCacheNeeded
    * @brief Checks whether media type needs caching or not.
    * @section function Function Flow
    * - Checks whether media type needs caching or not.
    *
    * @param[in] type : media type.<BR>
    *                   refer /git/src/interfaces/fidl/mm/PlayerTypes.fidl
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - YES, false - NO)
    */
    bool IsCacheNeeded(::v1::org::genivi::mediamanager::PlayerTypes::MediaType type);

    /**
    * @fn IsPlayerEngineReuse
    * @brief Checks whether media type reuses player-engine or not.
    * @section function Function Flow
    * - Checks whether media type needs reusing player-engine.
    *
    * @param[in] type : media type.<BR>
    *                   refer /git/src/interfaces/fidl/mm/PlayerTypes.fidl
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - YES, false - NO)
    */
    bool IsPlayerEngineReuse(::v1::org::genivi::mediamanager::PlayerTypes::MediaType type);
  
    /**
    * @fn resetPlaylistManager
    * @brief Creates new playlist::IPlaylistManager and manages playlist with it.
    * @section function Function Flow
    * - Checks playlist type, playback mode, sort mode, and shuffle option.
    * - Creates playlist manager and sets it as playlist manager.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return : None
    */
    // void resetPlaylistManager();

    /**
    * @fn blockPlayback
    * @brief Checks if playback should be blocked or not.
    * @section function Function Flow
    * - Checks whether no-mixed sub audio channel is connected to Audio Manager or not.
    *
    * @param : None
    * @section global_variable_none Global Variables : None
    * @section dependencies_none Dependencies : None
    * @return bool (true - YES, false - NO)
    */
    bool blockPlayback();
    // sub functions ->>

    // <<- playing
    //std::shared_ptr<playlist::IPlaylistManager> playlist_mgr_;
    std::vector<std::string> audio_tracks_;
    std::vector<std::string> subtitle_tracks_;
    std::map<std::string, int> pe_proxy_map_;
    std::string uri_last_open_;
    std::string uri_transcode_output_;
    std::string file_location;
    std::string platform_name;;
    ::v1::org::genivi::mediamanager::PlayerTypes::ACodecType codec_type_;
    ::v1::org::genivi::mediamanager::PlayerTypes::FileFormatType file_type_;

    bool need_to_open_when_play_[MAX_PLAYER_ENGINE_INSTANCE];
    std::vector<::v1::org::genivi::mediamanager::PlayerTypes::Track> track_opened_;
    ::v1::org::genivi::mediamanager::PlayerTypes::MediaType media_type_;
    PlaybackOption playback_option_;
    command::SetVideoWindowCommand::Info video_window_backup_;

    State state_[MAX_PLAYER_ENGINE_INSTANCE];
    bool is_playing_;
    bool is_need_new_index_;
    std::atomic<bool> is_Error_state; // For Eroor handling pause case (BAVN-6776)
    std::atomic<bool> is_EOS_state_; // For EOS pause case (BAVN-6023)
    std::atomic<bool> is_need_pause_; // For EOS pause case (BAVN-6023)
    int32_t seeking_position_idx;
    int last_fail_media_id_;
    int multi_channel_media_id_;
    std::string multi_channel_media_type_;
    std::string clean_connection_;
    std::string single_connection_name_;

    std::mutex _mutex;
    std::mutex _attribute_mutex;
    bool playback_blocked_;
    Direction play_direction_;
    bool updated_current_time_since_trickplay_;
    static std::string sender_name_;
    std::map<int, std::string> connection_map_;
    double saturation_;
    double brightness_;
    double contrast_;
    double saturation_2nd_;
    double brightness_2nd_;
    double contrast_2nd_;
    std::string cache_path_buffer_;

    std::map<std::string, std::string> pe_audio_slot_map_;
    std::string audio_slot_num_;
    std::string audio_slot_num_2ch_;
    std::string audio_slot_num_6ch_;
    std::map<std::string, bool> speed_ignore_buffering_map_;
};

} // namespace player
} // namespace mm
} // namespace lge

#endif  // PLAYERPROVIDER_H_

