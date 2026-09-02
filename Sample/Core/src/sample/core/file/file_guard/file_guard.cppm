module;
#include <unistd.h>

export module sample.core.file.file_guard;

import std;

export namespace sample {

/**
 * @brief A resource guard providing strict RAII management for a file descriptor.
 * This class ensures that the underlying system resource is automatically closed
 * when the guard goes out of scope. It enforces move-only semantics to prevent
 * accidental duplication of resource ownership.
 */
class FileGuard final
{
public:
    /**
     * @brief Constructs an empty FileGuard with no managed resource.
     */
    FileGuard() noexcept = default;

    /**
     * @brief Constructs a FileGuard owning the specified file descriptor.
     * @param descriptor The POSIX file descriptor to manage.
     */
    explicit FileGuard(int descriptor) noexcept : descriptor_{descriptor} {}
    ~FileGuard()
    {
        if(descriptor_ != -1) {
            ::close(descriptor_);
        }
    }
    FileGuard(const FileGuard&) = delete;
    auto operator=(const FileGuard&) -> FileGuard& = delete;
    FileGuard(FileGuard&& other) noexcept : descriptor_{std::exchange(other.descriptor_, -1)} {}
    auto operator=(FileGuard&& other) noexcept -> FileGuard&
    {
        if(this != &other) {
            reset(other.release());
        }
        return *this;
    }
    [[nodiscard]] auto operator==(const FileGuard& other) const noexcept -> bool = default;
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return descriptor_ != -1;
    }

    /**
     * @brief Replaces the managed descriptor with a new one. The old descriptor is closed.
     * @param descriptor The new file descriptor to manage (defaults to -1).
     */
    void reset(int descriptor = -1) noexcept
    {
        if(descriptor_ == descriptor) [[unlikely]] {
            return;
        }
        if(descriptor_ != -1) {
            ::close(descriptor_);
        }
        descriptor_ = descriptor;
    }

    /**
     * @brief Releases ownership of the managed descriptor without closing it.
     * @return The unmanaged file descriptor.
     */
    [[nodiscard]] auto release() noexcept -> int
    {
        return std::exchange(descriptor_, -1);
    }

    /**
     * @brief Accesses the underlying file descriptor.
     * @return The managed file descriptor.
     */
    [[nodiscard]] auto get() const noexcept -> int
    {
        return descriptor_;
    }

private:
    int descriptor_{-1};
};

} // namespace sample