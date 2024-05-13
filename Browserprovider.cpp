 /*
  * Copyright (C) 2014, Jaguar Land Rover
  *
  * Author: Jonatan Palsson <jonatan.palsson@pelagicore.com>
  *
  * This file is part of the GENIVI Media Manager Proof-of-Concept
  * For further information, see http://genivi.org/
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at http://mozilla.org/MPL/2.0/.
  */

#include "browserprovider.h"
#include "dir_tree_provider.h"
#include "player_logger.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <algorithm>

#include <locale>
#include <boost/locale.hpp>
#include <boost/locale/generator.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind.hpp>

#include "browser_collator.h"
#include "media_data_provider.h"
//#include "MP_DeckDataProvider.h"
#include "mtp_data_provider.h"
#include "extractor_interface.h"

#include "option.h"
#include "uSQLcClient.h"

namespace lge {
namespace mm {

#define USB_MEDIA_PATH  "/media/usb"

#ifdef USE_MTP_FEATURES
  #define MTP_MEDIA_PATH  "/media/mtp"
#endif
#ifdef USE_INTERNAL_FEATURES
  #define INTERNAL_STORAGE_PATH "/usr_data/media"
#endif

#define COLLATE_NAME "LOCALE"

/**
 * ================================================================================
 * @fn : BrowserProvider
 * @brief : constructor of BrowserProvide class.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - Create TaskParser instance.
 * - Create DirTreeProvider instance.
 * - Create thread pool of DirTreeWorker in DirTreeProvider.
 * - Create MtpDataProvider instance.
 * - Create ExternalDeviceManager.
 * - Create ExternalInterface instance.
 * - Create MediaDataProvider instance.
 * - Create DeckDataProvider instance. (HMC only)
 * @param[in] parent : unused.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
BrowserProvider::BrowserProvider()
    : ServiceProvider("com.lge.PlayerEngine"), mStub(nullptr) {
    mParser = new TaskParser();

    mDirTreeProvider = DirTreeProvider::getInstance();
#ifdef USE_INTERNAL_FEATURES
    mDirTreeProvider->setInternalStoragePath(INTERNAL_STORAGE_PATH);
#endif
    mDirTreeProvider->setUsbMediaPath(USB_MEDIA_PATH);
    mDirTreeProvider->setICUCompareFunc(&BrowserCollator::getInstance()->localeCompare);
    mDirTreeProvider->createIntialWorkerPool();

    mDirTreeProvider->scanCompleted.connect(boost::bind(&BrowserProvider::onScanCompleted, this, _1, _2, _3));
    mDirTreeProvider->mediaFileFound.connect(boost::bind(&BrowserProvider::onMediaFileFound, this, _1, _2, _3, _4, _5));

#ifdef USE_MTP_FEATURES
    mMtpDataProvider = new MtpDataProvider();
    mMtpDataProvider->setMtpMediaPath(MTP_MEDIA_PATH);
    mMtpDataProvider->setICUCompareFunc(&BrowserCollator::getInstance()->localeCompare);
    mMtpDataProvider->init();
#endif

    mUsbService = new USBService();
    mUsbService->usbMountSig.connect(boost::bind(&BrowserProvider::usbMounted, this, _1, _2, _3, _4));
    mUsbService->usbUnMountSig.connect(boost::bind(&BrowserProvider::usbUnMounted, this, _1, _2, _3, _4));

#ifdef USE_MTP_FEATURES
    mMtpService= new MTPService();
    mMtpService->connectionEventSig.connect(boost::bind(&BrowserProvider::mtpConnection, this, _1, _2, _3, _4));
    mMtpService->notifyEventSig.connect(boost::bind(&BrowserProvider::mtpNotification, this, _1, _2, _3, _4));
#endif

    mExtractor = new ExtractorInterface();
    mExtractor->imageExtractedSig.connect(boost::bind(&BrowserProvider::onImageExtracted, this, _1, _2, _3, _4, _5));
    mExtractor->coverArtExtractedSig.connect(boost::bind(&BrowserProvider::onCoverArtExtracted, this, _1, _2, _3, _4));
    mExtractor->singleAudioInfoExtractedSig.connect(boost::bind(&BrowserProvider::onSingleAudioInfoExtracted, this, _1, _2, _3, _4, _5, _6, _7, _8));
    mMediaDataProvider = new MediaDataProvider();
    mMediaDataProvider->setCollationName(COLLATE_NAME);
    mMediaDataProvider->setCompareFunc(&BrowserCollator::sqlite3LocaleCompare);
    mMediaDataProvider->init();
}

/**
 * ================================================================================
 * @fn : ~BrowserProvider
 * @brief : destrutor of BrowserProvide class.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - delete instances which were created in constructor.
 *   DirTreeProvider, MediaDataProvider, MtpDataProvider,
 *   ExternalDeviceManager, ExternalInterface, TaskParser
 *   DeckDataProvider(HMC only)
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
BrowserProvider::~BrowserProvider() {
   // dsconnect signal...
    if (mDirTreeProvider)
        delete mDirTreeProvider;
    if (mMediaDataProvider)
        delete mMediaDataProvider;
#ifdef USE_MTP_FEATURES
    if (mMtpDataProvider)
        delete mMtpDataProvider;
    if (mMtpService)
        delete mMtpService;
#endif
    if (mExtractor)
        delete mExtractor;
    if (mParser)
        delete mParser;
    if (mUsbService)
        delete mUsbService;
}

bool BrowserProvider::connectToUSBAutoMounter() {
    MMLogInfo("");
    mUsbService->connectUSBService();
    return true;
}

bool BrowserProvider::requestUsbList() {
    MMLogInfo("");
    mUsbService->requestUsbList();

#ifdef USE_MTP_FEATURES
    mMtpService->requestUsbList();
#endif

    return true;
}

bool BrowserProvider::addExternalDevice(ExternalDeviceInfo &deviceInfo) {
    for(ExternalDeviceInfo item : mExternalDevices) {
        if (item.mountPath == deviceInfo.mountPath) {
            MMLogError("Exist in external device list");
            return false;
        }
    }

    MMLogInfo("add to external device list");
    mExternalDevices.push_back(deviceInfo);
    return true;
}

bool BrowserProvider::removeExternalDevice(std::string mountPath) {
    std::list<ExternalDeviceInfo>::iterator it;
    for(it = mExternalDevices.begin() ; it != mExternalDevices.end() ; ++it) {
        if ((*it).mountPath == mountPath) {
            MMLogInfo("mount path = %s is deleted successfully", mountPath.c_str());
            mExternalDevices.erase(it);
            mDirTreeProvider->setUnmountedPath(mountPath);
            return true;
        }
    }
    MMLogInfo("mount path = %s does not exist in device list", mountPath.c_str());
    return false;
}

/**
 * ================================================================================
 * @fn : listGeneral
 * @brief : common sub-routine for listChildren, listContainers, listItems
 * @section : Function flow (Pseudo-code or Decision Table)
 * - Parse TaskInfo structure from path string.
 * - Set offset and count to TaskInfo
 * - Parse sort keys if exists.
 * - If protocol is file type  then,
 *    if object type is file or folder, call listDirTree()
 *    else call listGeneral() of MediaDataProvider
 * - Else if protocal is mtp type then,
 *    if object type is file or folder, call listMtpTree()
 *    else call listGeneral() of MediaDataProvider
 * - Else if protocal is Deck type then,
 *    call listGeneral() of DeckDataProvider.
 *
 * @param[in] listType : object type for listing. (items/children/containers)
 * @param[in] path : object path which can include devie path and object type
 *                  and property define of object
 * @param[in] offset : offset of list window.
 * @param[in] count : count of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKeys : column(including asc/desc(sorting direction)) list to use for sorting.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listGeneral(MediaListType listType,
                                  string path,
                                  uint64_t offset,
                                  uint64_t count,
                                  std::vector<string> filter,
                                  std::vector<MM::BrowserTypes::SortKey> sortKeys,
                                  MM::MediaTypes::ResultMapList &mapList)
{
    TaskInfo taskInfo;
    taskInfo.listType = listType;

    mParser->parseTaskInfo(path, taskInfo, (listType == eContainers));
    taskInfo.offset = offset;
    taskInfo.count = count;

    if (sortKeys.size() > 0)
    {
        parseSortKeys(taskInfo, sortKeys);
    }

    MMLogInfo("----------------------------------");
    MMLogInfo("protocol = %d", taskInfo.protocol);
    MMLogInfo("filePath = %s", taskInfo.filePath.c_str());
    MMLogInfo("isCountQuery = %d", taskInfo.isCountQuery);
    MMLogInfo("objectType = %d", taskInfo.objectType);
    MMLogInfo("objectName = %s", taskInfo.objectName.c_str());
    MMLogInfo("category = %d", taskInfo.category);
    MMLogInfo("query = %s"   , taskInfo.query.c_str());
    MMLogInfo("keyName = %s" , taskInfo.keyName.c_str());
    MMLogInfo("keyValue = %s", taskInfo.keyValue.c_str());

    if(taskInfo.protocol == eFileProtocol)
    {
        MMLogInfo("START listGeneral()");
        if (taskInfo.objectType == MEDIA_META_FOLDER ||
                taskInfo.objectType == MEDIA_META_FILE  )
            return listDirTree(taskInfo, filter, mapList);
        else
            return mMediaDataProvider->listGeneral(taskInfo, filter, mapList);
    }
    else if(taskInfo.protocol == eRemoteProtocol)
    {
        MMLogInfo("START Remote listGeneral()");
        if (taskInfo.objectType == MEDIA_META_FOLDER ||
                taskInfo.objectType == MEDIA_META_FILE  )
            return false;
        else
            return mMediaDataProvider->listGeneral(taskInfo, filter, mapList);
    }
    else if ( taskInfo.protocol == eMtpProtocol)
    {
        MMLogInfo("START listGeneral()");
        if (taskInfo.objectType == MEDIA_META_FOLDER ||
                taskInfo.objectType == MEDIA_META_FILE  )
            return listMtpTree(taskInfo, filter, mapList);
        else
            return mMediaDataProvider->listGeneral(taskInfo, filter, mapList);
    }
    else if(taskInfo.protocol == eDeckProtocol)
    {
        MMLogInfo("START (Deck) - listGeneral");
//   return mDeckDataProvider->listGeneral(taskInfo, filter, mapList);
           return false;
    }
    return false;
}

/**
 * ================================================================================
 * @fn : listChildren
 * @brief : list children of the object id, generally this function used to query sub folders and files.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - call listGeneral with listType (eChildren)
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listChildren(std::string path,
                                   uint64_t offset,
                                   uint64_t count,
                                   std::vector<std::string> filter,
                                   MM::MediaTypes::ResultMapList& mapList) {
    MMLogInfo("");
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    return listGeneral(eChildren, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listChildrenEx
 * @brief : add sorting feature to listChildren.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - add input sort key to vector of sort key.
 * - call listGeneral with listType (eChildren).
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKey : column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listChildrenEx(std::string path,
                                    uint64_t offset,
                                    uint64_t count,
                                    std::vector<std::string> filter,
                                    MM::BrowserTypes::SortKey sortKey,
                                     MM::MediaTypes::ResultMapList &mapList)
{
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    sortKeys.push_back(sortKey);
    return listGeneral(eChildren, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listChildrenEx2
 * @brief : provide multi sorting keys to listChildren.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - call listGeneral with listType (eChildren).
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKeys : list of column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listChildrenEx2(std::string path,
                 uint64_t offset,
                 uint64_t count,
                 std::vector<std::string> filter,
                 std::vector<MM::BrowserTypes::SortKey> sortKeys,
                 MM::MediaTypes::ResultMapList& mapList)
{
    return listGeneral(eChildren, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listContainers
 * @brief : list containers of the object id, generally this function used to query albums, artists...
 * @section : Function flow (Pseudo-code or Decision Table)
 * - call listGeneral with listType (eContainers)
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] count : size of list window.
 * @param[in] filter : column list to retrive.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listContainers(std::string path,
                                     uint64_t offset,
                                     uint64_t count,
                                     std::vector<std::string> filter,
                                     MM::MediaTypes::ResultMapList& mapList)
{
    MMLogInfo("path = %s", path.c_str());
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    return listGeneral(eContainers, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listContainersEx
 * @brief : add sorting feature to listContainers.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - add input sort key to vector of sort key.
 * - call listGeneral with listType (eContainers).
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKey : column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listContainersEx(std::string path,
                          uint64_t offset,
                          uint64_t count,
                          std::vector<std::string> filter,
                          MM::BrowserTypes::SortKey sortKey,
                          MM::MediaTypes::ResultMapList& mapList)
{
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    sortKeys.push_back(sortKey);
    return listGeneral(eContainers, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listContainersEx2
 * @brief : add multi-key sorting feature to listContainersEx.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - add input sort key to vector of sort key.
 * - call listGeneral with listType (eContainers).
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKey : column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listContainersEx2(std::string path,
                 uint64_t offset,
                 uint64_t count,
                 std::vector<std::string> filter,
                 std::vector<MM::BrowserTypes::SortKey> sortKeys,
                 MM::MediaTypes::ResultMapList& mapList)
{
    return listGeneral(eContainers, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listItems
 * @brief : list items of the object id, generally this function used to query songs, files.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - call listGeneral with listType (eItems)
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listItems(std::string path,
                                uint64_t offset,
                                uint64_t count,
                                std::vector<std::string> filter,
                                MM::MediaTypes::ResultMapList& mapList)
{
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    return listGeneral(eItems, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listItemsEx
 * @brief : add sorting feature to listItems.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - add input sort key to vector of sort key.
 * - call listGeneral with listType (eItems).
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKey : column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listItemsEx(std::string path,
                                 uint64_t offset,
                                 uint64_t count,
                                 std::vector<std::string> filter,
                                 MM::BrowserTypes::SortKey sortKey,
                                  MM::MediaTypes::ResultMapList& mapList)
{
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    sortKeys.push_back(sortKey);
    return listGeneral(eItems, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listItemsEx2
 * @brief : provide multi sorting keys to listItemsEx.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - call listGeneral with listType (eItems).
 * @param[in] path : object path
 * @param[in] offset : offset of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKeys : list of column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listItemsEx2(std::string path,
                 uint64_t offset,
                 uint64_t count,
                 std::vector<std::string> filter,
                 std::vector<MM::BrowserTypes::SortKey> sortKeys,
                 MM::MediaTypes::ResultMapList& mapList)
{
    return listGeneral(eItems, path, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : searchObjectsGeneral
 * @brief : common routine for searchObjects and searchObjectEx
 * @section : Function flow (Pseudo-code or Decision Table)
 * - Parse task info structure from path string.
 * - Parse sort key.
 * - if path contains file protocol, call searchObjects of MediaDataProvider.
 * @param[in] path : object path
 * @param[in] query : condition string to use for searching.
 * @param[in] offset : offset of list window.
 * @param[in] count : count of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKey : column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::searchObjectsGeneral(string path, string query,
                                           uint64_t offset, uint64_t count,
                                           std::vector<string> filter,
                                           std::vector<MM::BrowserTypes::SortKey> sortKeys,
                                           MM::MediaTypes::ResultMapList &mapList)
{
    TaskInfo taskInfo;
    mParser->parseTaskInfoForSearchObjects(path, taskInfo);

    taskInfo.offset = offset;
    taskInfo.count = count;
    taskInfo.sortKeys.clear();

    taskInfo.query = query;

    if (sortKeys.size() > 0)
    {
        parseSortKeys(taskInfo, sortKeys);
    }

    if(path.find("file://") == 0 || taskInfo.protocol == eRemoteProtocol)
    {
        MMLogInfo("path = %s", path.c_str());
        return mMediaDataProvider->searchObjects(taskInfo, filter, mapList);
    }
    MMLogInfo("Invalid path : path needs prefix files://");
    return false;
}

/**
 * ================================================================================
 * @fn : searchObjects
 * @brief : search items and containers matching to search condition.
 * @section : Function flow (Pseudo-code or Decision Table)
 * - call searchObjectsGeneral() with blank sort key.
 * @param[in] path : object path
 * @param[in] query : condition string to use for searching.
 * @param[in] offset : offset of list window.
 * @param[in] count : count of list window.
 * @param[in] filter : column list to retrive.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::searchObjects(std::string path,
                                    std::string query,
                                    uint64_t offset,
                                    uint64_t count,
                                    std::vector<std::string> filter,
                   MM::MediaTypes::ResultMapList& mapList)
{
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    return searchObjectsGeneral(path, query, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : searchObjectsEx
 * @brief : search items and containers matching to search condition.
 *  - call searchObjectsGeneral() with sort key list which contains single sort key.
 * @section : Function flow (Pseudo-code or Decision Table)
 * @param[in] path : object path
 * @param[in] query : condition string to use for searching.
 * @param[in] offset : offset of list window.
 * @param[in] count : count of list window.
 * @param[in] filter : column list to retrive.
 * @param[in] sortKey : column name to use for sorting and sorting direction.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::searchObjectsEx(std::string path,
                                      std::string query,
                                      uint64_t offset,
                                      uint64_t count,
                                      std::vector<std::string> filter,
                                      MM::BrowserTypes::SortKey sortKey,
                   MM::MediaTypes::ResultMapList& mapList)
{
    std::vector<MM::BrowserTypes::SortKey> sortKeys;
    sortKeys.push_back(sortKey);
    return searchObjectsGeneral(path, query, offset, count, filter, sortKeys, mapList);
}

/**
 * ================================================================================
 * @fn : listDirTree
 * @brief :  get list from DirTreeProvider for files and folders.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  - convert MediaCategoryType to MediaFileType
 *  - if list type is item type,
 *        if it is count query, call getAllDepthFileCount() of DirTreeProvider.
 *        else call getAllDepthFileList() of DirTreeProvider.
 *  - else if list type is child type,
 *        if  it is count query, call getSubFolderFileCount() of DirTreeProvider.
 *        else call getSubFolderFileList() of DirTreeProvider.
 *
 * @param[in] taskInfo : information to use in querying list.
 *            taskInfo->listType : list query type (Items/Children/Containers)
 * @param[in] filter : column list to retrive.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listDirTree(TaskInfo& taskInfo,
                                  std::vector<std::string> filter,
                                  MM::MediaTypes::ResultMapList &mapList)
{
    MMLogInfo("START");

    MediaFileType mediaType;

    switch(taskInfo.category)
    {
    case MEDIA_CATEGORY_AUDIO:
        mediaType = MediaFileAudio;
        break;
    case MEDIA_CATEGORY_VIDEO:
        mediaType = MediaFileVideo;
        break;
    case MEDIA_CATEGORY_PHOTO:
        mediaType = MediaFilePhoto;
        break;
    default:
        //mediaType = MediaFileInvalid;
        MMLogInfo("Using default type as audio");
        mediaType = MediaFileAudio;
    }
    switch(taskInfo.listType)
    {
     case eItems:
        {
            if (taskInfo.isCountQuery == false)
                return mDirTreeProvider->getAllDepthFileList(taskInfo.filePath,
                                           mediaType, taskInfo.offset, taskInfo.count, mapList);
            else
                return mDirTreeProvider->getAllDepthFileCount(taskInfo.filePath,
                                            mediaType, mapList);
        }
        break;
    case eChildren:
        {
            if (taskInfo.isCountQuery == false)
                return mDirTreeProvider->getSubFolderFileList(taskInfo.filePath,
                                            mediaType, taskInfo.offset, taskInfo.count, mapList);
            else
                return mDirTreeProvider->getSubFolderFileListCount(taskInfo.filePath,
                                                 mediaType, mapList);
        }
        break;

    default:
        break;
    }

    MMLogInfo("END");
    return false;
}

/**
 * ================================================================================
 * @fn : listDirTreeNear
 * @brief :  get list from DirTreeProvider for files and folders.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  - convert MediaCategoryType to MediaFileType
 *  - if list type is item type,
 *        nothing ( Not needed to implement yet. There is no requirement.)
 *  - else if list type is child type,
 *        if  it is count query, return false
 *        else call getSubFolderFileListNear() of DirTreeProvider.
 *
 * @param[in] taskInfo : information to use in querying list.
 *            taskInfo->listType : list query type (Items/Children/Containers)
 * @param[in] targetUrl : file path to find in the list.
 * @param[in] pageSize : size of the list window.
 * @param[in] filter : column list to retrive.
 * @param[out] mapList : ResultMapList, retrived data from browser.
 * @param[out] pageOffset : offset of the result page.
 * @param[out] targetIndex : index of the target found. if not found : 0.
 * @param[out] totalCount : total count of the whole list(not window list).
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::listMtpTree(TaskInfo &taskInfo, std::vector<string> filter, v1::org::genivi::mediamanager::MediaTypes::ResultMapList &mapList)
{
#ifdef USE_MTP_FEATURES
    MtpFileType mtpFileType;

    switch(taskInfo.category)
    {
    case MEDIA_CATEGORY_AUDIO:
        mtpFileType = MtpFileAudio;
        break;
    case MEDIA_CATEGORY_VIDEO:
        mtpFileType = MtpFileVideo;
        break;
    case MEDIA_CATEGORY_PHOTO:
        mtpFileType = MtpFilePhoto;
        break;
    default:
        mtpFileType = MtpFileInvalid;
    }
    switch(taskInfo.listType)
    {
    case eChildren:
        {
            if (taskInfo.isCountQuery == false)
                return mMtpDataProvider->listChildren(std::string(taskInfo.filePath),
                                            mtpFileType,
                                            taskInfo.offset, taskInfo.count,
                                            filter, mapList);
            else
                return mMtpDataProvider->listChildCount(std::string(taskInfo.filePath),
                                                 mtpFileType, filter, mapList);
        }
        break;

    default:
        break;
    }
    MMLogInfo("END");
#endif
    return false;
}

/**
 * ================================================================================
 * @fn : parseSortKeys
 * @brief : translate  MM::BrowserTypes::SortKey to local SortKey type.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  - for each sort key of MM::BrowserTypes::SortKey in input sortKeys
 *      assgin name and sort order type (asc/desc)
 *      check if need collation sorting and assign needCollation value.
 * @param[in] sortKeys : input sort key information.
 * @param[out] taskInfo : information to use in querying list.
 *             taskInfo.sortKeys : sorting keys
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
void BrowserProvider::parseSortKeys(TaskInfo& taskInfo, std::vector<MM::BrowserTypes::SortKey>& sortKeys)
{
    taskInfo.sortKeys.clear();
    for(uint32_t i = 0; i < sortKeys.size(); i++)
    {
        MMLogInfo("sortKey = %s", sortKeys[i].getKeyName().c_str());
        SortKey taskSortKey;
        taskSortKey.name = sortKeys[i].getKeyName();
        taskSortKey.descending = (sortKeys[i].getOrder() == MM::BrowserTypes::SortOrder::DESCENDING ? true : false);
        taskSortKey.needCollation = mParser->checkNeedCollation(taskSortKey.name);
        taskInfo.sortKeys.push_back(taskSortKey);
    }
}

/**
 * ================================================================================
 * @fn : getMountedList
 * @brief :  get External USB/MTP device list
 * @section : Function flow (Pseudo-code or Decision Table)
 * 1) get ExternalDeviceInfo list from ExternalDeviceManager.
 * 2) convert ExternalDeviceInfo type to MM::Browser::UsbDeviceInfo type.
 * 3) add to vector of MM::Browser::UsbDeviceInfo type.
 * @param[out] deviceList : information about connected USB/MTP device.
 *               device type / mount path / port / device name / product name
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::getMountedList(std::vector<MM::Browser::UsbDeviceInfo>& deviceList )
{
//    qWarning() ;
    deviceList.clear();
//    QList<ExternalDeviceInfo>& list = mExternalDeviceManger->getExternalDeviceList();

    mMountLock.lock();
    for (ExternalDeviceInfo device :  mExternalDevices)
    {
        MM::Browser::UsbDeviceInfo usbInfo;
        usbInfo.setDeviceType(device.deviceType == DEVICE_USB ?  "USB" : "MTP");
        usbInfo.setMountPath(device.mountPath);
        usbInfo.setPort(device.port);
        usbInfo.setDeviceName(device.deviceName);
        usbInfo.setProductName(device.productName);
        MMLogInfo("usbInfo.getMountPath() = = %s", usbInfo.getMountPath().c_str());
        deviceList.push_back(usbInfo);
    }
    mMountLock.unlock();
    return true;
}

/**
 * ================================================================================
 * @fn : getMediaFileInfo
 * @brief :  get media file count information of USB/MTP device.
 * @section : Function flow (Pseudo-code or Decision Table)
 * 1) get device list from ExternalDeviceManager.
 * 2) get media count (audio/video/photo) of each device
 * 3) add to vector of MM::Browser::UsbMediaFileInfo type.
 * @param[out] mediaInfoList : media file information about connected USB/MTP device.
 *               device type / mount path / audio count / video count / photo count.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::getMediaFileInfo(std::vector<MM::Browser::UsbMediaFileInfo> & mediaInfoList)
{
    MMLogInfo("");
    mMountLock.lock();
    //notify mediaFileFound
    for (ExternalDeviceInfo device :  mExternalDevices) {
        if (device.deviceType == DEVICE_USB) {
        MM::Browser::UsbMediaFileInfo fileInfo;
        std::string mountPath = device.mountPath;
        fileInfo.setMountPath(mountPath);
        fileInfo.setDeviceType("USB");
        fileInfo.setAudioCount(mDirTreeProvider->getAudioFileNum(mountPath));
        fileInfo.setVideoCount(mDirTreeProvider->getVideoFileNum(mountPath));
        fileInfo.setPhotoCount(mDirTreeProvider->getPhotoFileNum(mountPath));
        mediaInfoList.push_back(fileInfo);
      }
    }
    mMountLock.unlock();
    return true;
}

/**
 * ================================================================================
 * @fn : scan
 * @brief :  scan files/directories of input device path.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  call startScanFileSystem of DirTreeProvider and return the result of the call.
 * @param[in] path : device path to scan.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::scan(std::string path)
{
    MMLogInfo("path = %s", path.c_str());
    bool result = false;
    mMountLock.lock();
    if(mDirTreeProvider)
        result = mDirTreeProvider->startScanFileSystem(path, true);
    mMountLock.unlock();
    return result;
}

bool BrowserProvider::usbMounted(std::string path, int port, std::string label, std::string productName)
{
    MMLogInfo("path = %s, port = %d, label = %s, productName = %s", path.c_str(), port, label.c_str(), productName.c_str());

    ExternalDeviceInfo device;

    device.port = port;
    device.mountPath = path;
    device.deviceName = label;

    if (boost::starts_with(path, USB_MEDIA_PATH)) {
        device.deviceType = DEVICE_USB;
    } else {
        MMLogError("Invalid path =  %s", path.c_str());
        return false;
    }

    if (!addExternalDevice(device))
        return false;

    if (device.deviceType == DEVICE_USB) {
       mMountLock.lock();
       mDirTreeProvider->startScanFileSystem(device.mountPath, true);
       mMountLock.unlock();
	} else {
        MMLogError("Invalid Device Type = %d", device.deviceType);
    }

#if 1 // Notify to App.
    usleep(30000); // 30ms for preventing mount problem.
    MM::Browser::UsbDeviceInfo usbInfo;
    usbInfo.setDeviceType(device.deviceType == DEVICE_USB ?  "USB" : "MTP");
    usbInfo.setMountPath(device.mountPath);
    usbInfo.setPort(device.port);
    usbInfo.setDeviceName(device.deviceName);
    usbInfo.setProductName(device.productName);

    MMLogInfo("usbInfo.getMountPath() = %s", usbInfo.getMountPath().c_str());
    mStub->fireUsbMountedEvent(usbInfo);
#endif
    return true;
}

bool BrowserProvider::usbUnMounted(std::string path, int port, std::string label, std::string productName)
{
    MMLogInfo("path = %s, port = %d, label = %s, productName = %s", path.c_str(), port, label.c_str(), productName.c_str());
    if (!boost::starts_with(path, USB_MEDIA_PATH)) {
        MMLogError("Invalid path =  %s", path.c_str());
        return false;
    }

    mMountLock.lock();
    removeExternalDevice(path);
    mMountLock.unlock();

#if 1 // Notify to App.
    MMLogInfo("fireUsbUnmounted!");
    mStub->fireUsbUnmountedEvent(path);
#endif

    MMLogInfo("Send unmount event to remote ++");
    int err = 0;
    int self_seat = Option::get_current_seat();
    int idx = 0;
    char addr[32] = {0,};

    if (self_seat > 0 && self_seat >= Option::get_ip_address().size()) {
        MMLogWarn("Invalid seat[%d] - set to 0", self_seat);
        self_seat = 0;
    }

    std::vector<std::string> targetIP = Option::get_ip_address();
    for (auto it = targetIP.cbegin(); it != targetIP.cend(); it++, idx++) {
        if (self_seat == idx || it->compare("0.0.0.0") == 0) {
            // current or not used target : skip
            continue;
        }

        sprintf(addr, Option::get_ip_address().at(idx).c_str());
        uSQL *pSQL = new uSQL();
        uSQLCdb *udb = NULL;
        if (pSQL) {
            udb = pSQL->uSQLCopen("Level5", "newp2", addr, 3002, 5000, &err);
            if (udb) {
                pSQL->uSQLCtimeouts(udb, 200);
                pSQL->uSQLSendStatus(udb, 4/* REMOTE_USB_REMOVED */);
                pSQL->uSQLCclose(udb);
            }
            delete pSQL;
        }
    }
    MMLogInfo("Send unmount event to remote --");
    return true;
}

