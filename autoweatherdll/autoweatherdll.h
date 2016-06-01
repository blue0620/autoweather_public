// autoweatherdll.h

#pragma once
#define _USE_MATH_DEFINES
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cmath>
#include <string>
#include <fstream>
#include <stdio.h>
#include <msclr/marshal_cppstd.h>
#include <curl/curl.h>

namespace autoweatherdll {
	using namespace msclr::interop;
	using namespace System;
	using namespace std;
	
	int toInt(string s);
	double toDoub(string s);
	template<class T> string toString(T x);
	vector<string> split(const string &str, char delim);
	double getdl(double LAT, int doy);
	int query(string filename, string url);

	public ref class Xmlparse
	{
		// TODO: このクラスの、ユーザーのメソッドをここに追加してください。
		
	public:
		String^ shortcutgetter(String^ so_id, String^ st_id, int beginyear, int endyear);
		String^ xmlgetter(double lat, double lon, int beginyear, int endyear);
		String^ xmlsearcher(double lat, double lon, int beginyear, int endyear);
	};
}
