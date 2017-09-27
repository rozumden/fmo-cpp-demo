#ifndef FMO_DESKTOP_CALENDAR_HPP
#define FMO_DESKTOP_CALENDAR_HPP

#include <ctime>
#include <iostream>

/// Stores the date and time at the instant of construction and provides its string representation.
struct Date {
    Date();

    /// Timestamp with second precision unsafe to be used in filenames, but more readable and with
    /// time zone information.
    std::string preciseStamp() const;

    /// Timestamp with second precision safe to be used in filenames.
    std::string fileNameSafeStamp() const;

private:
    const time_t mTime;
};

#endif // FMO_DESKTOP_CALENDAR_HPP
