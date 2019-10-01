/*

Code copied from Apraxia Battery imitation task code on 3/8/2019 by A. Wong.
Code uses the State Machine version of the video player updated 3/8/2019.

*/


#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <istream>
#include <windows.h>
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"

#include "MouseInput.h"
#include "TrackBird.h"

#include "Circle.h"
#include "DataWriter.h"
#include "HandCursor.h"
#include "Object2D.h"
#include "Path2D.h"
#include "Region2D.h"
#include "Sound.h"
#include "Timer.h"
#include "Image.h"
#include "vlcVideoPlayerSM.h"

#include "config.h"

#include <gl/GL.h>
#include <gl/GLU.h>

/*
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2_mixer.lib")
#pragma comment(lib, "SDL2_ttf.lib")
#pragma comment(lib, "SDL2_image.lib")
#pragma comment(lib, "Bird.lib")
#pragma comment(lib, "ftd2xx.lib")
#pragma comment(lib, "ATC3DG.lib")
#pragma comment(lib, "libvlc.lib")
*/
#pragma push(1)

//state machine
enum GameState
{
	Idle = 0x01,		//00001
	WaitStim = 0x03,	//00011
	ShowVideo = 0x04,	//00100
	ShowDTStim = 0x05,	//00101
	WaitDTResp = 0x06,	//00101
	Active = 0x08,		//01000
	EndTrial = 0x10,	//01010
	Finished = 0x12		//01100
};



SDL_Event event;
SDL_Window *screen = NULL;
SDL_GLContext glcontext = NULL;

HandCursor* curs[BIRDCOUNT + 1];
HandCursor* player = NULL;
Circle* startCircle = NULL;  //circle to keep track of the "home" position
Image* endtext = NULL;
Image* readytext = NULL;
Image* trialinstructtext = NULL;
Image* stoptext = NULL;
Image* holdtext = NULL;
Image* redotext = NULL;
Image* trialnum = NULL;
Image* recordtext = NULL;
Image* proceedtext = NULL;
Image* returntext = NULL;
Image* mousetext = NULL;
Image* selecttext = NULL;
Image* imitatetext = NULL;
Image* tooslowtext = NULL;
Region2D selectbox;
Sound* startbeep = NULL;
Sound* dtrespbeep = NULL;
Sound* errorbeep = NULL;
Sound* correctbeep = NULL;
SDL_Color textColor = {0, 0, 0, 1};
DataWriter* writer = NULL;
GameState state;
Timer* trialTimer;
Timer* hoverTimer;
Timer* movTimer;
Timer* dtTimer;

//videos
Video *Vid = NULL;
int NVid;
std::string vpathbase;

//Dual-task variables 
Object2D* dualtaskimages[NIMAGES]; //stimuli
int subjresp = -1;
int NImages = 0;

//tracker variables
int trackstatus;
TrackSYSCONFIG sysconfig;
TrackDATAFRAME dataframe[BIRDCOUNT+1];
Uint32 DataStartTime = 0;
bool recordData = false;
bool didrecord = false;

//colors
float redColor[3] = {1.0f, 0.0f, 0.0f};
float greenColor[3] = {0.0f, 1.0f, 0.0f};
float blueColor[3] = {0.0f, 0.0f, 1.0f};
float cyanColor[3] = {0.0f, 0.5f, 1.0f};
float grayColor[3] = {0.6f, 0.6f, 0.6f};
float blkColor[3] = {0.0f, 0.0f, 0.0f};
float whiteColor[3] = {1.0f, 1.0f, 1.0f};
float orangeColor[3] = {1.0f, 0.5f, 0.0f};
float *cursColor = blueColor;


// Trial table structure, to keep track of all the parameters for each trial of the experiment
typedef struct {
	int trialType;		// Flag for trial type (i.e., type of dual task) - 1 = dualtask, 0 = imitation only, 2 = secondarytask only
	int video;			//video number
	int dttype;			//dual-task stim type: 1 = posture, 2 = trajectory, 3 = control
	int dtpori;			//pass a note whether this is a priming or interfering stimulus
	int dt0;			//dual-task stimulus #1
	int dt1;			//dual-task stimulus #2
	int dt2;			//dual-task stimulus #3
	int dtcorrectresp;	//dual-task correct response (1 = yes, 0 = no)
	int dtstimdur;		//duration to show the dual-task stim
	int dtrespdur;		//duration to wait for response for dual task
	int srdur;			//duration to wait after video playback before go cue (stim-response delay duration)
	int trdur;			//trial duration (max movement time for imitation)
} TRTBL;

#define TRTBL_SIZE 100
TRTBL trtbl [TRTBL_SIZE];

int NTRIALS = 0;
int CurTrial = -1;

#define curtr trtbl[CurTrial]

int score = 0;

//target structure; keep track of the target and other parameters, for writing out to data stream
TargetFrame Target;

//varibles that let the experimenter control the experiment flow
bool nextstateflag = false;
bool redotrialflag = false;
bool skiptrialflag = false;
int numredos = 0;


// Initializes everything and returns true if there were no errors
bool init();
// Sets up OpenGL
void setup_opengl();
// Performs closing operations
void clean_up();
// Draws objects on the screen
void draw_screen();
//file to load in trial table
int LoadTrFile(char *filename);
//file to load in a list of context strings and create a bunch of text images from them
int LoadContextFile(char *fname,Image* cText[]);
// Update loop (state machine)
void game_update();
//load a new video file
int VidLoad(const char* fname);

bool quit = false;  //flag to cue exit of program

