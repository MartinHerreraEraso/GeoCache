/******************************************************************************

GeoCache Hunt Project (GeoCache.cpp)

This is skeleton code provided as a project development guideline only.  You
are not required to follow this coding structure.  You are free to implement
your project however you wish.  

Team Number: 1

Team Members:

	1.Martin Herrera
	2.Miguel 
	
NOTES: 

You only have 32k of program space and 2k of data space.  You must
use your program and data space wisely and sparingly.  You must also be
very conscious to properly configure the digital pin usage of the boards,
else weird things will happen.

The Arduino GCC sprintf() does not support printing floats or doubles.  You should
consider using sprintf(), dtostrf(), strtok() and strtod() for message string
parsing and converting between floats and strings.

The GPS provides latitude and longitude in degrees minutes format (DDDMM.MMMM).  
You will need convert it to Decimal Degrees format (DDD.DDDD).  The switch on the
GPS Shield must be set to the "Soft Serial" position, else you will not receive
any GPS messages.

*******************************************************************************

Following is the GPS Shield "GPRMC" Message Structure.  This message is received
once a second.  You must parse the message to obtain the parameters required for
the GeoCache project.  GPS provides coordinates in Degrees Minutes (DDDMM.MMMM).
The coordinates in the following GPRMC sample message, after converting to Decimal
Degrees format(DDD.DDDDDD) is latitude(23.118757) and longitude(120.274060).  By 
the way, this coordinate is GlobalTop Technology in Taiwan, who designed and 
manufactured the GPS Chip.

"$GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C/r/n"

$GPRMC,         // GPRMC Message
064951.000,     // utc time hhmmss.sss
A,              // status A=data valid or V=data not valid
2307.1256,      // Latitude 2307.1256 (degrees minutes format dddmm.mmmm)
N,              // N/S Indicator N=north or S=south
12016.4438,     // Longitude 12016.4438 (degrees minutes format dddmm.mmmm)
E,              // E/W Indicator E=east or W=west
0.03,           // Speed over ground knots
165.48,         // Course over ground (decimal degrees format ddd.dd)
260406,         // date ddmmyy
3.05,           // Magnetic variation (decimal degrees format ddd.dd)
W,              // E=east or W=west
A               // Mode A=Autonomous D=differential E=Estimated
*2C             // checksum
/r/n            // return and newline

Following are approximate results calculated from above GPS GPRMC message
(when GPS_ON == 0) to the GEOLAT0/GEOLON0 tree location:

degMin2DecDeg() LAT 2307.1256 N = 23.118757 decimal degrees
degMin2DecDeg() LON 12016.4438 E = 120.274060 decimal degrees
calcDistance() to GEOLAT0/GEOLON0 target = 45335760 feet
calcBearing() to GEOLAT0/GEOLON0 target = 22.999655 degrees

The resulting relative target bearing to the tree is 217.519650 degrees

******************************************************************************/

/*
Configuration settings.

These defines makes it easy for you to enable/disable certain 
code during the development and debugging cycle of this project.
There may not be sufficient room in the PROGRAM or DATA memory to
enable all these libraries at the same time.  You must have have 
NEO_ON, GPS_ON and SDC_ON during the actual GeoCache Flag Hunt on
Finals Day
*/
#include <Adafruit_GPS.h>
#define NEO_ON 1	// NeoPixelShield
#define TRM_ON 1		// SerialTerminal
#define SDC_ON 0		// SecureDigital
#define GPS_ON 1		// Live GPS Message (off = simulated)

// define pin usage
#define NEO_TX	6		// NEO transmit
#define GPS_TX	7		// GPS transmit
#define GPS_RX	8		// GPS receive
#define BUTTON  2		// Button PIN
#define POTENTIOMETER  A0	// Potentiometer PIN 

// GPS message buffer
#define GPS_RX_BUFSIZ	128
char cstr[GPS_RX_BUFSIZ];

// global variables
uint8_t target = 0;		// target number
float heading = 0.0;	// target heading
float distance = 0.0;	// target distance

#if GPS_ON
#include <SoftwareSerial.h>
SoftwareSerial gps(GPS_RX, GPS_TX);
#endif

#if NEO_ON
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(40, NEO_TX, NEO_GRB + NEO_KHZ800);
#endif

#if SDC_ON
#include <SD.h>
#endif

/*
Following is a Decimal Degrees formatted waypoint for the large tree
in the parking lot just outside the front entrance of FS3B-116.
*/
#define GEOLAT0 28.594532
#define GEOLON0 -81.304437

