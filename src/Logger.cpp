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
#include <list>
#include <map>
#include <mutex>

#include <boost/thread.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

bool fDebug = false;

class Log::ManagerPrivate {
public:
    std::list<Channel*> channels;
    std::mutex lock;
    std::string lastTime;
    std::map<short, std::string> sectionNames;
    std::map<std::string, short> categoryMapping;
    std::set<short> enabledSections;
};

Log::Manager::Manager()
    : d(new ManagerPrivate())
{
    if (GetBoolArg("-printtoconsole", false))
        d->channels.push_back(new ConsoleLogChannel());
    d->channels.push_back(new FileLogChannel());

    d->sectionNames.emplace(Log::Validation, "Validation");
    d->sectionNames.emplace(Log::Bench, "Bench");
    d->sectionNames.emplace(Log::Prune, "Prune");
    d->sectionNames.emplace(Log::Net, "Net");
    d->sectionNames.emplace(Log::Addrman, "Addrman");
    d->sectionNames.emplace(Log::Proxy, "Proxy");
    d->sectionNames.emplace(Log::NWM, "NWM");
    d->sectionNames.emplace(Log::Tor, "Tor");
    d->sectionNames.emplace(Log::AdminServer, "AdminServer");
    d->sectionNames.emplace(Log::RPC, "RPC");
    d->sectionNames.emplace(Log::HTTP, "HTTP");
    d->sectionNames.emplace(Log::ZMQ, "ZMQ");
    d->sectionNames.emplace(Log::DB, "DB");
    d->sectionNames.emplace(Log::Coindb, "Coindb");
    d->sectionNames.emplace(Log::Wallet, "Wallet");
    d->sectionNames.emplace(Log::SelectCoins, "SelectCoins");
    d->sectionNames.emplace(Log::Internals, "Internals");
    d->sectionNames.emplace(Log::Mempool, "Mempool");
    d->sectionNames.emplace(Log::Random, "Random");

    // this is purely to be backwards compatible with the old style where the section was a string.
    d->categoryMapping.emplace("bench", Log::Bench);
    d->categoryMapping.emplace("addrman", Log::Addrman);
    d->categoryMapping.emplace("blk", Log::ExpeditedBlocks);
    d->categoryMapping.emplace("coindb", Log::Coindb);
    d->categoryMapping.emplace("db", Log::DB);
    d->categoryMapping.emplace("estimatefee", 502);
    d->categoryMapping.emplace("http", Log::HTTP);
    d->categoryMapping.emplace("libevent", Log::LibEvent);
    d->categoryMapping.emplace("mempool", Log::Mempool);
    d->categoryMapping.emplace("mempoolrej", Log::MempoolRej);
    d->categoryMapping.emplace("net", Log::Net);
    d->categoryMapping.emplace("partitioncheck", Global);
    d->categoryMapping.emplace("proxy", Log::Proxy);
    d->categoryMapping.emplace("prune", Log::Prune);
    d->categoryMapping.emplace("rand", Log::Random);
    d->categoryMapping.emplace("rpc", Log::RPC);
    d->categoryMapping.emplace("selectcoins", Log::SelectCoins);
    d->categoryMapping.emplace("thin", Log::ThinBlocks);
    d->categoryMapping.emplace("tor", Log::Tor);
    d->categoryMapping.emplace("zmq", Log::ZMQ);
    d->categoryMapping.emplace("reindex", 604);

    parseConfig();
}

Log::Manager::~Manager()
{
    clearChannels();
    delete d;
}

bool Log::Manager::isEnabled(short section) const
{
    if (d->enabledSections.count(section))
        return true;
    const short region = section - (section % 100);
    return d->enabledSections.count(region);
}

short Log::Manager::section(const char *category)
{
    if (!category)
        return Log::Global;
    auto iter = d->categoryMapping.find(category);
    assert (iter != d->categoryMapping.end());
    return iter->second;
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
    std::lock_guard<std::mutex> lock(d->lock);
    for (auto channel : d->channels) {
        if (logTimestamps && newTime.empty() && channel->formatTimestamp()) {
            newTime = DateTimeStrFormat("%Y-%m-%d %H:%M:%S", timeMillis/1000);
            if (newTime == d->lastTime) {
                std::ostringstream millis;
                millis.width(3);
                millis << timeMillis % 1000;
                newTime = "               ." + millis.str();
            } else {
                d->lastTime = newTime;
            }
        }
        try {
            channel->pushLog(timeMillis, newTime, item->d->stream.str(), item->d->filename, item->d->lineNum, item->d->methodName, item->d->verbosity);
        } catch (...) {}
    }
}

void Log::Manager::reopenLogFiles()
{
    std::lock_guard<std::mutex> lock(d->lock);
    for (auto channel : d->channels) {
        channel->reopenLogFiles();
    }
}

void Log::Manager::loadDefaultTestSetup()
{
    clearChannels();
    auto channel = new ConsoleLogChannel();
    channel->setPrintMethodName(true);
    channel->setFormatTimestamp(false);
    d->channels.push_back(channel);
}

void Log::Manager::parseConfig()
{
    d->enabledSections.clear();

    // Parse the old fashioned way of enabling/disabling log sections.
    bool all = false, none = false;
    for (auto cat : mapMultiArgs["-debug"]) {
        if (cat.empty() || cat == "1") { // turns all on.
            all = true;
            break;
        }
        if (cat == "0") { // all off
            none = true;
            break;
        }
        auto iter = d->categoryMapping.find(cat);
        if (iter == d->categoryMapping.end())
            continue; // silently ignore
        d->enabledSections.insert(iter->second);
    }
    if (!none)
        d->enabledSections.insert(Log::Global);
    if (all) {
        for (short i = 0; i <= 2000; i+=100)
            d->enabledSections.insert(i);
    }

    // TODO find a better way to load/save the sections.
}

void Log::Manager::clearChannels()
{
    for (auto channel : d->channels) {
        delete channel;
    }
    d->channels.clear();
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

Log::Item::Item(const char *filename, int line, const char *function, short section, int verbosity)
    : d(new State(filename, line, function, section))
{
    d->space = true;
    d->on = Manager::instance()->isEnabled(section);
    d->verbosity = verbosity;
    d->ref = 1;
}

Log::Item::Item(int verbosity)
    : d(new State(nullptr, 0, nullptr, Log::Global))
{
    d->space = true;
    d->on = Manager::instance()->isEnabled(Log::Global);
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

short Log::Item::section() const
{
    return d->section;
}

Log::Item operator<<(Log::Item item, const std::exception &ex) {
    if (item.isEnabled()) item << ex.what();
    // TODO configure if we want a stacktrace?
    return item.space();
}
