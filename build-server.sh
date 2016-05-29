echo ============== BUILD SERVER ===============

#save current dir
CD=`pwd`

#save related script dir
P=$(dirname $0);

#set the dir
cd $P

echo build server
cd ./server/build
cmake ..
make

#restore current dir
cd $CD



