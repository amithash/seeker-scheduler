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

/* Yeah, I could have used STL. But what's the fun in that?
 * This way, I get to learn templates, and have a library 
 * which exactly fit my needs */

#ifndef ARRAY_H_
#define ARRAY_H_
#include <iostream>
#include <fstream>   // file I/O
#include <iomanip>   // format manipulation
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

#define foreach(iter,data) for(int i=0;i<data.length();i++){ \
				iter = data[i];

template <class T = int> 
class Array{	
	private:
		int n;
		T * Data;
		int real_index(int ind);
	public:
		//Class methods.
		Array();
		Array(int num);
		Array(T * carr, int len);
		~Array();
		
		int length();
		T data(int index);
		Array<T> data(int from, int to);
		Array<T> data(Array<int> slice);
		void resize(int nsize);
		void fill(int index, T data);
		void append(T dat);
		void append(Array dat);
		void append(T * carr, int len);
		void print(void);
		void print(int col);
		void fprint(ofstream &outf);
		void fprint(ofstream &outf, const char *);
		void fprint(ofstream &outf, int col);
		void fprint(ofstream &outf, int col, const char *);
		void zeroes(void);
		void empty(void);
		
		// Operators overloaded.
		void operator=(Array N);
		Array<T> operator+(Array<T> A);
		Array<T> operator+(T B);
		Array<T> operator-(Array<T> A);
		Array<T> operator-(T B);
		Array<T> operator*(Array<T> A);
		Array<T> operator*(T B);
		Array<T> operator/(Array<T> A);
		Array<T> operator/(T B);
		Array<T> operator^(T B);
		bool operator==(Array<T> A);
		bool operator>=(Array<T> A);
		bool operator<=(Array<T> A);
		bool operator>(Array<T> A);
		bool operator<(Array<T> A);
		bool operator!=(Array<T> A);

		// Selection operators.
		Array<T> operator==(T A);
		Array<T> operator>=(T A);
		Array<T> operator<=(T A);
		Array<T> operator>(T A);
		Array<T> operator<(T A);
		Array<T> operator!=(T A);

		Array<T> operator++(void);
		Array<T> operator--(void);
		Array<T> operator++(int dummy);
		Array<T> operator--(int dummy);
		void operator+=(Array<T> A);
		void operator-=(Array<T> A);
		void operator*=(Array<T> A);
		void operator/=(Array<T> A);
		void operator*=(T A);
		void operator/=(T A);
		T& operator[](int index);
};
template <class T>
Array<T>::Array(){
	n = 0;
	Data = NULL;	
}
template <class T>
Array<T>::Array(int num){
	n = num;
	Data = new T[num];
	for(int i=0;i<n;i++){
		Data[i] = 0;
	}
}
template <class T>
Array<T>::Array(T * carr, int len){
	n = len;
	Data = new T[len];
	for(int i=0;i<n;i++){
		Data[i] = carr[i];
	}
}	
template <class T>
Array<T>::~Array(){
	n = 0;
	if(n > 0){
		delete [] Data;
	}
	Data = NULL;
}

template <class T>
int Array<T>::real_index(int ind){
	int real_index;
	if(ind >= 0 && ind < n){
		return ind;
	}
	else if(ind < 0 && (ind + n)>= 0){
		return ind + n;
	}
	else{
		return -1;
	}
}

template <class T>
int Array<T>::length(){
	return n;
}
template <class T>
void Array<T>::resize(int nsize){
	if(nsize != n){
		T * ne = new T[nsize];
		int len = n > nsize ? nsize : n;
		for(int i=0;i<len;i++){
			ne[i] = Data[i];
		}
		for(int i=len;i<nsize;i++){
			ne[i] = 0;
		}
		delete [] Data;
		Data = ne;
		n = nsize;
	}
}