int main(int argc, char* args[])
{
	int a = 0;
	int flagoffsetkey = -1;
	Target.key = ' ';

	//redirect stderr output to a file
	freopen( "./Debug/errorlog.txt", "w", stderr); 
	freopen( "./Debug/outlog.txt", "w", stdout); 

	std::cerr << "Start main." << std::endl;

	SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);
	//HIGH_PRIORITY_CLASS
	std::cerr << "Promote process priority to Above Normal." << std::endl;

	if (!init())
	{
		// There was an error during initialization
		std::cerr << "Initialization error." << std::endl;
		return 1;
	}

	DataStartTime = SDL_GetTicks();

	while (!quit)
	{
		int inputs_updated = 0;

		// Retrieve Flock of Birds data
		if (trackstatus>0)
		{
			// Update inputs from Flock of Birds
			inputs_updated = TrackBird::GetUpdatedSample(&sysconfig,dataframe);
		}

		// Handle SDL events
		while (SDL_PollEvent(&event))
		{
			// See http://www.libsdl.org/docs/html/sdlevent.html for list of event types
			if (event.type == SDL_MOUSEMOTION)
			{
				if (trackstatus <= 0)
				{
					MouseInput::ProcessEvent(event);
					inputs_updated = MouseInput::GetFrame(dataframe);

				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					Target.key = 'L';
					subjresp = 1;
					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}


				}
				else if (event.button.button == SDL_BUTTON_RIGHT)
				{
					Target.key = 'R';
					subjresp = 2;
					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}
				}
		
			}
			else if (event.type == SDL_KEYDOWN)
			{
				// See http://www.libsdl.org/docs/html/sdlkey.html for Keysym definitions
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else if (event.key.keysym.sym == SDLK_0)
				{
					flagoffsetkey = 1;
					Target.key = '0';
					//std::cerr << "Zero requested" << std::endl;
				}
				else if (event.key.keysym.sym == SDLK_r)
				{
					redotrialflag = true;
					Target.key = 'r';

					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}

					//std::cerr << "Redo requested" << std::endl;
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					skiptrialflag = true;
					Target.key = 'k';

					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}

					//std::cerr << "Skip Trial requested" << std::endl;
				}
				else if (event.key.keysym.sym == SDLK_SPACE)
				{
					nextstateflag = true;
					Target.key = 's';

					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}

					//std::cerr << "Advance requested" << std::endl;
				}
				else if (event.key.keysym.sym == SDLK_z)
				{
					Target.key = 'z';
					subjresp = 1;
					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}
				}
				else if (event.key.keysym.sym == SDLK_x)
				{
					Target.key = 'x';
					subjresp = 2;
					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}
				}
				else if (event.key.keysym.sym == SDLK_a)
				{
					Target.key = 'a';  //abort trial early
					if (trackstatus <= 0)
					{
						inputs_updated = 1;
						if (dataframe[0].ValidInput <= 0)
						{
							dataframe[0].x = 0;
							dataframe[0].y = 0;
							dataframe[0].z = 0;
							dataframe[0].azimuth = 0;
							dataframe[0].elevation = 0;
							dataframe[0].roll = 0;
							for (int c = 0; c<3; c++)
								for (int d = 0; d<3; d++)
									dataframe[0].anglematrix[c][d] = 0;
							dataframe[0].vel = 0;
							dataframe[0].time = SDL_GetTicks();
							dataframe[0].ValidInput = 1;
						}
					}
				}

				else //if( event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0 )
				{
					Target.key = *SDL_GetKeyName(event.key.keysym.sym);  //(char)event.key.keysym.unicode;
					//std::cerr << Target.flag << std::endl;
				}
			}
			else if (event.type == SDL_KEYUP)
			{
				Target.key = '0';
			}
			else if (event.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		if ((CurTrial >= NTRIALS) && (state == Finished) && (trialTimer->Elapsed() >= 10000))
			quit = true;

		// Get data from input devices
		if (inputs_updated > 0) // if there is a new frame of data
		{

			//updatedisplay = true;
			for (int a = ((trackstatus>0) ? 1 : 0); a <= ((trackstatus>0) ? BIRDCOUNT : 0); a++)
			{
				if (dataframe[a].ValidInput)
				{
					curs[a]->UpdatePos(float(dataframe[a].x),float(dataframe[a].y),float(dataframe[a].z));
					dataframe[a].vel = curs[a]->GetVel3D();

					if (recordData)  //only write out if we need to
						writer->Record(a, dataframe[a], Target);
					else
						Target.starttime = dataframe[a].time;
				}
			}

		} //end if inputs updated

		if (flagoffsetkey == 0) //set the offsets, if requested
		{
			if (trackstatus > 0)
			{
				sysconfig.PosOffset[0] = dataframe[HAND].x;
				sysconfig.PosOffset[1] = dataframe[HAND].y;
				sysconfig.PosOffset[2] = dataframe[HAND].z;
				std::cerr << "Data Pos Offsets set: " << sysconfig.PosOffset[0] << " , " << sysconfig.PosOffset[1] << " , "<< sysconfig.PosOffset[2] << std::endl;
			}
			else
				std::cerr << "Data Pos Requested but mouse is being used, request is ignored." << std::endl;
			flagoffsetkey = -1;
			Target.key = ' ';

		}
		else if (flagoffsetkey == 1) //set the startCircle position
		{	
			if (trackstatus > 0)
			{
				//startCircle->SetPos(curs[HAND]->GetMeanX(),curs[HAND]->GetMeanY(),curs[HAND]->GetMeanZ());
				startCircle->SetPos(curs[HAND]->GetX(),curs[HAND]->GetY(),curs[HAND]->GetZ());
				//std::cerr << "Curs " << HAND << ": " << curs[HAND]->GetX() << " , " << curs[HAND]->GetY() << " , " << curs[HAND]->GetZ() << std::endl;
			}
			else //mouse mode
			{
				//startCircle->SetPos(curs[0]->GetMeanX(),curs[0]->GetMeanY(),0.0f);
				startCircle->SetPos(curs[0]->GetX(),curs[0]->GetY(),0.0f);
				//std::cerr << "Curs0: " << curs[0]->GetX() << " , " << curs[0]->GetY() << " , " << curs[0]->GetZ() << std::endl;
			}

			//std::cerr << "Player: " << player->GetX() << " , " << player->GetY() << " , " << player->GetZ() << std::endl;
			std::cerr << "StartPos Offsets set: " << startCircle->GetX() << " , " << startCircle->GetY() << " , "<< startCircle->GetZ() << std::endl;

			flagoffsetkey = -1;
			Target.key = ' ';
		}

		if (!quit)
		{

			game_update(); // Run the game loop (state machine update)

			draw_screen();
		}

	}

	std::cerr << "Exiting..." << std::endl;

	clean_up();
	return 0;
}



//function to read in the name of the trial table file, and then load that trial table
int LoadTrFile(char *fname)
{

	//std::cerr << "LoadTrFile begin." << std::endl;

	char tmpline[100] = ""; 
	int ntrials = 0;

	//read in the trial file name
	std::ifstream trfile(fname);

	if (!trfile)
	{
		std::cerr << "Cannot open input file." << std::endl;
		return(-1);
	}
	else
		std::cerr << "Opened TrialFile " << TRIALFILE << std::endl;

	trfile.getline(tmpline,sizeof(tmpline),'\n');  //get the first line of the file, which is the name of the trial-table file

	while(!trfile.eof())
	{
		sscanf(tmpline, "%d %d %d %d %d %d %d %d %d %d %d %d", 
			&trtbl[ntrials].trialType,
			&trtbl[ntrials].video,
			&trtbl[ntrials].dttype,
			&trtbl[ntrials].dtpori,
			&trtbl[ntrials].dt0,
			&trtbl[ntrials].dt1,
			&trtbl[ntrials].dt2,
			&trtbl[ntrials].dtcorrectresp,
			&trtbl[ntrials].dtstimdur,
			&trtbl[ntrials].dtrespdur,
			&trtbl[ntrials].srdur,
			&trtbl[ntrials].trdur);

		ntrials++;
		trfile.getline(tmpline,sizeof(tmpline),'\n');
	}

	trfile.close();
	if(ntrials == 0)
	{
		std::cerr << "Empty input file." << std::endl;
		//exit(1);
		return(-1);
	}
	return ntrials;
}


