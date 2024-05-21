#include "event_system.h"

#include "glib_helper.h"
#include "player_logger.h"

namespace lge {
namespace mm {
namespace command {

EventSystem::EventSystem(std::shared_ptr<Queue>& sp_command_queue)
  : thread_sema_(),
    thread_(),
    gmain_context_(nullptr),
    pipe_{-1, -1},
    command_queue_(sp_command_queue),
    coro_(nullptr),
    loop_(nullptr),
    src_id_(nullptr){
  MMLogInfo("");
}

EventSystem::~EventSystem(){
    MMLogInfo("");

    command_queue_->Post(new QuitThreadCommand(nullptr));
    thread_sema_.wait();

    for(int i = 0; i < 2; i++)
        close(pipe_[i]);

    g_main_context_pop_thread_default(gmain_context_);
    g_main_loop_quit(this->loop_);
    g_main_loop_unref(this->loop_);
    g_main_context_unref(gmain_context_);

    thread_.reset();
    MMLogInfo("Coroutine thread is released");
}

void EventSystem::Init() {
    MMLogInfo("");

    InitCoro();
    InitMainLoop();
}

void EventSystem::InitCoro() {
    coro_ = new Coro::push_type(std::bind(&EventSystem::HandleEvent, this, std::placeholders::_1));
    EventAndParam eap;
    eap.first = EventType::None;
    eap.second = nullptr;

    try {
        (*coro_)(eap);
    } catch (boost::exception &e) {
        MMLogError("%s", boost::diagnostic_information(e).c_str());
    }
}

void EventSystem::InitMainLoop() {
    thread_.reset(new std::thread(std::bind(&EventSystem::MainLoop, this)));
    thread_sema_.wait();
}

void EventSystem::MainLoop() {
    MMLogInfo("");

	if (pipe(pipe_) == -1)
		MMLogError("error creating POSIX pipe");

    command_queue_->RegisterCallback(std::bind(&EventSystem::InformCommandArrived, this));

    gmain_context_ = g_main_context_new();
    g_main_context_push_thread_default(gmain_context_);

    this->src_id_ = GlibHelper::ConnectUnixFd(gmain_context_, pipe_[0], G_IO_IN, [this](gint fd, GIOCondition condition) -> gboolean {
      char data;
      ssize_t sz __attribute__((unused)) = read(fd, &data, 1);
      SetEvent(EventType::CommandArrived, nullptr);
      return TRUE;
    });

    GlibHelper::CallAsync(gmain_context_, [this]() -> gboolean {
      MMLogInfo("ThreadReady callabck");
      thread_sema_.notify();
      return FALSE;
    });

    this->loop_ = g_main_loop_new(gmain_context_, FALSE);
    g_main_loop_run(this->loop_);
}

bool EventSystem::WaitEvent(Coro::pull_type& in, uint32_t succeed_event, uint32_t fail_event) {
    EventAndParam eap;
    EventType event;

    MMLogInfo("success:0x%08x, fail:0x%08x", succeed_event, fail_event);

    while (1) {
        in();
        eap = in.get();
        event = eap.first;

        if ((uint32_t)event & succeed_event) {
            MMLogInfo("got succeed event: %08x", (uint32_t)event);
            return true;
        }

        if ((uint32_t)event & fail_event) {
            MMLogInfo("got fail event: %08x", (uint32_t)event);
            return false;
        }
    }

    return true;
}

void EventSystem::SetEvent(EventType event, void* param) {
    EventAndParam eap;
    eap.first = event;
    eap.second = param;
    (*coro_)(eap);
}

void EventSystem::HandleEvent(Coro::pull_type& in) {
    MMLogInfo("Coro is ready to receive event");

    BaseCommand* command = nullptr;
    EventAndParam eap;

    while(1) {
        in();
        eap = in.get();

        if (command == nullptr && eap.first == EventType::CommandArrived) {
            command = command_queue_->Pop();
            if (command == nullptr)
                continue;

            if(command->type == CommandType::QuitThread){
                delete command;
                GlibHelper::Disconnect(this->src_id_);
                MMLogInfo("QuitThread command is received");
                break;
            } else {
                command->Execute(in);
            }

            delete command;
            command = nullptr;

            if (command_queue_->Size())
                InformCommandArrived();
        } else {
            // ignore other commands, if command is not being processed.
        }
    }

    thread_sema_.notify();
    MMLogInfo("Coro thread is finished");
}

GMainContext* EventSystem::GetGMainContext() {
    return gmain_context_;
}

void EventSystem::InformCommandArrived() {
    char data = 0;
    write(pipe_[1], &data, 1);
}

} // namespace command
} // namespace mm
} // namespace lge

