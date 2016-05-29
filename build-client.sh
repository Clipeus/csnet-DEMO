echo ============== BUILD CLIENT ===============

#save current dir
CD=`pwd`

#save related script dir
P=$(dirname $0);

#set the dir
cd $P

echo build client
cd ./client/build
cmake ..
make

#restore current dir
cd $CD


