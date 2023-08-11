# Herelink QGC version - WIP

Use WIP version on Herelink - based on QGC master
* Support still not working
    * Video
    * Save video to SD card
* Download the latest APK from GitHub Actions Artifacts
* Put it on an sd card
* Put the sd card into your Herelink
* Swipe down from top of screen to get device panel
* At the bottom of the device panel you should see the sd card listed
* Click it
* Click the Herelink-QGroundContro.apk on the sd card to install

Switch back/forth between factory Herelink QGC and the WIP as your home app
* Swipe down from top of screen to get device panel
* Click the Gear icon to get into Android Settings
* Selects Apps
* Select current app you have as your home app
* Select Home App
* Switch it to what you want Factory or WIP

Change log
* Started repo from QGC master
    * Existing version of Herelink QGC was forked too deeply to be able to bring it up to date with latest QGC
    * Will use QGC custom build architecture instead of fork for Herelink version
* Added base custom build setup/organization
    * Using the custom build architecture for the Herelink version instead of a more forked approach means that as many changes as possible occur within the custom build source. It also means that mainline QGC code is generally left untouched. This makes bringing the Herelink version of QGC up to date with upstream QGC much easier. Way fewer merge conflicts since mainline source is left alone.
* Auto-connect settings to allow Herelink QGC to connect to Airunit are done through custom build settings overrides:
    * UDP defaults changed to connect to Airlink
    * Other auto-connect types disabled
    * Auto-connect settings UI hidden
* Other Settings and Options overrides
    * Turn off warning for calibrating over WiFi. This warning is for crappy ESP8266 setups. Airunit provides solid WiFi.
    * Hide the firmware upgrade page
    * Turn off multi-vehicle support
* Build changes specific to Herelink (all in custom.pri)
    * Disable TAISYNC support
* Android manifest changes
    * Make QGC be a home app
        * This currently requires direct modification of AndroindManifest.xml in mainline. Custom build arch should allow for this somehow. Will fix later.
* Allow for custom builds of Herelink QGC
    * This means that it should not be too painful to create your own custom version of the standard Herelink QGC for your own company/needs.
    * So far this means:
        * Be careful with custom.pri structuring to prevent merge conclicts both from upstream regular QGC as well as your own custom Herelink version.
        * Allow the Herelink QGCCorePlugin and QGCOptions classes to be used in the middle of a class hierarchy, not just final.