//function to read in the text strings and transform them into text images for display
int LoadContextFile(char *fname,Image* cText[])
{

	char tmpline[100] = ""; 
	int nlines = 0;

	//read in the trial file name
	std::ifstream trfile(fname);

	if (!trfile)
	{
		std::cerr << "Cannot open input file." << std::endl;
		return(-1);
	}
	else
		std::cerr << "Opened ContextFile " << fname << std::endl;

	trfile.getline(tmpline,sizeof(tmpline),'\n');  //get the first line of the file, which is the name of the trial-table file

	std::string textline;

	while(!trfile.eof())
	{
		textline.assign("");
		textline.assign(tmpline);
		cText[nlines] = Image::ImageText(cText[nlines],textline.c_str(),"arial.ttf", 48,textColor);
		cText[nlines]->Off();
		nlines++;
		trfile.getline(tmpline,sizeof(tmpline),'\n');
		std::cerr << "  Loaded context text: " << textline.c_str() << std::endl;
	}

	trfile.close();
	if(nlines == 0)
	{
		std::cerr << "Empty context file." << std::endl;
		return(-1);
	}
	return nlines;
}


int VidLoad(const char* fname)
{

	//call to (re)load a new video
	int errcode;

	if (Vid == NULL)
		Vid = new Video(fname,SCREEN_WIDTH/2,SCREEN_HEIGHT/2,VIDEO_WIDTH,VIDEO_HEIGHT,&errcode);
	else
		errcode = Vid->LoadNewVid(fname);

	if (errcode != 0)
	{
		std::cerr << "Video " << NVid << " did not load." << std::endl;
		Vid->SetValidStatus(0);
		NVid = -1;
	}		
	else
	{
		std::cerr << "   Video " << NVid << " loaded." << std::endl;
		std::cerr << "      Vid_size: " << VIDEO_WIDTH << "x" << VIDEO_HEIGHT << std::endl;
	}

	return(NVid);
}