template <class T>
T Array<T>::data(int index){
	int ind = real_index(index);
	assert(ind >= 0);
	return Data[ind];
}
template <class T>
Array<T> Array<T>::data(int from, int to){
	int re_f = real_index(from);
	int re_t = real_index(to);
	assert(re_f >= 0);
	assert(re_t >= 0);
	assert(re_t >= re_f);
	Array<T> ret(re_t-re_f+1);
	for(int i=re_f; i<=re_t;i++){
		ret.fill(i-re_f, Data[i]);
	}
	return ret; 
}
template <class T>
Array<T> Array<T>::data(Array<int> slice){
	Array<T> ret(slice.length());
	for(int i=0;i<slice.length();i++){
		assert(real_index(slice.data(i)) >= 0);
		ret.fill(i,Data[real_index(slice.data(i))]);
	}
	return ret;
}
template <class T>
void Array<T>::fill(int index, T data){
	int re_ind = real_index(index);
	assert(re_ind >= 0);
	Data[re_ind] = data;
}

template <class T>
void Array<T>::append(T dat){
	T * ne = new T[n+1];
	for(int i=0;i<n;i++){
		ne[i] = Data[i];
	}
	ne[n] = dat;
	n++;
	delete [] Data;
	Data = ne;
}
template <class T>
void Array<T>::append(Array<T> dat){
	T * ne = new T[n + dat.length()];
	for(int i=0;i<n;i++){
		ne[i] = Data[i];
	}
	for(int i=0;i<dat.length();i++){
		ne[n+i] = dat.data(i);
	}
	delete [] Data;
	Data = ne;
	n += dat.length();
}
template <class T>
void Array<T>::append(T * carr, int len){
	T * ne = new T[n + len];
	for(int i=0;i<n;i++){
		ne[i] = Data[i];
	}
	for(int i=0;i<len;i++){
		ne[n+i] = carr[i];
	}
	n += len;
	delete [] Data;
	Data = ne;
}
template <class T>
void Array<T>::print(void){
	for(int i=0;i<n;i++){
		cout << Data[i] << " ";
	}
	cout << endl;
}
template <class T>
void Array<T>::print(int col){
	for(int i=0;i<n;i++){
		cout << Data[i];
		if(i % col == col-1){
			cout << endl;
		}
		else{
			cout << " ";
		}
	}
}
template <class T>
void Array<T>::fprint(ofstream &outf){
	for(int i=0;i<n;i++){
		outf << Data[i];
		if(i < n-1){
			outf << ",";
		}
		else{
			outf << endl;
		}
	}
}

template <class T>
void Array<T>::fprint(ofstream &outf, const char *sep){
	for(int i=0;i<n;i++){
		outf << Data[i];
		if(i < n-1){
			outf << sep;
		}
		else{
			outf << endl;
		}
	}
}
template <class T>
void Array<T>::fprint(ofstream &outf, int col){
	for(int i=0;i<n;i++){
		outf << Data[i];
		if(i % col == col-1){
			outf << endl;
		}
		else{
			outf << ",";
		}
	}	
}

template <class T>
void Array<T>::fprint(ofstream &outf, int col, const char *sep){
	for(int i=0;i<n;i++){
		outf << Data[i];
		if(i % col == col-1){
			outf << endl;
		}
		else{
			outf << sep;
		}
	}	
}
template <class T>
void Array<T>::zeroes(void){
	for(int i=0;i<n;i++){
		Data[i] = 0;
	}
}	
template <class T>
void Array<T>::empty(void){
	delete [] Data;
	Data = NULL;
	n = 0;
}

// Operators overloaded.
template <class T>
void Array<T>::operator=(Array<T> N){
	if(n >= N.length()){
		for(int i=0;i<N.length();i++){
			Data[i] = N.data(i);
		}
	}
	else{
		T * ne = new T[N.length()];
		for(int i=0;i<N.length();i++){
			ne[i] = N.data(i);
		}
		delete [] Data;
		Data = ne;
		n = N.length();
	}
}

