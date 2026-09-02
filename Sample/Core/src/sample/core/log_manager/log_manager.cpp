module sample.core.log_manager;

namespace sample {
auto LogManager::get_logger(std::string_view module_name) -> Log
{
    return control_block_map_.around([module_name](ControlBlockMap& map) {
        const auto it = map.lower_bound(module_name);
        if(it != map.end() && it->first == module_name) {
            std::shared_ptr<Log::ControlBlock> control_block = it->second.lock();
            if(!control_block) {
                control_block = std::make_shared<Log::ControlBlock>();
                it->second = control_block;
            }
            return Log{module_name, std::move(control_block)};
        }
        auto control_block = std::make_shared<Log::ControlBlock>();
        map.emplace_hint(it, std::string{module_name}, control_block);
        return Log{module_name, std::move(control_block)};
    });
}

auto LogManager::set_module_level(std::string_view module_name, Log::Level level) noexcept -> bool
{
    return control_block_map_.around([module_name, level](ControlBlockMap& map) {
        if(const auto it = map.find(module_name); it != map.end()) {
            if(const std::shared_ptr<Log::ControlBlock> control_block = it->second.lock()) {
                control_block->level.store(level, std::memory_order_release);
                return true;
            }
            map.erase(it);
        }
        return false;
    });
}

auto LogManager::set_module_debug(std::string_view module_name, bool debug) noexcept -> bool
{
    return control_block_map_.around([module_name, debug](ControlBlockMap& map) {
        if(const auto it = map.find(module_name); it != map.end()) {
            if(const std::shared_ptr<Log::ControlBlock> control_block = it->second.lock()) {
                control_block->debug.store(debug, std::memory_order_release);
                return true;
            }
            map.erase(it);
        }
        return false;
    });
}

} // namespace sample