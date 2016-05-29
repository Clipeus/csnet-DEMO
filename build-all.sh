echo ============== BUILD ALL ===============

#save current dir
CD=`pwd`

#save related script dir
P=$(dirname $0);

#set the dir
#cd $P

$P/build-server.sh
$P/build-client.sh

#restore current dir
cd $CD
