/**************************************************************************
 * Copyright 2008 Amithash Prasad                                         *
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
#include "Array.H"

typedef struct{
	int A;
	char oper;
	int B;
}operation_t;

// ---------------------- START OF CONFIGURATION AREA ---------------------------

// This will be your output header
char header[200] = "INSTR,RE_IPC,RF_IPC,TEMPR";

// Never used this is for the users reference in filling out the next DS
#define INPUT_FORMAT "TS_CY,INSTR,RE_CY,RF_CY,TEMP"

// The total number of columns in the output string... (Minus the primary leading column)
#define TOTAL_COLUMNS 3

#define PRIMARY 1

// These are the operations to be performed on the data to get an interpolatable
// data. Each entry is IN_COLUMN_NUMBER_FOR_DATA1, OPERATION, IN_COLUMN_NUMBER_FOR_DATA2
// Example if inst is in 4 and cycle is in 5 then ipc with be filled as 
// {4,'/',5}. 
// And when you do not require an operation, then use ' ' as an operation and fill 0 to the
// next, example: {7,' ',0}
operation_t operations[TOTAL_COLUMNS] = {
	{1,'/',2},
	{1,'/',3},
	{4,' ',0}
};

// ---------------------- END OF CONFIGURATION AREA -------------------------

template <class T>
T oper(T A, T B, char oper){
	T ret;
	switch(oper){
		case '+':
			ret = A + B;
			break;
		case '-':
			ret = A - B;
			break;
		case '*':
			ret = A * B;
			break;
		case '/':
			ret = A / B;
			break;
		case '^':
			ret = (T)pow((double)A,(double)B);
			break;
		default:
			ret = A;
			break;
	}
	return ret;
}

template <class T>
Array<T> do_oper(Array<T> A){
	Array<T> ret(TOTAL_COLUMNS);
	for(int i=0; i<TOTAL_COLUMNS;i++){
		ret[i] = oper<T>(A.data(operations[i].A), A.data(operations[i].B),operations[i].oper);
	}
	return ret;
}

// This is the linear spline class.
class SPLINE{
	private:
		// Declaring an array allows us to do just a vector operation for 
		// different data like y1 = f(x), y2 = g(x) etc.. thus interpolating
		// all y1->k in 1 step.(As the same Xi is used.
		Array<double> slope;
		Array<double> Yint;
		// This is the range the piecewise spline function is defined.
		double low,high;
	public:
		SPLINE(){
			slope.empty();
			Yint.empty();
			low = high = 0;
		}
		SPLINE(int size){
			slope.resize(size);
			Yint.resize(size);
		}
		// Sets the spline function.
		void construct(double from, double to, Array<double> Y_from, Array<double> Y_to){
			slope = (Y_to - Y_from) / (to - from);
			Yint  = Y_from - (slope * from);
			low = from;
			high = to;
		}
		// This returns true if a given X lies within this spline, or if this
		// spline is defined in a region which includes X.
		bool within(double X){
			if(X > low && X <= high){
				return true;
			}
			else{
				return false;
			}
		}
		// This is a prediction part which returns
		// the Y part given an X. 
		// NOTE: This does not check if the spline
		// is valid for a given X. Use within before
		// using predict, if within returns true.
		Array<double> predict(double X){
			return Yint + (slope * X);
		}
};

