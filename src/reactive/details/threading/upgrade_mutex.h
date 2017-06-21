// From https://github.com/HowardHinnant/upgrade_mutex
// Modified: .cpp moved to .h

//---------------------------- upgrade_mutex.h ---------------------------------
// 
// This software is in the public domain.  The only restriction on its use is
// that no one can remove it from the public domain by claiming ownership of it,
// including the original authors.
// 
// There is no warranty of correctness on the software contained herein.  Use
// at your own risk.
// 
//------------------------------------------------------------------------------

#ifndef UPGRADE_MUTEX
#define UPGRADE_MUTEX

/*
    <upgrade_mutex.h> synopsis

namespace acme
{

class upgrade_mutex
{
public:
    upgrade_mutex();
    ~upgrade_mutex();

    upgrade_mutex(const upgrade_mutex&) = delete;
    upgrade_mutex& operator=(const upgrade_mutex&) = delete;

    // Exclusive ownership

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    // Shared ownership

    void lock_shared();
    bool try_lock_shared();
    template <class Rep, class Period>
        bool
        try_lock_shared_for(const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_shared_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_shared();

    // Upgrade ownership

    void lock_upgrade();
    bool try_lock_upgrade();
    template <class Rep, class Period>
        bool
        try_lock_upgrade_for(
                            const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_upgrade_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade();

    // Shared <-> Exclusive -- unused by Locks without std::lib cooperation

    bool try_unlock_shared_and_lock();
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_for(
                            const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_and_lock_shared();

    // Shared <-> Upgrade

    bool try_unlock_shared_and_lock_upgrade();
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_upgrade_for(
                            const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_upgrade_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade_and_lock_shared();

    // Upgrade <-> Exclusive

    void unlock_upgrade_and_lock();
    void unlock_and_lock_upgrade();

    //  unused by Locks without std::lib cooperation

    bool try_unlock_upgrade_and_lock();
    template <class Rep, class Period>
        bool
        try_unlock_upgrade_and_lock_for(
                            const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_upgrade_and_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
};

template <class Mutex>
class upgrade_lock
{
public:
    typedef Mutex mutex_type;

    ~upgrade_lock();
    upgrade_lock() noexcept;
    upgrade_lock(upgrade_lock const&) = delete;
    upgrade_lock& operator=(upgrade_lock const&) = delete;
    upgrade_lock(upgrade_lock&& ul) noexcept;
    upgrade_lock& operator=(upgrade_lock&& ul);

    explicit upgrade_lock(mutex_type& m);
    upgrade_lock(mutex_type& m, std::defer_lock_t) noexcept;
    upgrade_lock(mutex_type& m, std::try_to_lock_t);
    upgrade_lock(mutex_type& m, std::adopt_lock_t) noexcept;
    template <class Clock, class Duration>
        upgrade_lock(mutex_type& m,
                    const std::chrono::time_point<Clock, Duration>& abs_time);
    template <class Rep, class Period>
        upgrade_lock(mutex_type& m,
                    const std::chrono::duration<Rep, Period>& rel_time);

    // Shared <-> Upgrade

    upgrade_lock(std::shared_lock<mutex_type>&& sl, std::try_to_lock_t);

    template <class Clock, class Duration>
        upgrade_lock(std::shared_lock<mutex_type>&& sl,
                     const std::chrono::time_point<Clock, Duration>& abs_time);
    template <class Rep, class Period>
        upgrade_lock(std::shared_lock<mutex_type>&& sl,
                     const std::chrono::duration<Rep, Period>& rel_time);
    explicit operator std::shared_lock<mutex_type> () &&;

    // Exclusive <-> Upgrade

    explicit upgrade_lock(std::unique_lock<mutex_type>&& ul);
    explicit operator std::unique_lock<mutex_type> () &&;

    // Upgrade

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    void swap(upgrade_lock&& u);
    mutex_type* release();

    bool owns_lock() const;
    explicit operator bool () const;
    mutex_type* mutex() const;
};

template <class Mutex>
void
swap(upgrade_lock<Mutex>&  x, upgrade_lock<Mutex>&  y);

}  // acme
*/

#include <chrono>
#include <climits>
#include <mutex>
#include <shared_mutex>
#include <system_error>