template <class T>
Array<T> Array<T>::operator+(Array<T> A){
	int len = n >= A.length() ? n : A.length();
	Array<T> ret(len); 
	if(n >= A.length()){
		for(int i=0;i<A.length();i++){
			ret.fill(i,Data[i] + A.data(i));
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,Data[i]);
		}
	}
	else{
		for(int i=0;i<n;i++){
			ret.fill(i,Data[i] + A.data(i));
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,A.data(i));
		}
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator-(Array<T> A){
	int len = n >= A.length() ? n : A.length();
	Array<T> ret(len); 
	if(n >= A.length()){
		for(int i=0;i<A.length();i++){
			ret.fill(i,Data[i] - A.data(i));
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,Data[i]);
		}
	}
	else{
		for(int i=0;i<n;i++){
			ret.fill(i,Data[i] - A.data(i));
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,(0 - A.data(i)));
		}
	}
	return ret;	
}

template <class T>
Array<T> Array<T>::operator+(T B){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,Data[i] + B);
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator-(T B){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,Data[i] - B);
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator*(Array<T> A){
	int len = n >= A.length() ? n : A.length();
	Array<T> ret(len); 
	if(n >= A.length()){
		for(int i=0;i<A.length();i++){
			ret.fill(i,Data[i] * A.data(i));
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,0);
		}
	}
	else{
		for(int i=0;i<n;i++){
			ret.fill(i,Data[i] * A.data(i));
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,0);
		}
	}
	return ret;		
}

template <class T>
Array<T> Array<T>::operator*(T B){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,Data[i] * B);
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator/(Array<T> A){
	int len = n >= A.length() ? n : A.length();
	Array<T> ret(len); 
	if(n >= A.length()){
		for(int i=0;i<A.length();i++){
			if(A.data(i) != 0){
				ret.fill(i,Data[i] / A.data(i));
			}
			else{
				ret.fill(i,0);
			}
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,0);
		}
	}
	else{
		for(int i=0;i<n;i++){
			if(A.data(i) != 0){
				ret.fill(i,Data[i] / A.data(i));
			}
			else{
				ret.fill(i,0);
			}
		}
		for(int i=A.length();i<n;i++){
			ret.fill(i,0);
		}
	}
	return ret;	
}

template <class T>
Array<T> Array<T>::operator/(T B){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		if(B != 0){
			ret.fill(i,Data[i] / B);
		}
		else{
			ret.fill(i,0);
		}
	}
	return ret;	
}

template <class T>
Array<T> Array<T>::operator^(T B){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i, (T)pow((float)Data[i], (float)B));
	}
	return ret;
}

// comparasion operators

template <class T>
bool Array<T>::operator==(Array<T> A){
	if(n != A.length()){
		return false;
	}
	else{
		bool ret = true;
		for(int i=0;i<n;i++){
			if(Data[i] != A.data(i)){
				ret = false;
				break;
			}
		}
		return ret;
	}
}

template <class T>
bool Array<T>::operator>=(Array<T> A){
	if(n != A.length()){
		return false;
	}
	else{
		bool ret = true;
		for(int i=0;i<n;i++){
			if(Data[i] < A.data(i)){
				ret = false;
				break;
			}
		}
		return ret;
	}
}

template <class T>
bool Array<T>::operator<=(Array<T> A){
	if(n != A.length()){
		return false;
	}
	else{
		bool ret = true;
		for(int i=0;i<n;i++){
			if(Data[i] > A.data(i)){
				ret = false;
				break;
			}
		}
		return ret;
	}
		
	
}

template <class T>
bool Array<T>::operator>(Array<T> A){
	if(n != A.length()){
		return false;
	}
	else{
		bool ret = true;
		for(int i=0;i<n;i++){
			if(Data[i] <= A.data(i)){
				ret = false;
				break;
			}
		}
		return ret;
	}
}

template <class T>
bool Array<T>::operator<(Array<T> A){
	if(n != A.length()){
		return false;
	}
	else{
		bool ret = true;
		for(int i=0;i<n;i++){
			if(Data[i] >= A.data(i)){
				ret = false;
				break;
			}
		}
		return ret;
	}
}

