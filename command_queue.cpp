#include "command_queue.h"

#include "player_logger.h"

namespace lge {
namespace mm {
namespace command {

Queue::Queue()
  : queue_(),
    mutex_(),
    callback_(nullptr) {}

void Queue::RegisterCallback(CommandArrivedCallback cb) {
    callback_ = cb;
}

BaseCommand* Queue::Pop() {
    std::lock_guard<std::recursive_mutex> locker(mutex_);
    BaseCommand* bc = nullptr;

    if (queue_.empty())
        return bc;

    bc = queue_.front();
    queue_.pop_front();

    return bc;
}

void Queue::PostNoGurad(BaseCommand* bc, bool to_front) {
    if (to_front == true)
        queue_.push_front(bc);
    else
        queue_.push_back(bc);

    if (callback_ && queue_.size() == 1)
        callback_();
}

void Queue::PostFront(BaseCommand* bc) {
    std::lock_guard<std::recursive_mutex> locker(mutex_);

    PostNoGurad(bc, true);
}

void Queue::Post(BaseCommand* bc) {
    std::lock_guard<std::recursive_mutex> locker(mutex_);

    PostNoGurad(bc, false);
}

void Queue::PostIf(BaseCommand* bc, const std::function<bool(void)>& fn) {
    std::lock_guard<std::recursive_mutex> locker(mutex_);

    if (!fn())
        return;

    PostNoGurad(bc, false);
}

bool Queue::Exist(const std::vector<CommandType>& compare_list, std::string connection_name) {
    std::lock_guard<std::recursive_mutex> locker(mutex_);
    std::string name = "";

    if (queue_.empty())
        return false;

    for (auto& cmd : queue_) {
        name = cmd->connectionName;

        if (cmd->type == CommandType::OpenUri){
            MMLogInfo("Item in queue : [OpenUri] [%s]", name.c_str());
        } else if (cmd->type == CommandType::Pause) {
            MMLogInfo("Item in queue : [Pause] [%s]", name.c_str() );
        } else if (cmd->type == CommandType::Play) {
            MMLogInfo("Item in queue : [Play] [%s]", name.c_str() );
        } else if (cmd->type == CommandType::Seek) {
            MMLogInfo("Item in queue : [Seek] [%s]", name.c_str() );
        } else if (cmd->type == CommandType::SetPosition) {
            MMLogInfo("Item in queue : [SetPosition] [%s]", name.c_str() );
        } else if (cmd->type == CommandType::Stop) {
            MMLogInfo("Item in queue : [Stop] [%s]", name.c_str() );
        } else {
            //MMLogInfo("We do not check this command type");
        }

        for (auto& v : compare_list) {
            if ((cmd->type == v) && (connection_name.compare(cmd->connectionName) == 0)) {
                MMLogInfo("Find a duplicated command [%s]", connection_name.c_str() );
                return true;
            }
        }

        MMLogInfo("There is no command to [%s]", connection_name.c_str() );
    }

    return false;
}

uint64_t Queue::Size() {
    std::lock_guard<std::recursive_mutex> locker(mutex_);
    return queue_.size();
}

void Queue::Clear() {
    std::lock_guard<std::recursive_mutex> locker(mutex_);
    BaseCommand *command = nullptr;

    while(queue_.size()){
        command = queue_.front();
        delete command;
        queue_.pop_front();
    }
}

} // namespace command
} // namespace mm
} // namespace lge

