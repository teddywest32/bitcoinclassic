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
#include "Logger.h"
#include "LogChannels_p.h"
#include "util.h"

#include <string>
#include <set>

#include <boost/thread.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

bool fDebug = false;

Log::Manager::Manager()
{
    if (GetBoolArg("-printtoconsole", false))
        m_channels.push_back(new ConsoleLogChannel());
    m_channels.push_back(new FileLogChannel());
}

Log::Manager::~Manager()
{
    clearChannels();
}

bool Log::Manager::enabled(const char *region) const
{
    if (region != NULL) {
        if (!fDebug)
            return false;

        // Give each thread quick access to -debug settings.
        // This helps prevent issues debugging global destructors,
        // where mapMultiArgs might be deleted before another
        // global destructor calls LogPrint()
        static boost::thread_specific_ptr<std::set<std::string> > ptrCategory;
        if (ptrCategory.get() == nullptr) {
            const std::vector<std::string>& categories = mapMultiArgs["-debug"];
            ptrCategory.reset(new std::set<std::string>(categories.begin(), categories.end()));
            // thread_specific_ptr automatically deletes the set when the thread ends.
        }
        const std::set<std::string>& setCategories = *ptrCategory.get();

        // if not debugging everything and not debugging specific region, LogPrint does nothing.
        if (setCategories.count(std::string("")) == 0 &&
            setCategories.count(std::string("1")) == 0 &&
            setCategories.count(std::string(region)) == 0)
            return false;
    }
    return true;
}

bool Log::Manager::enabled(int category) const
{
    // TODO
    return true;
}

Log::Manager *Log::Manager::instance()
{
    static Log::Manager s_instance;
    return &s_instance;
}

void Log::Manager::log(Log::Item *item)
{
    bool logTimestamps = GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
    const int64_t timeMillis = GetTimeMillis();
    std::string newTime;
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto channel : m_channels) {
        if (logTimestamps && newTime.empty() && channel->formatTimestamp()) {
            newTime = DateTimeStrFormat("%Y-%m-%d %H:%M:%S", timeMillis/1000);
            if (newTime == m_lastTime) {
                std::ostringstream millis;
                millis.width(3);
                millis << timeMillis % 1000;
                newTime = "               ." + millis.str();
            } else {
                m_lastTime = newTime;
            }
        }
        try {
            channel->pushLog(timeMillis, newTime, item->d->stream.str(), item->d->filename, item->d->lineNum, item->d->methodName, item->d->verbosity);
        } catch (...) {}
    }
}

void Log::Manager::reopenLogFiles()
{
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto channel : m_channels) {
        channel->reopenLogFiles();
    }
}

void Log::Manager::loadDefaultTestSetup()
{
    clearChannels();
    auto channel = new ConsoleLogChannel();
    channel->setPrintMethodName(true);
    channel->setFormatTimestamp(false);
    m_channels.push_back(channel);
}

void Log::Manager::clearChannels()
{
    for (auto channel : m_channels) {
        delete channel;
    }
    m_channels.clear();
}

/////////////////////////////////////////////////

Log::MessageLogger::MessageLogger()
    : m_line(0), m_file(0), m_method(0)
{
}

Log::MessageLogger::MessageLogger(const char *filename, int line, const char *function)
    : m_line(line), m_file(filename), m_method(function)
{
}


/////////////////////////////////////////////////

Log::Item::Item(const char *filename, int line, const char *function, int verbosity)
    : d(new State(filename, line, function))
{
    d->space = true;
    d->on = true;
    d->verbosity = verbosity;
    d->ref = 1;
}

Log::Item::Item(int verbosity)
    : d(new State(nullptr, 0, nullptr))
{
    d->space = true;
    d->on = true;
    d->verbosity = verbosity;
    d->ref = 1;
}

Log::Item::Item(const Log::Item &other)
    : d(other.d)
{
    d->ref++;
}

Log::Item::~Item()
{
    if (!--d->ref) {
        if (d->on)
            Log::Manager::instance()->log(this);
        delete d;
    }
}

Log::Item Log::MessageLogger::debug(int section)
{
    Log::Item item(m_file, m_line, m_method);
    item.setVerbosity(Log::DebugLevel);
    item.setEnabled(Log::Manager::instance()->enabled(section));
    return item;
}

Log::Item Log::MessageLogger::warning(int section)
{
    Log::Item item(m_file, m_line, m_method);
    item.setVerbosity(Log::WarningLevel);
    item.setEnabled(Log::Manager::instance()->enabled(section));
    return item;
}

Log::Item Log::MessageLogger::info(int section)
{
    Log::Item item(m_file, m_line, m_method);
    item.setVerbosity(Log::InfoLevel);
    item.setEnabled(Log::Manager::instance()->enabled(section));
    return item;
}

Log::Item Log::MessageLogger::critical(int section)
{
    Log::Item item(m_file, m_line, m_method);
    item.setVerbosity(Log::CriticalLevel);
    item.setEnabled(Log::Manager::instance()->enabled(section));
    return item;
}

Log::Item Log::MessageLogger::fatal(int section)
{
    Log::Item item(m_file, m_line, m_method);
    item.setVerbosity(Log::FatalLevel);
    item.setEnabled(Log::Manager::instance()->enabled(section));
    return item;
}
