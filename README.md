# raspberrypicam
Code and things related to the raspberry pi camera project for in car racing videos.  Once you download the videos from your pi, you can immediately bring them into a Traqmate session without transcoding.  Hooray!  

Todo: 
    Increase framerates
    Add audio recording (it'd be nice to hear the engine)
    Incrase speed for downloading the videos 
    	()I did put the USB Wifi device at the end of a 8 foot USB cable and mounted it near the top of the car right in front of the windsheild and that helped drastically, but I think a faster Wifi device would be handy.)

Its 2015 and I've not done any updates the code since May, 2014 except exit prcd if there is no piface available. I used the unit at races last yearand it worked pretty well. The framerate was limited to 25fps, which turns out OK, but for racing something higher would be better. But there are new support framerates as high as 90, but I will be testing 48fps next month and will branch or update this project acccordingly.  The other thing that would be nice 

prcd uses the PiFace libs to control the leds and watch for button pushes and controls the camera video recording with raspivid and transcoding processing using MP4Box.  It also controls the WiFi stoping and starting

prc-web relies on apache, apache-wsgi, flask and python as a web based interface to change runtime parameters ofthe unit and the ability to view and download the videos

Hardware Deps:

1) Raspberry Pi B
2) PiFace IO card
3) Raspberry Pi Camera
4) EdiMax 150Mbps Wireless IEEE802.11b/g/n nano USB adaptor (got on amazon)
5) Downstep transformer doodad from 12VDC to 5VDC
6) Raspberry PiFace enclosure


Step 1) Install Raspbian and enable camera support and any piface requirements
Step 2) sudo su
Step 3) Get and place the piracecam-05-31-2014-1820.tar tarball on your pi
Step 4) Install MP4Box encoder
Step 5) create a bash script as follows:

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

Step 5) Run the script with the name of the filename of the tarball as the only argument
  NOTE THAT THIS TARBALL is going to write and change rc files to automatically start the prcd and apache, etc...
Step 6) Edit your base config files for pathing and customization
Step 7) Reboot and watch the leds on the piface
Step 8) Point your laptop to whatever IP you assigned to the pi's wifi AP


