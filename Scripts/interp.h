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
		void print(void){
			cout << "(" << low << "," << high << "]: m= ";
			cout << slope;
			cout << Yint;
			cout << endl;
		}
		// Sets the spline function.
		void construct(double from, double to, Array<double> Y_from, Array<double> Y_to){
			slope = (Y_to - Y_from) / (to - from);
			Yint  = Y_from - (slope * from);
			low = from;
			high = to;
		}
		void construct(double from, double to, double Y_from, Array<double> Y_to){
			slope = (Y_to - Y_from) / (to - from);
			Yint = ((slope * from) - Y_from) * -1;
			low = from;
			high = to;
		}
		// This returns true if a given X lies within this spline, or if this
		// spline is defined in a region which includes X.
		
		double lowest(void){
			return low;
		}
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

