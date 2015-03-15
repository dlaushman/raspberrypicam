#!/bin/bash
if [ $# -ne 1 ]; then
  echo "usage: $0 <name of piracecam tarball>"
  exit 1
fi
cd /
stat $1 2>/dev/null
if [ $? -ne 0 ]; then
	"Are you sure you specified the full filename to the installation tarball?"
	exit 1
fi
cd -
ret=0
rpi-update
ret=`expr $ret + $?`
apt-get -y update
ret=`expr $ret + $?`
apt-get install -y gpac
ret=`expr $ret + $?`
apt-get install -y isc-dhcp-server
ret=`expr $ret + $?`
apt-get install -y vsftpd
ret=`expr $ret + $?`
apt-get install -y hostapd
ret=`expr $ret + $?`
apt-get install -y apache2
ret=`expr $ret + $?`
apt-get install -y libapache2-mod-wsgi
ret=`expr $ret + $?`
apt-get install -y python-setuptools
ret=`expr $ret + $?`
easy_install configobj
ret=`expr $ret + $?`
easy_install flask
ret=`expr $ret + $?`
cd /
ret=`expr $ret + $?`
tar xvf $1
ret=`expr $ret + $?`
cp /opt/piracecam/bin/hostapd /usr/sbin/hostapd
ret=`expr $ret + $?`
chown www-data.www-data /opt/piracecam/etc/piracecam.conf
ret=`expr $ret + $?`

if [ $ret -ne 0 ]; then
	echo "Something didn't get installed or didn't run correctly."
	exit 1
fi
echo "Looks like everything was installed correctly.  You are now ready to use your RaspiberryPiCam."
echo "You may want to customize the configurations to suit your needs. They are in /opt/piracecam/etc/"
exit 0
