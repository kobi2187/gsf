//******************************************************************************************************
//  Convert.cpp - Gbtc
//
//  Copyright � 2018, Grid Protection Alliance.  All Rights Reserved.
//
//  Licensed to the Grid Protection Alliance (GPA) under one or more contributor license agreements. See
//  the NOTICE file distributed with this work for additional information regarding copyright ownership.
//  The GPA licenses this file to you under the MIT License (MIT), the "License"; you may not use this
//  file except in compliance with the License. You may obtain a copy of the License at:
//
//      http://opensource.org/licenses/MIT
//
//  Unless agreed to in writing, the subject software distributed under the License is distributed on an
//  "AS-IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. Refer to the
//  License for the specific language governing permissions and limitations.
//
//  Code Modification History:
//  ----------------------------------------------------------------------------------------------------
//  04/06/2012 - Stephen C. Wills
//       Generated original version of source code.
//
//******************************************************************************************************

#include "Convert.h"
#include <iomanip>
#include <sstream>
#include <locale>
#include <codecvt>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace std::chrono;
using namespace boost::uuids;
using namespace boost::posix_time;
using namespace GSF;

string PreparseTimestamp(const string& timestamp, time_duration& utcOffset)
{
    // 2018-03-14T19:23:11.665-04:00
    vector<string> dateTimeParts = Split(Replace(timestamp, "T", " "), " ", false);

    // Failed to understand timestamp format, just return input
    if (dateTimeParts.empty() || dateTimeParts.size() > 2)
        return timestamp;

    string updatedTimestamp{};
    string& datePart = dateTimeParts[0];
    string part{};
    vector<string> dateParts = Split(Replace(datePart, "/", "-", false), "-", false);

    if (dateParts.size() != 3)
        return timestamp;

    string year, month, day;

    for (int32_t i = 0; i < 3; i++)
    {
        part = dateParts[i];

        if (part.size() == 1)
            part.insert(0, "0");

        if (part.size() == 4)
            year = part;
        else if (month.empty())
            month = part;
        else
            day = part;
    }

    updatedTimestamp.append(year);
    updatedTimestamp.append("-");
    updatedTimestamp.append(month);
    updatedTimestamp.append("-");
    updatedTimestamp.append(day);

    if (dateTimeParts.size() == 1)
    {
        updatedTimestamp.append(" 00:00:00");
        return updatedTimestamp;
    }

    string& timePart = dateTimeParts[1];

    // Remove any time zone offset and hold on to it for later
    const bool containsMinus = Contains(timePart, "-", false);
    vector<string> timeParts = Split(timePart, containsMinus ? "-" : "+", false);
    string timeZoneOffset{};

    if (timeParts.size() == 2)
    {
        timePart = timeParts[0];

        // Swap timezone sign for conversion to UTC
        timeZoneOffset.append(containsMinus ? "+" : "-");
        timeZoneOffset.append(Replace(timeParts[1], ":", "", false));
    }

    timeParts = Split(timePart, ":", false);

    if (timeParts.size() == 2)
        timeParts.emplace_back("00");

    if (timeParts.size() != 3)
        return timestamp;

    updatedTimestamp.append(" ");

    for (int32_t i = 0; i < 3; i++)
    {
        string fractionalSeconds{};
        part = timeParts[i];

        if (i == 2 && Contains(part, ".", false))
        {
            vector<string> secondParts = Split(part, ".", false);

            if (secondParts.size() == 2)
            {
                part = secondParts[0];
                fractionalSeconds.append(".");
                fractionalSeconds.append(secondParts[1]);
            }
        }

        if (i > 0)
            updatedTimestamp.append(":");

        if (part.size() == 1)
            updatedTimestamp.append("0");

        updatedTimestamp.append(part);

        if (!fractionalSeconds.empty())
            updatedTimestamp.append(fractionalSeconds);
    }

    if (timeZoneOffset.size() == 5)
        utcOffset = time_duration(stoi(timeZoneOffset.substr(0, 3)), stoi(timeZoneOffset.substr(3)), 0);

    return updatedTimestamp;
}

void GSF::ToUnixTime(const int64_t ticks, time_t& unixSOC, uint16_t& milliseconds)
{
    // Unix dates are measured as the number of seconds since 1/1/1970
    unixSOC = (ticks - Ticks::UnixBaseTime) / Ticks::PerSecond;

    if (unixSOC < 0)
        unixSOC = 0;

    milliseconds = static_cast<uint16_t>(ticks / 10000 % 1000);
}