/**
 * ================================================================================
 * @fn : onScanCompleted
 * @brief :  slot function for signal scanCompleted from DirTreeProvider.
 *      send event "fireScanFilesystemCompletedEvent" to App. notify scan result.
 * @section : Function flow (Pseudo-code or Decision Table)
 * N/A
 * @param[out] path : device path to scan.
 * @param[out] rescanMode : re-scan mode (TRUE: forcely rescan), NOT USED.
 * @param[out] result : result of scan.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
void BrowserProvider::onScanCompleted(std::string path, bool rescanMode, bool result)
{
    MMLogInfo("path =  %s", path.c_str());
    mStub->fireScanFilesystemCompletedEvent(path, result);
}

/**
 * ================================================================================
 * @fn : onReadCompleted
 * @brief :  send event "fireScanFilesystemCompletedEvent" to App. notify scan result.
 *           when scan is requested and if there is caching result, this slot is called.
 *           Currently NOT USED.
 * @section : Function flow (Pseudo-code or Decision Table)
 * N/A
 * @param[out] path : device path to scan.
 * @param[out] result : result of scan.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
void BrowserProvider::onReadCompleted(std::string path, bool result)
{
    MMLogInfo("path =  %s", path.c_str());
    mStub->fireScanFilesystemCompletedEvent(path, result);
}

/**
 * ================================================================================
 * @fn : onMediaFileFound
 * @brief : slot function for signal mediaFileFound from DirTreeProvider.
 *          notify media file count information to App.
 * @section : Function flow (Pseudo-code or Decision Table)
 * call fireMediaFileFoundEvent (send event to app)
 * @param[out] path : device path to scan.
 * @param[out] rescanMode : NOT USED.
 * @param[out] audioCount : total audio count of the device.
 * @param[out] videoCount : total video count of the device.
 * @param[out] photoCount : total photo count of the device.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
void BrowserProvider::onMediaFileFound(std::string path, bool rescanMode, unsigned int audioCount, unsigned int videoCount, unsigned int photoCount) {
    MMLogInfo("");
    MMLogInfo("path =  %s, audioCount= %d, videoCount = %d, photoCount =%d", path.c_str(), audioCount, videoCount, photoCount);
    mStub->fireMediaFileFoundEvent(path, audioCount, videoCount, photoCount);
}

/**
 * ================================================================================
 * @fn : onImageExtracted
 * @brief :  slot function for imageExtracted signal from ExtractorInterface.
 * @section : Function flow (Pseudo-code or Decision Table)
 * 1) notify image extraction status, path, thumbnail, duration, playtime to App.
 * @param[out] status : result of extraction job ("ThumbnailDone", "Error", "Unknown")
 * @param[out] path : path of video file.
 * @param[out] thumbnail : thumbnail path extracted form video file.
 * @param[out] duration : video duration.
 * @param[out] playTime : video playtime.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
void BrowserProvider::onImageExtracted(std::string status, std::string path, std::string thumbnail, int duration, int playTime)
{
    MMLogInfo("path = %s, thumbnail = %s", path.c_str(), thumbnail.c_str());
    mStub->fireThumbnailExtractedSelective(status, path, thumbnail
                                     , duration, playTime, mStub->getSubscribersForThumbnailExtractedSelective());
}

/**
 * ================================================================================
 * @fn : onSingleAudioInfoExtracted
 * @brief :  slot function for singleAudioInfoExtracted signal from ExtractorInterface.
 * @section : Function flow (Pseudo-code or Decision Table)
 * 1) notify audio info extraction status, path, coverArt, title, album, artist, genre, duration to App.
 * @param[out] status : result of extraction job ("OK", "Error", "Unknown")
 * @param[out] path : path of audio file.
 * @param[out] coverArt : coverart path extracted form video file.
 * @param[out] title : audio title.
 * @param[out] album : audio album.
 * @param[out] artist : audio artist.
 * @param[out] genre : audio genre.
 * @param[out] duration : audio duration.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : none
 * ===================================================================================
 */
