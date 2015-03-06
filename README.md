# raspberrypicam
Code, packages and configurations related to the raspberry pi camera project for in car racing videos.  Once you download the videos from your pi, you can immediately bring them into a Traqmate session without transcoding.  Hooray!  :tada:

Todo: 
  *  Increase framerates
  *  Add audio recording (it'd be nice to hear the engine)
  *  Incrase speed for downloading the videos 
    (I did put the USB Wifi device at the end of a 8 foot USB cable and mounted it near the top of the car right in front of the windsheild and that helped drastically, but I think a faster Wifi device would be handy.)

Its 2015 and I've not done any updates the code since May, 2014 except exit prcd if there is no piface available. I used the unit at races last yearand it worked pretty well. The framerate was limited to 25fps, which turns out OK, but for racing something higher would be better. But there are new support framerates as high as 90, but I will be testing 48fps next month and will branch or update this project acccordingly.  The other thing that would be nice 

prcd uses the PiFace libs to control the leds and watch for button pushes and controls the camera video recording with raspivid and transcoding processing using MP4Box.  It also controls the WiFi stoping and starting

prc-web relies on apache, apache-wsgi, flask and python as a web based interface to change runtime parameters ofthe unit and the ability to view and download the videos

**Hardware Deps:**

* Raspberry Pi B
*  PiFace IO card
* Raspberry Pi Camera
* EdiMax 150Mbps Wireless IEEE802.11b/g/n nano USB adaptor (got on amazon)
* Downstep transformer doodad from 12VDC to 5VDC
* Raspberry PiFace enclosure

**Steps:**

* Install Raspbian and enable camera support and any piface requirements
* sudo su
* Get and place the piracecam-05-31-2014-1820.tar tarball on your pi
* Install MP4Box encoder
* Create a bash script as follows:
* Run the script with the name of the filename of the tarball as the only argument
* NOTE THAT THIS TARBALL is going to write and change rc files to automatically start the prcd and apache, etc...
* Edit your base config files for pathing and customization
* Reboot and watch the leds on the piface
* Point your laptop to whatever IP you assigned to the pi's wifi AP


