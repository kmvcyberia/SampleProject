module;
#include <nlohmann/json.hpp>

module sample.core.file.json;

import sample.error;

namespace sample {

struct Json::Node::Impl
{
    nlohmann::json data;
};

auto Json::Node::is_valid() const noexcept -> bool
{
    return pimpl_ != nullptr;
}

auto Json::Node::is_null() const noexcept -> bool
{
    return pimpl_ ? pimpl_->data.is_null() : false;
}

auto Json::Node::is_array() const noexcept -> bool
{
    return pimpl_ ? pimpl_->data.is_array() : false;
}

auto Json::Node::size() const noexcept -> std::size_t
{
    return pimpl_ ? pimpl_->data.size() : 0;
}

auto Json::Node::get_element(const std::size_t index) const noexcept -> Node
{
    if(!is_array() || index >= size()) {
        return Node{};
    }
    auto child = Node{};
    child.pimpl_ = std::make_shared<Impl>(pimpl_->data[index]);
    return child;
}

auto Json::Node::get_string() const -> std::optional<std::string>
{
    if(!is_valid() || !pimpl_->data.is_string()) {
        return std::nullopt;
    }
    return expand_env(pimpl_->data.get<std::string>());
}

auto Json::Node::get_int64() const -> std::optional<std::int64_t>
{
    if(!is_valid()) {
        return std::nullopt;
    }
    if(pimpl_->data.is_number_integer()) {
        try {
            return pimpl_->data.get<std::int64_t>();
        }
        catch(const nlohmann::json::out_of_range&) {
            return std::nullopt;
        }
    }
    if(pimpl_->data.is_string()) {
        auto value = pimpl_->data.get<std::string>();
        const std::string expanded = expand_env(value);
        int64_t result;
        auto [ptr, ec] =
            std::from_chars(expanded.data(), expanded.data() + expanded.size(), result);
        if(ec == std::errc{} && ptr == expanded.data() + expanded.size()) {
            return result;
        }
    }
    return std::nullopt;
}

auto Json::Node::get_uint64() const -> std::optional<uint64_t>
{
    if(!is_valid()) {
        return std::nullopt;
    }
    if(pimpl_->data.is_number_unsigned()) {
        try {
            return pimpl_->data.get<uint64_t>();
        }
        catch(const nlohmann::json::out_of_range&) {
            return std::nullopt;
        }
    }
    if(pimpl_->data.is_string()) {
        auto value = pimpl_->data.get<std::string>();
        const std::string expanded = expand_env(value);
        uint64_t result;
        auto [ptr, ec] =
            std::from_chars(expanded.data(), expanded.data() + expanded.size(), result);
        if(ec == std::errc{} && ptr == expanded.data() + expanded.size()) {
            return result;
        }
    }
    return std::nullopt;
}

auto Json::Node::get_double() const -> std::optional<double>
{
    if(!is_valid()) {
        return std::nullopt;
    }
    if(pimpl_->data.is_number_float()) {
        return pimpl_->data.get<double>();
    }
    if(pimpl_->data.is_string()) {
        auto value = pimpl_->data.get<std::string>();
        const std::string expanded = expand_env(value);
        double result;
        auto [ptr, ec] =
            std::from_chars(expanded.data(), expanded.data() + expanded.size(), result);
        if(ec == std::errc{} && ptr == expanded.data() + expanded.size()) {
            return result;
        }
    }
    return std::nullopt;
}

auto Json::Node::get_bool() const -> std::optional<bool>
{
    if(!is_valid()) {
        return std::nullopt;
    }
    if(pimpl_->data.is_boolean()) {
        return pimpl_->data.get<bool>();
    }
    if(pimpl_->data.is_string()) {
        auto value = pimpl_->data.get<std::string>();
        const std::string expanded = expand_env(value);
        if(expanded == "true") {
            return true;
        }
        if(expanded == "false") {
            return false;
        }
    }
    return std::nullopt;
}

auto Json::Node::get_child_internal(const std::span<const std::string_view> key_span) const -> Node
{
    if(!is_valid()) {
        return Node{};
    }
    const nlohmann::json* current = &pimpl_->data;
    try {
        for(const auto& key : key_span) {
            current = &current->at(key);
        }
        auto child = Node{};
        child.pimpl_ = std::make_shared<Impl>(*current);
        return child;
    }
    catch(const nlohmann::json::exception&) {
        return Node{};
    }
}

struct Json::Impl
{
    nlohmann::json data;
};

Json::Json(const std::filesystem::path& filepath) : pimpl_{std::make_unique<Impl>()}
{
    if(!std::filesystem::exists(filepath)) {
        throw error::FileNotFound(filepath);
    }
    std::ifstream file(filepath);
    if(!file.is_open()) {
        throw error::FileAccessDenied(filepath);
    }
    try {
        file >> pimpl_->data;
    }
    catch(const nlohmann::json::parse_error& e) {
        throw error::FileInvalidFormat(filepath, e.what());
    }
}

Json::~Json() = default;

auto Json::get_root_node() const -> Node
{
    auto node = Node{};
    node.pimpl_ = std::make_shared<Node::Impl>(pimpl_->data);
    return node;
}

} // namespace sample