void BrowserProvider::onSingleAudioInfoExtracted(std::string status, std::string path,
                                                 std::string coverArt, std::string title,
                                                 std::string album, std::string artist,
                                                 std::string genre, int64_t duration)
{
    MMLogInfo("fireAudioInfoExtractedSelective");
    mStub->fireAudioInfoExtractedSelective(status, path, coverArt, title,
                                           album, artist, genre, duration);
}

/**
 * ================================================================================
 * @fn : requestImageExtraction
 * @brief :  request video thumbnail extraction to ExtractorInterface.
 * @section : Function flow (Pseudo-code or Decision Table)
 * 1) change QStringList ->vector of string
 * 2) call requestImage() of ExtractorInterface.
 * @param[in] paths : list of video file path.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::requestImageExtraction(std::vector<std::string>& paths) {
    MMLogInfo("paths.size() = %d", paths.size());
    return mExtractor->requestImages(paths);
}

/**
 * ================================================================================
 * @fn : setPlayTime
 * @brief :  save play time of video file.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  call setPlayTime of ExtractorInteface.
 * @param[in] path : file path of video file.
 * @param[in] playTime : play time of the file path.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::setPlayTime(std::string path, uint32_t playTime) {
    MMLogInfo("path = %s", path.c_str());
    return mExtractor->setPlayTime(path, playTime);
}

/**
 * ================================================================================
 * @fn : requestCoverArts
 * @brief :  request audio coverart to ExtractorInterface.
 * @section : Function flow (Pseudo-code or Decision Table)
 * call requestCoverArts of ExtractorInterface.
 * @param[in] keys : list of key (album, artist).
 *                 key[0] = album name
 *                 key[1] = artist name
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::requestCoverArts(std::vector<std::vector<std::string>>& keys) {
    MMLogInfo("keys.size() = %d", keys.size());
    return mExtractor->requestCoverArts(keys);
}

void BrowserProvider::onCoverArtExtracted(std::string status, std::string artist, std::string album, std::string coverArtPath) {
    MMLogInfo("album = %s, artist = %s, album =  %s, coverArtPath = %s", album.c_str(), artist.c_str(), album.c_str(), coverArtPath.c_str());
    mStub->fireCoverArtExtractedSelective(status, artist, album, coverArtPath
                                       ,mStub->getSubscribersForCoverArtExtractedSelective());
}

bool BrowserProvider::connectToMtpDaemon(void) {
#ifdef USE_MTP_FEATURES
    MMLogInfo("");
    mMtpService->connectMTPService();
    return true;
#else
    return false;
#endif
}

/**
 * Add MTP device to external device information when receiving insertion or extraction of MTP device
 * and send it to AppMediaPlayer using Common API
 * returns: void
 * @param: MTP connection event info
 */