//initialization function - set up the experimental environment and load all relevant parameters/files
bool init()
{

	int a;
	char tmpstr[80];
	char fname[50] = TRIALFILE;


	//std::cerr << "Start init." << std::endl;

	std::cerr << std::endl;
	std::cout << "Initializing the tracker... " ;

	Target.starttime = 0;
	trackstatus = TrackBird::InitializeBird(&sysconfig);
	if (trackstatus <= 0)
	{
		std::cerr << "Tracker failed to initialize. Mouse Mode." << std::endl;
		std::cout << "failed. Switching to mouse mode." << std::endl;
	}
	else
		std::cout << "completed" << std::endl;

	std::cerr << std::endl;

	std::cout << "Initializing SDL and loading files... ";

	// Initialize SDL, OpenGL, SDL_mixer, and SDL_ttf
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "SDL failed to intialize."  << std::endl;
		return false;
	}
	else

		std::cerr << "SDL initialized." << std::endl;


	//screen = SDL_CreateWindow("Code Base SDL2",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | (WINDOWED ? 0 : SDL_WINDOW_FULLSCREEN)); //SCREEN_BPP,
	screen = SDL_CreateWindow("Code Base SDL2",
		//SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
		(WINDOWED ? SDL_WINDOWPOS_UNDEFINED : SDL_WINDOWPOS_CENTERED),(WINDOWED ? SDL_WINDOWPOS_UNDEFINED : SDL_WINDOWPOS_CENTERED),
		SCREEN_WIDTH, SCREEN_HEIGHT, 
		SDL_WINDOW_OPENGL | (WINDOWED ? 0 : (SDL_WINDOW_MAXIMIZED) ) ); //SCREEN_BPP, //
	//note, the fullscreen option doesn't play nicely with the video window, so we will make a "fake" fullscreen which is simply a maximized window. 
	//Also, "borderless" doesn't play nicely either so we can't use that option, we just have to make the window large enough that the border falls outside the window.
	//To get full screen (overlaying the taskbar), we need to set the taskbar to auto-hide so that it stays offscreen. Otherwise it has priority and will be shown on top of the window.
	if (screen == NULL)
	{
		std::cerr << "Screen failed to build." << std::endl;
		return false;
	}
	else
	{
		if (!WINDOWED)
		{
			//SDL_SetWindowBordered(screen, SDL_FALSE);
			SDL_Rect usable_bounds;
			int display_index = SDL_GetWindowDisplayIndex(screen);
			if (0 != SDL_GetDisplayUsableBounds(display_index, &usable_bounds)) 
				std::cerr << "SDL error getting usable screen bounds." << std::endl;

			SDL_SetWindowPosition(screen, usable_bounds.x, usable_bounds.y);
			SDL_SetWindowSize(screen, usable_bounds.w, usable_bounds.h);
		}
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

		//create OpenGL context
		glcontext = SDL_GL_CreateContext(screen);
		std::cerr << "Screen built." << std::endl;
	}


	SDL_GL_SetSwapInterval(0); //ask for immediate updates rather than syncing to vertical retrace

	setup_opengl();

	//a = Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 512);  //initialize SDL_mixer
	a = Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 2048);  //initialize SDL_mixer, may have to play with the chunksize parameter to tune this a bit
	if (a != 0)
	{
		std::cerr << "Audio failed to initialize." << std::endl;
		return false;
	}
	else
		std::cerr << "Audio initialized." << std::endl;


	//initialize SDL_TTF (text handling)
	if (TTF_Init() == -1)
	{
		std::cerr << "SDL_TTF failed to initialize." << std::endl;
		return false;
	}
	else
		std::cerr << "SDL_TTF initialized." << std::endl;


	std::cerr << std::endl;


	//load trial table from file
	NTRIALS = LoadTrFile(fname);
	//std::cerr << "Filename: " << fname << std::endl;

	if(NTRIALS == -1)
	{
		std::cerr << "Trial File did not load." << std::endl;
		return false;
	}
	else
		std::cerr << "Trial File loaded: " << NTRIALS << " trials found." << std::endl;

	//assign the data-output file name based on the trial-table name 
	std::string savfile;
	savfile.assign(fname);
	savfile.insert(savfile.rfind("."),"_data");

	std::strcpy(fname,savfile.c_str());

	std::cerr << "SavFileName: " << fname << std::endl;

	//writer = new DataWriter(&sysconfig,Target,"InstructFile");  //set up the data-output file
	//recordData = true;
	didrecord = false;
	recordData = false;


	// Load files and initialize pointers

	//initialize the video player and the video file
	std::stringstream vidfile;
	vidfile.str(std::string());
	NVid = trtbl[0].video;
	if (NVid >= 0)
	{
		vidfile << "Video" << NVid << ".mp4";
		VidLoad(vidfile.str().c_str());
	}
	else
	{
		vidfile << "Video0.mp4";
		VidLoad(vidfile.str().c_str());
	}


	//load dual-task images (1 copy of each)
	NImages = 0;
	Image* dtimages[NIMAGES];
	if (trtbl[0].dttype == 1)
	{
		//postural dual task

		for (a = 0; a < NIMAGES1; a++)
		{
			sprintf(tmpstr,"%s/Image%d.png",IMAGEPATH1,a);
			dtimages[a] = Image::LoadFromFile(tmpstr);
			if (dtimages[a] == NULL)
				std::cerr << "Image Trace" << a << " did not load." << std::endl;
			else
			{
				dualtaskimages[a] = new Object2D(dtimages[a]);
				std::cerr << "   Image " << a << " loaded." << std::endl;
				dualtaskimages[a]->SetPos(PHYSICAL_WIDTH / 2, PHYSICAL_HEIGHT / 2);
			}
		}
		NImages = NIMAGES1;
	}
	else if (trtbl[0].dttype == 2)
	{
		//trajectory dual task

		for (a = 0; a < NIMAGES2; a++)
		{
			sprintf(tmpstr,"%s/Image%d.png",IMAGEPATH2,a);
			dtimages[a] = Image::LoadFromFile(tmpstr);
			if (dtimages[a] == NULL)
				std::cerr << "Image Trace" << a << " did not load." << std::endl;
			else
			{
				dualtaskimages[a] = new Object2D(dtimages[a]);
				std::cerr << "   Image " << a << " loaded." << std::endl;
				dualtaskimages[a]->SetPos(PHYSICAL_WIDTH / 2, PHYSICAL_HEIGHT / 2);
			}
		}
		NImages = NIMAGES2;

	}
	else if (trtbl[0].dttype == 3)
	{
		//control dual task

		for (a = 0; a < NIMAGES3; a++)
		{
			sprintf(tmpstr,"%s/Image%d.png",IMAGEPATH3,a);
			dtimages[a] = Image::LoadFromFile(tmpstr);
			if (dtimages[a] == NULL)
				std::cerr << "Image Trace" << a << " did not load." << std::endl;
			else
			{
				dualtaskimages[a] = new Object2D(dtimages[a]);
				std::cerr << "   Image " << a << " loaded." << std::endl;
				dualtaskimages[a]->SetPos(PHYSICAL_WIDTH / 2, PHYSICAL_HEIGHT / 2);
			}
		}
		NImages = NIMAGES3;

	}

	std::cerr << "Images loaded." << std::endl;

	selectbox.SetNSides(4);
	selectbox.SetRegionCenter(PHYSICAL_WIDTH/2.0f,PHYSICAL_HEIGHT/2.0f);
	if (trtbl[0].trialType != 0)
		selectbox.SetCenteredRectDims(dualtaskimages[0]->GetWidth()*1.25,dualtaskimages[0]->GetHeight()*1.25);
	else
		selectbox.SetCenteredRectDims(PHYSICAL_WIDTH/5.0f,PHYSICAL_HEIGHT/5.0f);
	selectbox.SetRegionColor(blueColor);
	selectbox.BorderOff();
	selectbox.Off();


	//set up the start position
	startCircle = new Circle(PHYSICAL_WIDTH/2.0f, PHYSICAL_HEIGHT/2.0f, 0.0f, START_RADIUS*2, blkColor);
	startCircle->SetBorderWidth(0.001f);
	startCircle->SetBorderColor(blkColor);
	startCircle->BorderOff();
	startCircle->Off();
	//startCircle->On();
	std::cerr << "Start Circle: " << startCircle->GetX() << " , " << startCircle->GetY() << " : " << startCircle->drawState() << std::endl;

	std::cerr << std::endl;



	// set up the cursors
	if (trackstatus > 0)
	{
		/* Assign birds to the same indices of controller and cursor that they use
		* for the Flock of Birds
		*/
		for (a = 1; a <= BIRDCOUNT; a++)
		{
			curs[a] = new HandCursor(0.0f, 0.0f, CURSOR_RADIUS*2, cursColor);
			curs[a]->BorderOff();
			curs[a]->SetOrigin(0.0f, 0.0f);
		}

		player = curs[HAND];  //this is the cursor that represents the hand
		std::cerr << "Player = " << HAND << std::endl;
	}
	else
	{
		// Use mouse control
		curs[0] = new HandCursor(0.0f, 0.0f, CURSOR_RADIUS*2, cursColor);
		curs[0]->SetOrigin(0.0f, 0.0f);
		player = curs[0];
		std::cerr << "Player = 0"  << std::endl;
	}

	//player->On();
	player->Off();

	if (trackstatus>0)
		SDL_ShowCursor(0);  	//hide the computer mouse cursor if we aren't tracking the mouse!


	//load sound files
	startbeep = new Sound("Resources/beep.wav");
	dtrespbeep = new Sound("Resources/doublebeep.wav");
	errorbeep = new Sound("Resources/errorbeep1.wav");
	correctbeep = new Sound("Resources/coin.wav");

	//set up placeholder text
	endtext = Image::ImageText(endtext, "Block ended.","arial.ttf", 28, textColor);
	endtext->Off();

	readytext = Image::ImageText(readytext, "Get ready...","arial.ttf", 28, textColor);
	readytext->Off();
	stoptext = Image::ImageText(stoptext, "STOP!","arial.ttf", 32, textColor);
	stoptext->Off();
	holdtext = Image::ImageText(holdtext, "Wait until the go signal!","arial.ttf", 28, textColor);
	holdtext->Off();
	proceedtext = Image::ImageText(proceedtext, "Zero the tracker and proceed when ready.","arial.ttf", 28, textColor);
	proceedtext->Off();
	returntext = Image::ImageText(returntext, "Please return to start position.","arial.ttf", 28, textColor);
	returntext->Off();

	selecttext = Image::ImageText(selecttext, "Selection recorded.","arial.ttf", 12, textColor);
	selecttext->Off();
	imitatetext = Image::ImageText(imitatetext, "Imitate!","arial.ttf", 36, textColor);
	imitatetext->Off();

	tooslowtext = Image::ImageText(tooslowtext, "Respond sooner!","arial.ttf", 28, textColor);
	tooslowtext->Off();

	trialinstructtext = Image::ImageText(trialinstructtext, "Press (space) to advance, (r) to repeat, or (s) to skip trial.","arial.ttf", 12, textColor);
	trialinstructtext->Off();
	redotext = Image::ImageText(redotext, "Press (r) to repeat or (s) to skip trial.","arial.ttf", 12, textColor);
	redotext->Off();
	recordtext = Image::ImageText(recordtext, "Recording...","arial.ttf", 12, textColor);
	recordtext->Off();
	mousetext = Image::ImageText(mousetext, "Trackers not found! Mouse-emulation mode.","arial.ttf", 12, textColor);
	if (trackstatus > 0)
		mousetext->Off();
	else
		mousetext->On();

	//set up trial number text image
	trialnum = Image::ImageText(trialnum,"0_0","arial.ttf", 12,textColor);
	trialnum->On();

	hoverTimer = new Timer();
	trialTimer = new Timer();
	movTimer = new Timer();
	dtTimer = new Timer();

	// Set the initial game state
	state = Idle; 

	std::cerr << "Initialization complete." << std::endl;
	std::cout << "completed." << std::endl;

	return true;
}


