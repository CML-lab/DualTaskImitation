#ifndef CONFIG_H
#define CONFIG_H
#pragma once

#include "SDL_opengl.h"

////////// Set these parameters only! //////////////////////////

//set this to the data path in which the current set of data for this block should be stored.

//set the subject number
#define SUBJECT_ID "200.028"

//define the file that contains the name of the trial table
#define TRIALFILE "./TrialTables/dtbody_p.txt"

//define the folder where the data will go
//   (***THIS FOLDER MUST EXIST!***)
#define DATAPATH "./Data/200.028/"

///////////////////////////////////////////////////////////////

//set this parameter to determine the primary sensor -- should correspond to the index finger
#define HAND 1

//define paths
#define VIDEOPATH "Resources\\videos\\"

//28 trials * 2 postures/trial * 2 orienations
#define NIMAGES 96

#define IMAGEPATH1 "./Resources/IM_Images/Body/"
#define NIMAGES1 56

#define IMAGEPATH2 "./Resources/IM_Images/Trajectory/"
#define NIMAGES2 96

#define IMAGEPATH3 "./Resources/IM_Images/Control/"
#define NIMAGES3 24


////////////////////////////////////////////////////////////////




// Configurable constants

//tracker configuration parameters
//TRACKTYPE: type of tracker being used: FOB (0) or trakStar (1)
#define TRACKTYPE 1
#define BIRDCOUNT 4
//SAMPRATE: sampling rate (for 1 full measurement cycle, or time to activate all 3 axes.  for trakSTAR, we get samples returned at SAMPRATE*3 frequency.
#define SAMPRATE 140
//1 out of every REPORTRATE samples will be acquired from the trakSTAR, when in Synchronous mode
#define REPORTRATE 1
#define FILTER_WIDE true
#define FILTER_NARROW false
#define FILTER_DC 1.0f
#define HEMIFIELD FRONT
#define RANGE 72.0f
#define DOSYNC true
#define DOMETRIC true


//screen dimensions
//   (note, the Elitebook wants dimensions that are slightly larger than the screen, with WINDOWED set to true!)
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT  1080
//#define SCREEN_WIDTH  1440
//#define SCREEN_HEIGHT  900

//video dimensions
//Videos are in 16:9 format
#define VIDEO_WIDTH int(float(SCREEN_WIDTH)*0.95f)
#define VIDEO_RATIO (16.0f/9.0f)
#define VIDEO_HEIGHT int(float(VIDEO_WIDTH)/VIDEO_RATIO)
//#define VIDEO_WIDTH 1536
//#define VIDEO_HEIGHT 864


// Physical dimensions of the screen in meters
#define PHYSICAL_WIDTH  0.307
#define PHYSICAL_HEIGHT  0.173
//#define PHYSICAL_HEIGHT  0.192

// screen ratio, in meters per pixel
#define PHYSICAL_RATIO  (PHYSICAL_WIDTH / SCREEN_WIDTH)
//#define PHYSICAL_RATIOI  (SCREEN_WIDTH/PHYSICAL_WIDTH)

#define SCREEN_BPP  32
//switch WINDOWED and MIRRORED for kinereach/simulation runs
#define WINDOWED  true
#define MIRRORED  false

#define CURSOR_RADIUS 0.006f
#define START_RADIUS 0.05f

//define pi as a float
#define PI 3.14159265358979f

//calibration parameters -- set to zero here, we will set them dynamically later!
#define CALxOFFSET 0.0f   //meters
#define CALyOFFSET 0.0f   //meters
#define CALzOFFSET 0.0f   //meters
#define CALthetaROTANG 0.0f  //radians
#define CALphiROTANG 0.0f  //radians

//define the velocity threshold for detecting movement onset/offset, in m/sec
#define VEL_MVT_TH 0.05f
//define the time that vel must be below thresh to define movement end, in msec
#define VEL_END_TIME 2000 //hold still for 2 sec

//#define TRIAL_DURATION 10000 //10 sec, in msec
#define MAX_TRIAL_DURATION 600000 //10 min, in msec

#endif
