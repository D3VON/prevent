/*
  prevent.cpp
  
  By Alex Garcia, Devon McBride, Diane Stamboni, Yuanda Xu

  This is a program which receives a CO2 level from a sensor, together with a 
  temperature reading associated with that sensor.  It also collects the current 
  outside temperature, and 119 hourly temperature predictions (5 days) into the
  future. 
  
  With these data, the algorithm decides whether to "open the window" of the room
  associated with the sensors.  By "open the window" we mean activate a vent fan
  which blows in exterior air.  
  
  ********this is a work in progress, like just about everything nowadays. 
  
  ********the algorithm is supposed to decide when the best time to bring in 
  fresh air is, related to exterior temperature predictions. 

  compile thusly: g++ -o prevent prevent.cpp -lcurl

  You will need curl and libcurl installed. 
  
  




*/



#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <curl/curl.h>
#include <unistd.h>
#include <time.h>

using namespace std;
#define ROT 30
#define OYT 50
#define YGT 71

#define ROC 2500
#define OYC 1150
#define YGC 950

#define HISTCUT 7200

#define REDT 1 //if window is not opened add to history
#define ORANGET 1.5
#define YELLOWT 3.25
#define GREENT 5

#define REDC 120
#define ORANGEC 3
#define YELLOWC 1
#define GREENC 0

#define PREVENT_DAYS 3
#define PREVENT_HOURS 24

#define PREVENT_MULTIPLIER 1.5

#define APPROACH 2

//IP address may change, get from http://www.d3von.com/gotoCastle.php
#define FORECAST_INFO "http://108.183.57.218/grabTemp.php"


vector<double> history;	//store history on how many times a check was made and opening the window was put off.  Clear when window is opened
int forecast[120];

/*****************************************************************************************/

vector<int> marked;
vector<int> markedlifetime;
int currentRange[PREVENT_DAYS][PREVENT_HOURS];


string getTime(){
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	timeinfo = localtime(&rawtime);
	//cout << asctime(timeinfo) <<endl;
	return asctime(timeinfo);
}

int getHour(){
	string mytime = getTime();
	int end = mytime.find_first_of(':',0);
	int begin = end - 2;
	string myhour = mytime.substr(begin, 2);
	cout << "hour: "<< myhour <<endl;
	return atoi(myhour.c_str());
}
int getMin(){
	string mytime = getTime();
	int end = mytime.find_last_of(':',mytime.length());
	int begin = end - 2;
	string mymin = mytime.substr(begin, 2);
	//cout << "min: "<< mymin <<endl;
	return atoi(mymin.c_str());
}
int startOfHour(){
	int min = getMin();cout << "min: "<< min <<endl;
	if(min == 0){
		return true;
	}else{
		return false;
	}
}

int updateDay(int oldDay, int currentTime){
	if(startOfHour() && currentTime == 0){cout<<"NEW DAY"<<endl;
		oldDay++;
		if(oldDay == PREVENT_DAYS){
			oldDay = 0;
		}
	}
	return oldDay;
}


void mark(int hour){
	bool premarked = false;
	for(int i = 0; i < marked.size(); i++){
		if(marked[i] == hour){
			premarked = true;
		}
	}
	if(!premarked){
		marked.push_back(hour);
		markedlifetime.push_back(24*60*PREVENT_DAYS);
	}
}
void expire(int hour){
	marked.erase(marked.begin() + hour);
	markedlifetime.erase(marked.begin() + hour);
}
void deinc(){
	for(int i = 0; i < markedlifetime.size(); i++){
		markedlifetime[i] = markedlifetime[i] -1;
		if(markedlifetime[i] == 0){
			expire(i);
		}
	}
}
bool tendsToBeHigh(int hour){
	int sum = 0; 
	for(int i = 0; i < PREVENT_DAYS; i++){
		sum += currentRange[hour][i];
	}
	int avg = sum/PREVENT_DAYS;
	//cout<< "average co2: " <<avg<<endl;
	if(avg > OYC){
		return true;
	}else{
		return false;
	}
}
void init_prevent(){
	for(int i = 0; i < PREVENT_HOURS; i++){
		for(int j = 0; j < PREVENT_DAYS; j++){
			currentRange[i][j] = 0;
		}
	}
}

void prevent(){
	for(int i = 0; i < PREVENT_HOURS; i++){
		if(tendsToBeHigh(i)){
			mark(i);
		}
	}
}

void updateCurrentRange(int cotwolevel,int day, int currentTime){
	if(startOfHour() || cotwolevel > currentRange[currentTime][day]){
		cout<< "UPDATE day: "<< day << ", current time: " << currentTime << endl;
		currentRange[currentTime][day] = cotwolevel;
	}
}

bool approachingMarked(int currentTime){
	bool approaching = false;	
	for(int i = 0; i < marked.size(); i++){
		if(marked[i] - currentTime >= APPROACH){
			approaching = true;
		}
	}
	return approaching;
}

/*****************************************************************************************/

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


void updateforecast(){
	CURL *curl;
	CURLcode res;
	string readBuffer;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, FORECAST_INFO);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		//cout << readBuffer << endl;
	}
	int begin = 0;
	int end = readBuffer.find_first_of(' ');
	int i = 0;
	while(i < 120){
		forecast[i] = atoi(readBuffer.substr(begin, end-(begin)).c_str());
		begin = end+1;
		end = readBuffer.find_first_of(' ',begin);
		cout << forecast[i] << " ";		
		i++;
	}cout<< endl;
}

