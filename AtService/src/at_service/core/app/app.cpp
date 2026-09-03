module at_service.core.app;

import sample.core.log;
import at_service.core.utility.file;

namespace at_service {

using Watcher = sample::tty::Watcher;

App::App(AppConfig config) : log_{log_manager_.get_logger("tty")}
{
    if(config.log_info.debug) {
        log_manager_.set_module_debug("tty", *config.log_info.debug);
    }
    if(config.log_info.level) {
        log_manager_.set_module_level("tty", *config.log_info.level);
    }
    dispatcher_ = create_dispatcher(config.dictionary_path, log_);
    std::expected<std::shared_ptr<Watcher>, std::error_code> watcher =
        Watcher::create(make_watcher_config(std::move(config)));
    if(!watcher) {
        throw sample::error::SystemException{watcher.error().message()};
    }
    watcher_ = *std::move(watcher);
}

void App::start()
{
    if(running_) {
        throw sample::error::Logic{"Application already started"};
    }
    event_loop_.start();
    running_ = true;
}

auto App::reconfigure(AppConfig config) -> bool
{
    if(config.log_info.debug) {
        log_manager_.set_module_debug("tty", *config.log_info.debug);
    }
    if(config.log_info.level) {
        log_manager_.set_module_level("tty", *config.log_info.level);
    }
    std::unique_ptr<sample::tty::Dispatcher> dispatcher;
    try {
        dispatcher = create_dispatcher(config.dictionary_path, log_);
    }
    catch(const sample::error::FileException& e) {
        log_.error("Cannot load dictionary `{0}`, reason `{1}`", e.filepath(), e.what());
        return false;
    }
    std::expected<std::shared_ptr<Watcher>, std::error_code> watcher =
        Watcher::create(make_watcher_config(std::move(config)));
    if(!watcher) {
        log_.error(watcher.error().message());
        return false;
    }
    event_loop_.call_task(
        [this, watcher = *std::move(watcher), dispatcher = std::move(dispatcher)] mutable noexcept {
            watcher_ = std::move(watcher);
            dispatcher_ = std::move(dispatcher);
        });
    log_.info("Service reconfigured successfully.");
    return true;
}

void App::update_dictionary(const std::filesystem::path& dictionary_path)
{
    try {
        std::unique_ptr<sample::tty::Dispatcher> dispatcher =
            create_dispatcher(dictionary_path, log_);
        event_loop_.call_task([this, dispatcher = std::move(dispatcher)] mutable noexcept {
            dispatcher_ = std::move(dispatcher);
        });
    }
    catch(const sample::error::FileException& e) {
        log_.error("Cannot load dictionary `{0}`, reason `{1}`", e.filepath(), e.what());
        return;
    }
    log_.info("Dictionary updated successfully.");
}

auto App::make_watcher_config(AppConfig config) -> sample::tty::WatcherConfig
{
    return sample::tty::WatcherConfig{
        .event_loop = event_loop_,
        .log = log_,
        .process_line_cb =
            [this](
                std::string_view line, std::move_only_function<void(std::string_view)>& send_cb) {
                log_.info("Receive `{0}`", line);
                if(!dispatcher_->process_line(line, send_cb)) {
                    log_.error("Unknown command `{0}`", line);
                }
            },
        .subsystem = std::move(config.device_info.subsystem),
        .path = std::move(config.device_info.path),
        .vendor_id = config.device_info.vendor_id,
        .product_id = config.device_info.product_id,
        .virtual_path = std::move(config.virtual_path),
        .serial_info = config.serial_info};
}

auto App::create_dispatcher(const std::filesystem::path& dictionary_path, const sample::Log& log)
    -> std::unique_ptr<sample::tty::Dispatcher>
{
    std::expected<std::vector<utility::CommandSpec>, std::error_code> command_vec =
        utility::load_dictionary(dictionary_path);
    if(!command_vec) {
        throw sample::error::FileException{dictionary_path, command_vec.error().message()};
    }
    if(command_vec->empty()) {
        throw sample::error::FileInvalidFormat{dictionary_path, "Dictionary is empty"};
    }
    log.info("Dictionary loaded successfully.");
    auto dispatcher = std::make_unique<sample::tty::Dispatcher>();
    for(auto&& [pattern, response] : *command_vec) {
        std::string response_formatted = std::format("\r\n{0}\r\n", utility::unescape(response));
        dispatcher->add_command(
            std::move(pattern),
            [log,
             response_raw = std::move(response),
             response_formatted = std::move(response_formatted)](
                std::move_only_function<void(std::string_view data)>& send_cb) {
                send_cb(response_formatted);
                log.info("Send `{0}`", response_raw);
            });
    }
    return dispatcher;
}

} // namespace at_service