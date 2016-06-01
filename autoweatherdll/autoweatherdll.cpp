#include "Stdafx.h"
#include "autoweatherdll.h"
#include "tinyxml2.h"
#include <msclr/marshal_cppstd.h>
#include <iostream>
#include <fstream>

#define URL "http://agrid.diasjp.net/model/metbroker/"
namespace autoweatherdll
{
	using namespace std;
	using namespace System;
	using namespace msclr::interop;
	using namespace tinyxml2;
	struct wdata{
		int yy, mm, dd;
		double temp;
		double daylength;
	};
	int toInt(string s) { int v; istringstream sin(s); sin >> v; return v; }
	double toDoub(string s) { double v; istringstream sin(s); sin >> v; return v; }
	template<class T> string toString(T x) { ostringstream sout; sout << x; return sout.str(); }
	vector<string> split(const string &str, char delim){
		istringstream iss(str); string tmp; vector<string> res;
		while (getline(iss, tmp, delim)) res.push_back(tmp);
		return res;
	}
	string Replace(string String1,string String2, string String3)
	{
		string::size_type  Pos(String1.find(String2));

		while (Pos != string::npos)
		{
			String1.replace(Pos, String2.length(), String3);
			Pos = String1.find(String2, Pos + String3.length());
		}

		return String1;
	}