static void setup_opengl()
{
	glClearColor(1, 1, 1, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* The default coordinate system has (0, 0) at the bottom left. Width and
	* height are in meters, defined by PHYSICAL_WIDTH and PHYSICAL_HEIGHT
	* (config.h). If MIRRORED (config.h) is set to true, everything is flipped
	* horizontally.
	*/
	glOrtho(MIRRORED ? PHYSICAL_WIDTH : 0, MIRRORED ? 0 : PHYSICAL_WIDTH,
		0, PHYSICAL_HEIGHT, -1.0f, 1.0f);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);

}


//end the program; clean up everything neatly.
void clean_up()
{

	SDL_ShowCursor(1);

	std::cout << "Shutting down." << std::endl;

	delete startbeep;
	delete dtrespbeep;
	delete errorbeep;
	delete correctbeep;

	for (int a = 0; a < NImages; a++)
	{
		delete dualtaskimages[a];
	}


	delete endtext;
	delete readytext;
	delete stoptext;
	delete holdtext;
	delete proceedtext;
	delete returntext;
	delete selecttext;
	delete trialinstructtext;
	delete redotext;
	delete recordtext;
	delete mousetext;
	delete imitatetext;
	delete trialnum;

	Vid->CleanUp();
	delete Vid;

	//std::cerr << "Deleted all objects." << std::endl;

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(screen);
	Mix_CloseAudio();
	TTF_Quit();
	SDL_Quit();

	std::cerr << "Shut down SDL." << std::endl;

	if (trackstatus > 0)
	{
		TrackBird::ShutDownBird(&sysconfig);
		std::cerr << "Shut down tracker." << std::endl;
	}

	freopen( "CON", "w", stderr );

}

//control what is drawn to the screen
static void draw_screen()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	selectbox.Draw();

	//draw the image specified
	for (int a = 0; a < NImages; a++)
		dualtaskimages[a]->Draw();

	startCircle->Draw();

	player->Draw();

	if (NVid >= 0)
		Vid->Update();

	// Draw text
	endtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);

	proceedtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	readytext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	stoptext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	holdtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);
	returntext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*1.0f/2.0f);
	selecttext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*0.5f/24.0f);
	imitatetext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*3.0f/5.0f);
	tooslowtext->Draw(float(PHYSICAL_WIDTH)/2.0f,float(PHYSICAL_HEIGHT)*2.0f/3.0f);

	trialinstructtext->DrawAlign(PHYSICAL_WIDTH*0.5f/24.0f,PHYSICAL_HEIGHT*0.5f/24.0f,3);
	redotext->DrawAlign(PHYSICAL_WIDTH*0.5f/24.0f,PHYSICAL_HEIGHT*0.5f/24.0f,3);
	recordtext->DrawAlign(PHYSICAL_WIDTH*0.5f/24.0f,PHYSICAL_HEIGHT*0.5f/24.0f,3);
	mousetext->DrawAlign(PHYSICAL_WIDTH*23.5f/24.0f,PHYSICAL_HEIGHT*23.5f/24.0f,1);
	//write the trial number
	trialnum->Draw(PHYSICAL_WIDTH*23.0f/24.0f, PHYSICAL_HEIGHT*0.5f/24.0f);

	SDL_GL_SwapWindow(screen);
	glFlush();

}


//game update loop - state machine controlling the status of the experiment
bool mvtStarted = false;
bool falsestart = false;
bool trialabort = false;
bool vidEnded = false;
bool vidEndedAck = false;
bool donePlayingContext = false;
bool didresp = false;

bool reachedvelmin = false;
bool reachedvelmax = false;

bool mvmtEnded = false;
bool hitTarget = false;
bool hitRegion = false;
bool hitPath = false;

float LastPeakVel = 0;
bool returntostart = true;

bool writefinalscore;

bool loadVid;

