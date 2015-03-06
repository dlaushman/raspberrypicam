[UserConfigs]
   VFYear = string(min=2, max=4)
   VFLocation = string(max=9)
   VFSponsor = string(max=15)
   VFCar = string(max=9)
   VFDriver = string(max=15)
   VFOverwrite = option('0', '1')
   VCExposure = option('auto', 'night', 'nightpreview', 'backlight', 'spotlight', 'sports', 'snow', 'beach',' verylong', 'fixedfps', 'antishake', 'fireworks')
   VCMaxDuration = integer(5, 300)
   VCStopDelay = option('0', '1', '2','3', '4', '5')
   VCVidStab = option('0', '1')
   VCAutoStart = option('0', '1')
   VFSponsorFile = string(min=0, max=255)
   WiFiSSID = string(min=1, max=20)
   WiFiKey = string(min=10, max=10)
   WiFiIP = ip_addr
   WiFiMode = option('b', 'g', 'n')
   WiFiFlag = option('0', '1')