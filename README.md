# raspberrypicam
Code, packages and configurations related to the raspberry pi camera project for in car racing videos.  Once you download the videos from your pi, you can immediately bring them into a Traqmate session without transcoding.  Hooray!  :tada:

Todo: 
  *  Add audio recording (it'd be nice to hear the engine)
  *  Incrase speed for downloading the videos 
    (I did put the USB Wifi device at the end of a 8 foot USB cable and mounted it near the top of the car right in front of the windsheild and that helped drastically, but I think a faster Wifi device would be handy.)


**Main Components:**

* prcd uses the PiFace libs to control the leds and watch for button pushes and controls the camera video recording with raspivid and transcoding processing using MP4Box.  It also controls the WiFi stoping and starting

* prc-web relies on apache, apache-wsgi, flask and python as a web based interface to change runtime parameters ofthe unit and the ability to view and download the videos


**Hardware Deps:**

* Raspberry Pi B
* PiFace IO card
* Raspberry Pi Camera
* 1 Meter ribbon camera cable
* EdiMax 150Mbps Wireless IEEE802.11b/g/n nano USB adaptor (got on amazon)
* Downstep transformer doodad from 12VDC to 5VDC (to mount in an automobile)
* Raspberry PiFace enclosure


**Steps To Install on Pi B with wheezy raspbian:**

* Burn wheezy raspbian your SD card
* Connect your pi to the internet because the ./install.sh uses apt-get and rpi-update
* Boot for first time and you'll see the raspi-config setup menu
* Expand the filesystem
* Enable Camera
* Advanced -> Enable SPI (and Enable it to load module automatically)
* Advanced -> Enable SSH (if you want)
* exit menu
* sudo su and change root password to whatever you like
* reboot
* Login as root
* Copy the installation tarball and install.sh to your pi
* Run the installat.s file: # ./install.sh /root/piracecam-03-14-2015-1431.tar
*     NOTE: the pathname of the installation tarball needs to be absolute/full
* Edit your base config files for pathing and customization in /opt/piracecam/etc/

* PiFace Button 1 is the Recording toggle


**Development Tooos:**

The packaging and installation of the this system is pretty dodgy and needs some retooling. As it is now, if you want to create a new release tarball its simply a matter of changing whatever you want on a a pi with this system installed and then creating another tarball using the scripts I provided in the devenvtools.  
To compile prcd you'll need the piface libraries. https://github.com/piface


** Other Info:**

* Info on the video framerates:  http://www.raspberrypi.org/new-camera-mode-released
