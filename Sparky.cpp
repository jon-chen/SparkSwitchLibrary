#include <math.h>
#include "application.h"
#include "SparkDebug.h"
#include "SparkTime.h"
#include "Sparky.h"

void Sparky::DoTheRainbow()
{
    RGB.control(true);

    // Make it rainbow!
    // http://krazydad.com/tutorials/makecolors.php
    double frequency = 2 * 3.14 / 6;
    int r, g, b;

    for (int i = 0; i < 92; ++i)
    {
        r = sin(frequency * i + 0) * 127 + 128;
        g = sin(frequency * i + 2) * 127 + 128;
        b = sin(frequency * i + 4) * 127 + 128;
        RGB.color(r,g,b);
        delay(50);
    }

    RGB.control(false);
}

time_t Sparky::ParseTimeFromToday(SparkTime* rtc, int hour, int minute)
{
    time_t t = Time.now();
    int32_t offset = rtc->getZoneOffset(rtc->now());
    struct tm* timeinfo;

    // Alter the timestamp with the given parameters.
    timeinfo = localtime(&t);
    timeinfo->tm_hour = hour - offset;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = 0;

    // If it's 3 AM or earlier, consider it part of yesterday.
    if (rtc->hour(rtc->now()) < 3 && hour > 3)
    {
        timeinfo->tm_mday -= 1;
    }

    // Return unix timestamp of sunset.
    return mktime(timeinfo);
}

time_t Sparky::ParseTimeFromString(SparkTime* rtc, const char* data)
{
    int hour, minute;
    ParseTime(data, &hour, &minute);
    return ParseTimeFromToday(rtc, hour, minute);
}

void Sparky::ParseTime(const char* data, int* hour, int* minute)
{
    sscanf(data, "%d:%d", hour, minute);
}

void Sparky::ParseTimestamp(time_t timestamp, int* hour, int* minute)
{
    *hour = Time.hour(timestamp);
    *minute = Time.minute(timestamp);
}

String Sparky::ISODateString(SparkTime* rtc, time_t timestamp)
{
    int32_t offset = rtc->getZoneOffset(rtc->now());
    struct tm* timeinfo;

    // Alter the timestamp with the given parameters.
    timeinfo = localtime(&timestamp);
    String ISOString;
    ISOString += Time.year(timestamp);
    ISOString += "-";
    ISOString += Time.month(timestamp);
    ISOString += "-";
    ISOString += Time.day(timestamp);
    ISOString += "T";
    ISOString += Time.hour(timestamp);
    ISOString += ":";
    ISOString += Time.minute(timestamp);
    ISOString += ":";
    ISOString += Time.second(timestamp);

    // Guard against timezone problems
    if (offset>-24 && offset<24) {
      if (offset < 0) {
        ISOString = ISOString + "-" + _digits[-offset] + "00";
      } else {
        ISOString = ISOString + "+" + _digits[offset] + "00";
      }
    }
    return ISOString;
}