#if GPS_ON
/*
These are GPS command messages (only a few are used).
*/
#define PMTK_AWAKE "$PMTK010,002*2D"
#define PMTK_STANDBY "$PMTK161,0*28"
#define PMTK_Q_RELEASE "$PMTK605*31"
#define PMTK_ENABLE_WAAS "$PMTK301,2*2E"
#define PMTK_ENABLE_SBAS "$PMTK313,1*2E"
#define PMTK_CMD_HOT_START "$PMTK101*32"
#define PMTK_CMD_WARM_START "$PMTK102*31"
#define PMTK_CMD_COLD_START "$PMTK103*30"
#define PMTK_CMD_FULL_COLD_START "$PMTK104*37"
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"
#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_API_SET_FIX_CTL_1HZ  "$PMTK300,1000,0,0,0,0*1C"
#define PMTK_API_SET_FIX_CTL_5HZ  "$PMTK300,200,0,0,0,0*2F"
#define PMTK_SET_NMEA_OUTPUT_RMC "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
#define PMTK_SET_NMEA_OUTPUT_GGA "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

#endif // GPS_ON

/*************************************************
**** GEO FUNCTIONS - BEGIN ***********************
*************************************************/

/**************************************************
Convert Degrees Minutes (DDMM.MMMM) into Decimal Degrees (DDD.DDDD)

float degMin2DecDeg(char *cind, char *ccor)

Input:
	cind = string char pointer containing the GPRMC latitude(N/S) or longitude (E/W) indicator
	ccor = string char pointer containing the GPRMC latitude or longitude DDDMM.MMMM coordinate

Return:
	Decimal degrees coordinate.
	
**************************************************/
float degMin2DecDeg(char *cind, char *ccor)
{
	//convert from minutes to degrees
	float degrees = atof(ccor);
	//check if possition is negative
	if (cind == "S" || cind == "W")
		degrees = -(degrees);
	//return result
	return(degrees);
}

/************************************************** 
Calculate Great Circle Distance between to coordinates using
Haversine formula.

float calcDistance(float flat1, float flon1, float flat2, float flon2)

EARTH_RADIUS_FEET = 3959.00 radius miles * 5280 feet per mile

Input:
	flat1, flon1 = first latitude and longitude coordinate in decimal degrees
	flat2, flon2 = second latitude and longitude coordinate in decimal degrees

Return:
	distance in feet (3959 earth radius in miles * 5280 feet per mile)
**************************************************/
float calcDistance(float flat1, float flon1, float flat2, float flon2)
{
	float distance = 0.0;
	float R = 3959.00; //Earth Radius Feet.
	float lat1 = radians(flat1);
	float lat2 = radians(flat2);
	float dif1 = radians(flat2 - flat1);
	float dif2 = radians(flon1 - flon2);

	float A = (sinf(dif1 / 2) * cosf(dif1 / 2)) + (cosf(lat1) * cosf(lat2) * sinf(dif2 / 2) * sinf(dif2 / 2));
	float C = 2 * atan2f(sqrtf(A), sqrtf(1 - A));

	distance = R * C;

	return(distance);
}

/**************************************************
Calculate Great Circle Bearing between two coordinates

float calcBearing(float flat1, float flon1, float flat2, float flon2)

Input:
	flat1, flon1 = first latitude and longitude coordinate in decimal degrees
	flat2, flon2 = second latitude and longitude coordinate in decimal degrees

Return:
	angle in decimal degrees from magnetic north (normalize to a range of 0 to 360)
**************************************************/
float calcBearing(float flat1, float flon1, float flat2, float flon2)
{
	float bearing = 0.0;
	float R = 3959.00; //Earth Radius Feet.
	float firstlat = radians(flat1);
	float firstlon = radians(flon1);
	float secondlat = radians(flat2);
	float secondlon = radians(flon2);

	float y = sinf(secondlat - firstlat) * cosf(secondlon);
	float x = cosf(firstlat) * sinf(secondlat) - sinf(firstlat) * cosf(secondlat) * cosf(secondlat - firstlat);
	bearing = degrees(atan2f(y, x));

	return(bearing);
}

/*************************************************
**** GEO FUNCTIONS - END**************************
*************************************************/

