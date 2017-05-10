/*
 * This file is part of the bitcoin-classic project
 * Copyright (C) 2017 Tom Zander <tomz@freedommail.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LOGITEM_H
#define LOGITEM_H

#include <sstream>
#include <list>
#include <mutex>

#include "tinyformat.h"

extern bool fDebug;

static const bool DEFAULT_LOGTIMESTAMPS = true;

namespace Log {

class Item;
class SilentItem;
class Channel;

enum Verbosity {
    DebugLevel = 1,
    WarningLevel,
    InfoLevel,
    CriticalLevel,
    FatalLevel,
    MaxVerbosity
};

enum Sections {

};


class Manager
{
public:
    Manager();
    ~Manager();

    // backwards compatible.
    bool enabled(const char *region) const;
    bool enabled(int section) const;

    static Manager* instance();
    void log(Item *item);

    void reopenLogFiles();

    void loadDefaultTestSetup();

private:
    void clearChannels();
    std::list<Channel*> m_channels;
    std::mutex m_lock;
    std::string m_lastTime;
};

class Item
{
public:
    explicit Item(const char *filename, int lineNumber, const char *methodName, int verbosity = InfoLevel);
    explicit Item(int verbosity = InfoLevel);
    Item(const Item &other);
    ~Item();

    inline Item &nospace() { d->space = false; return *this; }
    inline Item &space() { d->space = true; d->stream << ' '; return *this; }
    inline Item &maybespace() { if (d->space) d->stream << ' '; return *this; }

    inline int verbosity() const {
        return d->verbosity;
    }
    inline void setVerbosity(int val) {
        d->verbosity = val;
    }
    inline bool isEnabled() const {
        return d->on;
    }
    inline void setEnabled(bool enabled) {
        d->on = enabled;
    }

    inline Item &operator<<(char c) { if(d->on)d->stream << c; return maybespace(); }
    inline Item &operator<<(const char *c) { if(d->on)d->stream << c; return maybespace(); }
    inline Item &operator<<(int i) { if(d->on)d->stream << i; return maybespace(); }
    inline Item &operator<<(double val) { if(d->on)d->stream << val; return maybespace(); }
    inline Item &operator<<(const std::string &str) { if(d->on){d->stream << '"' << str << '"';} return maybespace(); }
    inline Item &operator<<(bool val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(short val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(unsigned short val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(unsigned int val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(long val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(unsigned long val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(long long val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(unsigned long long val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(float val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(long double val) { if(d->on)d->stream << val ; return maybespace(); }
    inline Item &operator<<(void* val) { if(d->on)d->stream << val ; return maybespace(); }

    Item operator=(const Item &other) = delete;

private:
    friend class Manager;
    friend class MessageLogger;
    struct State {
        State(const char*filename, int lineNumber, const char *methodName) : lineNum(lineNumber), filename(filename), methodName(methodName) {}
        std::ostringstream stream;
        bool space;
        bool on;
        short verbosity;
        short ref;
        const int lineNum;
        const char *filename;
        const char *methodName;
    } *d;
};



class MessageLogger {
public:
    MessageLogger();
    explicit MessageLogger(const char *filename, int lineNumber, const char *methodName);

    Item debug(int section = 0);
    Item warning(int section = 0);
    Item info(int section = 0);
    Item critical(int section = 0);
    Item fatal(int section = 0);

    inline Log::SilentItem noDebug(int = 0);

#define MAKE_LOGGER_FUNCTIONS(n) \
    template<TINYFORMAT_ARGTYPES(n)> \
    inline void infoCompat(const char *section, const char* format, TINYFORMAT_VARARGS(n)) { \
        Item item(m_file, m_line, m_method); \
        item.setEnabled(Manager::instance()->enabled(section)); \
        item.d->stream << tfm::format(format, TINYFORMAT_PASSARGS(n)); \
    } \
    template<TINYFORMAT_ARGTYPES(n)> \
    inline Item debug(const char* format, TINYFORMAT_VARARGS(n)) { \
        Item item(m_file, m_line, m_method, DebugLevel); \
        item.d->stream << tfm::format(format, TINYFORMAT_PASSARGS(n)); \
        return item; \
    } \
    template<TINYFORMAT_ARGTYPES(n)> \
    inline Item info(const char* format, TINYFORMAT_VARARGS(n)) { \
        Item item(m_file, m_line, m_method); \
        item.d->stream << tfm::format(format, TINYFORMAT_PASSARGS(n)); \
        return item; \
    } \
    template<TINYFORMAT_ARGTYPES(n)> \
    inline Item warning(int section, const char* format, TINYFORMAT_VARARGS(n)) { \
        Item item(m_file, m_line, m_method, WarningLevel); \
        item.d->stream << tfm::format(format, TINYFORMAT_PASSARGS(n)); \
        return item; \
    } \
    template<TINYFORMAT_ARGTYPES(n)> \
    inline Item critical(const char* format, TINYFORMAT_VARARGS(n)) { \
        Item item(m_file, m_line, m_method, CriticalLevel); \
        item.d->stream << tfm::format(format, TINYFORMAT_PASSARGS(n)); \
        return item; \
    } \
    template<TINYFORMAT_ARGTYPES(n)> \
    inline Item fatal(const char* format, TINYFORMAT_VARARGS(n)) { \
        Item item(m_file, m_line, m_method, FatalLevel); \
        item.d->stream << tfm::format(format, TINYFORMAT_PASSARGS(n)); \
        return item; \
    }
TINYFORMAT_FOREACH_ARGNUM(MAKE_LOGGER_FUNCTIONS)

    inline void infoCompat(const char *section, const char* format = 0) {
        Item item(m_file, m_line, m_method);
        item.setEnabled(Manager::instance()->enabled(section));
        if (format)
            item.d->stream << format;
    }
    inline Item debug(const char* format) {
        Item item(m_file, m_line, m_method, DebugLevel);
        item.d->stream << format;
        return item;
    }
    inline Item info(const char* format) {
        Item item(m_file, m_line, m_method);
        item.d->stream << format;
        return item;
    }
    inline Item warning(const char* format) {
        Item item(m_file, m_line, m_method, WarningLevel);
        item.d->stream << format;
        return item;
    }
    inline Item critical(const char* format) {
        Item item(m_file, m_line, m_method, CriticalLevel);
        item.d->stream << format;
        return item;
    }
    inline Item fatal(const char* format) {
        Item item(m_file, m_line, m_method, FatalLevel);
        item.d->stream << format;
        return item;
    }

private:
    const int m_line;
    const char *m_file, *m_method;
};


class SilentItem {
public:
    inline int verbosity() const {
        return FatalLevel;
    }
    inline SilentItem &nospace() { return *this; }
    inline SilentItem &space() { return *this; }
    inline SilentItem &maybespace() { return *this; }
    inline SilentItem &operator<<(char) { return *this; }
    inline SilentItem &operator<<(const char *) {  return *this; }
    inline SilentItem &operator<<(int) {  return *this; }
    inline SilentItem &operator<<(double) { return *this; }
    inline SilentItem &operator<<(const std::string&) { return *this; }
    inline SilentItem &operator<<(bool) { return *this; }
    inline SilentItem &operator<<(short) { return *this; }
    inline SilentItem &operator<<(unsigned short) { return *this; }
    inline SilentItem &operator<<(unsigned int) { return *this; }
    inline SilentItem &operator<<(long) {  return *this; }
    inline SilentItem &operator<<(unsigned long) { return *this; }
    inline SilentItem &operator<<(long long) {  return *this; }
    inline SilentItem &operator<<(unsigned long long) { return *this; }
    inline SilentItem &operator<<(float) {  return *this; }
    inline SilentItem &operator<<(long double) {  return *this; }
    inline SilentItem &operator<<(void*) { return *this; }
};

inline SilentItem MessageLogger::noDebug(int) { return SilentItem(); }

} // namespace Log

#ifdef BTC_LOGCONTEXT
  #define BTC_MESSAGELOG_FILE __FILE__
  #define BTC_MESSAGELOG_LINE __LINE__
  #define BTC_MESSAGELOG_FUNC __PRETTY_FUNCTION__
#else
  #define BTC_MESSAGELOG_FILE nullptr
  #define BTC_MESSAGELOG_LINE 0
  #define BTC_MESSAGELOG_FUNC nullptr
#endif

#define logDebug Log::MessageLogger(BTC_MESSAGELOG_FILE, BTC_MESSAGELOG_LINE, BTC_MESSAGELOG_FUNC).debug
#define logInfo Log::MessageLogger(BTC_MESSAGELOG_FILE, BTC_MESSAGELOG_LINE, BTC_MESSAGELOG_FUNC).info
#define logWarning Log::MessageLogger(BTC_MESSAGELOG_FILE, BTC_MESSAGELOG_LINE, BTC_MESSAGELOG_FUNC).warning
#define logCritical Log::MessageLogger(BTC_MESSAGELOG_FILE, BTC_MESSAGELOG_LINE, BTC_MESSAGELOG_FUNC).critical
#define logFatal Log::MessageLogger(BTC_MESSAGELOG_FILE, BTC_MESSAGELOG_LINE, BTC_MESSAGELOG_FUNC).fatal

#define BTC_NO_DEBUG_MACRO while (false) Log::MessageLogger().noDebug

#if defined(BTC_NO_DEBUG_OUTPUT)
#  undef logDebug
#  define logDebug BTC_NO_DEBUG_MACRO
#endif
#if defined(BTC_NO_INFO_OUTPUT)
#  undef logInfo
#  define logInfo BTC_NO_DEBUG_MACRO
#endif
#if defined(BTC_NO_WARNING_OUTPUT)
#  undef logWarning
#  define logWarning BTC_NO_DEBUG_MACRO
#endif

#endif
