#include "playback_option.h"

#include "common.h"
#include "player_logger.h"

namespace MM = ::v1::org::genivi::mediamanager;

namespace lge {
namespace mm {
namespace player {

template<typename T>
static bool GetPlaybackOption(MM::PlayerTypes::OptionMap& om, const std::string& key, T& value) {
    auto got = om.find(key);
    if (got == om.end()) {
        MMLogDebug("Option [%s] is not exist", key.c_str());
        return false;
    }

    MM::PlayerTypes::OptionUnion ou = om[key];
    if (!ou.isType<T>()) {
        MMLogDebug("Option Type is not matched");
        return false;
    }

    value = ou.get<T>();
    return true;
}

PlaybackOption::PlaybackOption()
  : audio_mute_on_trick(true),
    set_position_on_prev(true),
    set_position_on_prev_threshold_us(TimeConvert::SecToUs(3)),
    set_position_on_trick_and_repeat_single(false),
    what_to_do_on_trick_and_BOS("stop"),
    what_to_do_on_trick_and_EOS("stop"),
    auto_resume_on_user_action(false),
    auto_resume_on_next_or_prev(false),
    auto_next_command_on_error(true),
    auto_stop_command_on_last_list(true),
    entertainment_mode(true),
    set_playback_error_and_stop(false),
    set_potion_on_stop_state(false) {
    Print();
}

void PlaybackOption::Update(MM::PlayerTypes::OptionMap& om) {
    #define GET_PLAYBACK_OPTION(option) GetPlaybackOption(om, #option, option)

    GET_PLAYBACK_OPTION(audio_mute_on_trick);
    GET_PLAYBACK_OPTION(set_position_on_prev);
    GET_PLAYBACK_OPTION(set_position_on_prev_threshold_us);
    GET_PLAYBACK_OPTION(set_position_on_trick_and_repeat_single);
    GET_PLAYBACK_OPTION(what_to_do_on_trick_and_BOS);
    GET_PLAYBACK_OPTION(what_to_do_on_trick_and_EOS);
    GET_PLAYBACK_OPTION(auto_resume_on_user_action);
    GET_PLAYBACK_OPTION(auto_resume_on_next_or_prev);
    GET_PLAYBACK_OPTION(entertainment_mode);
    GET_PLAYBACK_OPTION(set_playback_error_and_stop);

    Print();
}

void PlaybackOption::Get(MM::PlayerTypes::OptionMap& om) {
    #define SET_PLAYBACK_OPTION(option) om[#option] = option

    om.clear();
    SET_PLAYBACK_OPTION(audio_mute_on_trick);
    SET_PLAYBACK_OPTION(set_position_on_prev);
    SET_PLAYBACK_OPTION(set_position_on_prev_threshold_us);
    SET_PLAYBACK_OPTION(set_position_on_trick_and_repeat_single);
    SET_PLAYBACK_OPTION(what_to_do_on_trick_and_BOS);
    SET_PLAYBACK_OPTION(what_to_do_on_trick_and_EOS);
    SET_PLAYBACK_OPTION(auto_resume_on_user_action);
    SET_PLAYBACK_OPTION(auto_resume_on_next_or_prev);
    SET_PLAYBACK_OPTION(entertainment_mode);
    SET_PLAYBACK_OPTION(set_playback_error_and_stop);
}

void PlaybackOption::Print() {
    #define PRINT_PLAYBACK_OPTION(delimeter, option) MMLogInfo(#option ": " delimeter, option)

    PRINT_PLAYBACK_OPTION("%d", audio_mute_on_trick);
    PRINT_PLAYBACK_OPTION("%d", set_position_on_prev);
    PRINT_PLAYBACK_OPTION("%llu", set_position_on_prev_threshold_us);
    PRINT_PLAYBACK_OPTION("%d", set_position_on_trick_and_repeat_single);
    PRINT_PLAYBACK_OPTION("%s", what_to_do_on_trick_and_BOS.c_str());
    PRINT_PLAYBACK_OPTION("%s", what_to_do_on_trick_and_EOS.c_str());
    PRINT_PLAYBACK_OPTION("%d", auto_resume_on_user_action);
    PRINT_PLAYBACK_OPTION("%d", auto_resume_on_next_or_prev);
    PRINT_PLAYBACK_OPTION("%d", entertainment_mode);
    PRINT_PLAYBACK_OPTION("%d", set_playback_error_and_stop);
}

} // namespace player
} // namespace mm
} // namespace lge

