export module sample.core.file.json;

import std;
export import sample.error;

export namespace sample {

/**
 * @class Json
 * @brief Represents a loaded JSON configuration file and manages its life cycle.
 * Provides a type-safe, exception-aware, and fluent interface to query JSON elements
 * with support for environment variable expansion inside strings.
 */
class Json final
{
public:
    /**
     * @class Node
     * @brief Represents a specific element (object, array, primitive) within the JSON structure.
     */
    class Node final
    {
    public:
        friend class Json;
        ~Node() = default;

        /**
         * @brief Checks if the node points to a valid JSON element.
         * @return true if the node is valid, false otherwise.
         */
        [[nodiscard]] auto is_valid() const noexcept -> bool;

        /**
         * @brief Checks if the node represents a JSON 'null' value.
         * @return true if the node is null, false otherwise.
         */
        [[nodiscard]] auto is_null() const noexcept -> bool;

        /**
         * @brief Checks if the node is a JSON array.
         * @return true if the node is an array, false otherwise.
         */
        [[nodiscard]] auto is_array() const noexcept -> bool;

        /**
         * @brief Gets the number of elements in the JSON array.
         * @return The size of the array, or 0 if the node is not an array.
         */
        [[nodiscard]] auto size() const noexcept -> std::size_t;

        /**
         * @brief Retrieves an element from a JSON array by its index.
         * @param index The zero-based index of the element.
         * @return A new Node instance representing the element. Returns an invalid node if out of
         * bounds.
         */
        [[nodiscard]] auto get_element(std::size_t index) const noexcept -> Node;

        /**
         * @brief Converts the node's value to the requested C++ type.
         * Supported conversions:
         * - @c std::string : Extracts string and automatically expands environment variables in
         * `${VAR}` format.
         * - @c std::is_integral_v : Extracts as integer and casts to Type.
         * - @c std::is_floating_point_v : Extracts as double and casts to Type.
         * - @c bool : Extracts boolean value.
         * - Containers (with `value_type`): Extracts array elements recursively. Calls `.reserve()`
         * if supported.
         * - Custom Types: Looks up user-defined `from_json_node(*this, std::type_identity<Type>{})`
         * via ADL.
         * @tparam Type The target type to deserialize into.
         * @return std::optional<Type> Containing the value if successful, or std::nullopt if
         * conversion fails or node is invalid.
         */
        template <typename Type> auto as() const -> std::optional<Type>
        {
            if(!is_valid() || is_null()) {
                return std::nullopt;
            }
            if constexpr(std::is_same_v<Type, std::string>) {
                return get_string();
            } else if constexpr(std::is_same_v<Type, bool>) {
                return get_bool();
            } else if constexpr(std::is_integral_v<Type>) {
                if constexpr(std::is_unsigned_v<Type>) {
                    return get_uint64().transform([](auto v) { return static_cast<Type>(v); });
                } else {
                    return get_int64().transform([](auto v) { return static_cast<Type>(v); });
                }
            } else if constexpr(std::is_floating_point_v<Type>) {
                return get_double();
            } else if constexpr(requires { typename Type::value_type; }) {
                if(!is_array()) {
                    return std::nullopt;
                }
                Type container;
                const std::size_t element_size = size();
                if constexpr(requires { container.reserve(element_size); }) {
                    container.reserve(element_size);
                }
                for(std::size_t i = 0; i < element_size; ++i) {
                    Node element = get_element(i);
                    if(auto val = element.as<typename Type::value_type>()) {
                        container.push_back(std::move(*val));
                    } else {
                        return std::nullopt;
                    }
                }
                return container;
            } else {
                if constexpr(requires {
                                 {
                                     from_json_node(*this, std::type_identity<Type>{})
                                 } -> std::same_as<std::optional<Type>>;
                             })
                {
                    return from_json_node(*this, std::type_identity<Type>{});
                }
                return std::nullopt;
            }
        }

        /**
         * @brief Traverses deeper into the JSON object structure using a list of string keys.
         * @tparam Args Variadic pack of types convertible to @c std::string_view.
         * @param args The sequence of keys representing the path to the desired node.
         * @return A nested Node at the specified path.
         */
        template <typename... Args> auto get_child(Args&&... args) const -> Node
        {
            const std::string_view key_array[] = {std::string_view(args)...};
            return get_child_internal(key_array);
        }

