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

#include "indexerstub.h"

#include <iostream>

#include "common.h"
#include "player_logger.h"

namespace MM = ::v1::org::genivi::mediamanager;

namespace lge {
namespace mm {
namespace indexer {

IndexerStubImpl::IndexerStubImpl (LMSProvider *lms) {
    m_lms = lms;
    m_lms->stub = this; //hkchoi_temp
}

const MM::Indexer::IndexerStatus& IndexerStubImpl::getIndexerStatusAttribute (void) {
    MmError *error = NULL;
    std::string strStatus;

    strStatus = m_lms->getIndexerStatus(&error);

    if(strStatus == "RUNNING")
        m_indexerStatus = MM::Indexer::IndexerStatus::RUNNING;
    else if (strStatus=="STOPPED")
        m_indexerStatus = MM::Indexer::IndexerStatus::STOPPED;
    else if (strStatus=="IDLE")
        m_indexerStatus = MM::Indexer::IndexerStatus::IDLE;
    else
        m_indexerStatus = MM::Indexer::IndexerStatus::IDLE; // default value

    MMLogInfo("indexer status value: %d \n", static_cast<int32_t>(m_indexerStatus));

    return m_indexerStatus;
}

void IndexerStubImpl::getDatabasePath(const std::shared_ptr<CommonAPI::ClientId> _client, getDatabasePathReply_t _reply) {
    std::string output;
    MM::Indexer::IndexerError e;
    getDatabasePath(output, e);
    _reply(output, e);
}

void IndexerStubImpl::getDatabasePath(std::string &output, MM::Indexer::IndexerError &e) {
    MmError *error = NULL;
    m_lms->getDatabasePath(output, &error);

    if (error) {
        MMLogError("Setting error to BACKEND_UNREACHABLE \n");
        e = MM::Indexer::IndexerError::BACKEND_UNREACHABLE;
        delete error;
    } else {
        e = MM::Indexer::IndexerError::NO_ERROR;
    }
}

void IndexerStubImpl::startIndexing(const std::shared_ptr<CommonAPI::ClientId> _client, startIndexingReply_t _reply) {
    MM::Indexer::IndexerError e;
    startIndexing(e);
    _reply(e);
}

void IndexerStubImpl::startIndexing(MM::Indexer::IndexerError &e) {
    MmError *error = NULL;
    std::vector<std::string> path;

    m_lms->startIndexing(path, &error);

    if (error) {
        MMLogError("Setting error to BACKEND_UNREACHABLE \n");
        e = MM::Indexer::IndexerError::BACKEND_UNREACHABLE;
    } else {
        e = MM::Indexer::IndexerError::NO_ERROR;
    }
}

void IndexerStubImpl::stopIndexing(const std::shared_ptr<CommonAPI::ClientId> _client, stopIndexingReply_t _reply) {
    MM::Indexer::IndexerError e;
    stopIndexing(e);
    _reply(e);
}

void IndexerStubImpl::stopIndexing(MM::Indexer::IndexerError &e) {
    MmError *error = NULL;
    m_lms->stopIndexing(&error);

    if (error) {
        MMLogError("Setting error to BACKEND_UNREACHABLE \n");
        e = MM::Indexer::IndexerError::BACKEND_UNREACHABLE;
    } else {
        e = MM::Indexer::IndexerError::NO_ERROR;
    }
}

void IndexerStubImpl::startIndexingEx(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector<std::string> _paths, startIndexingExReply_t _reply) {
    MM::Indexer::IndexerError e;
    startIndexingEx(_paths, e);
    _reply(e);
}

void IndexerStubImpl::startIndexingEx(std::vector<std::string> paths, MM::Indexer::IndexerError& e){
    MmError *error = NULL;
    m_lms->startIndexing(paths, &error);

    if (error) {
        MMLogError("Setting error to BACKEND_UNREACHABLE \n");
        e = MM::Indexer::IndexerError::BACKEND_UNREACHABLE;
    } else {
        e = MM::Indexer::IndexerError::NO_ERROR;
    }
}

void IndexerStubImpl::refreshLocaleIndex(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _locale, refreshLocaleIndexReply_t _reply) {
    MM::Indexer::IndexerError e;
    refreshLocaleIndex(_locale, e);
    _reply(e);
}

void IndexerStubImpl::refreshLocaleIndex(std::string _locale, MM::Indexer::IndexerError &e) {
    MmError *error = NULL;
    m_lms->refreshLocaleIndex(_locale, &error);

    if (error) {
        MMLogError("Setting error to BACKEND_UNREACHABLE \n");
        e = MM::Indexer::IndexerError::BACKEND_UNREACHABLE;
    } else {
        e = MM::Indexer::IndexerError::NO_ERROR;
    }
}

void IndexerStubImpl::setPlayNG(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _path, setPlayNGReply_t _reply) {
    MM::Indexer::IndexerError e;
    setPlayNG(_path, e);
    _reply(e);
}

void IndexerStubImpl::setPlayNG(std::string _path, MM::Indexer::IndexerError &e) {
    MmError *error = NULL;
    m_lms->setPlayNG(_path, &error);

    if (error) {
        MMLogError("Setting error to BACKEND_UNREACHABLE \n");
        e = MM::Indexer::IndexerError::BACKEND_UNREACHABLE;
    } else {
        e = MM::Indexer::IndexerError::NO_ERROR;
    }
}

} // namespace indexer
} // namespace mm
} // namespace lge

