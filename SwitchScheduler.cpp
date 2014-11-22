#include <math.h>
#include "SparkDebug.h"
#include "Sparky.h"
#include "SwitchScheduler.h"
#include "JsonParser.h"
#include "arraylist.h"
#include "Uri.h"
#include "rest_client.h"

SwitchSchedulerTask::SwitchSchedulerTask(String start, String end, void (*pCallback)(int))
{
    startTime = start;
    endTime = end;
    callback = pCallback;
}

SwitchScheduler::SwitchScheduler(SwitchSchedulerConfiguration* config, SparkTime* rtc)
{
    configuration = new SwitchSchedulerConfiguration();
    this->rtc = rtc;

    // time gets automagically sync'd on start up
    lastTimeSync = millis();

    tasks = new SwitchSchedulerTask*[10];
    homeMobileIds = new arraylist<const char*>();

    initialize(config);
}

void SwitchScheduler::initialize(SwitchSchedulerConfiguration* config)
{
    setAstronomyApiUrl(config->astronomyApiUrl);
    setAstronomyApiCheckTime(config->astronomyApiCheckTime);
    setIsEnabled(config->isEnabled);
    setHomeOnlyModeEnabled(config->homeOnlyModeEnabled);
}

void SwitchScheduler::setAstronomyApiUrl(String apiUrl)
{
    configuration->astronomyApiUrl = apiUrl;
}

void SwitchScheduler::setAstronomyApiCheckTime(String checkTime)
{
    configuration->astronomyApiCheckTime = checkTime;
}

void SwitchScheduler::setIsEnabled(bool enabled)
{
    configuration->isEnabled = enabled;
}

void SwitchScheduler::setHomeOnlyModeEnabled(bool enabled)
{
    configuration->homeOnlyModeEnabled = enabled;
    checkToggledState();
}

void SwitchScheduler::checkToggledState()
{
    if (tasksLength > 0)
    {
        SwitchSchedulerEvent::SwitchSchedulerEventEnum event =
            (isSchedulerEnabled() && shouldBeToggled()) ?
                SwitchSchedulerEvent::StartEvent :
                SwitchSchedulerEvent::EndEvent;

        tasks[0]->callback(static_cast<int>(event));
    }
}

bool SwitchScheduler::isUsingAstronomyData()
{
    return _isUsingAstronomyData;
}

bool SwitchScheduler::isDst()
{
    return rtc->isUSDST(rtc->now());
}

time_t SwitchScheduler::getSunriseTime()
{
    return sunriseTime;
}

time_t SwitchScheduler::getSunsetTime()
{
    return sunsetTime;
}

int SwitchScheduler::getCurrentHomeCount()
{
    return homeMobileIds->length();
}

void SwitchScheduler::setHomeStatus(const char* mobileId)
{
    if (homeMobileIds->indexOf(mobileId) == -1)
    {
        // TODO: Save LastUpdated time as well
        homeMobileIds->add(mobileId);
    }

    checkToggledState();
}

void SwitchScheduler::setAwayStatus(const char* mobileId)
{
    int index = homeMobileIds->indexOf(mobileId);
    if (index != -1)
    {
        homeMobileIds->remove(index);
    }

    checkToggledState();
}

void SwitchScheduler::resetHomeStatus()
{
    homeMobileIds->empty();
    checkToggledState();
}

unsigned long SwitchScheduler::getLastTimeSync()
{
    return lastTimeSync;
}

SwitchSchedulerConfiguration* SwitchScheduler::getConfiguration()
{
    return configuration;
}

SwitchSchedulerTask** SwitchScheduler::getTasks()
{
    return tasks;
}

int SwitchScheduler::getTasksLength()
{
    return tasksLength;
}

bool SwitchScheduler::shouldBeToggled()
{
    uint32_t now = rtc->nowEpoch();

    for (int i = 0; i < tasksLength; i++)
    {
        time_t toggleOn = getTime(tasks[i]->startTime);
        time_t toggleOff = getTime(tasks[i]->endTime);

        if (now > toggleOn && now < toggleOff)
        {
            return true;
        }
    }

    return false;
}

void SwitchScheduler::addSchedulerTask(SwitchSchedulerTask* task)
{
    DEBUG_PRINT(task->startTime);

    // check to see if we need to retrieve astronomy data
    if (task->startTime == "sunrise" ||
        task->startTime == "sunset" ||
        task->endTime == "sunrise" ||
        task->endTime == "sunset")
    {
        _isUsingAstronomyData = true;
    }

    tasks[tasksLength++] = task;
}

void SwitchScheduler::tock()
{
    if (configuration == NULL)
    {
        DEBUG_PRINT("Configuration not set.");
        return;
    }

    if (lastLoopCheck == 0)
    {
        uint32_t tNow = rtc->now();
        while (rtc->second(tNow) != 0)
        {
            DEBUG_PRINT("Second: ");
            DEBUG_PRINT(rtc->second(tNow));
            DEBUG_PRINT("\n");
            DEBUG_PRINT(rtc->ISODateString(rtc->now()) + "\n");
            SPARK_WLAN_Loop();
            delay(1000);
            tNow = rtc->now();
        }
    }

    unsigned long now = millis();

    if (now - lastLoopCheck >= checkLoopInterval ||
        lastLoopCheck == 0)
    {
        DEBUG_PRINT("Tock...");
        DEBUG_PRINT(rtc->ISODateString(rtc->now()) + "\n");

        // Sync the time daily.
        syncTime();

        // Try to get the astronomy data if it's time.
        retrieveAstronomyData();

        if (isSchedulerEnabled())
        {
            // Turn on/off the outlet switch if it's time.
            checkSchedulerTasks();
        }

        lastLoopCheck = now;
    }
}

