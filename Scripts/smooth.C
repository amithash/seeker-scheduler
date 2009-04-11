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
/* Auto Regressive Moving Average Process */
#include "Array.H"

template <class T>
Array<T> mva(Array<Array<T> > * data, int ind, int window);
template <class T>
Array<T> index(Array<Array<T> > * data, int ind);


int main(int argc, char *argv[]){
	char input_file_name[200];
	char output_file_name[200];
	if(argc != 4){
		cout << "Usage: ./smooth /path/to/input/file /path/to/output/file WINDOW_SIZE" << endl;
		exit(1);
	}
	strcpy(input_file_name,argv[1]);
	strcpy(output_file_name,argv[2]);
	int window_length = atoi(argv[3]);
	ifstream infile;
	ofstream outfile;
	infile.open(input_file_name);
	outfile.open(output_file_name);
	outfile.setf(ios_base::fixed);
	outfile.precision(12);
	char header[200] = "";

	infile.getline(header,200);

	Array<Array<double> > Data(100000);
	char Sline[500];
	int k = 0;
	while(1){
		infile.getline(Sline,500);
		if(infile.eof()){
			break;
		}
		Data[k] = split<double>(",",Sline);
		k++;
		if(k ==Data.length()){
			Data.resize(Data.length() + 100000);
		}
	}
	Data.resize(k);
	infile.close();
	Array<double> out;
	outfile << header << endl;
	for(int i = 0;i<k;i++){
		out = mva(&Data,i,window_length);
		out.fprint(outfile);
	}
	outfile.close();

	return 0;
}

template <class T>
Array<T> mva(Array<Array<T> > * data, int ind, int window){
	Array<T> ret(data->data(0).length());
	double n = window + 1;
	ret = data->data(ind) * n;
	for(int i = 1; i <= window; i++){
		ret += ((index<double>(data, ind-i) + (index<double>(data,ind+i))) * (n - (double)i));
	}
	ret /= (n * n);
	return ret;
}

template <class T>
inline Array<T> index(Array<Array<T> > * data, int ind){
	Array<T> ret(data->data(0).length());
	ret.zeroes();
	if(ind < 0 || ind >= data->length()){
		return ret;
	}
	else{
		return data->data(ind);
	}
}