	double getdl(double LAT, int doy){
		//ここから日長計算
		double eta = 2.0*M_PI / 365.0*(double)doy;
		double delta = asin(0.398*sin(4.871 + eta + 0.033*sin(eta)));
		double phi = LAT / 180.0*M_PI;
		double r = 0.01;
		double A = sin(M_PI*0.25 + (phi - delta + r)*0.5)*sin(M_PI*0.25 - (phi - delta - r)*0.5);
		double H = 2.0*asin(pow((A / (cos(phi)*cos(delta))), 0.5));
		double N = 2.0*H / 0.2618;//理論日長
		return N;
	}
	int query(string filename, string url){
		//libcurl呼び出し

		FILE *fp;
		fopen_s(&fp, filename.c_str(), "w");
		CURL *curl; CURLcode res;
		curl = curl_easy_init();//ハンドラの初期化
		curl_easy_setopt(curl, CURLOPT_URL, url); //URLの登録
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite); //BODYを書き込む関数ポインタを登録
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp); //↑で登録した関数が読み取るファイルポインタ
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		res = curl_easy_perform(curl); //いままで登録した情報で色々実行
		if (res != CURLE_OK)return -1;
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code); //ステータスコードの取得
		curl_easy_cleanup(curl); //ハンドラのクリーンアップ
		fclose(fp);
		return 0;
		//libcurl終わり
	}

	String^ Xmlparse::shortcutgetter(String^ so_id, String^ st_id, int beginyear, int endyear){
		
		string sourceid = marshal_as<std::string>(so_id);
		string nameid = marshal_as<std::string>(st_id);
		string dataset = "dataset.xml?source=" + sourceid+ "&station=" + nameid+"&lang=en";

		//std::cout << "set range of observation days you need" << endl;
		//std::cout << "for example, \"2006 2007\" " << endl;

		string url = URL;

		string interval = "&interval=" + toString(beginyear - 1) + "/12/31-" + toString(endyear) + "/12/31";
		tinyxml2::XMLDocument xml2;
		string element = "&elements=airtemperature";
		string resolution = "&resolution=daily";
		string filename = "weather_data.xml";
		if (query(filename, url + dataset + interval + element + resolution) == -1)return "Not_Found";
		//cout << url + dataset + interval + element + resolution << endl;
		xml2.LoadFile("weather_data.xml");
		XMLElement* root2 = xml2.FirstChildElement();

		string dirname = "envdata\\", fname = "test";
		vector<wdata>mint, maxt, avet;
		double lat; int doy = 1; string beftemp;
		for (XMLElement* elem = root2->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()){

			string eleName = elem->Value();
			if (eleName == "station")for (XMLElement* elem2 = elem->FirstChildElement(); elem2 != NULL; elem2 = elem2->NextSiblingElement()){
				string eleName2 = elem2->Value();
				if (eleName2 == "name")fname = (string)(elem2->GetText());
				if (eleName2 == "place")lat = toDoub((string)(elem2->Attribute("lat")));
			}


			if (eleName == "element")for (XMLElement* elem2 = elem->FirstChildElement(); elem2 != NULL; elem2 = elem2->NextSiblingElement()){
				string eleName2 = elem2->Value();

				if (eleName2 == "subelement" && (string)elem2->Attribute("id") == "Min"){

					for (XMLElement* elem3 = elem2->FirstChildElement(); elem3 != NULL; elem3 = elem3->NextSiblingElement()){
						string eleName3 = elem3->Value();
						if (eleName3 == "value"){
							vector<string>ymd;
							//cout << elem3->Attribute("date") << endl;
							ymd = split((string)(elem3->Attribute("date")), '/');
							const char *tp = elem3->GetText();
							string temp = (tp != nullptr) ? tp : beftemp;
							if (toInt(ymd[1]) == 1 && toInt(ymd[2]) == 1)doy = 1;
							double dl = getdl(lat, doy);
							doy++;
							wdata wd = { toInt(ymd[0]), toInt(ymd[1]), toInt(ymd[2]), toDoub(temp), dl };
							mint.push_back(wd);
							beftemp = temp;
						}
					}
				}
				else if (eleName2 == "subelement" && (string)elem2->Attribute("id") == "Max"){

					for (XMLElement* elem3 = elem2->FirstChildElement(); elem3 != NULL; elem3 = elem3->NextSiblingElement()){
						string eleName3 = elem3->Value();
						if (eleName3 == "value"){
							vector<string>ymd;
							ymd = split((string)(elem3->Attribute("date")), '/');
							const char *tp = elem3->GetText();
							string temp = (tp != nullptr) ? tp : beftemp;
							wdata wd = { toInt(ymd[0]), toInt(ymd[1]), toInt(ymd[2]), toDoub(temp), 0.0 };
							maxt.push_back(wd);
							beftemp = temp;
						}
					}
				}
				else if (eleName2 == "subelement" && (string)elem2->Attribute("id") == "Ave"){

					for (XMLElement* elem3 = elem2->FirstChildElement(); elem3 != NULL; elem3 = elem3->NextSiblingElement()){
						string eleName3 = elem3->Value();
						if (eleName3 == "value"){
							vector<string>ymd;
							ymd = split((string)(elem3->Attribute("date")), '/');
							const char *tp = elem3->GetText();
							string temp = (tp != nullptr) ? tp : beftemp;
							wdata wd = { toInt(ymd[0]), toInt(ymd[1]), toInt(ymd[2]), toDoub(temp), 0.0 };
							avet.push_back(wd);
							beftemp = temp;
						}
					}

				}
			}
		}
		fname = Replace(fname, " ", "_");
		fname = Replace(fname,"/","");

		ofstream outcsv(dirname + sourceid+fname + ".csv");
		outcsv << sourceid<< "," << nameid<<","<<beginyear<<","<<endyear;
		outcsv << ",,," << endl;

		outcsv << "year,month,day,Tave,Tmax,Tmin,P" << endl;
		for (int i = 0; i < (int)min(maxt.size(), min(mint.size(), avet.size())); i++){
			outcsv << mint[i].yy << "," << mint[i].mm << "," << mint[i].dd << "," << avet[i].temp << "," << maxt[i].temp << "," << mint[i].temp << "," << mint[i].daylength << endl;
		}

		return marshal_as<System::String^>(sourceid+fname);
	}


	String^ Xmlparse::xmlgetter(double xcin, double ycin, int beginyear, int endyear) {

		tinyxml2::XMLDocument xml;
		std::string url = URL;
		double range = 1.0;

		std::string cordinate = "stationlist.xml?area=";
		cordinate += toString(xcin + range) + "," + toString(ycin - range) + "," + toString(xcin - range) + "," + toString(ycin + range);
		std::string filename = "cordinate.xml";
		if(query(filename, url + cordinate)==-1)return "Not_Found";
		xml.LoadFile("cordinate.xml");
		XMLElement* root = xml.FirstChildElement();
		map<std::string, map<std::string, std::string> > station_id;//stationname,stationid,sourceid
		vector<pair<double, pair<string, string> > >distance_name;//distance,stationname,stationid

		for (XMLElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()){
			std::string sourceid = elem->Attribute("id");
			if (sourceid == "GD-DR&TR")continue;

			for (XMLElement* elem2 = elem->FirstChildElement(); elem2 != NULL; elem2 = elem2->NextSiblingElement()){
				string sourceName = elem2->Value();
				if (sourceName == "name"){
					//std::cout << "database:" << elem2->GetText() << endl;
				}
				else if (sourceName == "region"){
					for (XMLElement* elem3 = elem2->FirstChildElement(); elem3 != NULL; elem3 = elem3->NextSiblingElement()){
						string regionName = elem3->Value();
						if (regionName == "name"){
							//std::cout << "  |---region: " << elem3->GetText() << endl;
						}
						else if (regionName == "station"){
							string stationid = elem3->Attribute("id");
							double dis = 1000000;
							string stname = "", st = "", et = "";
							for (XMLElement* elem4 = elem3->FirstChildElement(); elem4 != NULL; elem4 = elem4->NextSiblingElement()){
								string stationName = elem4->Value();
								
								if (stationName == "name"){
									//std::cout << "     |---station: " << elem4->GetText() << "(id:" << stationid << ")";
									stname = elem4->GetText();
									station_id[stname][stationid] = sourceid;
								}
								else if (stationName == "place"){
									//std::cout << setprecision(4) << " (alt:" << toDoub(elem4->Attribute("alt")) << "m lat:" << toDoub(elem4->Attribute("lat")) << " lon:" << toDoub(elem4->Attribute("lon")) << ")" << endl;
									double lt = toDoub(elem4->Attribute("lat")); double ln = toDoub(elem4->Attribute("lon"));
									dis = pow(lt - xcin, 2) + pow(ln - ycin, 2);

								}
								else if (stationName == "operational"){
									const char * endtime = elem4->Attribute("end");
									const char * starttime = elem4->Attribute("start");
									st = (starttime != nullptr) ? starttime : "now";
									int stnum = toInt(st.substr(7, 4));
									et = (endtime != nullptr) ? endtime : "now";
									

									//std::cout << "      from  " << st << " to " << et << endl;
								}
								else if (stationName == "elements"){
									for (XMLElement* elem5 = elem4->FirstChildElement(); elem5 != NULL; elem5 = elem5->NextSiblingElement()){
										//親がelementな子を掘る
										string elemid = (string)elem5->Attribute("id");
										if (elemid == "air temperature"){//要素にair tempratureが必要
											station_id[stname][stationid] = sourceid;
											if (toInt(st.substr(7, 4)) < beginyear&&et == "now"){
												distance_name.push_back(make_pair(dis, make_pair(stname, stationid)));
											}
											else if (toInt(st.substr(7, 4))<beginyear&&et != "now"&&toInt(et.substr(7, 4))>endyear){
												distance_name.push_back(make_pair(dis, make_pair(stname, stationid)));
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if (station_id.size() == 0){
			//std::cout << "cannot find any data in this cordinate" << endl;
			return "Not_Found";
		}
		sort(distance_name.begin(), distance_name.end());
		return shortcutgetter(marshal_as<System::String^>(station_id[distance_name[0].second.first][distance_name[0].second.second]), marshal_as<System::String^>(distance_name[0].second.second), beginyear, endyear);
	}
	String^ Xmlparse::xmlsearcher(double xcin, double ycin, int beginyear, int endyear) {

		tinyxml2::XMLDocument xml;
		std::string url = URL;
		double range = 1.0;

		std::string cordinate = "stationlist.xml?area=";
		cordinate += toString(xcin + range) + "," + toString(ycin - range) + "," + toString(xcin - range) + "," + toString(ycin + range);
		std::string filename = "cordinate.xml";
		std::cout << url+cordinate << std::endl;
		if (query(filename, url + cordinate) == -1)return "Not_Found";
		xml.LoadFile("cordinate.xml");
		XMLElement* root = xml.FirstChildElement();
		map<std::string, map<std::string, std::string> > station_id;//stationname,stationid,sourceid
		vector<pair<double, pair<string, string> > >distance_name;//distance,stationname,stationid
		//ofstream ofs("debug.txt");
		

		for (XMLElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()){
			std::string sourceid = elem->Attribute("id");
			
			if (sourceid == "GD-DR&TR")continue;

			for (XMLElement* elem2 = elem->FirstChildElement(); elem2 != NULL; elem2 = elem2->NextSiblingElement()){
				string sourceName = elem2->Value();
				if (sourceName == "name"){
					std::cout << "database:" << elem2->GetText() << endl;
				}
				else if (sourceName == "region"){
					for (XMLElement* elem3 = elem2->FirstChildElement(); elem3 != NULL; elem3 = elem3->NextSiblingElement()){
						string regionName = elem3->Value();
						
						if (regionName == "name"){
							std::cout << "  |---region: " << elem3->GetText() << endl;
						}
						else if (regionName == "station"){
							string stationid = elem3->Attribute("id");
							double dis = 1000000; 
							string stname = "",st="",et="";

							for (XMLElement* elem4 = elem3->FirstChildElement(); elem4 != NULL; elem4 = elem4->NextSiblingElement()){
								string stationName = elem4->Value();
								if (stationName == "name"){
									std::cout << "     |---station: " << elem4->GetText() << "(id:" << stationid << ")";
									stname = elem4->GetText();
									
									//station_id[stname][stationid] = sourceid;
								}
								else if (stationName == "place"){
									std::cout << setprecision(4) << " (alt:" << toDoub(elem4->Attribute("alt")) << "m lat:" << toDoub(elem4->Attribute("lat")) << " lon:" << toDoub(elem4->Attribute("lon")) << ")" << endl;
									double lt = toDoub(elem4->Attribute("lat")); double ln = toDoub(elem4->Attribute("lon"));
									dis = pow(lt - xcin, 2) + pow(ln - ycin, 2);

								}
								else if (stationName == "operational"){
									const char * endtime = elem4->Attribute("end");
									const char * starttime = elem4->Attribute("start");
									st = (starttime != nullptr) ? starttime : "now";
									
									et = (endtime != nullptr) ? endtime : "now";
									//ofs << stname << endl;
									

									std::cout << "      from  " << st << " to " << et << endl;
								}
								else if (stationName=="elements"){
									for (XMLElement* elem5 = elem4->FirstChildElement(); elem5 != NULL; elem5 = elem5->NextSiblingElement()){
										//親がelementな子を掘る
										string elemid = (string)elem5->Attribute("id");
										if (elemid == "air temperature"){//要素にair tempratureが必要
											station_id[stname][stationid] = sourceid;
											//cout << toInt(st.substr(0, 4)) << endl;
											if (toInt(st.substr(0, 4)) < beginyear&&et == "now"){
												distance_name.push_back(make_pair(dis, make_pair(stname, stationid)));
											}
											else if (toInt(st.substr(0, 4))<beginyear&&et != "now"&&toInt(et.substr(0, 4))>endyear){
												distance_name.push_back(make_pair(dis, make_pair(stname, stationid)));
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if (station_id.size() == 0||distance_name.size()==0){
			//std::cout << "cannot find any data in this cordinate" << endl;
			return "Not_Found";
		}
		sort(distance_name.begin(), distance_name.end());
		/*FILE *degp;
		fopen_s(&degp, "debug.txt", "w");
		fprintf(degp, "%s", station_id[distance_name[0].second.first][distance_name[0].second.second].c_str());
		fclose(degp);*/
		
		//sourceid,stationid,stationname
		std::cout << distance_name[0].second.second + "," + distance_name[0].second.first << endl;
		std::cout << station_id[distance_name[0].second.first][distance_name[0].second.second] << endl;
		System::String^ res = msclr::interop::marshal_as<System::String^>(station_id[distance_name[0].second.first][distance_name[0].second.second] + "," + distance_name[0].second.second + "," + distance_name[0].second.first);
		return res;
	}
	
}

int main(){
	auto xm = gcnew autoweatherdll::Xmlparse();
	std::string name;
	double ido, keido;
	std::ifstream ifs("idokeido.txt");
	std::ofstream ofs("outputlist.csv");
	while (ifs >> name >> ido >> keido){
		auto str = msclr::interop::marshal_as<std::string>(xm->xmlsearcher(ido, keido, 2010, 2015));
		if (str == "Not_Found"){
			ofs << name << "," << str << std::endl;
			continue;
		}
		std::string ss1 = "", ss2 = "";
		int cnt = 0;
		for (int i = 0; i < str.size(); i++){
			if (str[i] == ','){
				cnt++; continue;
			}
			if (cnt == 0)ss1 += str[i];
			else if (cnt == 1)ss2 += str[i];
		}
		System::String^ s1 = msclr::interop::marshal_as<System::String^>(ss1);
		System::String^ s2 = msclr::interop::marshal_as<System::String^>(ss2);
		//ofs << ss1 << " " << ss2 << std::endl;
		auto fname = msclr::interop::marshal_as<std::string>(xm->shortcutgetter(s1, s2, 2010, 2015));
		ofs << name << "," << fname << std::endl;
	}
	return 0;
}