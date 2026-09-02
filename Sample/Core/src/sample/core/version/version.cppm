export module sample.core.version;

import std;
import :constant;

export namespace sample {

struct Version
{
    std::uint16_t major;
    std::uint16_t minor;
    std::uint16_t revision;
    [[nodiscard]] auto to_string() const -> std::string
    {
        return std::format("{0}.{1}.{2}", major, minor, revision);
    }
    constexpr Version() noexcept : major{0}, minor{0}, revision{0} {}
    constexpr explicit Version(
        std::uint16_t major,
        std::uint16_t minor = 0,
        std::uint16_t revision = 0) noexcept :
        major{major},
        minor{minor},
        revision{revision}
    {}
    auto operator<=>(const Version&) const noexcept = default;
};

inline constexpr Version program_version{version_major, version_minor, version_revision};

} // namespace sample