DateTime GSF::FromUnixTime(time_t unixSOC, uint16_t milliseconds)
{
    return from_time_t(unixSOC) + boost::posix_time::milliseconds(milliseconds);
}

DateTime GSF::FromTicks(const int64_t ticks)
{
    time_t unixSOC;
    uint16_t milliseconds;

    ToUnixTime(ticks, unixSOC, milliseconds);
    return FromUnixTime(unixSOC, milliseconds);
}

int64_t GSF::ToTicks(const DateTime& time)
{
    static float64_t baseFraction = pow(10.0, time_duration::num_fractional_digits());

    time_duration timeOfDay = time.time_of_day();

    return
        /* time.date().day_count() * Ticks::PerDay + */
        timeOfDay.total_seconds() * Ticks::PerSecond +
        timeOfDay.fractional_seconds() / baseFraction * Ticks::PerSecond;
}

uint32_t GSF::TicksToString(char* ptr, uint32_t maxsize, string format, int64_t ticks)
{
    time_t fromSeconds;
    uint16_t milliseconds;

    ToUnixTime(ticks, fromSeconds, milliseconds);

    stringstream formatStream;
    uint32_t formatIndex = 0;

    while (formatIndex < format.size())
    {
        char c = format[formatIndex];
        ++formatIndex;

        if (c != '%')
        {
            // Not a format specifier
            formatStream << c;
            continue;
        }

        // Check for %f and %t format specifiers and handle them
        // accordingly. All other specifiers get forwarded to strftime
        c = format[formatIndex];
        ++formatIndex;

        switch (c)
        {
            case 'f':
            {
                stringstream temp;
                temp << setw(3) << setfill('0') << static_cast<int>(milliseconds);
                formatStream << temp.str();
                break;
            }

            case 't':
                formatStream << ticks;
                break;

            default:
                formatStream << '%' << c;
                break;
        }
    }

    struct tm timeinfo{};

#ifdef _WIN32
    gmtime_s(&timeinfo, &fromSeconds);
#else
    gmtime_r(&fromSeconds, &timeinfo);
#endif

    return strftime(ptr, maxsize, formatStream.str().data(), &timeinfo);
}

DateTime GSF::LocalFromUtc(const DateTime& timestamp)
{
    return boost::date_time::c_local_adjustor<DateTime>::utc_to_local(timestamp);
}

std::string GSF::ToString(const Guid& value)
{
    return boost::uuids::to_string(value);
}

std::string GSF::ToString(const DateTime& value, const char* format)
{
    using namespace boost::gregorian;

    stringstream stream;

    time_facet* facet = new time_facet();
    facet->format(format);
    stream.imbue(locale(locale::classic(), facet));
    stream << value;

    return stream.str();
}

std::string GSF::ToString(const TimeSpan& value)
{
    // TODO: Consider improving elapsed time string with hours, minutes, etc.
    const float64_t seconds = value.total_milliseconds() / 1000.0;
    return ToString(seconds) + " seconds";
}

wstring GSF::ToUTF16(const string& value)
{
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.from_bytes(value);
}

string GSF::ToUTF8(const wstring& value)
{
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(value);
}

bool GSF::ParseBoolean(const string& value)
{
    if (value.empty())
        return false;

    const string result = Trim(value);

    if (!result.empty())
    {
        if (IsEqual(result, "true"))
            return true;

        if (IsEqual(result, "false"))
            return false;

        try
        {
            return stoi(result) != 0;
        }
        catch (...)
        {
            const char first = toupper(result[0]);
            return first == 'T' || first == 'Y';
        }
    }

    return false;
}