#if NEO_ON
/*
Sets target number, heading and distance on NeoPixel Display

NOTE: Target number, bearing and distance parameters used
by this function do not need to be passed in, since these
parameters are in global data space.

*/
void setNeoPixel(uint8_t target, float heading, float distance)
{
	strip.clear();
	strip.setBrightness(map(analogRead(POTENTIOMETER), 0, 1023, 0, 255));
	// Display target
	if (target == 1)
	{
		strip.setPixelColor(0, strip.Color(255, 0, 0));
		strip.setPixelColor(1, strip.Color(255, 0, 0));
	}
	else if (target == 2)
	{
		strip.setPixelColor(2, strip.Color(0, 255, 0));
		strip.setPixelColor(3, strip.Color(0, 255, 0));
	}
	else if (target == 3)
	{
		strip.setPixelColor(4, strip.Color(0, 0, 255));
		strip.setPixelColor(5, strip.Color(0, 0, 255));
	}
	else if (target == 4)
	{
		strip.setPixelColor(6, strip.Color(255, 0, 255));
		strip.setPixelColor(7, strip.Color(255, 0, 255));
	}
	// Display heading
	if (heading > 180)
		heading = 0 - (heading - 180);
	strip.setPixelColor(11, strip.Color(0, 255, 0));
	strip.setPixelColor(12, strip.Color(0, 255, 0));
	strip.setPixelColor(19, strip.Color(0, 255, 0));
	strip.setPixelColor(20, strip.Color(0, 255, 0));
	if (heading > 10)
		strip.setPixelColor(12, strip.Color(255, 153, 0));
	if (heading > 20)
		strip.setPixelColor(20, strip.Color(255, 153, 0));
	if (heading > 30)
		strip.setPixelColor(13, strip.Color(255, 153, 0));
	if (heading > 40)
		strip.setPixelColor(21, strip.Color(255, 153, 0));
	if (heading > 50)
		strip.setPixelColor(14, strip.Color(255, 153, 0));
	if (heading > 60)
		strip.setPixelColor(22, strip.Color(255, 153, 0));
	if (heading > 70)
		strip.setPixelColor(15, strip.Color(255, 153, 0));
	if (heading > 80)
		strip.setPixelColor(23, strip.Color(255, 153, 0));
	if (heading > 90)
		strip.setPixelColor(12, strip.Color(255, 0, 0));
	if (heading > 100)
		strip.setPixelColor(20, strip.Color(255, 0, 0));
	if (heading > 110)
		strip.setPixelColor(13, strip.Color(255, 0, 0));
	if (heading > 120)
		strip.setPixelColor(21, strip.Color(255, 0, 0));
	if (heading > 130)
		strip.setPixelColor(14, strip.Color(255, 0, 0));
	if (heading > 140)
		strip.setPixelColor(22, strip.Color(255, 0, 0));
	if (heading > 150)
		strip.setPixelColor(15, strip.Color(255, 0, 0));
	if (heading > 160)
		strip.setPixelColor(23, strip.Color(255, 0, 0));
	if (heading < -10)
		strip.setPixelColor(11, strip.Color(255, 153, 0));
	if (heading < -20)
		strip.setPixelColor(19, strip.Color(255, 153, 0));
	if (heading < -30)
		strip.setPixelColor(10, strip.Color(255, 153, 0));
	if (heading < -40)
		strip.setPixelColor(18, strip.Color(255, 153, 0));
	if (heading < -50)
		strip.setPixelColor(9, strip.Color(255, 153, 0));
	if (heading < -60)
		strip.setPixelColor(17, strip.Color(255, 153, 0));
	if (heading < -70)
		strip.setPixelColor(8, strip.Color(255, 153, 0));
	if (heading < -80)
		strip.setPixelColor(16, strip.Color(255, 153, 0));
	if (heading < -90)
		strip.setPixelColor(11, strip.Color(255, 0, 0));
	if (heading < -100)
		strip.setPixelColor(19, strip.Color(255, 0, 0));
	if (heading < -110)
		strip.setPixelColor(10, strip.Color(255, 0, 0));
	if (heading < -120)
		strip.setPixelColor(18, strip.Color(255, 0, 0));
	if (heading < -130)
		strip.setPixelColor(9, strip.Color(255, 0, 0));
	if (heading < -140)
		strip.setPixelColor(17, strip.Color(255, 0, 0));
	if (heading < -150)
		strip.setPixelColor(8, strip.Color(255, 0, 0));
	if (heading < -160)
		strip.setPixelColor(16, strip.Color(255, 0, 0));
	// Display distance
	for (size_t i = 0; i < 48; i++)
	{
		if (distance > 1)
		{
			if (distance > (10 * i))
				strip.setPixelColor(24 + i, strip.Color(0, 255, 0));
		}
		if (distance > 160)
		{
			if (distance > (160 + (10 * i)))
				strip.setPixelColor(24 + i, strip.Color(255, 153, 0));
		}
		if (distance > 320)
		{
			if (distance > (320 + (10 * i)))
				strip.setPixelColor(24 + i, strip.Color(255, 0, 0));
		}
	}

	strip.show();
}