void BrowserProvider::mtpConnection(std::string status, std::string mntPath, std::string deviceName, int port) {
#ifdef USE_MTP_FEATURES
    MMLogInfo("status = %s, mntPath =  %s, deviceName = %s, port = %d",
        status.c_str(), mntPath.c_str(), deviceName.c_str(), port);

    ExternalDeviceInfo info;

    info.port = port;
    info.mountPath = mntPath;
    info.deviceName = deviceName;
    info.deviceType = DEVICE_MTP;

    if (boost::starts_with(mntPath, MTP_MEDIA_PATH)) {
        info.deviceType = DEVICE_MTP;
    } else {
        MMLogError("Invalid path =  %s", mntPath.c_str());
        return;
    }

    if (status == "0") {                                              //connected
        if (!addExternalDevice(info))
            return;

        mMtpDataProvider->getMtpTreeProvider()->addWorker(info.mountPath);

        MM::Browser::UsbDeviceInfo usbInfo;
        usbInfo.setDeviceType("MTP");
        usbInfo.setMountPath(info.mountPath);
        usbInfo.setPort(info.port);
        usbInfo.setDeviceName(info.deviceName);
        usbInfo.setProductName(info.productName);

        mStub->fireUsbMountedEvent(usbInfo);
    } else if (status == "1") {                                          //disconnected
        removeExternalDevice(mntPath);
        mMtpDataProvider->getMtpTreeProvider()->removeWorker(mntPath);     //remove MTP worker and MTP cache tree

        mStub->fireUsbUnmountedEvent(mntPath);
    } else {
      MMLogError("Error code = %s", status.c_str());
    }
#endif
}

