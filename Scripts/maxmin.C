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

int main(int argc, char *argv[])
{
	Array <Array<double> > Max(100000);
	Array<Array<double> > Min(100000);
	if (argc < 4) {
		cout <<
		    "Usage: ./maxmin /path/to/output/file /infile1 /infile2 [/infile3 ..]\n";
		exit(1);
	}
	char header[200] = "";
	char outname1[200], outname2[200];
	strcpy(outname1, argv[1]);
	strcpy(outname2, argv[1]);
	strcat(outname1, ".max");
	strcat(outname2, ".min");

	int k = 0;
	ifstream infile;
	char Line[500];
	int first = 1;
	for (int i = 2; i < argc; i++) {
		infile.open(argv[i]);
		k = 0;

		// Get the header or ignore the header if already present 
		if (strcmp(header, "") == 0) {
			infile.getline(header, 200);
		} else {
			infile.getline(Line, 500);
		}
		while (1) {
			infile.getline(Line, 500);
			if (infile.eof()) {
				break;
			}
			if (first) {
				Max[k] = split<double>(",", Line);
				Min[k] = Max[k];
			} else {
				Max[k] =
				    max(Max[k], split<double>(",", Line));
				Min[k] =
				    min(Min[k], split<double>(",", Line));
			}
			k++;
			if (k == Max.length()) {
				Max.resize(Max.length() + 100000);
			}
			if (k == Min.length()) {
				Min.resize(Min.length() + 100000);
			}
		}
		first = 0;
		infile.close();
	}
	Max.resize(k);
	Min.resize(k);
	ofstream outfile1, outfile2;
	outfile1.open(outname1);
	outfile2.open(outname2);
	outfile1.setf(ios_base::fixed);
	outfile1.precision(12);
	outfile2.setf(ios_base::fixed);
	outfile2.precision(12);
	outfile1 << header << endl;
	outfile2 << header << endl;

	for (int i = 0; i < Max.length(); i++) {
		Max[i].fprint(outfile1);
		Min[i].fprint(outfile2);
	}

	return 0;
}
