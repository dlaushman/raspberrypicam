#!/bin/bash
if [ $? -ne 2 ]; then
  echo "usage: $0 <name of piracecam.tgz>"
  exit 1
fi
apt-get -y update
apt-get install -y gpac
apt-get install -y isc-dhcp-server
apt-get install -y vsftpd
apt-get install -y hostapd
apt-get install apache2
apt-get install libapache2-mod-wsgi
cd /
tar zxvf $1
cp /opt/piracecam/bin/hostapd /usr/sbin/hostapd
easy_install configobj
chown www-data.www-data /opt/piracecam/etc/piracecam.conf
exit