bool GSF::TryParseDouble(const string& value, float64_t& result)
{
    try
    {
        result = stod(value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string GSF::RegExEncode(const char value)
{
    std::stringstream stream;
    stream << std::hex << static_cast<int>(value);
    return "\\u" + PadLeft(stream.str(), 4, '0');
}

Guid GSF::ParseGuid(const uint8_t* data, bool swapEndianness, bool useGEPEncoding)
{
    Guid id;
    uint8_t swappedBytes[16];
    uint8_t* encodedBytes;

    if (swapEndianness || useGEPEncoding)
    {
        uint8_t copy[8];

        for (uint32_t i = 0; i < 16; i++)
        {
            swappedBytes[i] = useGEPEncoding ? data[15 - i] : data[i];

            if (i < 8)
                copy[i] = swappedBytes[i];
        }

        if (swapEndianness)
        {
            // Convert Microsoft encoding to RFC
            swappedBytes[3] = copy[0];
            swappedBytes[2] = copy[1];
            swappedBytes[1] = copy[2];
            swappedBytes[0] = copy[3];

            swappedBytes[4] = copy[5];
            swappedBytes[5] = copy[4];

            swappedBytes[6] = copy[7];
            swappedBytes[7] = copy[6];
        }

        encodedBytes = swappedBytes;
    }
    else
    {
        encodedBytes = const_cast<uint8_t*>(data);
    }

    for (Guid::iterator iter = id.begin(); iter != id.end(); ++iter, ++encodedBytes)
        *iter = static_cast<Guid::value_type>(*encodedBytes);

    return id;
}

Guid GSF::ParseGuid(const char* data)
{
    const string_generator generator;
    return generator(data);
}

void GSF::SwapGuidEndianness(Guid& value, bool useGEPEncoding)
{
    // Convert RFC encoding to Microsoft or vice versa
    uint8_t* data = value.data;
    uint8_t copy[8];

    for (uint32_t i = 0; i < 8; i++)
        copy[i] = data[i];

    // The following uint32 and two uint16 values are little-endian encoded in Microsoft implementations,
    // boost follows RFC encoding rules and encodes the bytes as big-endian. For proper Guid interpretation
    // by .NET applications the following bytes must be swapped before wire-serialization:
    data[3] = copy[0];
    data[2] = copy[1];
    data[1] = copy[2];
    data[0] = copy[3];

    data[4] = copy[5];
    data[5] = copy[4];

    data[6] = copy[7];
    data[7] = copy[6];

    // GEP encodes Guid bytes in reverse order
    if (useGEPEncoding)
    {
        uint8_t swappedBytes[16];

        for (uint32_t i = 0; i < 16; i++)
            swappedBytes[i] = data[15 - i];

        for (uint32_t i = 0; i < 16; i++)
            data[i] = swappedBytes[i];
    }
}

const char* GSF::Coalesce(const char* data, const char* nonEmptyValue)
{
    if (data == nullptr)
        return nonEmptyValue;

    if (data[0] == '\0')
        return nonEmptyValue;

    return data;
}

// Parse a timestamp string, e.g.: 2018-03-14T19:23:11.665-04:00
bool GSF::TryParseTimestamp(const char* time, DateTime& timestamp, bool parseAsUTC)
{
    static const locale formats[] = {
        locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S%F")),
        locale(locale::classic(), new time_input_facet("%Y%m%dT%H%M%S%F"))
    };

    static const int32_t formatsCount = sizeof(formats) / sizeof(formats[0]);

    for (int32_t i = 0; i < formatsCount; i++)
    {
        time_duration utcOffset{};
        istringstream stream(PreparseTimestamp(time, utcOffset));

        stream.imbue(formats[i]);
        stream >> timestamp;

        if (bool(stream))
        {
            if (parseAsUTC)
                timestamp += utcOffset;

            return true;
        }
    }

    return false;
}

DateTime GSF::ParseTimestamp(const char* time, bool parseAsUTC)
{
    DateTime timestamp;

    if (TryParseTimestamp(time, timestamp, parseAsUTC))
        return timestamp;

    throw runtime_error("Failed to parse timestamp \"" + string(time) + "\"");
}

StringMap<string> GSF::ParseKeyValuePairs(const string& value, const char parameterDelimiter, const char keyValueDelimiter, const char startValueDelimiter, const char endValueDelimiter)
{
    if (parameterDelimiter == keyValueDelimiter ||
        parameterDelimiter == startValueDelimiter ||
        parameterDelimiter == endValueDelimiter ||
        keyValueDelimiter == startValueDelimiter ||
        keyValueDelimiter == endValueDelimiter ||
        startValueDelimiter == endValueDelimiter)
        throw invalid_argument("All delimiters must be unique");

    const string& escapedParameterDelimiter = RegExEncode(parameterDelimiter);
    const string& escapedKeyValueDelimiter = RegExEncode(keyValueDelimiter);
    const string& escapedStartValueDelimiter = RegExEncode(startValueDelimiter);
    const string& escapedEndValueDelimiter = RegExEncode(endValueDelimiter);
    const string& escapedBackslashDelimiter = RegExEncode('\\');
    const string& parameterDelimiterStr = string(1, parameterDelimiter);
    const string& keyValueDelimiterStr = string(1, keyValueDelimiter);
    const string& startValueDelimiterStr = string(1, startValueDelimiter);
    const string& endValueDelimiterStr = string(1, endValueDelimiter);
    const string& backslashDelimiterStr = "\\";

    StringMap<string> keyValuePairs;
    vector<string> escapedValue;
    bool valueEscaped = false;
    uint32_t delimiterDepth = 0;

    // Escape any parameter or key/value delimiters within tagged value sequences
    //      For example, the following string:
    //          "normalKVP=-1; nestedKVP={p1=true; p2=false}")
    //      would be encoded as:
    //          "normalKVP=-1; nestedKVP=p1\\u003dtrue\\u003b p2\\u003dfalse")
    for (uint32_t i = 0; i < value.size(); i++)
    {
        const char character = value[i];

        if (character == startValueDelimiter)
        {
            if (!valueEscaped)
            {
                valueEscaped = true;
                continue;   // Don't add tag start delimiter to final value
            }

            // Handle nested delimiters
            delimiterDepth++;
        }

        if (character == endValueDelimiter)
        {
            if (valueEscaped)
            {
                if (delimiterDepth > 0)
                {
                    // Handle nested delimiters
                    delimiterDepth--;
                }
                else
                {
                    valueEscaped = false;
                    continue;   // Don't add tag stop delimiter to final value
                }
            }
            else
            {
                throw runtime_error("Failed to parse key/value pairs: invalid delimiter mismatch. Encountered end value delimiter '" + endValueDelimiterStr + "' before start value delimiter '" + startValueDelimiterStr + "'.");  // NOLINT
            }
        }

        if (valueEscaped)
        {
            // Escape any delimiter characters inside nested key/value pair
            if (character == parameterDelimiter)
                escapedValue.push_back(escapedParameterDelimiter);
            else if (character == keyValueDelimiter)
                escapedValue.push_back(escapedKeyValueDelimiter);
            else if (character == startValueDelimiter)
                escapedValue.push_back(escapedStartValueDelimiter);
            else if (character == endValueDelimiter)
                escapedValue.push_back(escapedEndValueDelimiter);
            else if (character == '\\')
                escapedValue.push_back(escapedBackslashDelimiter);
            else
                escapedValue.emplace_back(1, character);
        }
        else
        {
            if (character == '\\')
                escapedValue.push_back(escapedBackslashDelimiter);
            else
                escapedValue.emplace_back(1, character);
        }
    }

    if (delimiterDepth != 0 || valueEscaped)
    {
        // If value is still escaped, tagged expression was not terminated
        if (valueEscaped)
            delimiterDepth = 1;

        const bool moreStartDelimiters = delimiterDepth > 0;

        throw runtime_error(
            "Failed to parse key/value pairs: invalid delimiter mismatch. Encountered more " +
            (moreStartDelimiters ? "start value delimiters '" + startValueDelimiterStr + "'" : "end value delimiters '" + endValueDelimiterStr + "'") + " than " +
            (moreStartDelimiters ? "end value delimiters '" + endValueDelimiterStr + "'" : "start value delimiters '" + startValueDelimiterStr + "'") + ".");
    }

    // Parse key/value pairs from escaped value
    vector<string> pairs = Split(boost::algorithm::join(escapedValue, ""), parameterDelimiterStr, false);

    for (uint32_t i = 0; i < pairs.size(); i++)
    {
        // Separate key from value
        vector<string> elements = Split(pairs[i], keyValueDelimiterStr, false);

        if (elements.size() == 2)
        {
            // Get key
            const string key = Trim(elements[0]);

            // Get unescaped value
            const string unescapedValue = Replace(Replace(Replace(Replace(Replace(Trim(elements[1]),
                escapedParameterDelimiter, parameterDelimiterStr, false),
                escapedKeyValueDelimiter, keyValueDelimiterStr, false),
                escapedStartValueDelimiter, startValueDelimiterStr, false),
                escapedEndValueDelimiter, endValueDelimiterStr, false),                    
                escapedBackslashDelimiter, backslashDelimiterStr, false);

            // Add or replace key elements with unescaped value
            keyValuePairs[key] = unescapedValue;
        }
    }

    return keyValuePairs;
}