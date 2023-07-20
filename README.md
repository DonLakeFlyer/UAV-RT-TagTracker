# Herelink QGC version - WIP

* Started repo from QGC master
    * Existing version of Herelink QGC was forked to deeply to be able to bring it up to date with latest QGC
    * Instead will use QGC custom build architecture instead of fork for Herelink version
* Added base custom build setup/organization
    * Using the custom build architecture for the Herelink version instead of a more forked approach means that as many changes as possible occur withing the custom build source. It also means that mainline QGC code is generally left alone. This makes bringing the Herelink version of QGC up to date with upstream QGC much easier. Way fewer merge conflicts since mainline is left alone.
* Auto-connect settings to allow Herelink QGC to connect to Airunit are done through custom build settings overrides:
    * UDP defaults changed to connect to Airlink
    * Other auto-connect types disabled
    * Auto-connect settings UI hidden
* Other Settings and Options overrides
    * Turn off warning for calibrating over WiFi. This is for flay ESP8266 setups. Airunit provides solid WiFi.
    * Hide the firmware upgrade page
    * Turn off multi-vehicle support
* Build changes specific to Herelink (all in custom.pri)
    * Disable TAISYNC support
* Android manifest changes
    * Make QGC be the home app
        * This currently requires direct modification of AndroindManifest.xml in mainline. Custom build arch should allow for this somehow. Will fix later.
* Allow for custom builds of Herelink QGC
    * This means that it should not be too painful to create your own custom version of the standard Herelink QGC for your own company/needs.
    * So far this means:
        * Be careful with custom.pri structuring to prevent merge conclicts
        * Allow the Herelink QGCCorePlugin and QGCOptions classes to be used in the middle of a class hierarchy, not just final.