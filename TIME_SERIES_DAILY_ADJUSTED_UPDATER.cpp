
// Please read the the readme.txt file for explanations of how to use this file to update your stocks list

// Author: Michael B. 2019 using Linux Mint Mate and C++11  


#include <cstring>
#include <memory>
// my added includes
#include <chrono>// for sllep_for
#include <thread>// for sleep_for
#include <future>// for async


// for void getFileCreationTime(char *path)
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>// for strcpy strncpy

#include <algorithm>// for 'any_of'
#include <cctype>
#include <fstream>
#include <ctime>
// original file includes
#include <string>
#include <iostream>
#include <sstream>
#include "Avapi/AvapiConnection.hpp"
#include "Avapi/TIME_SERIES/TIME_SERIES_DAILY_ADJUSTED.hpp"

using namespace std;
using namespace Avapi;

const time_t ctt = time(0);




std::time_t getFileCreationTime(char *path) {
    struct stat attr;
    stat(path, &attr);
    printf("Last modified time: %s", ctime(&attr.st_mtime));
    return attr.st_mtime;
}

int check_if_the_csv_file_was_updates(string str_ticker_csv,int time_limit_for_file_update)
{
	bool debug = false;

    cout << "Checking_if_the_csv_file_was_updates:'" << str_ticker_csv << "'" << endl;

	char dest[1024];//https://stackoverflow.com/questions/13294067/how-to-convert-string-to-char-array-in-c
	strncpy(dest, str_ticker_csv.c_str(), sizeof(dest));
	dest[sizeof(dest) - 1] = 0;
	cout << endl << "str_ticker_csv in thread: " << dest << endl;

    int max_seconds = time_limit_for_file_update;// max seconds to wait for the file to be modified (with all the data trasfred by the internet) before returning to the main function
    int one_iteration = 2;// 2 seconds
    int max_loops = max_seconds/one_iteration;
    
    std::time_t t,t0; 
    int seconds_it_took_to_update_file = -1;// -1 will note the file wasn't updated
    int i=0;
    for(i=0;i<max_loops;i++) {
		t = getFileCreationTime(dest);
		if (i==0) t0 = t;
		else {
			if (std::difftime(t, t0) > 0) {
				seconds_it_took_to_update_file = one_iteration * i;
				if (debug) cout << "Seconds it took to update file " << seconds_it_took_to_update_file << "s (from inside the thread)" << endl << endl;
				//the csv data file was modified and we can let the main function continue to the next ticker in the main function tickers loop
			    return seconds_it_took_to_update_file;	
			}
			else
				cout << "waiting " << (i+1)*one_iteration << "s ,";
		}
		std::this_thread::sleep_for(std::chrono::milliseconds((one_iteration *1000)));
    }// for(int i=0;i<max_loops;i++)
    
    if (debug) cout << "FILE WAS NOT UPDATED WITHIN " << max_seconds << "s (from inside the thread)"  << endl<< endl;
    return -1;// didn't update the file
}




// Returns true if the input string had any digits in it
bool has_any_digits(const std::string &s)
{
    return std::any_of(s.begin(), s.end(), ::isdigit);
}




// Trims whitespaces from both sides of a string and returns the trimmed version
std::string trim(const std::string &s)
{
   auto  wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
   return std::string(wsfront,std::find_if_not(s.rbegin(),std::string::const_reverse_iterator(wsfront),[](int c){return std::isspace(c);}).base());
}



// This finction gets the newest record data from the input ticker csv file and returns it in: f_day,f_month,f_year
// It also puts all the record lines in the csv file to a string vector array named str_data which holds them in memory
// so we can write them back after the new data (because the records order in the file is newest first)
void get_newest_record_date_from_the_csv_file_and_fill_str_data(string &str_ticker_file,std::vector<std::string> &str_data, int &f_day,int &f_month,int &f_year,bool rewrite_the_ticker_file_newest_record) {

    cout << "trying to open the file '" << str_ticker_file << "' to read" << endl << endl;

    string line,first_date_line;

    ifstream file (str_ticker_file);
    if (file.is_open())
    {
        std::stringstream stock_data_buffer;
        stock_data_buffer << file.rdbuf();// read the entire file to the buffer
        file.close();

        // operations on the buffer...
        bool first_date_line_was_found = false;
        bool first_time_in_rewrite_the_ticker_file_newest_record = true;
        while ( getline (stock_data_buffer,line) )
        {
            //cout << line << '\n';// for debug

            if ( ! first_date_line_was_found){
                if (has_any_digits(line)) {
                	// here we skip the first record in the file so we will write it again as if it wasn't in the file
                    if ((rewrite_the_ticker_file_newest_record) && (first_time_in_rewrite_the_ticker_file_newest_record)) {
                    	first_time_in_rewrite_the_ticker_file_newest_record = false;
                        continue;
                    }

                    first_date_line = line;
                    first_date_line_was_found = true;      
                }
                else continue;// this is so we want enter the first line ("Date,Open,High,Low,Close,Volume,Adj Close") into the str_data vector
            }// if (has_any_digits(line))
            str_data.push_back(line);
        }// while ( getline (stock_data_buffer,line) )
        stock_data_buffer.flush();
        // cout << endl;
    }    
    else {
        cout << "Unable to open the file " << str_ticker_file << " to read, Creating a new one." << endl;
        return;
    }
    /*  ------------------------------ IMPORTANT -----------------------------
        this is for the basic format like that that it start in the 2nd row:
            Date,Open,High,Low,Close,Volume,Adj Close
            2016-05-06,49.92,50.39,49.66,50.39,24787301,50.39
    */
    int day;    int month;    int year;
    cout << "first_date_line: |"<< first_date_line << "|"<< endl;
    string Sdate = first_date_line;
    int lenth = Sdate.length();
    string str_d,str_m,str_y;
    //////////////////////////
    int f = Sdate.find('-');
    if ( f == 4) {	// new format  2016-05-06
	    str_y = Sdate.substr(0,4);	    str_m = Sdate.substr(5,2);	    str_d = Sdate.substr(8,2);
	    day = stoi ( str_d);	    month = stoi ( str_m);	    year = stoi ( str_y);
    } else {				        // old date format "10-Feb-97", "7-Feb-97"
	    if (f == 2) {// "10-Feb-97"
		    str_d = Sdate.substr(0,2);		    str_m = Sdate.substr(3,3);	        str_y = Sdate.substr(7,2);

	    } else {// "7-Feb-97"
		    str_d = Sdate.substr(0,1);		    str_m = Sdate.substr(2,3);	        str_y = Sdate.substr(6,2);
	    }
        cout << "day: "<< str_d << ", month: "<< str_m << ", year: "<< str_y << endl;
	    string Months = "JanFebMarAprMayJunJulAugSepOctNovDec";
	    month = Months.find(str_m) / 3 + 1;	    day = stoi ( str_d);	    year = stoi ( str_y);
	    if (year<50) year+=2000; else year+=1900;
    }// else

    f_day = day;//newest_file_day
    f_month = month;//newest_file_month
    f_year = year;//newest_fileyear
    cout << "day: "<< day << ", month: "<< month << ", year: "<< year << endl;
}// void get_newest_record_date_from_the_csv_file_and_fill_str_data(string &str_ticker_file, int &f_day,int &f_month,int &f_year)



