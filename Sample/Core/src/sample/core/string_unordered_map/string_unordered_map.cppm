export module sample.core.string_unordered_map;

import std;

export namespace sample {

struct string_hash
{
    using is_transparent = void; ///< Signals the container that heterogeneous lookup is supported.

    /**
     * @brief Hashes a @c std::string_view.
     * @param key The string_view to hash.
     * @return Generated hash value.
     */
    std::size_t operator()(std::string_view key) const noexcept
    {
        return std::hash<std::string_view>{}(key);
    }

    /**
     * @brief Hashes a @c std::string by viewing it as a @c std::string_view.
     * @param key The string object to hash.
     * @return Generated hash value bit-identical to the string_view overload.
     */
    std::size_t operator()(const std::string& key) const noexcept
    {
        return std::hash<std::string_view>{}(key);
    }
};

/**
 * @brief A high-performance drop-in replacement for std::unordered_map with std::string keys.
 * Utilizes `string_hash` and @c std::equal_to<> to achieve zero-allocation lookups
 * when querying elements with `std::string_view` or raw string literals.
 * @tparam Type The type of the mapped value.
 */
template <typename Type>
using string_unordered_map = std::unordered_map<std::string, Type, string_hash, std::equal_to<>>;

} // namespace sample
