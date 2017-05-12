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
#include "LogChannels_p.h"
#include "util.h"

#include <iostream>
#include <strings.h>

ConsoleLogChannel::ConsoleLogChannel()
    : Channel(true),
     m_printLineNumber(false),
     m_printMethodName(false),
     m_printFilename(false)
{
}

void ConsoleLogChannel::pushLog(int64_t, const std::string &timestamp, const std::string &line, const char * /*filename*/, int /*lineNumber*/, const char *methodName, int logLevel)
{
    std::ostream &out = logLevel >= Log::WarningLevel ? std::clog : std::cout;
    if (m_printMethodName && methodName) {
        const char *start = strchr(methodName, ' ');
        const char *end = strchr(methodName, '(');
        if (start && end) {
            ++start;
            ++end;
            std::string copy(start, end - start);
            out << copy << ") ";
        }
    }
    if (m_formatTimestamp)
        out << timestamp << ' ';
    out << line;
    if (line.empty() || line.back() != '\n')
        out << std::endl;
    out.flush();
}

bool ConsoleLogChannel::printLineNumber() const
{
    return m_printLineNumber;
}

void ConsoleLogChannel::setPrintLineNumber(bool printLineNumber)
{
    m_printLineNumber = printLineNumber;
}

bool ConsoleLogChannel::printMethodName() const
{
    return m_printMethodName;
}

void ConsoleLogChannel::setPrintMethodName(bool printMethodName)
{
    m_printMethodName = printMethodName;
}

bool ConsoleLogChannel::printFilename() const
{
    return m_printFilename;
}

void ConsoleLogChannel::setPrintFilename(bool printFilename)
{
    m_printFilename = printFilename;
}

void ConsoleLogChannel::setFormatTimestamp(bool printTimestamp)
{
    m_formatTimestamp = printTimestamp;
}

FileLogChannel::FileLogChannel()
    : Channel(true),
      m_fileout(0)
{
    reopenLogFiles();
}

FileLogChannel::~FileLogChannel()
{
    if (m_fileout)
        fclose(m_fileout);
}

static void FileWriteStr(const std::string &str, FILE *fp) {
    fwrite(str.data(), 1, str.size(), fp);
}

void FileLogChannel::pushLog(int64_t, const std::string &timestamp, const std::string &line, const char*, int, const char *, int)
{
    if (m_fileout) {
        FileWriteStr(timestamp, m_fileout);
        FileWriteStr(" ", m_fileout);
        FileWriteStr(line, m_fileout);
        if (line.empty() || line.back() != '\n')
            FileWriteStr("\n", m_fileout);
    }
}

void FileLogChannel::reopenLogFiles()
{
    if (m_fileout)
        fclose(m_fileout);

    boost::filesystem::path pathDebug = GetDataDir() / "debug.log";
    m_fileout = fopen(pathDebug.string().c_str(), "a");
    if (m_fileout)
        setbuf(m_fileout, NULL); // unbuffered
}
