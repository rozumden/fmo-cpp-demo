#define _CRT_SECURE_NO_WARNINGS // using std::localtime is insecure
#include "calendar.hpp"
#include <iomanip>
#include <sstream>

Date::Date() : mTime(std::time(nullptr)) {}

std::string Date::preciseStamp() const {
    std::ostringstream result;
    std::tm* ltm = std::localtime(&mTime);
    result << std::put_time(ltm, "%F %T %z");
    return result.str();
}

std::string Date::fileNameSafeStamp() const {
    std::ostringstream result;
    std::tm* ltm = std::localtime(&mTime);
    result << std::put_time(ltm, "%F-%H%M%S");
    return result.str();
}
