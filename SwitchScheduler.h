#ifndef SWITCH_SCHEDULER_H_
#define SWITCH_SCHEDULER_H_

#include "application.h"
#include "arraylist.h"
#include "SparkTime.h"

struct SwitchSchedulerConfiguration
{
    String astronomyApiUrl;
    String astronomyApiCheckTime;
    bool isEnabled;
    bool homeOnlyModeEnabled;
};

struct SwitchSchedulerEvent
{
    enum SwitchSchedulerEventEnum
    {
        StartEvent,
        EndEvent
    };
};

class SwitchSchedulerTask
{
    public:
        SwitchSchedulerTask(String,String,void (*)(int));
        String startTime;
        String endTime;
        void (*callback)(int);
};

class SwitchScheduler
{
    public:
        // ctor
        SwitchScheduler(SwitchSchedulerConfiguration*, SparkTime*);

        void setAstronomyApiUrl(String apiUrl);

        void setAstronomyApiCheckTime(String checkTime);

        void setIsEnabled(bool enabled);

        void setHomeOnlyModeEnabled(bool enabled);

        bool isUsingAstronomyData();

        bool isDst();

        // Enabled and "Home Only Mode" is disabled or enabled and people
        // are home.
        bool isSchedulerEnabled();

        SparkTime* getTime();

        // Today's sunrise time.
        time_t getSunriseTime();

        // Today's sunset time.
        time_t getSunsetTime();

        int getCurrentHomeCount();

        void setHomeStatus(const char* mobileId);

        void setAwayStatus(const char* mobileId);

        void resetHomeStatus();

        // The last time the spark core sync'd itself.
        unsigned long getLastTimeSync();

        SwitchSchedulerConfiguration* getConfiguration();

        // Get a list of currently registered scheduler tasks.
        SwitchSchedulerTask** getTasks();

        // Get the length of the task array.
        int getTasksLength();

        // Checks to see if the switch should be toggled on or off.
        bool shouldBeToggled();

        // Adds a task that the scheduler should keep track of.
        void addSchedulerTask(SwitchSchedulerTask*);

        // To be called in the Spark loop.
        void tock();
    private:
        // A day in milliseconds.
        const unsigned long dayInMilliseconds = (24 * 60 * 60 * 1000);

        // The last time the time was sync'd.
        unsigned long lastTimeSync;

        // The last time the Tock method was run.
        unsigned long lastLoopCheck;

        // How often
        unsigned long checkLoopInterval = (60 * 1000); // every 1 minute

        bool _isUsingAstronomyData;

        time_t sunriseTime;

        // The time that the sunset will occur for today.
        time_t sunsetTime;

        arraylist<const char*>* homeMobileIds;

        SparkTime* rtc;

        SwitchSchedulerConfiguration* configuration;

        // TODO: Convert this to arraylist
        SwitchSchedulerTask** tasks;

        int tasksLength;

        void initialize(SwitchSchedulerConfiguration*);

        void checkToggledState();

        void checkSchedulerTasks();

        void checkSchedulerTask(SwitchSchedulerTask*, String, SwitchSchedulerEvent::SwitchSchedulerEventEnum);

        // Try to sync the time. It will only sync once a day.
        void syncTime();

        // Try to get the astronomy data. It will only get it at the
        // configured time.
        void retrieveAstronomyData();

        // Retrieve the sunset data from the API and parse it into
        // a unix timestamp.
        bool parseAndSetAstronomyData();

        // Retrieve the sunset data from the API.
        const char* getAstronomyDataResponse();

        time_t getTime(String);
};

#endif // SWITCH_SCHEDULER_H_