    private:
        Node() = default;
        template <typename Type> auto expand_env(Type&& val) const -> std::decay_t<Type>
        {
            if constexpr(std::is_same_v<std::decay_t<Type>, std::string>) {
                static const std::regex env_re{R"(\$\{([^}]+)\})"};
                if(std::smatch match; std::regex_search(val, match, env_re)) {
                    std::string result{val};
                    std::size_t search_pos{0};
                    while(std::regex_search(
                        result.cbegin() + static_cast<std::ptrdiff_t>(search_pos),
                        result.cend(),
                        match,
                        env_re))
                    {
                        const std::string var_name = match[1].str();
                        const auto match_offset = static_cast<std::size_t>(match.position(0));
                        const auto match_len = static_cast<std::size_t>(match.length(0));
                        const auto pos = search_pos + match_offset;
                        if(const char* env_val = std::getenv(var_name.c_str())) {
                            result.replace(pos, match_len, env_val);
                            search_pos = pos + std::strlen(env_val);
                        } else {
                            search_pos = pos + match_len;
                        }
                    }
                    return result;
                }
            }
            return std::forward<Type>(val);
        }

        [[nodiscard]] auto get_string() const -> std::optional<std::string>;
        [[nodiscard]] auto get_int64() const -> std::optional<std::int64_t>;
        [[nodiscard]] auto get_uint64() const -> std::optional<std::uint64_t>;
        [[nodiscard]] auto get_double() const -> std::optional<double>;
        [[nodiscard]] auto get_bool() const -> std::optional<bool>;
        [[nodiscard]] auto get_child_internal(std::span<const std::string_view> key_span) const
            -> Node;
        struct Impl;
        std::shared_ptr<Impl> pimpl_;
    };

    /**
     * @brief Constructs a new Json object by parsing a file from the specified path.
     * @param filepath The path to the JSON file on the filesystem.
     */
    explicit Json(const std::filesystem::path& filepath);
    ~Json();

    /**
     * @brief Extracts a value of the specified type from a specific path in the JSON.
     * @tparam Type The target type to deserialize into.
     * @tparam Args Pack of types convertible to std::string_view.
     * @param args The sequence of keys representing the path to the desired value.
     * @return std::optional<Type> Containing the requested value if found and successfully parsed;
     * std::nullopt otherwise.
     */
    template <typename Type, typename... Args>
    [[nodiscard]] auto value(Args&&... args) const -> std::optional<Type>
    {
        const std::string_view keys[] = {std::string_view(args)...};
        return get_root_node().get_child_internal(keys).template as<Type>();
    }

    /**
     * @brief Extracts a value from the JSON path or throws an exception if it does not exist or
     * fails to parse.
     * @tparam Type The target type to deserialize into.
     * @tparam Args Pack of types convertible to std::string_view.
     * @param filepath The name/path of the file to report in the exception context.
     * @param message An error message to include in the exception if the retrieval fails.
     * @param args The sequence of keys representing the path to the desired value.
     * @return The extracted value of type Type.
     * @throws sample::error::FileInvalidFormat if the value cannot be extracted.
     */
    template <typename Type, typename... Args>
    [[nodiscard]] auto
    value_or_throw(const std::string_view filepath, const std::string_view message, Args&&... args)
        const -> Type
    {
        std::optional<Type> opt = value<Type>(std::forward<Args>(args)...);
        if(!opt) {
            throw error::FileInvalidFormat{std::string{filepath}, std::string{message}};
        }
        return std::move(*opt);
    }

    /**
     * @brief Attempts to extract a value from the JSON path and moves it into the destination
     * reference.
     * @tparam Type The target type to deserialize into.
     * @tparam Args Pack of types convertible to std::string_view.
     * @param destination The reference where the successfully parsed value will be moved.
     * @param args The sequence of keys representing the path to the desired value.
     * @return true if the value was successfully extracted and assigned; false otherwise
     * (destination is left untouched).
     */
    template <typename Type, typename... Args>
    auto exchange(Type& destination, Args&&... args) const -> bool
    {
        if(auto val = value<Type>(std::forward<Args>(args)...)) {
            destination = std::move(*val);
            return true;
        }
        return false;
    }

    /**
     * @brief Gets the root node of the parsed JSON document.
     * @return The root Node instance.
     */
    [[nodiscard]] auto get_root_node() const -> Node;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace sample