/*
 * Case Sensitive Implementation of endsWith()
 * It checks if the string 'mainStr' ends with given string 'toMatch'
 */
bool endsWith(const std::string &mainStr, const std::string &toMatch)
{
	if(mainStr.size() >= toMatch.size() &&
			mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
			return true;
		else
			return false;
}



// this function opens the "stock_list.txt"that is passed by str_all_tickers_file the first const reference
// a file of all the tickers seperated by lines, and the ticker is the first string before a comma like msft,microsoft \n in one line 
// we fill the tickers_array vectors with the tickers from this file

string fill_tickers_array(const string &str_all_tickers_file,std::vector<std::string> &tickers_array) {

    bool debug = true;// true // false

    cout << "trying to open the file '" << str_all_tickers_file << "' to read" << endl << endl;

    string line,str_ticker,your_api_key="";
    string str_your_api_key =  "My personal api key:";//this text should appear on the stock text tickers files at the begining followed on the same line with the api key the user got from Alpha Vantage
	bool api_key_found = false;

    ifstream myfile (str_all_tickers_file);

    if (myfile.is_open())
    {
        std::stringstream tickers_names_buffer;
        tickers_names_buffer << myfile.rdbuf();// read the entire file to the buffer
        myfile.close();
    
        while ( getline (tickers_names_buffer,line) )
        {
            //if (debug) cout << line << '\n';// for debug

            line = trim(line);
            int f = line.find("//");// used for comments
            if(f==0) continue;
            
            // retreive Alpha Vantage api_key from a line in the stock list txt file begining with "my api key:"
            std::size_t api_key_pos_found = line.find(str_your_api_key);
  			if (api_key_pos_found!=std::string::npos) {
				your_api_key = trim(line.substr(api_key_pos_found+str_your_api_key.length()));
				cout << "your api key is: '"<< your_api_key << "'" << endl << endl;
				api_key_found = true;
				continue;//continue to the next line in the file
			}

            int f1 = line.find(',');
            str_ticker = line.substr(0,f1);
            str_ticker = trim(str_ticker);
            // if the filename already ends with ".csv", we strip it off because the main function adds it to the str_ticker
            if (endsWith(str_ticker , ".csv")) str_ticker = str_ticker.substr(0,str_ticker.size()-4);

            if (str_ticker == "")//skip empty lines
                continue;

            if (debug) cout << "'" << str_ticker << "'" << '\n';// for debug

			if (api_key_found)
            	tickers_array.push_back(str_ticker);
            else {
            	cout << endl << "Important: The first line in the stock tickers file must begin with the words '" << str_your_api_key << "' followed on the same line by the api key (without any quotation marks) you got from Alpha Vantage web site (https://www.alphavantage.co/support/#api-key)." << endl << endl;
            	break;
            	}
        }
        tickers_names_buffer.flush();
        cout << endl;
    }
    else cout << "Unable to open the file " << str_all_tickers_file << " to read" << endl;

	return your_api_key;
}// void fill_tickers_array






///////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_split_line_data converts a string from the split file to data and returns the data
void get_split_line_data(string temp, int &day,int &month,int &year,double &divider) {

    string str_d,str_m,str_y;

    std::size_t Mul1_begin = temp.rfind('[');
    std::size_t Mul1_end = temp.rfind(':');
    std::size_t Mul2_end = temp.rfind(']');

    if ((Mul1_begin == std::string::npos) && (Mul2_end == std::string::npos)) {// Newest Alpha Ventage Split format: "2018-05-23,1.5"

	    str_y = temp.substr(0,4);	str_m = temp.substr(5,2);		str_d = temp.substr(8,2);
        year = stoi ( str_y);       month =  stoi ( str_m);		    day = stoi ( str_d);	   

	    string Mul = temp.substr(11);// after the comma till the end
	    divider = stod(Mul);

    } else {// previous 2 yahoo formats

	    string Mul1 = temp.substr(Mul1_begin+1,Mul1_end-Mul1_begin-1);
	    string Mul2 = temp.substr(Mul1_end+1,Mul2_end-Mul1_end-1);
	    divider = stod(Mul1) / (double)stod(Mul2);

	    if(temp.find('-') == std::string::npos) {// new yahoo splits format: "Sep 11, 2007 [3:2]"

		    string Sdate = temp.substr(0,temp.rfind('[')-1);
		
		    str_m = Sdate.substr(0,3);						
		    str_y = Sdate.substr(Sdate.size() - 4);

		    std::size_t psik_char_place = Sdate.find(',');
		    int start_day_place = 4;
		    str_d = Sdate.substr(start_day_place,psik_char_place-start_day_place);

		    string Months = "JanFebMarAprMayJunJulAugSepOctNovDec";
		    month = Months.find(str_m) / 3 + 1;
		    day = stoi ( str_d);
		    year = stoi ( str_y);

	    } else {//previous yahoo split format  : "11-Sep-07 [3:2]"
		    string Sdate = temp.substr(0,9);

		    int length = Sdate.size();

		    if (length == 9) {
			    str_d = Sdate.substr(0,2);
			    str_m = Sdate.substr(3,3);
		    } else {
			    str_d = Sdate.substr(0,1);
			    str_m = Sdate.substr(2,3);
		    }
		    str_y = Sdate.substr(Sdate.length() - 2);

		    string Months = "JanFebMarAprMayJunJulAugSepOctNovDec";
		    month = Months.find(str_m) / 3 + 1;
		    day = stoi ( str_d);
		    year = stoi ( str_y);

		    if (year<20) year+=2000; else year+=1900;

	    } //else previous yahoo split format

    }// end of			else		if ((Mul1_begin == std::string::npos) && (Mul2_end == std::string::npos)) 

    


}//get_split_line_data()

///////////////////////////////////////////////////////////////////////////////////////////////////////////












int main( int argc, char *argv[] )  {

   string str_tickers_file;
   if( argc == 2 ) 	str_tickers_file = argv[1];
   else				str_tickers_file = "Stock_list_short.txt";

   cout << "using " << str_tickers_file << " as the file for all the tickers names to be updated"   << endl << endl; 
      
      
	//bool debug = true;
	bool debug_ONLY_output_ticker_names_from_Stock_list_txt = false;// true // false // this is for checking that the tickers from the file "stock_list.txt" were read correctly and it does not download any file just output the names

    // I M P O R T A N T   I N P U T S:
    // ---------------------------------------
    // Register to Alpha Vantage web site and get your personal api key (https://www.alphavantage.co/support/#api-key). It's for free!
    // ---------------------------------------
    // ---------------------------------------

    // ---------------------------------------
    // ---------------------------------------
    // A file of all the tickers seperated by lines, and the ticker is the first string before a comma like msft,microsoft \n in one line 
    // 
    
    // https://www.alphavantage.co/support/#support
    // Are there usage/frequency limits for the API service? 
    // We are proud to provide free API service for our global community of users and recommend that you make API requests sparingly (up to 5 API requests per minute and 500 requests per day) to achieve the best server-side performance. If you would like to target a larger API call volume, please visit premium membership.
    bool free_API_service = true;// USES THESE LIMITS: (up to 5 API requests per minute and 500 requests per day)
    int max_API_requests_per_minute = 5;// if that limit is reached the program sleeps for the rest of that minute and resume after that
    int max_API_requests_per_day = 500;// if the program reach this limit it output a list of all the tickers that were not updated and sleeps for 24 hours !!! and resume after that
    
    
	int time_limit_for_file_update = 15;// this is seconds ... after that time we move to the next ticker in the loop and print the non updated files at the end // till now 6 was the highest before it successeded//
    string stocks_data_directory = "./stocks";

    string splits_file_suffix ="_spl.csv";
    string splits_directory = "splits";// if the ticker is X,csv => it splits file will be ./splits/X_spl.csv
    bool splits_file_newest_dates_first = false;// because my split files from the old yahoo were in the order: oldest first, I write the str_split_data to the splits file in reverse order

	// I made it false because I thought that it will cause double split dates in the split file so now it is false till I find a solution  
	bool rewrite_the_ticker_file_newest_record = true;// if you update the csv file during the ticker trading hours the data of the newest record is not final but is constanly updated so this will rewrite the last record of the csv file









    std::vector<std::string> str_data;// this array holds the data in the ticker csv file that was before we downloaded the new records and will later be appended after the new records( because the records order in the file is newest first)
    std::vector<std::string> str_split_data;// this array holds the NEW split data of the ticker that will be written to the splits_directory folder
    std::vector<std::string> tickers_array;//this will hold the array of the tickers that there csv file should be appended like msft,xom,...



    

    string your_api_key = fill_tickers_array(str_tickers_file,tickers_array);
	if (your_api_key == "")
		return EXIT_FAILURE;// exit the program - the fill_tickers_array() function already outputed a message for how the user should get and insert his personal api key to the begining of the stock list txt file 

    unsigned int number_of_tickers = tickers_array.size();


    if (debug_ONLY_output_ticker_names_from_Stock_list_txt) {
        cout << "number_of_tickers found in the file '" << str_tickers_file << "' : " << number_of_tickers << endl << endl;
        for(unsigned int i=0;i<number_of_tickers;i++) {
            string str_ticker = tickers_array[i];//"msft";
            cout << (i+1) << " / " << number_of_tickers << " : '" << str_ticker << "'"  << '\n';// for debug
        }// for(unsigned int i=0;i<number_of_tickers;i++)

        cout << '\n' << "debug_num_of_tickers_found = true; so we return 0 and quit the program here without downloading any data"<< endl;
        return 0;
    }// if (debug_num_of_tickers_found)

    // -----------------------------------------------------------------------------------
    //                               Connect to Alpha Ventage - start
    // -----------------------------------------------------------------------------------

    // ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //          the only thing I cahnged here is the api_key ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    string lastHttpRequest = "";
    AvapiConnection* avapi_connection;

    try
    {
        avapi_connection = AvapiConnection::getInstance();
        avapi_connection->set_ApiKey(your_api_key);
    }
    catch(AvapiConnectionError& e)
    {
        cout <<  e.get_error() << endl;
        return EXIT_FAILURE;
    }

    auto& QueryObject = avapi_connection->GetQueryObject_TIME_SERIES_DAILY_ADJUSTED();

    // -----------------------------------------------------------------------------------
    //                               Connect to Alpha Ventage - end
    // -----------------------------------------------------------------------------------



    // loop over all the tickers and fill their data from Alpha Ventage API


	vector<string> list_of_files_that_was_not_updated;// we try for 60 sec and if not we move to the next ticker
	
    int count_API_requests_per_minute = 0;// max_API_requests_per_minute = 5;
    int count_API_requests_per_day = 0;// max_API_requests_per_day = 500;
    std::chrono::steady_clock::time_point start_time_point_for_counting_requests_per_minute = std::chrono::steady_clock::now();
   	
 	int max_time_it_took_to_update_a_file = -1;//init
	
	int trying_once_more = 0;
	for(int trying_iteration=1;;trying_iteration++) {// each end of the loop we check if (list_of_files_that_was_not_updated.size() < tickers_array.size())	// and if it is we copy the list_of_files_that_was_not_updated to the tickers_array and try again 
	
		number_of_tickers = tickers_array.size();
	
		for(unsigned int i=0;i<number_of_tickers;i++) {


			if (free_API_service) {// free_API_service has a limit of 500 API_requests per_day & 5 per minute
			
				// if we have reached the limit of max request per day, output a message and output the list of tickers not updated yet.
				// then go to sleep for the next 24 hours and continue after that time
				if (count_API_requests_per_day >= max_API_requests_per_day) {
					// output the remaining tickers list that hasn't been updated yet in case of the user wants to shut down his computer and continue later by copying the remaining list	to the stock_list.csv file	
					cout << endl<< endl<< endl<< endl << "==========================================================================" << endl;
					
					auto time_point_now = std::chrono::system_clock::now();
					std::time_t time_now = std::chrono::system_clock::to_time_t(time_point_now);
					cout  << endl << "Current time is: "  << std::ctime(&time_now) << endl;
					cout << "The free_API_service reached the limit of " << max_API_requests_per_day << " API_requests_per_day & going to sleep and will continue after 24 hours sleep" << endl << endl;	
					
					int num_of_tickers_not_yet_updated_before_long_sleep = list_of_files_that_was_not_updated.size() + (number_of_tickers-i+1);
					cout << "The following is the list of " << num_of_tickers_not_yet_updated_before_long_sleep << " tickers that have not been updated yet:" << endl << endl;
					for(unsigned int j=0;j<list_of_files_that_was_not_updated.size();j++) {// tickers that was already requested but not updated in this iteration
						cout << list_of_files_that_was_not_updated[j] << endl;
		   			}
					for(unsigned int j=i;j<number_of_tickers;j++) {// tickers that hasn't been requested yet in this iteration
						cout << tickers_array[j] << endl;
					}// for(unsigned int j=i;j<number_of_tickers;j++)
					cout  << endl << "==========================================================================" << endl << endl << endl << endl;
					std::this_thread::sleep_for(std::chrono::hours(24));// SLEEP FOR 24 HOURS ! 
					
					time_point_now = std::chrono::system_clock::now();
					time_now = std::chrono::system_clock::to_time_t(time_point_now);
					cout << endl << "The free_API_service continues after 24 hours sleep." << endl << "Current time is: " << std::ctime(&time_now) << endl;
						
					time_point_now = std::chrono::system_clock::now();
					time_now = std::chrono::system_clock::to_time_t(time_point_now);
					
					count_API_requests_per_day = 0;// reset
				}// if (count_API_requests_per_day > max_API_requests per_day)
				
				// check the time duration elapsed from the last time we started to count how many requests per minute
				std::chrono::steady_clock::time_point current_time_point = std::chrono::steady_clock::now();
				unsigned int duration_count_seconds_from_first_request_per_minute = std::chrono::duration_cast<std::chrono::seconds>(current_time_point - start_time_point_for_counting_requests_per_minute).count();

				// if we have reached the limit of max request per minute output a message and go to sleep for the rest of that minute
				if ((duration_count_seconds_from_first_request_per_minute < 60) && (count_API_requests_per_minute >= max_API_requests_per_minute)) {
					unsigned int seconds_to_sleep_for_less_than_a_minute = 60 - duration_count_seconds_from_first_request_per_minute;
					cout << "The free_API_service reached the limit of " << max_API_requests_per_minute << " max_API_requests_per_minute & going to sleep  for " << seconds_to_sleep_for_less_than_a_minute << " seconds " << endl;
					std::this_thread::sleep_for(std::chrono::seconds(seconds_to_sleep_for_less_than_a_minute));
					cout << "The free_API_service continues after a short sleep" << endl << endl;
					count_API_requests_per_minute = 0;// reset
					start_time_point_for_counting_requests_per_minute = std::chrono::steady_clock::now();// reset time_point
				}// if (count_API_requests_per_day > max_API_requests per_day)	
				else if (duration_count_seconds_from_first_request_per_minute >= 60) {// if a minute has elapsed reset the count & update the count start time
					count_API_requests_per_minute = 0;// reset
					start_time_point_for_counting_requests_per_minute = std::chrono::steady_clock::now();// reset time_point
				}// else if (count_seconds_from_first_request_per_minute >= 60)	
			}// if (free_API_service)


		    // str_data.clear() removes the previous ticker original data obtained from get_newest_record_date_from_the_csv_file_and_fill_str_data()
		    str_data.clear();// clear() function is used to remove all the elements of the vector container , thus making it size 0.
		    str_split_data.clear();// clears the precious ticker splits data
		    string str_ticker = stocks_data_directory + "/" + tickers_array[i];//"msft";
		    string str_ticker_csv = str_ticker + ".csv";
		    
		    // Launch a thread to check every 2 seconds if the csv file got all the data and was modified so we can continue to the next ticker in the loop
		    // I use join() at the end of the tickers loop to wait for the thread to end

		    
		   // thread t1(check_if_the_csv_file_was_updates,std::ref(str_ticker_csv),time_limit_for_file_update);
		    std::future<int> fu = std::async(std::launch::async, check_if_the_csv_file_was_updates,std::ref(str_ticker_csv),time_limit_for_file_update);
		    

		    // get rid of the directories in the ticker name and set it as str_ticker_without_directories
		    string str_ticker_without_directories = str_ticker;// this is ticker name that will be used in the Alpha Ventage Query after we upper case it
		    std::size_t found = str_ticker.rfind("/");
		    if (found!=std::string::npos)      // for example: "./S&P_500/X" => "X"
		        str_ticker_without_directories = str_ticker.substr(found+1);    

		    // make str_ticker uppercase - this is the ticker name that we will use in Alpha Ventage Query
		    string str_ticker_upper_case = str_ticker_without_directories;    for (auto & c: str_ticker_upper_case) c = toupper(c);

		    cout << (i+1) << " / " << number_of_tickers << " : '" << str_ticker << "' and the ticker name without directories and in uppercase is :'"<< str_ticker_upper_case <<"'"  << endl;// for debug

		    string str_ticker_file = str_ticker + ".csv";

		    // Splits directory filename
		    // -------------------------
		    string str_ticker_file_split = str_ticker;
		    string splits_directory_with_slashes = "/" + splits_directory + "/";
		    string key="/";
		    found = str_ticker.rfind(key);
		    if (found!=std::string::npos){      // for example: "./S&P_500/X" => "./S&P_500/splits/X_spl.csv"
		        str_ticker_file_split = str_ticker;     
		        str_ticker_file_split.replace (found,key.length(),splits_directory_with_slashes);     
		        str_ticker_file_split += splits_file_suffix;
		    }else // X => "./splits/X_spl.csv"
		        str_ticker_file_split = "." + splits_directory_with_slashes + str_ticker + splits_file_suffix;
		    // Output splits directory filename
		    cout << (i+1) << " / " << number_of_tickers  << " : '" << "str_ticker_file_split : '" << str_ticker_file_split << endl;// for debug



		    // these values will be filled with the previous newest record time before we get the new data
		    int f_day=1; //newest_file_day
		    int f_month=1; //newest_file_day
		    int f_year=1000; //newest_file_day
		    // get_newest_record_date_from_the_csv_file_and_fill_str_data
		    get_newest_record_date_from_the_csv_file_and_fill_str_data(str_ticker_file,str_data,f_day,f_month,f_year,rewrite_the_ticker_file_newest_record);// all the varibales are references

		    // get the time now
		    std::time_t t = std::time(0);   // get time now
		    std::tm* now = std::localtime(&t);
		    std::cout << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << "\n";
		    int current_day = now->tm_mday;    int current_month = now->tm_mon + 1;    int current_year = now->tm_year + 1900;
		
		    // find the number of days elapsed from the last data date record in the ticker csv file
		    // https://stackoverflow.com/questions/14218894/number-of-days-between-two-dates-c
		    struct std::tm a = {0,0,0,f_day,(f_month-1),(f_year-1900)};// {0,0,0,5,6,104};  July 5, 2004
		    struct std::tm b = *now;
		    std::time_t x = std::mktime(&a);
		    std::time_t y = std::mktime(&b);
		    double difference_init_val = -99999;// to be compared later
		    double difference = difference_init_val;
		    if ( x != (std::time_t)(-1) && y != (std::time_t)(-1) )
		    {
		        difference = std::difftime(y, x) / (60 * 60 * 24);
		        std::cout << std::ctime(&x);
		        std::cout << std::ctime(&y);
		        std::cout << "difference = " << difference << " days" << std::endl;
		    }

		    // find the approximate records elapsed from the last data date record in the ticker csv file
		    int approx_market_days_diff = (int)(difference * (252.0/365.25));// in 2017 there were 251 https://en.wikipedia.org/wiki/Trading_day
		    std::cout << "approx_market_days_diff = " << approx_market_days_diff << " market days" << std::endl;

		    // determine if we need more the compact size data of 100 records 
		    bool compact_mode = true;
		    if ((difference == difference_init_val) || 
		        (approx_market_days_diff > 90)) compact_mode = false;// compact_mode is 100 market days so I use 90 to be safe

		    if ( ! compact_mode) std::cout << "using the full stock data download instead of compact because there are more than 90 new records missing" << std::endl;

		    // set the download file size parameter
		    Const_TIME_SERIES_DAILY_ADJUSTED_outputsize outputsize;
		    if (compact_mode)
		        outputsize = Const_TIME_SERIES_DAILY_ADJUSTED_outputsize::compact;
		    else
		        outputsize = Const_TIME_SERIES_DAILY_ADJUSTED_outputsize::full;
		        
		    // updating counter for the free_API_request 
		    count_API_requests_per_day++;
		    count_API_requests_per_minute++; 
		       
		    // -------------------------------------------------------------------------------------------------------------
		    // -------------------------------------------------------------------------------------------------------------
		    // This is the continued Alpha Ventage AvApi with our ticker: 'str_ticker_upper'_case and the 'outputsize' we need
		    // -------------------------------------------------------------------------------------------------------------
		    auto Response = QueryObject.Query(   
		             str_ticker_upper_case
		             ,outputsize
		             ,Const_TIME_SERIES_DAILY_ADJUSTED_datatype::json);

		    //cout << endl <<  "******** RAW DATA TIME SERIES DAILY ********"<< endl;
		    // the next cout outputd all the data that was downloaded in json format
		    // cout <<  Response.get_RawData() << endl << endl;// for debug

		    //cout << "******** STRUCTURED DATA TIME SERIES DAILY ********"<< endl;

		    if(Response.get_Data().isError())
		    {
		        cerr << Response.get_Data().get_ErrorMessage() << endl;
		    }
		    else {

		        auto& MetaData  = Response.get_Data().get_MetaData();
		        auto& TimeSeries = Response.get_Data().get_TimeSeries();
	cout << "Current Time : "<< asctime(localtime(&ctt)) << std::endl;
		        //cout << "========================" << endl;
		        //cout << "Information: " <<  MetaData.get_Information() << endl;
		        cout << "Symbol: " <<  MetaData.get_Symbol() << endl;
		        //cout << "LastRefreshed: " <<  MetaData.get_LastRefreshed() << endl;
		        cout << "OutputSize: " <<  MetaData.get_OutputSize() << endl;
		        //cout << "TimeZone: " <<  MetaData.get_TimeZone() << endl;
		        //cout << "========================" << endl;
		        cout << "========================" << endl;

		        // -------------------------------------------------------------------------------------------------------------
		        // -------------------------------------------------------------------------------------------------------------




		        // -------------------
		        // OUTPUT RECORDS DATA
		        // -------------------
		        
		        int oldest_acquired_split_year=-1,oldest_acquired_split_month=-1,oldest_acquired_split_day=-1;                  
	 
		        // Now we start writing the aquired data to the file in 3 steps: 1. write the first titles line 2. write the the new aquired data 3. write the original data that was in ticker file before
		        ofstream outFile (str_ticker_file);// open the file for writing
		        if (outFile.is_open())
		        {
		            std::stringstream stock_data_buffer;

		            // 1. write the first titles line
		            stock_data_buffer << "Date,Open,High,Low,Close,Volume,Adj Close\n";//first line in the file

		            // 2. write the the new acquired data
		            int year,month,day; 
		            string str_date;
		            string str_d,str_m,str_y;
		            // ----------------------------------------------------------------------------------------------
		            //   get each day data and put it in the file - till we find a date that is already in the file
		            // ----------------------------------------------------------------------------------------------
		                for(auto& element : TimeSeries)
		                {
		                    str_date = element.get_DateTime();// "2018-05-15"
		                    str_y = str_date.substr(0,4);   str_m = str_date.substr(5,2);   str_d = str_date.substr(8,2);
		                    year = stoi ( str_y);month = stoi ( str_m);     day = stoi ( str_d);
		                   	//cout << " year - month - day  "<< year << month << day << endl;
		                    // if the day is already in the file , we don't need to write more data to the file
		                    if (((year <= f_year) && (month <= f_month)) && (day <= f_day))
		                        break;

		                    double split_val = stod(element.get_SplitCoefficient());
		                    
		                   	// this is debug for testing last file record update if the last record in the data file is 2016-05-06
		                   	// and there is already a split in the split file with that date - so we need to skip it so we won't write it twice
		                   	//cout << " year << month << day  "<< year << month << day << endl;
		                   	/*
		                    if ((year == 2016) && (month == 5) && (day == 6)) {
		                    	split_val = 2.0;
		                    	cout << "setting split  "<< year << "-" << month << "-" << day << " : " << split_val << endl;                        	
		                    }
		                    */
		                    
		                    if (split_val != 1) {
		                        //output 1 line of new data from Alpha Ventage to the file
		                        string str_split = str_date + "," + element.get_SplitCoefficient();
		                        // push it to the array - order: newest first (this is Alpha Ventage order)
		                        str_split_data.push_back(str_split); 
		                        
		                        oldest_acquired_split_year = year;
		                        oldest_acquired_split_month = month;
		                        oldest_acquired_split_day = day;
		                    }

		                    //output 1 line of new data from Alpha Ventage to the file
		                    stock_data_buffer  << str_date <<","<< element.get_Open() <<","<< element.get_High() <<","<< element.get_Low() <<","<< element.get_Close() <<","<< element.get_Volume() <<","<< element.get_AdjustedClose() << endl;
		                }// for(auto& element : TimeSeries)
		            // -----------------------------------------------------------------------------------
		            // -----------------------------------------------------------------------------------

		            // 3. write the original data that was in ticker file before
		            for(auto const& value: str_data)
		              stock_data_buffer << value << endl;

					// Writing stringstream contents into ofstream
		            outFile << stock_data_buffer.rdbuf();
		            outFile.close();;// close the open file
		        }//  if (myfile.is_open())
		        else cout << "Unable to open the file " << str_ticker_file << " to write" << endl;

		        if (str_split_data.size() > 0) {// only if there is a new split data we open the split data file
		            cout << "number of new splits data acquired:" << str_split_data.size() << endl;
		            
		        	// -------------------
		            // OUTPUT SPLIT DATA
		            // -------------------
		                    
		            // comment: because we can update the last record data in the csv file(because maybe the csv file was updated during the trading hours)
		            // we can have double split data for the same day in the split data file
		            // so we check for this 
		            int nw_split_file_year=0,nw_split_file_month=0,nw_split_file_day=0;//newest split file date
		            int split_file_year=0,split_file_month=0,split_file_day=0;
		            double divider;
		            string line,first_date_line;
		            std::vector<std::string> str_previous_splits_data;
		            
		            fstream my_split_file(str_ticker_file_split, ios::in | ios::out);
		            
		            if (my_split_file.is_open())
		            {
		                bool first_date_line_was_found = false;
		                unsigned int newest_split_day_index = 0;
		                int id=-1;
		                while ( getline (my_split_file,line) )
		                {
							if (line == "")
								continue;
		                    id++;
		                    //cout << "getline (my_split_file) : " << line << '\n';// for debug
		                    get_split_line_data(line,split_file_day,split_file_month,split_file_year,divider); 
		                    //cout << "getline (my_split_file) => split_file_year : " << split_file_year << '\n';// for debug
							if (split_file_year != 0){
		                        if ((split_file_year > nw_split_file_year)
		                        || ((split_file_year == nw_split_file_year)&&(split_file_month >= nw_split_file_month))
		                        || ((split_file_year == nw_split_file_year)&&(split_file_month == nw_split_file_month) && (split_file_day > nw_split_file_day))                            
	)
		                            		 {
		                                newest_split_day_index = id;
		                                nw_split_file_year = split_file_year;
		                                nw_split_file_month = split_file_month;
		                                nw_split_file_day = split_file_day;
		                                //cout << "mid newest split file line: " << nw_split_file_day << " , " << nw_split_file_month << " , " << nw_split_file_year  << '\n';// for debug   
								}                  
							}// if (split_file_year != 0){

		                    str_previous_splits_data.push_back(line);
		                }// while ( getline (myfile,line) )
				 cout << "oldest_acquired_split: " << oldest_acquired_split_day << " , " << oldest_acquired_split_month << " , " << oldest_acquired_split_year  << '\n';// for debug 
				 cout << "newest split file line: " << nw_split_file_day << " , " << nw_split_file_month << " , " << nw_split_file_year  << '\n';// for debug                        
				 
						 // checking if the newest split in the split file is the oldest acquired split from Alpha Vetage
						 // if so we will skip writing it again
					       
		          			 cout << "newest split file line index: " << newest_split_day_index << '\n';// for debug      
		                
		                bool skip_writing_the_oldest_acquired_split_data = false;
		                if ((oldest_acquired_split_year == nw_split_file_year)&&
		                            (oldest_acquired_split_month == nw_split_file_month)&&
		                            (oldest_acquired_split_day == nw_split_file_day))
		                {
		                    skip_writing_the_oldest_acquired_split_data = true;
		                    cout << "skipping writing the oldest acquired split data because it is already written in the split file" + '\n';// for debug      
		                }

		                my_split_file.clear();// Sets a new value for the stream's internal error state flags.
		                my_split_file.seekp(ios_base::beg);// write at the beginning of the split file
		                
		                // Now we start writing the aquired data to the file in 3 steps: 1. write the first titles line 2. write the the new aquired data 3. write the original data that was in ticker file before
		                //ofstream my_split_file(str_ticker_file_split,  ios::out | ios::app);// ios::out: Open for output operations // ios::app: appending the content to the current content of the file.
		                //if (my_split_file.is_open())
		                //{
		                // because my split files from the old yahoo were in the order: oldest first, I write the str_split_data to the splits file in reverse order
		                
		                
		                if (splits_file_newest_dates_first) {
		                    // newly acquired split data from AlphaVentage
		                    if(str_split_data.size() > 0){
		                        for (unsigned i=0; i < str_split_data.size(); i++) {
		                            if (skip_writing_the_oldest_acquired_split_data) {
		                                if (i==(str_split_data.size()-1))
		                                    break;
		                            }
		                            my_split_file << str_split_data[i] << endl;
		                        }
		                    }// if(str_split_data.size() > 0)
		                    
		                    // previous data from the orginal split file
		                    if(str_previous_splits_data.size() > 0){
		                        if(newest_split_day_index==0) {
		                            for (unsigned i=0; i < str_previous_splits_data.size(); i++)
		                                my_split_file << str_previous_splits_data[i] << endl;
		                        }// if(newest_split_day_index==0)
		                        else {// we out put it in reverse because the newest is not the first line so it must be the last line
		                            for (unsigned i = str_previous_splits_data.size(); i-- > 0; )
		                                my_split_file << str_previous_splits_data[i] << endl;
		                        }// else of if(newest_split_day_index==0)
		                    }// if(str_previous_splits_data.size() > 0)
		                } else {
		                    // split file order: oldest dates first
		                    // ====================================
		                    // previous data from the orginal split file
		                    if(str_previous_splits_data.size() > 0){
		                        if(newest_split_day_index==0) {
		                            for (unsigned i = str_previous_splits_data.size(); i-- > 0; )
		                                my_split_file << str_previous_splits_data[i] << endl;
		                        }// if(newest_split_day_index==0)
		                        else {// we out put it in reverse because the newest is not the first line so it must be the last line
		                            for (unsigned i=0; i < str_previous_splits_data.size(); i++)
		                                my_split_file << str_previous_splits_data[i] << endl;
		                        }// else of if(newest_split_day_index==0)
		                    }// if(str_previous_splits_data.size() > 0)

		                    // newly acquired split data from AlphaVentage
		                    if(str_split_data.size() > 0){
		                        for (unsigned i = str_split_data.size(); i-- > 0; ) {
		                            if (skip_writing_the_oldest_acquired_split_data) {
		                                if (i==0)
		                                    break;
		                            }
		                            my_split_file << str_split_data[i] << endl;
		                        }
		                    }// if(str_split_data.size() > 0){
		                }// end of else of if (splits_file_newest_dates_first)

		                my_split_file.close();;// close the open file
		            }//  if (my_split_file.is_open())
		            else {
		                // before we create a new split file we check if there any new splits acquired from AlphaVentage
		                if(str_split_data.size() > 0){
		                    cout << "Unable to open the split file " << str_ticker_file_split << " to write creating a new one" << endl;
		                    
		                    ofstream my_split_file (str_ticker_file_split);// open a new file for writing
		                    if (my_split_file.is_open()) {
		                        if (splits_file_newest_dates_first) {// newly acquired split data from AlphaVentage
		                           for (unsigned i=0; i < str_split_data.size(); i++) my_split_file << str_split_data[i] << endl;
		                        } else {
		                           for (unsigned i = str_split_data.size(); i-- > 0; ) my_split_file << str_split_data[i] << endl;
		                        }// end of else of if (splits_file_newest_dates_first)
		                    }// end of if (myfile.is_open())
		                    my_split_file.close();;// close the open file
		                }// if(str_split_data.size() > 0)

		            }// end of else of if (my_split_file.is_open())

		        }// if (str_split_data.size() > 0)


		        // -----------------------------------------------------------------------------------
		        // -----------------------------------------------------------------------------------

		    }// else of if(Response.get_Data().isError())

		// -----------------------------------------------------------------------------------
		// -----------------------------------------------------------------------------------
		
			//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		    int seconds_it_took_to_update_file = fu.get();
		    
		    if (seconds_it_took_to_update_file > -1) {
				cout << "Finished waiting: file was updated within " << seconds_it_took_to_update_file << " seconds" << endl << endl;
				max_time_it_took_to_update_a_file = max(max_time_it_took_to_update_a_file, seconds_it_took_to_update_file);
			} else {
		    	cout << "Finished waiting: file wasn't updated for some unknown reason " << endl << endl;	
		    	list_of_files_that_was_not_updated.push_back(tickers_array[i]);
		    }	

		//t1.join(); // somehow we forgot to join this to main thread - will cause a crash.
		}// for(unsigned int i=0;i<number_of_tickers;i++) {
		
		if (max_time_it_took_to_update_a_file > -1) {
	   		 cout  << endl << endl << "Maximum time it took to update a file: " << max_time_it_took_to_update_a_file << " seconds" << endl;// 6 second was the maximum I got till now
		} else {
			cout  << endl << endl << "No file was updated" << endl;
		}
		
		
		if (list_of_files_that_was_not_updated.size() > 0) { 
			// write a list of all the tickers that were not updated for some reason    
			cout << endl << endl << "List of " << list_of_files_that_was_not_updated.size() << " out of " << number_of_tickers << " tickers that were not updated within the time limit of " << time_limit_for_file_update <<" seconds:" << endl << endl;
			

			string str_not_updated_filename;
			string str_stocks_data_directory_long = stocks_data_directory + "/";
			std::size_t found;
			for(unsigned int i=0;i<list_of_files_that_was_not_updated.size();i++) {
				cout << list_of_files_that_was_not_updated[i] << endl;
		   	}
		} else cout << endl << endl << "All the tickers were updated successfully" << endl << endl;
	
		if (list_of_files_that_was_not_updated.size() == tickers_array.size()) {
			// 	break;
		
			if (trying_once_more >= 1)
				break;
			else {
				list_of_files_that_was_not_updated.clear();// RESETS THIS ARRAY
				//time_limit_for_file_update = max_time_it_took_to_update_a_file;// we dont want to wait too long
				trying_once_more++;
			} 
		} else {// if (list_of_files_that_was_not_updated.size() < tickers_array.size())
			trying_once_more = 0;// Reset because now something was updated
			
			cout << endl << endl << "=========================================" << endl << endl;
			cout << endl << endl << " TRYING AGAIN // RESETS THIS ARRAY ITERATION NUMBER: " << trying_iteration << endl << "FOR THE LIST OF FILES THAT WERE NOT UPDATED" <<  endl;
			cout << " BECAUSE SOME TICKERS WERE SUCCESSFULY UPDATED BY THE LAST ITERATION" << endl << endl;
			cout << endl << endl << "=========================================" << endl << endl;
			tickers_array.clear();// RESETS THIS ARRAY
			// Copying vector by copy function 
			copy(list_of_files_that_was_not_updated.begin(), list_of_files_that_was_not_updated.end(), back_inserter(tickers_array));
			list_of_files_that_was_not_updated.clear();// RESETS THIS ARRAY
		}
	
	}//	for(int trying_iteration=1;;trying_iteration++)
	
	
    return EXIT_SUCCESS;
}
