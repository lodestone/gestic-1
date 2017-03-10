#include "demo.h"
#include "rest.h"

#include <sys/time.h>

static const char source[] = { 'S', 'W', 'N', 'E', 'C' };

static const char *gestures[] = {
    "-                       ",
    "Flick West > East       ",
    "Flick East > West       ",
    "Flick South > North     ",
    "Flick North > South     ",
    "Circle clockwise        ",
    "Circle counter-clockwise"
};

static const int MAX_VOLUME = 45;
static const int MIN_VOLUME = 0;
static const int MAX_AIRWHEEL = 255;
static int volume = 15;
static int old_wheel_counter = 0;

static const char* STATION_BODY[] = {
	"{\"playQueueItem\":{\"behaviour\":\"impulsive\",\"station\":{\"id\":\"s69060\",\"tuneIn\":{\"stationId\":\"s69060\",\"location\":\"\"},\"name\":\"DR P5 (K..benhavn)\",\"image\":[]}}}",
	"{\"playQueueItem\":{\"behaviour\":\"impulsive\",\"station\":{\"id\":\"s24861\",\"tuneIn\":{\"stationId\":\"s24861\",\"location\":\"\"},\"name\":\"DR P3 (K..benhavn)\",\"image\":[]}}}",
	"{\"playQueueItem\":{\"behaviour\":\"impulsive\",\"station\":{\"id\":\"s49642\",\"tuneIn\":{\"stationId\":\"s49642\",\"location\":\"\"},\"name\":\"DR P4 Midt & Vest (Holstebro)\",\"image\":[]}}}"
};

static const char* VOLUME_BODY =
  "{\"level\":%d}";

static const char* PAUSE_BODY =
  "{\"toBeReleased\":false}";

static int currentStation = 0;
static const int MAX_STATION = 2; // 3 stations
static int playing = 0;

static double lastWheelCommand = 0.0;

#define HOST "http://192.168.1.108:8080"
static const char* STATION_URL = HOST "/BeoZone/Zone/PlayQueue?instantplay";
static const char* VOLUME_URL = HOST "/BeoZone/Zone/Sound/Volume/Speaker/Level";
static const char* PAUSE_URL = HOST "/BeoZone/Zone/Stream/Pause";

#define RESULT_SIZE 2000
static char result[RESULT_SIZE];


static double getMillis()
{
  struct timeval time;
  gettimeofday(&time, NULL);
  return time.tv_sec * 1000.0 + time.tv_usec / 1000.0;
}

static void updateVolume(void* volume)
{
  int v = (int) volume;
  char body[200];
  sprintf(body, VOLUME_BODY, v);
  restPut(VOLUME_URL, body, result, RESULT_SIZE);
}

static void updateCurrentStation()
{
  restPost(STATION_URL, STATION_BODY[currentStation], result, RESULT_SIZE);
}

void updateProduct(data_t* data)
{
	/* Print update of gestures */
	if (data->gestic_gesture != NULL)
	{
		/* Remember last gesture and reset it after 1s = 200 updates */
		if (data->gestic_gesture->gesture > 0 && data->gestic_gesture->gesture <= 6 && data->gestic_gesture->last_event == 0)
		{
			bool update = true;
			switch (data->gestic_gesture->gesture)
			{
				case 1:
					currentStation++;
					if (currentStation > MAX_STATION)
					{
						currentStation = 0;
					}
					break;

				case 2:
					currentStation--;
					if (currentStation < 0)
					{
						currentStation = MAX_STATION;
					}
					break;

				default:
					// Unknown gesture
					update = false;
					break;
			}
			if (update)
			{
				updateCurrentStation();
			}
		}
	}

	/* Print update of touch */
	if (data->gestic_touch && data->gestic_pos && data->gestic_touch->last_event == 0)
	{
    if (data->gestic_touch->flags & gestic_touch_center)
    {
      if (playing)
      {
        restPost(PAUSE_URL, PAUSE_BODY, result, RESULT_SIZE);
        playing = 0;
      }
      else
      {
        restPost(STATION_URL, STATION_BODY[currentStation], result, RESULT_SIZE);
        playing = 1;
      }
    }
	}

	/* Print update of AirWheel */
	if (data->gestic_air_wheel != NULL) {
		if (old_wheel_counter != data->gestic_air_wheel->counter)
    {
      double now = getMillis();
      if (now > (lastWheelCommand + 200.0))
      {
        int step = data->gestic_air_wheel->counter - old_wheel_counter;
        int direction = step > 0 ? 1 : -1;
        //adjust for overrun
        direction *= abs(step) > 128 ? -1 : 1;
        volume += 3 * direction;
        if (volume > MAX_VOLUME)
        {
          volume = MAX_VOLUME;
        }
        if (volume < MIN_VOLUME)
        {
          volume = MIN_VOLUME;
        }
        old_wheel_counter = data->gestic_air_wheel->counter;

        pthread_t t;
        pthread_create(&t, NULL, updateVolume, (void*)volume);
        lastWheelCommand = now;
      }
    }
	}
}

static void testInitData(data_t* data)
{
	data->gestic_pos = malloc(sizeof(gestic_position_t));
	data->gestic_pos->x = 32768;
	data->gestic_pos->y = 32768;
	data->gestic_pos->z = 32768;

	data->gestic_gesture = malloc(sizeof(gestic_gesture_t));
	data->gestic_gesture->gesture = 0;
	data->gestic_gesture->flags = 0; // Not used
	data->gestic_gesture->last_event = 0;

	data->gestic_touch = malloc(sizeof(gestic_touch_t));
	data->gestic_touch->flags = 0;
	data->gestic_touch->last_event = 0;
	data->gestic_touch->tap_flags = 0; // Not used
	data->gestic_touch->last_tap_event = 0; // Not used
	data->gestic_touch->last_touch_event_start = 0; // Not used

	data->gestic_air_wheel = malloc(sizeof(gestic_air_wheel_t));
	data->gestic_air_wheel->counter = 0;
	data->gestic_air_wheel->active = 0; // Not used
	data->gestic_air_wheel->last_event = 0; // Not used
}

static void testCleanup(data_t* data)
{
	free(data->gestic_pos);
	free(data->gestic_gesture);
	free(data->gestic_touch);
	free(data->gestic_air_wheel);
}

void runDemoTest()
{
	data_t data;
	init_data(&data);
	testInitData(&data);

	updateProduct(&data);
	data.gestic_touch->flags = 3; // Multi touch
	updateProduct(&data);
	data.gestic_touch->flags = 0;
	int i;
	for (i = 255; i > 250; --i)
	{
		data.gestic_air_wheel->counter = i;
		updateProduct(&data);
	}
	for (i = 250; i < 255; ++i)
	{
		data.gestic_air_wheel->counter = i;
		updateProduct(&data);
	}
	for (i = 254; i > 100; --i)
	{
		data.gestic_air_wheel->counter = i;
		updateProduct(&data);
	}
	for (i = 100; i < 120; ++i)
	{
		data.gestic_air_wheel->counter = i;
		updateProduct(&data);
	}
	data.gestic_touch->flags = 1;
	updateProduct(&data);
	data.gestic_touch->last_event = 1;
	updateProduct(&data);

	testCleanup(&data);
}
