# FileConfig

## Introduction

FileConfig is an Arduino library for reading settings from a configuration file on an SD card.  
It is based on SDFileConfig library by Claus Mancini (https://https://github.com/Fuzzer11/SDconfig).

It brings up the following improvements and features:  
- support of section to organize key/value pairs
- case insensivity option on key name matching
- support of blank between the key, '=' and the value
- ignore error option on badly formatted line and continue to the next valid key/value pair
- since you now provide a FS, this library works fine with SD_MMC and not only SD

Given a configuration file with settings whose format is:
```
# for comments
setting=value

# Blank support
setting1     = some_value
    longSetting2 = another_value

# Badly formatted section
[badsection] unexpected

# Badly formatted line
unexpected format

[section1]
# for any string
setting3 = some_string
# able to be parsed as a boolean
setting4 = true
# duplicated setting in different section
dupSetting = Value1

[section2]
# able to be parsed as an integer
setting5   = 123
# able to be parsed as an IP address
setting6   = 10.1.1.2
# duplicated setting in different section
dupSetting = Value2  
```

It is necessary to specify the configuration file name (ex: **example.cfg**) to load its contents through the library methods.

## Usage

The following code shows the basic usage of the library taking the settings from the previous configuration file example:

```C
#include <SD_MMC.h>
#include <FileConfig.h>

FileConfig cfg;

void setup() {
  int maxLineLength = 30;
  int maxSectionLength = 20;
  bool ignoreCase = true;
  bool ignoreError = true;

  Serial.begin(115200);
  SD_MMC.begin();

  // Could be SD, SD_MMC or any other FS compatible object
  fs::FS &fs = SD_MMC;
  if (cfg.begin(fs, "/example.cfg", maxLineLength, maxSectionLength, ignoreCase, ignoreError)) {
    while (cfg.readNextSetting()) {
      if (cfg.nameIs("setting3")) {
        char *string = cfg.copyValue();
        Serial.printf("setting3 = %s.\n", string);
      } else if (cfg.nameIs("setting4")) {
        bool boolean = cfg.getBooleanValue();
        Serial.printf("setting4 = %s.\n", boolean ? "true" : "false");
      } else if (cfg.nameIs("setting5")) {
        int i = cfg.getIntValue();
        Serial.printf("setting5 = %d.\n", i);
      } else if (cfg.nameIs("setting6")) {
        IPAddress ip = cfg.getIPAddress();
        Serial.print("setting6 = ");
        Serial.print(ip);
        Serial.println(".");
      } else if (cfg.sectionIs("section2") && cfg.nameIs("dupSetting")) {
        // Notice that once the getValue is called with trim = true,
        // the whole raw value is no longer available: it remains right-trimed.
        Serial.printf("dupSetting in section2: raw value    = \"%s\".\n", cfg.getValue(false));
        Serial.printf("dupSetting in section2: trimed value = \"%s\".\n", cfg.getValue());
      } else {
        Serial.printf("Current setting is: %s = %s.\n", cfg.getName(), cfg.getValue());
      }
    }
    Serial.flush();
    cfg.end();
  }
  SD_MMC.end();
}

void loop() {
}
```