template <class T>
bool Array<T>::operator!=(Array<T> A){
	if(n != A.length()){
		return false;
	}
	else{
		bool ret = true;
		for(int i=0;i<n;i++){
			if(Data[i] == A.data(i)){
				ret = false;
				break;
			}
		}
		return ret;
	}
}

// Implicit operators

template <class T>
Array<T> Array<T>::operator++(void){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,++Data[i]);
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator--(void){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,--Data[i]);
	}
	return ret;
}
template <class T>
Array<T> Array<T>::operator++(int dummy){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,Data[i]++);
	}
	return ret;
}
template <class T>
Array<T> Array<T>::operator--(int dummy){
	Array<T> ret(n);
	for(int i=0;i<n;i++){
		ret.fill(i,Data[i]--);
	}
	return ret;
}

template <class T>
void Array<T>::operator+=(Array<T> A){
	if(n >= A.length()){
		for(int i=0;i<A.length();i++){
			Data[i] += A.data(i);
		}
	}
	else{
		T *ne = new T[A.length()];
		for(int i=0;i<n;i++){
			ne[i] = Data[i] + A.data(i);
		}
		for(int i=n;i<A.length();i++){
			ne[i] = A.data(i);
		}
		delete [] Data;
		Data = ne;
		n = A.length();
	}
}

template <class T>
void Array<T>::operator*=(Array<T> A){
	if(n >= A.length()){
		for(int i=0;i<n;i++){
			if(i < A.length()){
				Data[i] *= A.data(i);
			}
			else{
				Data[i] = 0;
			}
		}
	}
	else{
		T *ne = new T[A.length()];
		for(int i=0;i<n;i++){
			ne[i] = Data[i] * A.data(i);
		}
		for(int i=n;i<A.length();i++){
			ne[i] = 0;
		}
		delete [] Data;
		Data = ne;
		n = A.length();
	}	
}

template <class T>
void Array<T>::operator-=(Array<T> A){
	if(n >= A.length()){
		for(int i=0;i<A.length();i++){
			Data[i] -= A.data(i);
		}
	}
	else{
		T *ne = new T[A.length()];
		for(int i=0;i<n;i++){
			ne[i] = Data[i] - A.data(i);
		}
		for(int i=n;i<A.length();i++){
			ne[i] = A.data(i);
		}
		delete [] Data;
		Data = ne;
		n = A.length();
	}
}

template <class T>
void Array<T>::operator/=(Array<T> A){
	if(n >= A.length()){
		for(int i=0;i<n;i++){
			if(i < A.length()){
				if(A.data(i) != 0){
					Data[i] /= A.data(i);
				}
				else{
					Data[i] = 0;
				}
			}
			else{
				Data[i] = 0;
			}
		}
	}
	else{
		T *ne = new T[A.length()];
		for(int i=0;i<n;i++){
			if(A.data(i) != 0){
				ne[i] = Data[i] / A.data(i);
			}
			else{
				ne[i] = 0;
			}
		}
		for(int i=n;i<A.length();i++){
			ne[i] = 0;
		}
		delete [] Data;
		Data = ne;
		n = A.length();
	}	
}	
template <class T>
void Array<T>::operator*=(T A){
	for(int i=0;i<n;i++){
		Data[i] *= A;
	}
}
template <class T>
void Array<T>::operator/=(T A){
	for(int i=0;i<n;i++){
		if(A != 0){
			Data[i] /= A;
		}
	}
}
template <class T>
T& Array<T>::operator[](int index){
	int re_ind = real_index(index);
	assert(re_ind >= 0);
	return Data[re_ind];
}