/**
 * Update MTP cache if MTP notification event hit MTP cache,
 * and then send MTP notification event to AppMediaPlayer using common API in case of hit
 * returns: void
 * @param: MTP daemon notification event information
 */
void BrowserProvider::mtpNotification(std::string id, std::string target, std::string event, std::string path) {
#ifdef USE_MTP_FEATURES
    MMLogInfo("id = %s, target =  %s, event = %s, path = %s", id.c_str(), target.c_str(), event.c_str(), path.c_str());

    bool ret;
    mMountLock.lock();
    ret = mMtpDataProvider->notificationEvent(id, target, event, path);
    mMountLock.unlock();

    if (ret) {
        mStub->fireMtpNotificationEvent(id, target, event, path);
    }
#endif
}

/**
 * ================================================================================
 * @fn : stopCoverArts
 * @brief :  request thumbnail-extractor to stop extracting audio cover art.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  call stopCoverArts() of ExtractorInterface.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::stopCoverArts()
{
    return mExtractor->stopCoverArts();
}

/**
 * ================================================================================
 * @fn : stopThumbnails
 * @brief :  request thumbnail-extractor to stop extracting video thumbnail.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  call stopThumbnails() of ExtractorInterface.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::stopThumbnails()
{
    return mExtractor->stopThumbnails();
}

/**
 * ================================================================================
 * @fn : setLocale
 * @brief :  set locale value and reload collation rule.
 * @section : Function flow (Pseudo-code or Decision Table)
 *  call setLocale() of BrowserCollator.
 * @param[in] locale : locale string ( "zh_CN", "ko_KR", ...)
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::setLocale(std::string locale)
{
    return BrowserCollator::getInstance()->setLocale(locale);
}

/**
 * ================================================================================
 * @fn : getSongCountByDevice
 * @brief : get song count information of each device from LMS database(indexed data).
 * @section : Function flow (Pseudo-code or Decision Table)
 * 1) call getSongCountInfo of MediaDataProvider and get song count of each device.
 * 2) make ReultMapList (to transfer over CommonAPI)
 * @param[out] songCountList : ResultMapList, each ResultMap includes "Device" and "Count"
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::getSongCountByDevice(MM::MediaTypes::ResultMapList &songCountList)
{
    std::unordered_map<std::string, uint64_t> infoList;
    bool result;
    result = mMediaDataProvider->getSongCountInfo(infoList);
    if (result)
    {
        for ( auto it = infoList.begin() ; it != infoList.end() ; ++it )
        {
            MM::MediaTypes::ResultMap map;
            std::pair<std::string, MM::MediaTypes::ResultUnion> elem;
            elem.first = "Device";
            elem.second = MM::MediaTypes::ResultUnion(it->first);
            map.insert(elem);
            elem.first = "Count";
            elem.second = MM::MediaTypes::ResultUnion((int64_t)it->second);
            map.insert(elem);

            songCountList.push_back(map);
        }
        MMLogWarn("size = %d", songCountList.size());
    }
    else
    {
        MMLogError("getSongCountInfo error");
    }

    return result;
}

/**
 * ================================================================================
 * @fn : reopenDatabase
 * @brief :  close and reopen LMS database.
 * @section : Function flow (Pseudo-code or Decision Table)
 * call reopenDatabase() of MediaDataProvider.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::reopenDatabase()
{
    bool result = false;
    if(mMediaDataProvider)
        result = mMediaDataProvider->reopenDatabase();

    return result;
}

/**
 * ================================================================================
 * @fn : requestAudioInfo
 * @brief :  request audio metadata information to ExtractorInterface.
 * @section : Function flow (Pseudo-code or Decision Table)
 * call requestCoverArts of ExtractorInterface.
 * @param[in] path : audio file path.
 * @section Global Variables: none
 * @section Dependencies: none
 * @return : bool (TRUE: success FALSE: fail)
 * ===================================================================================
 */
bool BrowserProvider::requestAudioInfo(std::string path)
{
    return mExtractor->requestAudioInfo(path);
}

} // namespace mm
} // namespace lge