void game_update()
{

	switch (state)
	{
	case Idle:
		// This state only happens at the start of the experiment, and is just a null pause until the experimenter is ready to begin.

		Target.trial = -1;
		Target.instruct = -1;
		Target.lat = 0;
		Target.redo = -1;
		Target.trial = -1;
		Target.vidstatus = -1;
		Target.video = -1;
		Target.TrType = trtbl[0].trialType;
		Target.dtlat = 0;

		Target.dttype = -1;
		Target.dtpori = -1;
		Target.dtstim[0] = -1;
		Target.dtstim[1] = -1;
		Target.dtstim[2] = -1;

		endtext->Off();
		readytext->Off();
		stoptext->Off();
		trialinstructtext->Off();
		holdtext->Off();
		recordtext->Off();
		proceedtext->On();
		imitatetext->Off();

		loadVid = false;
		trialabort = false;

		if (!returntostart)
		{
			//if we haven't yet gotten back to the start target yet
			if (player->Distance(startCircle) < START_RADIUS)
				returntostart = true;
		}

		//shut off all images
		for (int a = 0; a < NImages; a++)
			dualtaskimages[a]->Off();

		if (NVid >= 0 && Vid->VisibleState() != 0)
			Vid->Invisible();

		if( returntostart && nextstateflag)  //hand is in the home position and the experimenter asked to advance the experiment
		{
			proceedtext->Off();
			returntext->Off();

			nextstateflag = false;

			redotrialflag = false;
			skiptrialflag = false;

			falsestart = false;
			mvtStarted = false;

			Target.key = ' ';

			hoverTimer->Reset();
			trialTimer->Reset();

			std::cerr << "Leaving IDLE state." << std::endl;

			state = WaitStim;
		}
		break;


	case WaitStim:
		//this state is just a "pause" state between trials.

		trialnum->On();

		returntext->Off();
		Target.video = -1;
		Target.vidstatus = -1;

		Target.dttype = -1;
		Target.dtpori = -1;
		Target.dtstim[0] = -1;
		Target.dtstim[1] = -1;
		Target.dtstim[2] = -1;
		Target.dtlat = 0;
		Target.dtcorr = -1;
		Target.dtresp = -1;
		Target.go = 0;

		vidEnded = false;
		vidEndedAck = false;
		donePlayingContext = false;

		//make sure all the dual-task images are hidden
		for (int a = 0; a < NImages; a++)
			dualtaskimages[a]->Off();

		//show "pause" text, and instructions to experimenter at the bottom of the screen
		readytext->On();
		holdtext->Off();
		selecttext->Off();
		imitatetext->Off();
		if (falsestart && hoverTimer->Elapsed() < 1000)
		{
			readytext->Off();
			if (!trialabort)
				holdtext->On();
		}

		if (!falsestart)
			trialinstructtext->On();
		else
			redotext->On();

		if ((curtr.trialType == 2 && !falsestart) || (curtr.trialType == 2 && falsestart && hoverTimer->Elapsed() >= 1000) )
			nextstateflag = true;  // we'll just jump ahead if we're in the dual-task only condition!

		if (hoverTimer->Elapsed() > 1000 && (nextstateflag || redotrialflag || skiptrialflag) )
		{

			if (skiptrialflag || (!falsestart && (nextstateflag || (redotrialflag && CurTrial < 0) ))) //if accidentally hit "r" before the first trial, ignore this error!  //if false start but want to just move on, can use skip trial option
			{
				nextstateflag = false;
				redotrialflag = false;
				skiptrialflag = false;
				falsestart = false;
				numredos = 0;
				CurTrial++;  //we started the experiment with CurTrial = -1, so now we are on the "next" trial (or first trial)
				Target.trial = CurTrial+1;
				Target.redo = 0;

				Target.key = ' ';
			}
			else if ( (!falsestart && (redotrialflag && CurTrial >= 0)) ||  (falsestart && (nextstateflag || redotrialflag)) )
			{
				redotrialflag = false;
				falsestart = false;
				numredos++;
				Target.trial = CurTrial+1; //we do not update the trial number
				Target.video = curtr.video;
				Target.redo = numredos; //count the number of redos
				Target.key = ' ';
			}

			if (curtr.video >= 0 && NVid == curtr.video && Vid->IsValid())
			{
				//video is already loaded successfully, do nothing
				std::cerr << "Video " << NVid << " already loaded." << std::endl;
				//Vid->ResetVid();  //make sure the video is reset so it can be played
			}
			else if (curtr.video >= 0)
			{
				std::stringstream vidfile;
				vidfile.str(std::string());
				NVid = curtr.video;
				vidfile << vpathbase.c_str() <<  "Video" << NVid << ".mp4";
				VidLoad(vidfile.str().c_str());
			}
			else
				NVid = curtr.video;

			Target.vidstatus = 0;
			vidEnded = false;
			vidEndedAck = false;

			readytext->Off();
			redotext->Off();
			holdtext->Off();
			trialinstructtext->Off();

			trialabort = false;

			//if we have reached the end of the trial table, quit
			if (CurTrial >= NTRIALS)
			{
				std::stringstream texttn;
				texttn << "You earned " << score << " points!"; 
				endtext = Image::ImageText(endtext,texttn.str().c_str(),"arial.ttf", 28,textColor);

				std::cerr << "Going to FINISHED state." << std::endl;
				trialTimer->Reset();
				writer->Close();
				state = Finished;
			}
			else
			{
				std::stringstream texttn;
				texttn << CurTrial+1 << "_" << numredos+1;  //CurTrial starts from 0, so we add 1 for convention.
				trialnum = Image::ImageText(trialnum,texttn.str().c_str(),"arial.ttf", 12,textColor);
				std::cerr << "Trial " << CurTrial+1 << " Redo " << numredos+1 << " started at " << SDL_GetTicks() << std::endl;

				//set up the new data file and start recording
				recordData = true;
				recordtext->On();
				//Target.starttime = SDL_GetTicks();

				int createrecord = false;
				if (!didrecord)  //detect if a file has ever been opened; if so shut it. if not, skip this.
				{
					didrecord = true;
					createrecord = true;
				}
				else if (curtr.trialType != 2)  //if we are doing imitation create a new file for each trial, otherwise just put it all in one file
				{
					writer->Close();  //this will this cause an error if no writer is open
					createrecord = true;
				}

				if (createrecord)
				{
					std::string savfname;
					savfname.assign(TRIALFILE);
					savfname = savfname.substr(savfname.rfind("/")+1);  //cut off the file extension
					savfname = savfname.replace(savfname.rfind(".txt"),4,"");  //cut off the file extension
					std::stringstream datafname;
					//datafname << savfname.c_str() << "_" << SUBJECT_ID << "_Item" << curtr.item << "-" << numredos+1 << "_" << (curtr.vision == 1 ? "VF" : "NVF");
					datafname << DATAPATH << savfname.c_str() << "_" << SUBJECT_ID << "_TR" << CurTrial+1 << "-" << numredos+1 << "_Vid" << curtr.video ;

					writer = new DataWriter(&sysconfig,Target,datafname.str().c_str());  //create the data-output file
				}

				//reset the trial timers
				hoverTimer->Reset();
				trialTimer->Reset();

				didresp = false;
				subjresp = -1;

				mvtStarted = false;
				mvmtEnded = false;

				Target.video = curtr.video;

				//play the video if correct trial type and the video is valid
				if (curtr.trialType != 2 && NVid >= 0)
				{
					Vid->Play();
					Target.vidstatus = 1;
					state = ShowVideo;

					std::cerr << "Leaving WAITSTATE state to ShowVideo state." << std::endl;

				}
				else if (curtr.trialType == 2)
				{
					//skip the video and only do the dual task
					dualtaskimages[curtr.dt0]->SetPos(PHYSICAL_WIDTH/2,PHYSICAL_HEIGHT*2/3);
					dualtaskimages[curtr.dt0]->On();

					dualtaskimages[curtr.dt1]->SetPos(PHYSICAL_WIDTH*1/3,PHYSICAL_HEIGHT*1/3);
					dualtaskimages[curtr.dt1]->On();

					dualtaskimages[curtr.dt2]->SetPos(PHYSICAL_WIDTH*2/3,PHYSICAL_HEIGHT*1/3);
					dualtaskimages[curtr.dt2]->On();

					Target.dttype = curtr.dttype;
					Target.dtpori = curtr.dtpori;
					Target.dtstim[0] = curtr.dt0;
					Target.dtstim[1] = curtr.dt1;
					Target.dtstim[2] = curtr.dt2;

					nextstateflag = false;
					hoverTimer->Reset();
					trialTimer->Reset();
					dtTimer->Reset();

					subjresp = -1;

					state = ShowDTStim;
					std::cerr << "Leaving WAITSTATE state to ShowDTstim State." << std::endl;
				}

			}

		}
		break;

	case ShowVideo:
		//show the stimulus and wait

		if (!mvtStarted && (player->Distance(startCircle) > START_RADIUS))
		{	//detected movement too early; 
			//std::cerr << "Player: " << player->GetX() << " , " << player->GetY() << " , " << player->GetZ() << std::endl;
			//std::cerr << "Circle: " << startCircle->GetX() << " , " << startCircle->GetY() << " , " << startCircle->GetZ() << std::endl;
			//std::cerr << "Distance: " << player->Distance(startCircle) << std::endl;
			mvtStarted = true;
			Vid->Stop();
			Vid->Invisible();
			Target.vidstatus = 0;
			holdtext->On();
			hoverTimer->Reset();
		}

		if (mvtStarted && !falsestart && (hoverTimer->Elapsed() > 1000) )  //moved too soon!
		{
			//state = WaitStim;
			Vid->ResetVid();
			Target.vidstatus = 0;
			returntext->On();
			nextstateflag = false;
			redotrialflag = false;
			falsestart = true;
			Target.key = ' ';
			trialTimer->Reset();
		}

		if (falsestart && (player->Distance(startCircle) < START_RADIUS)) //(falsestart && (trialTimer->Elapsed() > 1000))
		{
			nextstateflag = false;
			redotrialflag = false;
			Target.key = ' ';
			recordtext->Off();
			hoverTimer->Reset();
			state = WaitStim;
			std::cerr << "False start; returning to WAITSTIM state." << std::endl;
		}

		if (!vidEnded && (Vid->HasEnded() == 1))
		{
			vidEnded = true;
			hoverTimer->Reset();
		}


		//video has finished playing, we can shut it off and flag ready to proceed
		if (!falsestart && vidEnded && !vidEndedAck && (hoverTimer->Elapsed() > 500) )
		{
			vidEndedAck = true;
			std::cerr << "Vid finished playing. " << std::endl;
			trialTimer->Reset();
			Target.vidstatus = 2;
			Vid->Stop();
			Vid->ResetVid();

			Target.vidstatus = 2;

			//std::cerr << mvtStarted << " " << vidEnded << " " << trialTimer->Elapsed() << " " << curtr.srdur << std::endl;
		}


		//if video finished playing and we are ready to move on...
		if (!mvtStarted && vidEndedAck && (trialTimer->Elapsed() > curtr.srdur) )  //prompt start signal
		{

			//turn off the video, we don't need it again
			Vid->Invisible();
			Target.vidstatus = 0;

			if (curtr.trialType == 1)
			{
				//go to dual task state
				dualtaskimages[curtr.dt0]->SetPos(PHYSICAL_WIDTH/2,PHYSICAL_HEIGHT*2/3);
				dualtaskimages[curtr.dt0]->On();

				dualtaskimages[curtr.dt1]->SetPos(PHYSICAL_WIDTH*1/3,PHYSICAL_HEIGHT*1/3);
				dualtaskimages[curtr.dt1]->On();

				dualtaskimages[curtr.dt2]->SetPos(PHYSICAL_WIDTH*2/3,PHYSICAL_HEIGHT*1/3);
				dualtaskimages[curtr.dt2]->On();

				Target.dttype = curtr.dttype;
				Target.dtpori = curtr.dtpori;
				Target.dtstim[0] = curtr.dt0;
				Target.dtstim[1] = curtr.dt1;
				Target.dtstim[2] = curtr.dt2;

				nextstateflag = false;
				hoverTimer->Reset();
				trialTimer->Reset();
				dtTimer->Reset();

				subjresp = -1;

				state = ShowDTStim;
				std::cerr << "Leaving SHOWVIDEO state to ShowDTstim State." << std::endl;
			}
			else
			{
				//no dual task, go directly to imitation state

				//play start tone
				Target.go = 1;
				startbeep->Play();
				mvtStarted = false;
				mvmtEnded = false;
				movTimer->Reset();
				hoverTimer->Reset();
				trialTimer->Reset();

				readytext->Off();
				holdtext->Off();
				selecttext->Off();

				imitatetext->On();

				subjresp = -1;
				didresp = false;

				nextstateflag = false;

				std::cerr << "Leaving ShowVideo state to Active State." << std::endl;
				state = Active;

			}

		}


		if (Target.key == 'a')
		{
			//abort the trial and go back to the WaitState

			Vid->Stop();
			Vid->Invisible();
			Vid->ResetVid();
			Target.vidstatus = 0;
			falsestart = true;
			Target.key = ' ';
			trialTimer->Reset();
			hoverTimer->Reset();
			recordtext->Off();

			readytext->Off();
			holdtext->Off();
			selecttext->Off();

			trialabort = true;

			state = EndTrial;
			std::cerr << "Trial aborted; returning to WAITSTIM state." << std::endl;
		
		}


		break;

	case ShowDTStim:

		//if the subject moves, cancel the trial
		if (!mvtStarted && (player->Distance(startCircle) > START_RADIUS))
		{	//detected movement too early; 
			mvtStarted = true;
			dualtaskimages[curtr.dt0]->Off();
			dualtaskimages[curtr.dt1]->Off();
			dualtaskimages[curtr.dt2]->Off();
			Target.dtstim[0] = -1;
			Target.dtstim[1] = -1;
			Target.dtstim[2] = -1;

			selectbox.Off();

			holdtext->On();
			movTimer->Reset();
		}
		if (mvtStarted && !falsestart && (movTimer->Elapsed() > 500) )  //moved too soon!
		{
			//state = WaitStim;
			returntext->On();
			nextstateflag = false;
			redotrialflag = false;
			falsestart = true;
			Target.key = ' ';
			trialTimer->Reset();
		}
		if (falsestart && (player->Distance(startCircle) < START_RADIUS)) //(falsestart && (trialTimer->Elapsed() > 1000))
		{
			nextstateflag = false;
			redotrialflag = false;
			Target.key = ' ';
			recordtext->Off();
			movTimer->Reset();
			state = WaitStim;
			std::cerr << "False start; returning to WAITSTIM state." << std::endl;
		}


		//if we waited longer than the stim duration, blank the display but wait for a response until respdur
		if (hoverTimer->Elapsed() > curtr.dtstimdur)
		{
			//shut off stimuli
			dualtaskimages[curtr.dt0]->Off();
			dualtaskimages[curtr.dt1]->Off();
			dualtaskimages[curtr.dt2]->Off();
			Target.dtstim[0] = -1;
			Target.dtstim[1] = -1;
			Target.dtstim[2] = -1;

			selectbox.Off();
		}

		if (!didresp && subjresp >= 0 && !(mvtStarted || falsestart))
		{
			Target.dtresp = subjresp;
			Target.dtlat = dtTimer->Elapsed();
			selecttext->On();

			if (subjresp == 1) //chose left
				selectbox.SetRegionCenter(PHYSICAL_WIDTH*1/3,PHYSICAL_HEIGHT*1/3);
			else //chose right
				selectbox.SetRegionCenter(PHYSICAL_WIDTH*2/3,PHYSICAL_HEIGHT*1/3);
			if (hoverTimer->Elapsed() <= curtr.dtstimdur)
				selectbox.On();  //show the select box if the stimuli are still on the screen

			if (subjresp == curtr.dtcorrectresp)
			{
				Target.dtcorr = 1;
				score += 1;
			}
			else
				Target.dtcorr = 0;

			didresp = true;
			//hoverTimer->Reset();

			//highlight selection to acknowledge input

			subjresp = -1;
		}


		if (hoverTimer->Elapsed() > curtr.dtrespdur && !(mvtStarted || falsestart))
		{
			
			//if we exceed the respdur time, regardless of whether we have an input or not, call it a failed trial and move on

			//if (!didresp)
			//	errorbeep->Play();
			//else
			
			selectbox.Off();

			if (curtr.trialType == 2)
			{
				//only doing the dual task; return to the wait state
				if (!didresp)
				{
					//no response yet; wait for a response to come in!

					tooslowtext->On();

					std::cerr << "Leaving ShowDT state to WaitDT state." << std::endl;

					//trialTimer->Reset();

					state = WaitDTResp;

				}
				else
				{
					subjresp = -1;

					mvtStarted = false;
					mvmtEnded = false;
					hoverTimer->Reset();
					trialTimer->Reset();
					movTimer->Reset();
					
					readytext->Off();
					holdtext->Off();
					selecttext->Off();

					std::cerr << "Leaving ShowDT state to EndTrial state." << std::endl;

					nextstateflag = false;
					
					state = EndTrial;
				}

			}
			else
			{

				startbeep->Play(); //play start tone
				Target.go = 1;
				imitatetext->On();
				mvtStarted = false;
				mvmtEnded = false;
				hoverTimer->Reset();
				trialTimer->Reset();
				movTimer->Reset();
					
				readytext->Off();
				holdtext->Off();
				selecttext->Off();

				std::cerr << "Leaving ShowDT state to Active state." << std::endl;

				nextstateflag = false;

				state = Active;
			}
		}

		if (Target.key == 'a')
		{
			//abort the trial and go back to the WaitState

			mvtStarted = true;
			dualtaskimages[curtr.dt0]->Off();
			dualtaskimages[curtr.dt1]->Off();
			dualtaskimages[curtr.dt2]->Off();
			Target.dtstim[0] = -1;
			Target.dtstim[1] = -1;
			Target.dtstim[2] = -1;

			selectbox.Off();
			readytext->Off();
			holdtext->Off();
			selecttext->Off();

			recordtext->Off();
			trialTimer->Reset();
			movTimer->Reset();
			hoverTimer->Reset();
			nextstateflag = false;
			redotrialflag = false;
			falsestart = true;
			Target.key = ' ';

			trialabort = true;

			state = WaitStim;
			std::cerr << "Trial aborted; returning to WAITSTIM state." << std::endl;
		
		}

		break;


	case WaitDTResp:
		//wait for a response before moving on

		if (!didresp && subjresp >= 0)
		{
			Target.dtresp = subjresp;
			Target.dtlat = dtTimer->Elapsed();
			selecttext->On();

			if (subjresp == curtr.dtcorrectresp)
			{
				Target.dtcorr = 1;
				score += 1;
			}
			else
				Target.dtcorr = 0;

			didresp = true;

			subjresp = -1;

			hoverTimer->Reset();
		}

		//end the trial, we got a response
		if (didresp && hoverTimer->Elapsed() > 500)
		{
			subjresp = -1;

			tooslowtext->Off();

			mvtStarted = false;
			mvmtEnded = false;
			hoverTimer->Reset();
			trialTimer->Reset();
			movTimer->Reset();
					
			readytext->Off();
			holdtext->Off();
			selecttext->Off();

			std::cerr << "Leaving WaitDT state to EndTrial state." << std::endl;

			nextstateflag = false;
				
			state = EndTrial;
		}

		if (Target.key == 'a')
		{
			//abort the trial and go back to the WaitState

			tooslowtext->Off();

			mvtStarted = false;
			mvmtEnded = false;
			nextstateflag = false;
			redotrialflag = false;
			hoverTimer->Reset();
			trialTimer->Reset();
			movTimer->Reset();
					
			readytext->Off();
			holdtext->Off();
			selecttext->Off();
			falsestart = true;
			Target.key = ' ';

			trialabort = true;

			state = WaitStim;
			std::cerr << "Trial aborted; returning to WAITSTIM state." << std::endl;
		
		}

		break;

	case Active:

		//check if there is a late response; record it but do not score it
		if (!didresp && subjresp >= 0)
		{
			Target.dtresp = subjresp;
			Target.dtlat = dtTimer->Elapsed();

			selecttext->On();

			if (subjresp == curtr.dtcorrectresp)
				Target.dtcorr = 1;
			else
				Target.dtcorr = 0;

			didresp = true;
			subjresp = -1;
		}

		//detect the onset of hand movement, for calculating latency
		if (!mvtStarted && (player->Distance(startCircle) > START_RADIUS))
		{
			mvtStarted = true;
			Target.lat = movTimer->Elapsed();
			movTimer->Reset();
		}

		//detect movement offset
		if (!mvmtEnded && mvtStarted && (player->GetVel3D() < VEL_MVT_TH) && (movTimer->Elapsed()>200))
		{
			mvmtEnded = true;
			//Target.dur = movTimer->Elapsed();
			hoverTimer->Reset();
			//std::cerr << "Mvmt Ended: " << float(SDL_GetTicks()) << std::endl;
		}

		if (mvmtEnded && (hoverTimer->Elapsed() >= VEL_END_TIME))
		{
			Target.dur = movTimer->Elapsed()-VEL_END_TIME;  //if the hand was still long enough, call this the "end" of the movement!
		}
		else if (mvmtEnded && (hoverTimer->Elapsed() < VEL_END_TIME) && (player->GetVel3D() > VEL_MVT_TH))
			mvmtEnded = false;  //just kidding, the movement is still going...

		//if trial duration is exceeded, display a "stop" signal but do not change state until experimenter prompts
		if (!nextstateflag && ((curtr.trdur > 0 && trialTimer->Elapsed() > curtr.trdur) || (trialTimer->Elapsed() > MAX_TRIAL_DURATION) ))
		{
			//Vid->Stop();
			//Vid->Invisible();
			//Target.vidstatus = 0;
			imitatetext->Off();
			stoptext->On();
		}

		//if the experimenter ends the trial
		if (nextstateflag)
		{
			nextstateflag = false;
			Target.key = ' ';

			if (!mvmtEnded)
			{
				//if the movement hasn't ended yet, we will call this the "duration"
				Target.dur = movTimer->Elapsed();
			}

			//Vid->Pause();
			//Vid->ResetVid();
			//Target.vidstatus = 0;

			returntostart = false;

			stoptext->Off();
			imitatetext->Off();

			//stop recording data
			recordtext->Off();
			recordData = false;

			//go to next state
			trialTimer->Reset();// = SDL_GetTicks();
			state = EndTrial;

		}

		if (Target.key == 'a')
		{
			//abort the trial and go back to the WaitState

			mvtStarted = false;
			mvmtEnded = false;
			nextstateflag = false;
			redotrialflag = false;
			hoverTimer->Reset();
			trialTimer->Reset();
			movTimer->Reset();
			
			recordtext->Off();
			readytext->Off();
			holdtext->Off();
			selecttext->Off();
			falsestart = true;
			Target.key = ' ';

			trialabort = true;

			state = WaitStim;
			std::cerr << "Trial aborted; returning to WAITSTIM state." << std::endl;
		
		}

		break;

	case EndTrial:

		returntext->On();
		selecttext->Off();

		Target.video = -1;

		subjresp = -1;
		didresp = false;

		if (player->Distance(startCircle) > START_RADIUS)  
			returntostart = false;
		else
			returntostart = true;

		//wait for participant to return to start before allowing entry into next trial
		if (returntostart)
		{
			returntext->Off();

			//make sure that all the videos have stopped playing and are reset (reset also makes them invisible)
			//Vid->Stop();
			//Vid->ResetVid();


			nextstateflag = false;
			std::cerr << "Ending Trial." << std::endl;
			state = WaitStim;

		}

		break;

	case Finished:
		// Trial table ended, wait for program to quit

		endtext->On();

		if (trialTimer->Elapsed() > 5000)
			quit = true;


		break;

	}
}