template <class T>
Array<T> Array<T>::operator==(T A){
	int k=0;
	for(int i=0;i<n;i++){
		if(Data[i] == A){
			k++;
		}
	}
	Array<T> ret(k);
	if(k > 0){
		k = 0;
		for(int i=0;i<n;i++){
			if(data[i] == A){
				ret.fill(k++,Data[i]);
			}
		}
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator>=(T A){
	int k=0;
	for(int i=0;i<n;i++){
		if(Data[i] >= A){
			k++;
		}
	}
	Array<T> ret(k);
	if(k > 0){
		k = 0;
		for(int i=0;i<n;i++){
			if(data[i] >= A){
				ret.fill(k++,Data[i]);
			}
		}
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator<=(T A){
	int k=0;
	for(int i=0;i<n;i++){
		if(Data[i] <= A){
			k++;
		}
	}
	Array<T> ret(k);
	if(k > 0){
		k = 0;
		for(int i=0;i<n;i++){
			if(data[i] <= A){
				ret.fill(k++,Data[i]);
			}
		}
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator>(T A){
	int k=0;
	for(int i=0;i<n;i++){
		if(Data[i] > A){
			k++;
		}
	}
	Array<T> ret(k);
	if(k > 0){
		k = 0;
		for(int i=0;i<n;i++){
			if(data[i] > A){
				ret.fill(k++,Data[i]);
			}
		}
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator<(T A){
	int k=0;
	for(int i=0;i<n;i++){
		if(Data[i] < A){
			k++;
		}
	}
	Array<T> ret(k);
	if(k > 0){
		k = 0;
		for(int i=0;i<n;i++){
			if(data[i] < A){
				ret.fill(k++,Data[i]);
			}
		}
	}
	return ret;
}

template <class T>
Array<T> Array<T>::operator!=(T A){
	int k=0;
	for(int i=0;i<n;i++){
		if(Data[i] != A){
			k++;
		}
	}
	Array<T> ret(k);
	if(k > 0){
		k = 0;
		for(int i=0;i<n;i++){
			if(data[i] != A){
				ret.fill(k++,Data[i]);
			}
		}
	}
	return ret;
}


// Regular functions..
template <class T>
istream& operator>>(istream& in, Array<T> &A){
	T temp;
	for(int i=0;i<A.length();i++){
		in >> temp;
		A.fill(i,temp);
	}
	return in;
}
template <class T>
ostream& operator<<(ostream& out, Array<T> A){
	A.fprint((ofstream&)out);
}

template <class T>
Array<T> split(const char *sep, char *str){
	Array<T> ret;
	char * result=NULL;
	result = strtok(str,sep);
	while(result != NULL){
		ret.append(atof(result));
		result = strtok(NULL, sep);
	}
	return ret;
}

Array<int> split(char *sep, char*str){
	Array<int> ret;
	char * result=NULL;
	result = strtok(str,sep);
	while(result != NULL){
		ret.append(atoi(result));
		result = strtok(NULL, sep);
	}
	return ret;
}

template <class T, class U>
Array<T> typecast(Array<U> B){
	Array<T> A(B.length());
	for(int i=0;i<B.length();i++){
		A.fill(i,(T)B.data(i));
	}
	return A;
}
template <class T>
Array<T> max(Array<T> A, Array<T> B){
	int len = A.length() > B.length() ? A.length() : B.length();
	Array<T> ret(len);
	for(int i=0;i<len;i++){
		if(i >= A.length()){
			ret[i] = B.data(i);
		}
		else if(i >= B.length()){
			ret[i] = A.data(i);
		}
		else{
			ret[i] = A.data(i) > B.data(i) ? A.data(i) : B.data(i);
		}
	}
	return ret;
}
template <class T>
Array<T> min(Array<T> A, Array<T> B){
	int len = A.length() > B.length() ? A.length() : B.length();
	Array<T> ret(len);
	for(int i=0;i<len;i++){
		if(i >= A.length()){
			ret[i] = B.data(i);
		}
		else if(i >= B.length()){
			ret[i] = A.data(i);
		}
		else{
			ret[i] = A.data(i) < B.data(i) ? A.data(i) : B.data(i);
		}
	}
	return ret;
}



#endif /*ARRAY_H_*/
