/*
 * FileConfig is a library for reading settings from a configuration file stored in a FS (ex: SD or SD_MMC).
 * Based on SDConfig by Claus Mancini (https://https://github.com/Fuzzer11/SDconfig)
 * Based on SDConfigFile by Bradford Needham (https://github.com/bneedhamia/sdconfigfile)
 * Licensed under LGPL version 2.1
 * a version of which should have been supplied with this file.
 *
 * The library supports one #define:
 *   #define FILE_CONFIG_DEBUG 1 // to print file error messages.
 */

#ifndef FileConfig_h
#define FileConfig_h

#include <Arduino.h>
#include "FS.h"
#include <Ethernet.h>

class FileConfig
{
private:
  File _file;                                 // the open configuration file
  boolean _atEnd;                             // If true, there is no more of the file to read.
  char *_line;                                // the current line of the file (see _lineLength)
                                              // Allocated by begin().
  uint16_t _lineCounter;                      // Line counter helpful in case of error
  uint8_t _lineSize;                          // size (bytes) of _line[]
  uint8_t _lineLength;                        // length (bytes) of the current line so far.
  int8_t _valueIndex;                         // position in _line[] where the value starts
                                              //  (or -1 if none)
                                              // (the name part is at &_line[0])
  char *_section;                             // Current section name.
                                              // "" until a section is discovered.
  bool _sectionChanged;                       // true when a new section has been discovered.
  uint8_t _sectionSize;                       // size (bytes) of _section[]
  int (*_strcmp)(const char *, const char *); // According to ignoreCase value, it could be
                                              // strcmp or strcasecmp
  bool _ignoreError;                          // When true, ignore badly formatted lines until the next valid key/value pair

public:
  boolean begin(fs::FS &fs, const char *configFileName, uint8_t maxLineLength, uint8_t maxSectionLength, bool ignoreCase, bool ignoreError);
  void end();
  boolean readNextSetting();
  boolean sectionIs(const char *section);
  boolean sectionChanged();
  const char *getSection();
  boolean nameIs(const char *name);
  const char *getName();
  const char *getValue();
  const char *getValue(bool trim);
  int getIntValue();
  IPAddress getIPAddress();
  boolean getBooleanValue();
  char *copyValue();
  char *copyValue(bool trim);
};

// This enum represents
// the states of the state machine
// run in readNextSetting to interpret
// each line of the file.
enum ReadStep
{
  FirstANChar, // Looking for the first Alpha Numeric Character
  Comment,     // The current line is a comment
  SectionName, // The current line is a section
  SectionEnd,  // The section end has been detected
  KeyName,     // The process is reading the key
  Equal,       // The process is looking for the '=' symbol
  Value,       // The process is reading the value
  BadFormat    // Badly formatted line detected
};

// Debug is enabled by setting 
// #define FILE_CONFIG_DEBUG 1
#ifdef FILE_CONFIG_DEBUG
#define fileConfigDebug(args...) \
  Serial.print("[FileConfig] "); \
  Serial.printf(args);
#else
#define fileConfigDebug(args...)
#endif

#endif