namespace acme
{

// upgrade_mutex

class upgrade_mutex
{
    std::mutex              mut_;
    std::condition_variable gate1_;
    std::condition_variable gate2_;
    unsigned                state_;

    static const unsigned write_entered_ = 1U << (sizeof(unsigned)*CHAR_BIT - 1);
    static const unsigned upgradable_entered_ = write_entered_ >> 1;
    static const unsigned n_readers_ = ~(write_entered_ | upgradable_entered_);

public:
    upgrade_mutex()
		: state_(0)
	{}

    ~upgrade_mutex() = default;

    upgrade_mutex(const upgrade_mutex&) = delete;
    upgrade_mutex& operator=(const upgrade_mutex&) = delete;

    // Exclusive ownership

    void lock() 
	{
		std::unique_lock<std::mutex> lk(mut_);
		while (state_ & (write_entered_ | upgradable_entered_))
			gate1_.wait(lk);
		state_ |= write_entered_;
		while (state_ & n_readers_)
			gate2_.wait(lk);
	}

    bool try_lock()
	{
		std::unique_lock<std::mutex> lk(mut_);
		if (state_ == 0)
		{
			state_ = write_entered_;
			return true;
		}
		return false;
	}
    template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_lock_until(std::chrono::steady_clock::now() + rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock()
	{
		std::lock_guard<std::mutex> _(mut_);
		state_ = 0;
		gate1_.notify_all();
	}

    // Shared ownership

    void lock_shared()
	{
		std::unique_lock<std::mutex> lk(mut_);
		while ((state_ & write_entered_) || (state_ & n_readers_) == n_readers_)
			gate1_.wait(lk);
		unsigned num_readers = (state_ & n_readers_) + 1;
		state_ &= ~n_readers_;
		state_ |= num_readers;
	}

    bool try_lock_shared()
	{
		std::unique_lock<std::mutex> lk(mut_);
		unsigned num_readers = state_ & n_readers_;
		if (!(state_ & write_entered_) && num_readers != n_readers_)
		{
			++num_readers;
			state_ &= ~n_readers_;
			state_ |= num_readers;
			return true;
		}
		return false;
	}

    template <class Rep, class Period>
        bool
        try_lock_shared_for(const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_lock_shared_until(std::chrono::steady_clock::now() +
                                         rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_lock_shared_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_shared()
	{
		std::lock_guard<std::mutex> _(mut_);
		unsigned num_readers = (state_ & n_readers_) - 1;
		state_ &= ~n_readers_;
		state_ |= num_readers;
		if (state_ & write_entered_)
		{
			if (num_readers == 0)
				gate2_.notify_one();
		}
		else
		{
			if (num_readers == n_readers_ - 1)
				gate1_.notify_one();
		}
	}

    // Upgrade ownership

    void lock_upgrade()
	{
		std::unique_lock<std::mutex> lk(mut_);
		while ((state_ & (write_entered_ | upgradable_entered_)) ||
			(state_ & n_readers_) == n_readers_)
			gate1_.wait(lk);
		unsigned num_readers = (state_ & n_readers_) + 1;
		state_ &= ~n_readers_;
		state_ |= upgradable_entered_ | num_readers;
	}

    bool try_lock_upgrade()
	{
		std::unique_lock<std::mutex> lk(mut_);
		unsigned num_readers = state_ & n_readers_;
		if (!(state_ & (write_entered_ | upgradable_entered_))
			&& num_readers != n_readers_)
		{
			++num_readers;
			state_ &= ~n_readers_;
			state_ |= upgradable_entered_ | num_readers;
			return true;
		}
		return false;
	}

    template <class Rep, class Period>
        bool
        try_lock_upgrade_for(
                            const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_lock_upgrade_until(std::chrono::steady_clock::now() +
                                         rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_lock_upgrade_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade()
	{
		{
			std::lock_guard<std::mutex> _(mut_);
			unsigned num_readers = (state_ & n_readers_) - 1;
			state_ &= ~(upgradable_entered_ | n_readers_);
			state_ |= num_readers;
		}
		gate1_.notify_all();
	}


    // Shared <-> Exclusive

    bool try_unlock_shared_and_lock()
	{
		std::unique_lock<std::mutex> lk(mut_);
		if (state_ == 1)
		{
			state_ = write_entered_;
			return true;
		}
		return false;
	}

    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_for(
                            const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_unlock_shared_and_lock_until(
                                   std::chrono::steady_clock::now() + rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_and_lock_shared()
	{
		{
			std::lock_guard<std::mutex> _(mut_);
			state_ = 1;
		}
		gate1_.notify_all();
	}


    // Shared <-> Upgrade

	bool try_unlock_shared_and_lock_upgrade()
	{
		std::unique_lock<std::mutex> lk(mut_);
		if (!(state_ & (write_entered_ | upgradable_entered_)))
		{
			state_ |= upgradable_entered_;
			return true;
		}
		return false;
	}
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_upgrade_for(
                            const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_unlock_shared_and_lock_upgrade_until(
                                   std::chrono::steady_clock::now() + rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_upgrade_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
	void unlock_upgrade_and_lock_shared()
	{
		{
			std::lock_guard<std::mutex> _(mut_);
			state_ &= ~upgradable_entered_;
		}
		gate1_.notify_all();
	}

    // Upgrade <-> Exclusive

	void unlock_upgrade_and_lock()
	{
		std::unique_lock<std::mutex> lk(mut_);
		unsigned num_readers = (state_ & n_readers_) - 1;
		state_ &= ~(upgradable_entered_ | n_readers_);
		state_ |= write_entered_ | num_readers;
		while (state_ & n_readers_)
			gate2_.wait(lk);
	}
	bool try_unlock_upgrade_and_lock()
	{
		std::unique_lock<std::mutex> lk(mut_);
		if (state_ == (upgradable_entered_ | 1))
		{
			state_ = write_entered_;
			return true;
		}
		return false;
	}
    template <class Rep, class Period>
        bool
        try_unlock_upgrade_and_lock_for(
                            const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_unlock_upgrade_and_lock_until(
                                   std::chrono::steady_clock::now() + rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_unlock_upgrade_and_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
	void unlock_and_lock_upgrade()
	{
		{
			std::lock_guard<std::mutex> _(mut_);
			state_ = upgradable_entered_ | 1;
		}
		gate1_.notify_all();
	}
};

template <class Clock, class Duration>
bool
upgrade_mutex::try_lock_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    std::unique_lock<std::mutex> lk(mut_);
    if (state_ & (write_entered_ | upgradable_entered_))
    {
        while (true)
        {
            std::cv_status status = gate1_.wait_until(lk, abs_time);
            if ((state_ & (write_entered_ | upgradable_entered_)) == 0)
                break;
            if (status == std::cv_status::timeout)
                return false;
        }
    }
    state_ |= write_entered_;
    if (state_ & n_readers_)
    {
        while (true)
        {
            std::cv_status status = gate2_.wait_until(lk, abs_time);
            if ((state_ & n_readers_) == 0)
                break;
            if (status == std::cv_status::timeout)
            {
                state_ &= ~write_entered_;
                return false;
            }
        }
    }
    return true;
}

template <class Clock, class Duration>
bool
upgrade_mutex::try_lock_shared_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    std::unique_lock<std::mutex> lk(mut_);
    if ((state_ & write_entered_) || (state_ & n_readers_) == n_readers_)
    {
        while (true)
        {
            std::cv_status status = gate1_.wait_until(lk, abs_time);
            if ((state_ & write_entered_) == 0 &&
                                             (state_ & n_readers_) < n_readers_)
                break;
            if (status == std::cv_status::timeout)
                return false;
        }
    }
    unsigned num_readers = (state_ & n_readers_) + 1;
    state_ &= ~n_readers_;
    state_ |= num_readers;
    return true;
}

template <class Clock, class Duration>
bool
upgrade_mutex::try_lock_upgrade_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    std::unique_lock<std::mutex> lk(mut_);
    if ((state_ & (write_entered_ | upgradable_entered_)) ||
        (state_ & n_readers_) == n_readers_)
    {
        while (true)
        {
            std::cv_status status = gate1_.wait_until(lk, abs_time);
            if ((state_ & (write_entered_ | upgradable_entered_)) == 0 &&
                                             (state_ & n_readers_) < n_readers_)
                break;
            if (status == std::cv_status::timeout)
                return false;
        }
    }
    unsigned num_readers = (state_ & n_readers_) + 1;
    state_ &= ~n_readers_;
    state_ |= upgradable_entered_ | num_readers;
    return true;
}

template <class Clock, class Duration>
bool
upgrade_mutex::try_unlock_shared_and_lock_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    std::unique_lock<std::mutex> lk(mut_);
    if (state_ != 1)
    {
        while (true)
        {
            std::cv_status status = gate2_.wait_until(lk, abs_time);
            if (state_ == 1)
                break;
            if (status == std::cv_status::timeout)
                return false;
        }
    }
    state_ = write_entered_;
    return true;
}

template <class Clock, class Duration>
bool
upgrade_mutex::try_unlock_shared_and_lock_upgrade_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    std::unique_lock<std::mutex> lk(mut_);
    if ((state_ & (write_entered_ | upgradable_entered_)) != 0)
    {
        while (true)
        {
            std::cv_status status = gate2_.wait_until(lk, abs_time);
            if ((state_ & (write_entered_ | upgradable_entered_)) == 0)
                break;
            if (status == std::cv_status::timeout)
                return false;
        }
    }
    state_ |= upgradable_entered_;
    return true;
}

template <class Clock, class Duration>
bool
upgrade_mutex::try_unlock_upgrade_and_lock_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    std::unique_lock<std::mutex> lk(mut_);
    if ((state_ & n_readers_) != 1)
    {
        while (true)
        {
            std::cv_status status = gate2_.wait_until(lk, abs_time);
            if ((state_ & n_readers_) == 1)
                break;
            if (status == std::cv_status::timeout)
                return false;
        }
    }
    state_ = write_entered_;
    return true;
}

// upgrade_lock

template <class Mutex>
class upgrade_lock
{
public:
    typedef Mutex mutex_type;

private:
    mutex_type* m_;
    bool        owns_;

    struct __nat {int _;};

public:
    ~upgrade_lock()
    {
        if (owns_)
            m_->unlock_upgrade();
    }

    upgrade_lock() noexcept
        : m_(nullptr), owns_(false) {}

    upgrade_lock(upgrade_lock const&) = delete;
    upgrade_lock& operator=(upgrade_lock const&) = delete;

    upgrade_lock(upgrade_lock&& ul) noexcept
        : m_(ul.m_), owns_(ul.owns_)
        {
            ul.m_ = nullptr;
            ul.owns_ = false;
        }

    upgrade_lock& operator=(upgrade_lock&& ul)
    {
        if (owns_)
            m_->unlock_upgrade();
        m_ = ul.m_;
        owns_ = ul.owns_;
        ul.m_ = nullptr;
        ul.owns_ = false;
        return *this;
    }

    explicit upgrade_lock(mutex_type& m)
        : m_(&m), owns_(true)
        {m_->lock_upgrade();}

    upgrade_lock(mutex_type& m, std::defer_lock_t) noexcept
        : m_(&m), owns_(false) {}

    upgrade_lock(mutex_type& m, std::try_to_lock_t)
        : m_(&m), owns_(m.try_lock_upgrade()) {}

    upgrade_lock(mutex_type& m, std::adopt_lock_t) noexcept
        : m_(&m), owns_(true) {}

    template <class Clock, class Duration>
        upgrade_lock(mutex_type& m,
                    const std::chrono::time_point<Clock, Duration>& abs_time)
        : m_(&m), owns_(m.try_lock_upgrade_until(abs_time)) {}
    template <class Rep, class Period>
        upgrade_lock(mutex_type& m,
                    const std::chrono::duration<Rep, Period>& rel_time)
        : m_(&m), owns_(m.try_lock_upgrade_for(rel_time)) {}

    // Shared <-> Upgrade

    upgrade_lock(std::shared_lock<mutex_type>&& sl, std::try_to_lock_t)
        : m_(nullptr), owns_(false)
    {
        if (sl.owns_lock())
        {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade())
            {
                m_ = sl.release();
                owns_ = true;
            }
        }
        else
            m_ = sl.release();
    }

    template <class Clock, class Duration>
        upgrade_lock(std::shared_lock<mutex_type>&& sl,
                     const std::chrono::time_point<Clock, Duration>& abs_time)
        : m_(nullptr), owns_(false)
    {
        if (sl.owns_lock())
        {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_until(abs_time))
            {
                m_ = sl.release();
                owns_ = true;
            }
        }
        else
            m_ = sl.release();
    }

    template <class Rep, class Period>
        upgrade_lock(std::shared_lock<mutex_type>&& sl,
                     const std::chrono::duration<Rep, Period>& rel_time)
        : m_(nullptr), owns_(false)
    {
        if (sl.owns_lock())
        {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_for(rel_time))
            {
                m_ = sl.release();
                owns_ = true;
            }
        }
        else
            m_ = sl.release();
    }

    explicit operator std::shared_lock<mutex_type> () &&
    {
        if (owns_)
        {
            m_->unlock_upgrade_and_lock_shared();
            return std::shared_lock<mutex_type>(*release(), std::adopt_lock);
        }
        if (m_ != nullptr)
            return std::shared_lock<mutex_type>(*release(), std::defer_lock);
        return std::shared_lock<mutex_type>{};
    }

    // Exclusive <-> Upgrade

    explicit upgrade_lock(std::unique_lock<mutex_type>&& ul)
        : m_(ul.mutex()), owns_(ul.owns_lock())
    {
        if (owns_)
            m_->unlock_and_lock_upgrade();
        ul.release();
    }

    explicit operator std::unique_lock<mutex_type> () &&
    {
        if (owns_)
        {
            m_->unlock_upgrade_and_lock();
            return std::unique_lock<mutex_type>(*release(), std::adopt_lock);
        }
        if (m_ != nullptr)
            return std::unique_lock<mutex_type>(*release(), std::defer_lock);
        return std::unique_lock<mutex_type>{};
    }

    // Upgrade

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time)
        {
            return try_lock_until(std::chrono::steady_clock::now() + rel_time);
        }
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const std::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    void swap(upgrade_lock&& u)
    {
        std::swap(m_, u.m_);
        std::swap(owns_, u.owns_);
    }

    mutex_type* release()
    {
        mutex_type* r = m_;
        m_ = 0;
        owns_ = false;
        return r;
    }

    bool owns_lock() const {return owns_;}
    explicit operator bool () const {return owns_;}
    mutex_type* mutex() const {return m_;}
};

template <class Mutex>
void
upgrade_lock<Mutex>::lock()
{
    if (m_ == nullptr)
        throw std::system_error(std::error_code(EPERM, std::system_category()),
                                   "upgrade_lock::lock: references null mutex");
    if (owns_)
        throw std::system_error(std::error_code(EDEADLK, std::system_category()),
                                          "upgrade_lock::lock: already locked");
    m_->lock_upgrade();
    owns_ = true;
}

template <class Mutex>
bool
upgrade_lock<Mutex>::try_lock()
{
    if (m_ == nullptr)
        throw std::system_error(std::error_code(EPERM, std::system_category()),
                               "upgrade_lock::try_lock: references null mutex");
    if (owns_)
        throw std::system_error(std::error_code(EDEADLK, std::system_category()),
                                      "upgrade_lock::try_lock: already locked");
    owns_ = m_->try_lock_upgrade();
    return owns_;
}

template <class Mutex>
template <class Clock, class Duration>
bool
upgrade_lock<Mutex>::try_lock_until(
                       const std::chrono::time_point<Clock, Duration>& abs_time)
{
    if (m_ == nullptr)
        throw std::system_error(std::error_code(EPERM, std::system_category()),
                         "upgrade_lock::try_lock_until: references null mutex");
    if (owns_)
        throw std::system_error(std::error_code(EDEADLK, std::system_category()),
                                "upgrade_lock::try_lock_until: already locked");
    owns_ = m_->try_lock_upgrade_until(abs_time);
    return owns_;
}

template <class Mutex>
void
upgrade_lock<Mutex>::unlock()
{
    if (!owns_)
        throw std::system_error(std::error_code(EPERM, std::system_category()),
                                "upgrade_lock::unlock: not locked");
    m_->unlock_upgrade();
    owns_ = false;
}

template <class Mutex>
inline
void
swap(upgrade_lock<Mutex>&  x, upgrade_lock<Mutex>&  y)
{
    x.swap(y);
}

}  // acme

#endif  //  UPGRADE_MUTEX