bool SwitchScheduler::isSchedulerEnabled()
{
    return
        // If the scheduler is enabled
        configuration->isEnabled &&
        // if "Home Only Mode" is disabled or enabled and people are home.
        (!configuration->homeOnlyModeEnabled ||
        (configuration->homeOnlyModeEnabled && getCurrentHomeCount() > 0));
}

void SwitchScheduler::checkSchedulerTasks()
{
    DEBUG_PRINT("Checking scheduled tasks... ");
    DEBUG_PRINT(rtc->ISODateString(rtc->now()) + "\n");

    for (int i = 0; i < tasksLength; i++)
    {
        checkSchedulerTask(tasks[i], tasks[i]->startTime, SwitchSchedulerEvent::StartEvent);
        checkSchedulerTask(tasks[i], tasks[i]->endTime, SwitchSchedulerEvent::EndEvent);
    }
}

void SwitchScheduler::checkSchedulerTask(SwitchSchedulerTask* task,
    String timeString, SwitchSchedulerEvent::SwitchSchedulerEventEnum event)
{
    int checkHour, checkMinute;
    uint32_t now = rtc->now();
    time_t checkTime = getTime(timeString);
    Sparky::ParseTimestamp(checkTime, &checkHour, &checkMinute);

    if (rtc->hour(now) == checkHour && rtc->minute(now) == checkMinute)
    {
        task->callback(static_cast<int>(event));
    }
}

void SwitchScheduler::syncTime()
{
    unsigned long now = millis();

    // just sync the time once a day
    if (now - lastTimeSync > dayInMilliseconds)
    {
        DEBUG_PRINT("Syncing time... ");
        DEBUG_PRINT(rtc->ISODateString(rtc->now()) + "\n");

        Spark.syncTime();
        lastTimeSync = now;
    }
}

void SwitchScheduler::retrieveAstronomyData()
{
    // Only retrieve sunset data if we actually need it.
    if (_isUsingAstronomyData)
    {
        int checkHour, checkMinute;
        uint32_t now = rtc->now();
        Sparky::ParseTime((configuration->astronomyApiCheckTime).c_str(),
            &checkHour, &checkMinute);

        // If the check time has arrived and the last check time wasn't this
        // minute, or if we've never retrieved the sunset data yet.
        if ((rtc->hour(now) == checkHour && rtc->minute(now) == checkMinute) ||
            (sunriseTime == 0 || sunsetTime == 0))
        {
            DEBUG_PRINT("Retrieving sunset data... ");
            DEBUG_PRINT(rtc->ISODateString(rtc->now()) + "\n");

            if (parseAndSetAstronomyData())
            {
                DEBUG_PRINT("Sunset data retrieved! ");
                DEBUG_PRINT(rtc->ISODateString(rtc->now()) + "\n");
            }
            else
            {
                DEBUG_PRINT("Failed to retrieve sunset data, retrying in a few seconds...");
                delay(5000);
                retrieveAstronomyData();
            }
        }
    }
}

bool SwitchScheduler::parseAndSetAstronomyData()
{
    const char* response = getAstronomyDataResponse();

    // Parse response if we have one.
    if (strlen(response) != 0)
    {
        ArduinoJson::Parser::JsonParser<64> parser;
        ArduinoJson::Parser::JsonObject root = parser.parse(const_cast<char*>(response));

        if(root.success())
        {
            int hour, minute;

            hour = atoi(root["moon_phase"]["sunset"]["hour"]);
            minute = atoi(root["moon_phase"]["sunset"]["minute"]);
            sunsetTime = Sparky::ParseTimeFromToday(rtc, hour, minute);

            DEBUG_PRINT("Sunset time: " + Time.timeStr(sunsetTime));

            hour = atoi(root["moon_phase"]["sunrise"]["hour"]);
            minute = atoi(root["moon_phase"]["sunrise"]["minute"]);
            sunriseTime = Sparky::ParseTimeFromToday(rtc, hour, minute);

            DEBUG_PRINT("Sunrise time: " + Time.timeStr(sunriseTime));

            return true;
        }
    }

    return false;
}

const char* SwitchScheduler::getAstronomyDataResponse()
{
    Uri apiUri = Uri::Parse(configuration->astronomyApiUrl);

    RestClient client = RestClient(apiUri.Host.c_str());
    String response = "";

    int statusCode = client.get(apiUri.Path.c_str(), &response);

    if (statusCode == 200)
    {
        DEBUG_PRINT("Successfully retrieved JSON response\n");
        return response.c_str();
    }
    else
    {
        DEBUG_PRINT("Failed to retrieve sunset data with status: ");
        DEBUG_PRINT(statusCode);
        DEBUG_PRINT("\n");
        return "";
    }
}

time_t SwitchScheduler::getTime(String timeString)
{
    if (timeString == "sunrise")
    {
        return sunriseTime;
    }
    else if (timeString == "sunset")
    {
        return sunsetTime;
    }

    return Sparky::ParseTimeFromString(
        rtc,
        timeString.c_str());
}
