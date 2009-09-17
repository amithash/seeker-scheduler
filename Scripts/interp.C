/******************************************************************************\
 * FILE: interp.cpp
 * DESCRIPTION: This implements a linear spline interpolator and once the 
 * splines for the data is constructed, it resamples the data at regular
 * intervals of the primary column.
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

#include "Array.H"
#include <stdlib.h>
#include <string.h>
#include "iconfig.h"

//#define DEBUG 1

int main(int argc, char *argv[]){

	if(argc != 4){
		cout << "USAGE: ./interp /path/to/input/merged/file /path/to/output/file INTERVAL_LENGTH(Millions)" << endl;
		exit(1);
	}
	// Get the parameters.
	char input_file_name[200];
	char output_file_name[200];
	strcpy(input_file_name, argv[1]);
	strcpy(output_file_name, argv[2]);
	double interval = atof(argv[3]) * 1000000;

	// open files
	ifstream infile;
	ofstream outfile;
	infile.open(input_file_name);
	outfile.open(output_file_name);
	outfile.setf(ios_base::fixed);
	outfile.precision(12);

	// Array of Y's. as each Y is an array, this is an array of arrays.
	Array<Array<double> > Y(100000);
	// Array of X's
	Array<double> X(100000);

	// This is the currently read line.
	Array<double> Sample;

	// This is the currently read string
	char Sline[200];

	cout << "STAGE 1: Building data set. " << endl;

	// Build the X and Y structures.
	int first = 1;
	int k = 0;
	while(1){
		infile.getline(Sline,200);
		if(infile.eof()){
			break;
		}
		Sample.append(split<double>(INSEP, Sline));
		// cumilate the X's as these are just samples...
		if(first == 1){
			first = 0;
			X.fill(0,Sample.data(PRIMARY));
		}
		else{
			X.fill(k,X[k-1] + Sample.data(PRIMARY));
		}
		// Resize Y's -> using append will be easier,
		// But caused performance degradation.
		Y[k].resize(TOTAL_COLUMNS);
		// fill the Y's
		Y[k] = do_oper(Sample);
		Sample.empty();
		k++;

		// Increase the buffer size if required.
		if(k == X.length()){
			X.resize(X.length() + 100000);
			Y.resize(Y.length() + 100000);
		}
	}
	// resize X's and Y's to free up consumed space
	Y.resize(k);
	X.resize(k);

	// Build the splines.
	cout << "STAGE 2: Construction splines. " << endl;
	
	// allocate space for splines
	int spline_length = X.length() - 1;
	SPLINE * splines = new SPLINE[spline_length+1];
	splines[0].construct(0,X[0],0,Y[0]);
#ifdef DEBUG
	splines[0].print();
#endif
	for(int i=1;i<X.length();i++){
		splines[i].construct(X[i-1], X[i], Y[i-1], Y[i]);
#ifdef DEBUG
		splines[i].print();
#endif
	}

	cout << "STAGE 3: Prediction and data output " << endl;
	// Start prediction
	int last_index = 0;
	double end = X[-1];
	X.empty();
	Y.empty();

	Array<double> out;
	outfile << header << endl;
	for(double i=interval; i< end; i += interval){
		while(!splines[last_index].within(i)){
			last_index++;
		}
		out.append(i / 1000000);
		out.append(splines[last_index].predict(i));
		out.fprint(outfile,OUTSEP);
		out.empty();
	}
}

