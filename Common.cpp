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

#include "common.h"

namespace lge {
namespace mm {

Semaphore::Semaphore(int count)
  : count_(count),
    mtx_(),
    cv_() {
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> locker(mtx_);
    while (count_ == 0) {
        cv_.wait(locker);
    }
    count_--;
}

void Semaphore::notify() {
    std::unique_lock<std::mutex> locker(mtx_);
    count_++;
    cv_.notify_all();
}

} // namespace mm
} // namespace lge

