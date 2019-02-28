#!/bin/bash 

# init
function pause(){
   read -p "$*"
}


echo "Running DAILY_ADJUSTED_UPDATER"
echo "______________________________"

cd /home/mb/AvapiExample/

# if you want to recompile the c++ file use the following line without the comment symbol # at the begining (after changing the directories names)
# g++ -L /home/mb/AvapiExample/Avapi/ -I /home/mb/AvapiExample/ -std=c++11 TIME_SERIES_DAILY_ADJUSTED_UPDATER.cpp -o DAILY_ADJUSTED_UPDATER -lavapi -lpthread

export LD_LIBRARY_PATH=/home/mb/AvapiExample/Avapi/:$LD_LIBRARY_PATH

./DAILY_ADJUSTED_UPDATER Stock_list.txt > output.txt

# keep the terminal open
# call it
pause 'Press [Enter] key to continue...'
