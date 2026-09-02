export module sample.core.thread_safe_data;

import std;

export namespace sample {

/**
 * @brief A thread-safe container that synchronizes access to an underlying object of type T.
 * This class provides an atomic-like interface for arbitrary types by encapsulating the data
 * alongside a mutual exclusion lock (`std::mutex`). It guarantees thread safety for simple
 * operations (load/store/exchange) as well as compound operations via functional accessors.
 * @tparam T The type of the protected data encapsulated within the container.
 */
template <typename T> class ThreadSafeData final
{
public:
    /**
     * @brief Default constructs the underlying data object.
     * Requires the type T to be default constructible.
     */
    ThreadSafeData() = default;

    /**
     * @brief Constructs the underlying object in-place using the provided arguments.
     * @tparam Args Variadic template argument types to forward to T's constructor.
     * @param args Arguments forwarded to the constructor of the internal data member.
     */
    template <typename... Args>
    explicit ThreadSafeData(Args&&... args) : data_{std::forward<Args>(args)...}
    {}

    /**
     * @brief Safely retrieves a copy of the underlying data.
     * Locks the container, makes a copy of the data, and returns it.
     * @note This method is only available if the underlying type T satisfies the `std::copyable`
     * concept.
     * @return T A copy of the protected data.
     */
    [[nodiscard]] auto load() const -> T
        requires std::copyable<T>
    {
        std::lock_guard lock{mutex_};
        return data_;
    }

    /**
     * @brief Overwrites the stored value using copy assignment.
     * Acquires the lock, performs a copy-assignment from the provided value, and releases the lock.
     * @param value The new value to be copied into the container.
     */
    void store(const T& value)
        requires std::assignable_from<T&, const T&>
    {
        std::lock_guard lock{mutex_};
        data_ = value;
    }

    /**
     * @brief Overwrites the stored value using move assignment.
     * Acquires the lock, performs a move-assignment from the provided rvalue, and releases the
     * lock. Any heavy allocation/construction required by the type conversion happens on the
     * caller's thread before the lock is acquired.
     * @param value The rvalue to be moved into the container.
     */
    void store(T&& value)
        requires std::assignable_from<T&, T&&>
    {
        std::lock_guard lock{mutex_};
        data_ = std::move(value);
    }

    /**
     * @brief Replaces the internal value with a copy and returns the old value.
     * Performs an atomic-like exchange operation under the internal mutex.
     * @param value The new value to copy into the container.
     * @return T The old value that was replaced.
     */
    [[nodiscard]] auto exchange(const T& value) -> T
        requires std::assignable_from<T&, const T&> && std::move_constructible<T>
    {
        std::lock_guard lock{mutex_};
        return std::exchange(data_, value);
    }

    /**
     * @brief Replaces the internal value with an rvalue and returns the old value.
     * Performs an atomic-like exchange operation under the internal mutex using move semantics.
     * @param value The new rvalue to move into the container.
     * @return T The old value that was replaced.
     */
    [[nodiscard]] auto exchange(T&& value) -> T
        requires std::assignable_from<T&, T&&> && std::move_constructible<T>
    {
        std::lock_guard lock{mutex_};
        return std::exchange(data_, std::move(value));
    }

    /**
     * @brief Executes a user-defined callable within the protected critical section.
     * Locks the container, passes a reference to the internal data directly into the provided
     * functional object, executes it, and unlocks the container upon scope exit. This allows safely
     * executing multiple operations on the internal object under a single lock acquisition.
     * @tparam Func The type of the callable object.
     * @param func A callable object (e.g., lambda) that accepts a reference to T (const T& or T&).
     * @return auto The return value of the invoked callable `func`.
     */
    template <typename Func>
    auto around(this auto& self, Func&& func)
        requires std::invocable<Func, decltype((self.data_))>
    {
        std::lock_guard lock{self.mutex_};
        return std::forward<Func>(func)(self.data_);
    }

    /**
     * @brief Blocks the thread until a predicate is met, then safely runs a operation under the
     * lock. Acquires a unique lock, safely waits on the provided `std::condition_variable` while
     * evaluating the predicate under the lock, and once the predicate evaluates to `true`, invokes
     * the target function.
     * @tparam Predicate The type of the condition check criteria.
     * @tparam Func The type of the operation to execute after the predicate is satisfied.
     * @param cv The external condition variable used to block and wake up the thread.
     * @param pred A callable acting as a condition (must accept a reference to T and return
     * `bool`).
     * @param func The operation to execute once the predicate matches.
     * @return auto The return value of the invoked operation `func`.
     */
    template <typename Predicate, typename Func>
    auto wait(this auto& self, std::condition_variable& cv, Predicate&& pred, Func&& func)
        requires std::invocable<Func, decltype((self.data_))> &&
                 std::invocable<Predicate, decltype((self.data_))>
    {
        std::unique_lock lock{self.mutex_};
        cv.wait(lock, [&] { return pred(self.data_); });
        return std::forward<Func>(func)(self.data_);
    }

    /**
     * @brief Blocks the current thread until the predicate is satisfied or a specific time point is
     * reached. This method locks the underlying mutex and waits on the provided condition variable.
     * It periodically checks the predicate under the lock. Once the predicate returns true
     * or the timeout is reached, the provided function is executed on the protected data.
     * @tparam Clock The clock type defining the time base (e.g., std::chrono::steady_clock).
     * @tparam Duration A std::chrono::duration type representing the time precision.
     * @tparam Predicate A callable type that accepts the data and returns a bool-convertible value.
     * @tparam Func A callable type executed on the data after the wait ends.
     * @param cv The external condition variable used to manage the blocking and notification.
     * @param timeout_time The absolute time point at which the waiting stops.
     * @param pred A predicate function that evaluates whether the condition to stop waiting is met.
     * @param func A function to execute on the data once the thread is unblocked.
     * @return The result of invoking `func` with the underlying data.
     * @throws Any exception thrown by the internal mutex lock, `cv.wait_until`, `pred`, or `func`.
     */
    template <typename Clock, typename Duration, typename Predicate, typename Func>
    auto wait_until(
        this auto& self,
        std::condition_variable& cv,
        const std::chrono::time_point<Clock, Duration>& timeout_time,
        Predicate&& pred,
        Func&& func)
        requires std::invocable<Func, decltype((self.data_))> &&
                 std::invocable<Predicate, decltype((self.data_))>
    {
        std::unique_lock lock{self.mutex_};
        cv.wait_until(lock, timeout_time, [&] { return pred(self.data_); });
        return std::forward<Func>(func)(self.data_);
    }

    /**
     * @brief Thread-safe copy assignment operator.
     * Delegates to `store(value)` to update the internal state under a lock.
     * @param value The value to copy into the container.
     * @return ThreadSafeData& Reference to `*this`.
     */
    auto operator=(const T& value) -> ThreadSafeData&
        requires std::assignable_from<T&, const T&>
    {
        store(value);
        return *this;
    }

    /**
     * @brief Thread-safe move assignment operator.
     * Delegates to `store(std::move(value))` to move the state under a lock.
     * @param value The rvalue to move into the container.
     * @return ThreadSafeData& Reference to `*this`.
     */
    auto operator=(T&& value) -> ThreadSafeData&
        requires std::assignable_from<T&, T&&>
    {
        store(std::move(value));
        return *this;
    }

    /**
     * @brief Thread-safe equality comparison operator.
     * Acquires the internal lock and checks if the underlying data matches the provided value.
     * @param value The value to compare against the internal data.
     * @return true If the data is equal to `value`.
     * @return false Otherwise.
     */
    [[nodiscard]] auto operator==(const T& value) const -> bool
    {
        std::lock_guard lock{mutex_};
        return data_ == value;
    }

private:
    mutable std::mutex mutex_;
    T data_;
};

template class ThreadSafeData<std::vector<std::uint8_t>>;
template class ThreadSafeData<std::vector<std::uint16_t>>;
template class ThreadSafeData<std::vector<std::uint32_t>>;
template class ThreadSafeData<std::vector<std::uint64_t>>;
template class ThreadSafeData<std::vector<int>>;
template class ThreadSafeData<std::vector<float>>;
template class ThreadSafeData<std::vector<double>>;
template class ThreadSafeData<std::uint8_t>;
template class ThreadSafeData<std::uint16_t>;
template class ThreadSafeData<std::uint32_t>;
template class ThreadSafeData<std::uint64_t>;
template class ThreadSafeData<int>;
template class ThreadSafeData<float>;
template class ThreadSafeData<double>;
template class ThreadSafeData<std::string>;

} // namespace sample
