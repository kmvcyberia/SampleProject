module;
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

module sample.core.event_loop;

import sample.error;

namespace sample {

EventLoop::EventLoop() :
    epoll_descriptor_{::epoll_create1(EPOLL_CLOEXEC)},
    wakeup_descriptor_{::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)}
{
    if(!epoll_descriptor_) {
        throw error::SystemException{"Cannot create epoll descriptor"};
    }
    if(!wakeup_descriptor_) {
        throw error::SystemException{"Cannot create wakeup descriptor"};
    }
    const int wakeup_descriptor = wakeup_descriptor_.get();
    epoll_event event{.events = EPOLLIN, .data = {.fd = wakeup_descriptor}};
    if(::epoll_ctl(epoll_descriptor_.get(), EPOLL_CTL_ADD, wakeup_descriptor, &event) == -1) {
        throw error::SystemException{std::format(
            "Failed to add file descriptor to epoll, "
            "descriptor `{0}`, reason `{1}`",
            wakeup_descriptor,
            error::get_last_system_error().message())};
    }
}

EventLoop::~EventLoop()
{
    thread_.request_stop();
    wakeup();
}

void EventLoop::start()
{
    if(thread_.joinable()) {
        throw error::Logic{"EventLoop is already running"};
    }
    thread_ = std::jthread{&EventLoop::process_loop, this};
}

auto EventLoop::enable_write_listener(int descriptor) noexcept
    -> std::expected<void, std::error_code>
{
    return modify_epoll(descriptor, EPOLLIN | EPOLLOUT | EPOLLET);
}

auto EventLoop::disable_write_listener(int descriptor) noexcept
    -> std::expected<void, std::error_code>
{
    return modify_epoll(descriptor, EPOLLIN | EPOLLET);
}

auto EventLoop::add_listener(int descriptor, const std::shared_ptr<EventListener>& listener)
    -> std::expected<void, std::error_code>
{
    if(!listener) {
        throw error::Logic{"EventListener is nullptr"};
    }
    event_listener_map_.around([descriptor, &listener](auto& map) {
        auto [it, status] = map.try_emplace(descriptor, listener);
        if(!status) {
            throw error::Logic{std::format("Descriptor `{0}` already exists", descriptor)};
        }
    });
    epoll_event event{
        .events = static_cast<std::uint32_t>(EPOLLIN | EPOLLET), .data = {.fd = descriptor}};
    if(::epoll_ctl(epoll_descriptor_.get(), EPOLL_CTL_ADD, descriptor, &event) == -1) [[unlikely]] {
        std::error_code ec = error::get_last_system_error();
        event_listener_map_.around([descriptor](auto& map) { map.erase(descriptor); });
        return std::unexpected{ec};
    }
    return {};
}

void EventLoop::remove_listener(int descriptor) noexcept
{
    ::epoll_ctl(epoll_descriptor_.get(), EPOLL_CTL_DEL, descriptor, nullptr);
    event_listener_map_.around([descriptor](auto& map) noexcept { map.erase(descriptor); });
}

void EventLoop::call_task(std::move_only_function<void()> task)
{
    task_vector_.around([&task](auto& task_vector) { task_vector.push_back(std::move(task)); });
    wakeup();
}

void EventLoop::wakeup() const noexcept
{
    constexpr std::uint64_t signal{1};
    ::write(wakeup_descriptor_.get(), &signal, sizeof(signal));
}

void EventLoop::process_loop(std::stop_token stop_token)
{
    constexpr std::size_t epoll_buffer_size{4096};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<epoll_event, epoll_buffer_size> event_array;
    while(!stop_token.stop_requested()) {
        const int event_count =
            ::epoll_wait(epoll_descriptor_.get(), event_array.data(), epoll_buffer_size, -1);
        if(stop_token.stop_requested()) {
            return;
        }
        if(event_count == -1) {
            if(error::is_system_error(error::SystemCode::interrupted)) {
                continue;
            }
            break;
        }
        for(int i = 0; i < event_count; ++i) {
            const int descriptor = event_array[i].data.fd;
            const std::uint32_t events = event_array[i].events;
            if(descriptor == wakeup_descriptor_.get()) {
                std::uint64_t signal{0};
                ::read(wakeup_descriptor_.get(), &signal, sizeof(signal));
                if(stop_token.stop_requested()) {
                    return;
                }
                std::vector<std::move_only_function<void()>> snapshot =
                    task_vector_.around([](auto& task_vector) { return std::move(task_vector); });
                for(auto& task : snapshot) {
                    std::move(task)();
                }
                continue;
            }
            const auto listener = event_listener_map_.around(
                [descriptor](const auto& map) -> std::shared_ptr<EventListener> {
                    if(auto it = map.find(descriptor); it != map.end()) {
                        return it->second.lock();
                    }
                    return nullptr;
                });
            if(!listener) {
                continue;
            }
            if(events & EPOLLOUT) {
                listener->write();
            }

            if(events & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
                listener->read();
            }
        }
    }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
auto EventLoop::modify_epoll(int descriptor, std::uint32_t events) noexcept
    -> std::expected<void, std::error_code>
{
    epoll_event event{.events = events, .data = {.fd = descriptor}};
    if(::epoll_ctl(epoll_descriptor_.get(), EPOLL_CTL_MOD, descriptor, &event) == -1) {
        return std::unexpected{error::get_last_system_error()};
    }
    return {};
}

} // namespace sample