#endif	// NEO_ON

#if GPS_ON
/*
Get valid GPS message. This function returns ONLY once a second.

NOTE: DO NOT CHANGE THIS CODE !!!

void getGPSMessage(void)

Side affects:
	Message is placed in global "cstr" string buffer.

Input:
	none

Return:
	none
	
*/
void getGPSMessage(void)
{
	uint8_t x=0, y=0, isum=0;

	memset(cstr, 0, sizeof(cstr));
		
	// get nmea string
	while (true)
	{
		if (gps.peek() != -1)
		{
			cstr[x] = gps.read();
			
			// if multiple inline messages, then restart
			if ((x != 0) && (cstr[x] == '$'))
			{
				x = 0;
				cstr[x] = '$';
			}
			
			// if complete message
			if ((cstr[0] == '$') && (cstr[x++] == '\n'))
			{
				// nul terminate string before /r/n
				cstr[x-2] = 0;

				// if checksum not found
				if (cstr[x-5] != '*') 
				{
					x = 0;
					continue;
				}
								
				// convert hex checksum to binary
				isum = strtol(&cstr[x-4], NULL, 16);
				
				// reverse checksum
				for (y=1; y < (x-5); y++) isum ^= cstr[y];
				
				// if invalid checksum
				if (isum != 0) 
				{
					x = 0;
					continue;
				}
				
				// else valid message
				break;
			}
		}
	}
}

#else
/*
Get simulated GPS message once a second.

This is the same message and coordinates as described at the top of this
file.  You could edit these coordinates to point to the tree out front (GEOLAT0,
GEOLON0) to test your distance and direction calculations.  Just note that the
tree coordinates are in Decimal Degrees format, and the message coordinates are 
in Degrees Minutes format.

NOTE: DO NOT CHANGE THIS CODE !!!

void getGPSMessage(void)

Side affects:
	Static GPRMC message is placed in global "cstr" null terminated char string buffer.

Input:
	none

Return:
	none

*/
void getGPSMessage(void)
{
	static unsigned long gpsTime = 0;
	
	// simulate waiting for message
	while (gpsTime > millis()) delay(100);
	
	// do this once a second
	gpsTime = millis() + 1000;
	
	memcpy(cstr, "$GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C", sizeof(cstr));

	return;	
}

#endif	// GPS_ON

void setup(void)
{
	#if TRM_ON
	// init serial interface
	Serial.begin(115200);
	#endif	

	#if NEO_ON
	// init NeoPixel Shield
	#endif	

	#if SDC_ON
	/*
	Initialize the SecureDigitalCard and open a numbered sequenced file
	name "MyMapNN.txt" for storing your coordinates, where NN is the 
	sequential number of the file.  The filename can not be more than 8
	chars in length (excluding the ".txt").
	*/
	#endif

	#if GPS_ON
	// enable GPS sending GPRMC message
	gps.begin(9600);
	gps.println(PMTK_SET_NMEA_UPDATE_1HZ);
	gps.println(PMTK_API_SET_FIX_CTL_1HZ);
	gps.println(PMTK_SET_NMEA_OUTPUT_RMC);
	#endif		

	// init target button here
	pinMode(BUTTON, INPUT_PULLUP);
	strip.begin();
	strip.clear();
	for (size_t i = 0; i < strip.numPixels(); i++)
	{
		strip.setPixelColor(i, strip.Color(255, 0, 0));
		strip.show();
		delay(50);
		strip.clear();
	}
}

void loop(void)
{
	// max 1 second blocking call till GPS message received
	getGPSMessage();
	
	// if button pressed, set new target
	if (digitalRead(BUTTON) == LOW)
	{
		target++;
		if (target > 4)
			target = 1;
	}
	// if GPRMC message (3rd letter = R)
	while (cstr[3] == 'R')
	{
		// parse message parameters
		
		// calculated destination heading
		
		// calculated destination distance
		
		#if SDC_ON
		// write current position to SecureDigital then flush
		#endif

		break;
	}

	#if NEO_ON
	// set NeoPixel target display
	setNeoPixel(target, heading, distance);
	heading += 10;
	if (heading > 360)
		heading = 0;
	distance += 10;
	if (distance > 480)
		distance = 0;
	#endif		

	#if TRM_ON
	// print debug information to Serial Terminal
	Serial.println(cstr);	
	#endif		
}