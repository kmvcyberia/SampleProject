module;
#include <sample/compiler_fix.h>

module sample.core.console_parser;

import :constant;
import sample.core.version;

namespace sample {

ConsoleParser::ConsoleParser(std::string program_name) noexcept :
    program_name_{std::move(program_name)}
{}

auto ConsoleParser::parse(const int argc, char* argv[]) -> Result
{
    std::optional<std::string> current_key;
    for(int i = 1; i < argc; ++i) {
        std::string_view param{argv[i]};
        if(current_key.has_value()) {
            parameter_map_.find(*current_key)->second.value.emplace(param);
            current_key.reset();
            continue;
        }
        if(param.size() < 2 || param[0] != '-') {
            continue;
        }
        param.remove_prefix(1);
        if(param[0] == '-') {
            param.remove_prefix(1);
            if(param == console_parser::version_long_key) {
                return Result{.status = Status::version_request, .message = print_version()};
            }
            if(param == console_parser::help_long_key) {
                return Result{.status = Status::help_request, .message = print_help()};
            }
            if(auto it = std::ranges::find_if(
                   parameter_map_, [&](const auto& pair) { return param == pair.second.long_key; });
               it != parameter_map_.end())
            {
                if(!it->second.flag) {
                    current_key.emplace(it->first);
                } else if(!it->second.value.has_value()) {
                    it->second.value.emplace("");
                }
            } else {
                return Result{
                    .status = Status::error,
                    .message = print_error(std::format("option `--{0}` is unsupported", param))};
            }
        } else {
            const std::size_t param_size = param.size();
            for(std::size_t j = 0; j < param_size; ++j) {
                char key = param[j];
                if(key == console_parser::version_short_key) {
                    return Result{.status = Status::version_request, .message = print_version()};
                }
                if(key == console_parser::help_short_key) {
                    return Result{.status = Status::help_request, .message = print_help()};
                }
                if(auto it = std::ranges::find_if(
                       parameter_map_,
                       [&](const auto& pair) { return key == pair.second.short_key; });
                   it != parameter_map_.end())
                {
                    if(!it->second.flag) {
                        if(j == param_size - 1) {
                            current_key.emplace(it->first);
                        } else {
                            return Result{
                                .status = Status::error,
                                .message = print_error(
                                    std::format(
                                        "option `-{0}` requires value and cannot be grouped",
                                        key))};
                        }
                    } else if(!it->second.value.has_value()) {
                        it->second.value.emplace("");
                    }
                } else {
                    return Result{
                        .status = Status::error,
                        .message = print_error(std::format("option `-{0}` is unsupported", key))};
                }
            }
        }
    }
    if(current_key.has_value()) {
        const auto& param = parameter_map_.find(*current_key)->second;
        return Result{
            .status = Status::error,
            .message = print_error(std::format("option `--{0}` requires a value", param.long_key))};
    }
    for(const auto& parameter :
        parameter_map_ | std::views::values | std::views::filter([](const auto& value) {
            return value.required && !value.value.has_value();
        }))
    {
        return Result{
            .status = Status::error,
            .message = print_error(std::format("option `--{0}` is required", parameter.long_key))};
    }
    return Result{.status = Status::success};
}

auto ConsoleParser::add_parameter(
    std::string name,
    char short_key,
    std::string long_key,
    std::string description,
    bool flag,
    bool required) -> bool
{
    auto [it, inserted] = parameter_map_.try_emplace(
        std::move(name), short_key, std::move(long_key), std::move(description), flag, required);
    return inserted;
}

auto ConsoleParser::get_parameter(std::string_view name) const noexcept
    -> std::optional<std::string>
{
    const auto it = parameter_map_.find(name);
    if(it == parameter_map_.end()) {
        return std::nullopt;
    }
    return it->second.value;
}

auto ConsoleParser::print_version() const -> std::string
{
    return std::format("{0} version: {1}", program_name_, program_version.to_string());
}

auto ConsoleParser::print_help() const -> std::string
{
    std::string result = std::format("Usage:\n  {0} [options]\n\nOptions:\n", program_name_);
    result += std::format(
        "  -{}, --{}\tDisplay this help and exit\n",
        console_parser::help_short_key,
        console_parser::help_long_key);
    result += std::format(
        "  -{}, --{}\tDisplay version of the program and exit\n",
        console_parser::version_short_key,
        console_parser::version_long_key);
    for(const auto& param : parameter_map_ | std::views::values) {
        result +=
            std::format("  -{0}, --{1}\t{2}\n", param.short_key, param.long_key, param.description);
    }
    return result;
}

auto ConsoleParser::print_error(const std::string_view message) const -> std::string
{
    return std::format(
        "{0}: {1}\n\n"
        "Usage:\n"
        " {0} [options]\n"
        " Try '{0} --help'\n"
        "  or '{0} -h'\n"
        " for additional help text\n",
        program_name_,
        message);
}

} // namespace sample