int getSensorDataC(){
	CURL *curl;
	CURLcode res;
	string readBuffer;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://108.183.57.218:5000/node/s2/6");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		//cout << readBuffer << endl;
	}
	int cotwo = atoi(readBuffer.c_str());
	cout<< "co2: " << cotwo<<endl;
 	return atoi(readBuffer.c_str());
}

int getSensorDataT(){
		CURL *curl;
	CURLcode res;
	string readBuffer;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://108.183.57.218:5000/node/s1/7");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		cout<<"raw temp: " << readBuffer << endl;
	}
	int mytemp = atoi(readBuffer.c_str());
	cout << "temp: " << mytemp<<endl;
 	return atoi(readBuffer.c_str());
}

int findhightemp(int hours){
	int high = 0;
	for(int i = 0; i < hours; i++){
		if(forecast[i] > forecast[high]){
			high = i;
		}
	}
	cout<<"high temp: " <<high<<endl;
	return high;
}

double getHistVal(){
	double val = 0;
	for(int i = 0; i < history.size(); i++){
		val += history[i];
	}
	cout << "history val: " << val<<endl;
	return val;
}

void openwindow(){
	int CO2level = getSensorDataC();
	double outsideTemp = forecast[0];
	int maxCO2;
	
	// the colder it gets, the higher the CO2 level we can endure
	if (outsideTemp > 60) maxCO2 = 900;
	else if (outsideTemp > 50) maxCO2 = 1000;
	else if (outsideTemp > 40) maxCO2 = 1100;
	else if (outsideTemp > 30) maxCO2 = 1200;
	else if (outsideTemp > 20) maxCO2 = 1300;
	else if (outsideTemp > 10) maxCO2 = 1400;
	else maxCO2 = 1500;
	
	history.clear();
	history.resize(0);
	if ( CO2level > maxCO2 ){
		cout<< "Open Window" << endl;
		system("br a1 on");
	}
	else{
		cout<<"Sorry, too cold outside right now."<<endl;
	}
	while ( CO2level > maxCO2 ){
		sleep(60);// check level every mintue.
		CO2level = getSensorDataC();
	}
	system("br a1 off");
	cout<< "Close Window" << endl;
}

void rcocgt(){
	openwindow();
}

void ocrt(){
	int hr = findhightemp(36);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(ORANGEC*REDT);
	}
}
void ocot(){
	int hr = findhightemp(24);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(ORANGEC*ORANGET);
	}
}
void ocyt(){
	int hr = findhightemp(12);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(ORANGEC*YELLOWT);
	}
}

void ocgt(){
	int hr = findhightemp(6);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(ORANGEC*GREENT);
	}
}

void ycrt(){
	int hr = findhightemp(120);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(YELLOWC*REDT);
	}
}
void ycot(){
	int hr = findhightemp(96);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(YELLOWC*ORANGET);
	}
}
void ycyt(){
	int hr  = findhightemp(48);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(YELLOWC*YELLOWT);
	}
}


void ycgt(){
	int hr = findhightemp(24);
	double histval = getHistVal();
	if(hr == 0 || histval > HISTCUT){
		openwindow();
	}else{
		history.push_back(YELLOWC*GREENT);
	}
}

int main(){
	printf("hi\n");
	init_prevent();
	int day = 0;
	int minute = 0;
	int currentTime = getHour();
		cout<< "INIT day: "<< day << ", current time: " << currentTime << endl;
	while(1){
		cout<<("!");
		int col = getSensorDataC();
		int temp = getSensorDataT();
		currentTime = getHour();
		day = updateDay(day, currentTime);
		updateCurrentRange(col, day, currentTime);
		prevent();
		
		// wunderground maximum connections: 10 per minute, 500 per day.
		// we do not need updated forecastes frequently: forecasts will be 
		// almost the same within an hour. 
		if (minute < 10) 
		{ 
			minute++; 
		}
		else
		{ 
			updateforecast(); 
			minute = 0; 
		} 
		deinc();
		if(approachingMarked(currentTime)){
			col = col * PREVENT_MULTIPLIER;
		}

		if(col >= ROC ||(col >= OYC && temp >= YGT)){cout << "============if 1\n"; rcocgt(); }

		else if(col >= OYC && temp <= ROT)			{cout << "============if 2\n";  ocrt(); }
		else if(col >= OYC && temp <= OYT)			{cout << "============if 3\n";  ocot(); }
		else if(col >= OYC && temp <= YGT)			{cout << "============if 4\n";  ocyt(); }
		else if(col >= OYC && temp >  YGT)			{cout << "============if 5\n";  ocgt(); } 

		else if(col >= YGC && temp <= ROT)			{cout << "============if 6\n";  ycrt(); }
		else if(col >= YGC && temp <= OYT)			{cout << "============if 7\n";  ycot(); }
		else if(col >= YGC && temp <= YGT)			{cout << "============if 8\n";  ycyt(); }
		else if(col >= YGC && temp >  YGT)			{cout << "============if 9\n";  ycgt(); }
	cout<<"history: "<<endl;	
	for(int i = 0; i < history.size(); i++){		
		cout << history[i] << " ";		
	}cout<< endl;
	cout<< "marked hours: "<<endl;
	for(int i = 0; i < marked.size(); i++){		
		cout <<"("<<marked[i] <<", "<< markedlifetime[i] <<  "), ";		
	}cout<< endl;
	cout<< "current range: "<<endl;
	for(int j = 0; j < PREVENT_DAYS; j++){
		for(int i = 0; i < PREVENT_HOURS; i++){
			cout<< currentRange[i][j] << " ";
		}cout<< endl;
	}cout<< endl;
		sleep(60);
	}

}
