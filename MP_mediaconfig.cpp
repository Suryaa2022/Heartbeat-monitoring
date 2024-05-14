#include "MP_MediaConfig.h"

namespace lge {
namespace mm {

MediaConfig* MediaConfig::mInstance = NULL;

/**
 * ================================================================================
 * @fn : getInstance
 * @brief : Get pointer of MediaConfig instance.
 * @section : Function flow (Pseudo-code or Decision Table)
 *   if mInstance is not null, return mInstance
 *   else make new instance and return the instance.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : MediaConfig* (pointer to single instance)
 * ===================================================================================
 */
MediaConfig *MediaConfig::getInstance()
{
    if (mInstance) {
        return mInstance;
    }
    else
    {
        mInstance = new MediaConfig();
        return mInstance;
    }
}

/**
 * ================================================================================
 * @fn : setAudioFormat
 * @brief : Set supported audio format.
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : None
 * ===================================================================================
 */
void MediaConfig::setAudioFormat(std::string format)
{
    mAudioFormats.push_back(format);
}

/**
 * ================================================================================
 * @fn : setVideoFormat
 * @brief : Set supported video format.
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : None
 * ===================================================================================
 */
void MediaConfig::setVideoFormat(std::string format)
{
    mVideoFormats.push_back(format);
}

/**
 * ================================================================================
 * @fn : setAudioVideoFormat
 * @brief : Set supported audio/video format.(ex> .asf)
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : None
 * ===================================================================================
 */
void MediaConfig::setAudioVideoFormat(std::string format)
{
    mAudioVideoFormats.push_back(format);
}

/**
 * ================================================================================
 * @fn : getAudioFormats
 * @brief : Get supported audio format list.
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : QStringList ( format string list )
 * ===================================================================================
 */
std::deque<std::string> &MediaConfig::getAudioFormats()
{
    return mAudioFormats;
}

/**
 * ================================================================================
 * @fn : getVideoFormats
 * @brief : Get supported video format list.
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : QStringList ( format string list )
 * ===================================================================================
 */
std::deque<std::string> &MediaConfig::getVideoFormats()
{
    return mVideoFormats;
}

/**
 * ================================================================================
 * @fn : getAudioVideoFormats
 * @brief : Get supported audio/video format list.
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : QStringList ( format string list )
 * ===================================================================================
 */
std::deque<std::string> &MediaConfig::getAudioVideoFormats()
{
    return mAudioVideoFormats;
}

/**
 * ================================================================================
 * @fn : getPhotoFormats
 * @brief : Get supported photo format list.
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : QStringList ( format string list )
 * ===================================================================================
 */
std::deque<std::string> &MediaConfig::getPhotoFormats()
{
    return mPhotoFormats;
}

/**
 * ================================================================================
 * @fn : getIgnoreDirs
 * @brief : Get directory list to ignore
 * @section : Function flow (Pseudo-code or Decision Table)
 *
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : QStringList ( pattern string list to ignore)
 * ===================================================================================
 */
std::deque<std::string> &MediaConfig::getIgnoreDirs()
{
    return mIgnoreDirs;
}

/**
 * ================================================================================
 * @fn : MediaConfig
 * @brief : constructor of MediaConfig class.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  initialize mAudioFormats, mVideoFormats, mAudioVideoFomats, mPhotoFormats, mIgnoreDirs.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
MediaConfig::MediaConfig()
{
    //photo lowercase
    mPhotoFormats.push_back(".bmp");
    mPhotoFormats.push_back(".gif");
    mPhotoFormats.push_back(".jpg");
    mPhotoFormats.push_back(".jpeg");
    mPhotoFormats.push_back(".png");
    mPhotoFormats.push_back(".tif");
    mPhotoFormats.push_back(".tiff");

    mIgnoreDirs.push_back("po");
    mIgnoreDirs.push_back("CVS");
    mIgnoreDirs.push_back("core-dumps");
    mIgnoreDirs.push_back("lost+found");
    mIgnoreDirs.push_back("/media/sdcard");
    mIgnoreDirs.push_back("$RECYCLE.BIN");
}

}
}
