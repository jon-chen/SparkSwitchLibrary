#ifndef SPARKY_H_
#define SPARKY_H_

#define StringVariableMaxLength 622

#include "application.h"
#include "SparkTime.h"

class Sparky
{
    public:
        // ================================
        // LED helper methods
        // ================================

        // Makes the Spark LED emit a pretty rainbow pattern.
        static void DoTheRainbow();

        // ================================
        // Date/Time helper methods
        // ================================

        // Parse the time into a unix timestamp.
        static time_t ParseTimeFromToday(SparkTime* rtc, int hour, int minute);

        // Parse the time from a string to a unix timestamp.
        static time_t ParseTimeFromString(SparkTime* rtc, const char* data);

        // Parses a time out of the string in the format h:mm
        static void ParseTime(const char* data, int* hour, int* minute);

        // Parse a unix timestamp and retrieve the hour and minute.
        static void ParseTimestamp(time_t timestamp, int* hour, int* minute);

        static String ISODateString(SparkTime* rtc, time_t timestamp);
};

#endif // SPARKY_H_
