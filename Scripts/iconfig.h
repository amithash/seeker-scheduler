/**************************************************************************
 * Copyright 2009 Amithash Prasad                                         *
 *                                                                        *
 * This file is part of Seeker                                            *
 *                                                                        *
 * Seeker is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/

#include "oper.h"


// ---------------------- START OF CONFIGURATION AREA ---------------------------

// This will be your output header
char header[200] = "INSTR,RE_IPC,RF_IPC,STATE";

// Never used this is for the users reference in filling out the next DS
#define INPUT_FORMAT "TOT_INST,TOT_RFCY,INT,INST,RFCY,CPU,IPC,STATE,REQ,GIV"

// The total number of columns in the output string... (Minus the primary leading column)
#define TOTAL_COLUMNS 3

// This is the X while all other columns are Y's. This is the one which is sampled
// evenly. NOTE: Count starts with 0. 
#define PRIMARY 3

// The separator for the input file.
#define INSEP " "

// The separator for the output file.
#define OUTSEP " "

// These are the operations to be performed on the data to get an interpolatable
// data. Each entry is IN_COLUMN_NUMBER_FOR_DATA1, OPERATION, IN_COLUMN_NUMBER_FOR_DATA2
// Example if inst is in 4 and cycle is in 5 then ipc with be filled as 
// {4,'/',5}. 
// And when you do not require an operation, then use ' ' as an operation and fill 0 to the
// next, example: {7,' ',0}
operation_t operations[TOTAL_COLUMNS] = {
	{6,' ',0},
	{3,'/',4},
	{7,' ',0}
};

#include "interp.h"

// ---------------------- END OF CONFIGURATION AREA -------------------------

