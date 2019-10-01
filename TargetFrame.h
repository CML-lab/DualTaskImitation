#ifndef TGTFRAME_H
#define TGTFRAME_H
#pragma once

#include "SDL.h"

// Data type used to store trial data for data writer
struct TargetFrame
{
	int trial;
	int redo;
	double starttime;

	int video;
	int instruct;
	int vidstatus;
	int go;

	int TrType;

	int dttype;
	int dtpori;
	int dtstim[3];
	int dtresp;
	int dtcorr;
	Uint32 dtlat;

	char key;

	Uint32 lat;
	int dur;

};

#endif
