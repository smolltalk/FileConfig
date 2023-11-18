/*
 * FileConfig is a library for reading settings from a configuration file stored in a FS (ex: SD or SD_MMC).
 * Based on SDConfig by Claus Mancini (https://https://github.com/Fuzzer11/SDconfig)
 * Based on SDConfigFile by Bradford Needham (https://github.com/bneedhamia/sdconfigfile)
 * Licensed under LGPL version 2.1
 * a version of which should have been supplied with this file.
 */

#include "FileConfig.h"

/*
 * Opens the given file on the SD card.
 * Returns true if successful, false if not.
 *
 * configFileName = the name of the configuration file on the SD card.
 *
 * NOTE: SD.begin() must be called before calling our begin().
 */
boolean FileConfig::begin(fs::FS &fs, const char *configFileName, uint8_t maxLineLength, uint8_t maxSectionLength, bool ignoreCase, bool ignoreError)
{
  _lineLength = 0;
  _lineSize = 0;
  _valueIndex = -1;
  _atEnd = true;
  _strcmp = ignoreCase ? strcasecmp : strcmp;
  _ignoreError = ignoreError;
  _sectionChanged = false;
  _lineCounter = 1;

  /*
   * Allocate a buffer for the current line.
   */
  _lineSize = maxLineLength + 1;
  _line = (char *)malloc(_lineSize);
  if (_line == NULL)
  {
    fileConfigDebug("Line: %s\n", "out of memory.");
    _atEnd = true;
    return false;
  }

  /*
   * Allocate a buffer for the current section.
   */
  _sectionSize = maxSectionLength + 1;
  _section = (char *)malloc(_sectionSize);
  if (_section == NULL)
  {
    fileConfigDebug("Section: %s\n", "out of memory.");
    _atEnd = true;
    return false;
  }
  // section = ""
  _section[0] = '\0';

  /*
   * To avoid stale references to configFileName
   * we don't save it. To minimize memory use, we don't copy it.
   */

  _file = fs.open(configFileName, FILE_READ);
  if (!_file)
  {
    fileConfigDebug("Could not open file: %s\n", configFileName);
    _atEnd = true;
    return false;
  }

  // Initialize our reader
  _atEnd = false;

  return true;
}

/*
 * Cleans up our SDCOnfigFile object.
 */
void FileConfig::end()
{
  if (_file)
  {
    _file.close();
  }

  if (_line != NULL)
  {
    free(_line);
    _line = NULL;
  }

  if (_section != NULL)
  {
    free(_section);
    _section = NULL;
  }

  _atEnd = true;
}

/*
 * Reads the next name=value setting from the file.
 * Returns true if the setting was successfully read,
 * false if an error occurred or end-of-file occurred.
 */
boolean FileConfig::readNextSetting()
{
  char chRead;

  if (_atEnd)
  {
    return false; // already at end of file (or error).
  }

  bool settingFound = false;
  while (!(_atEnd || settingFound))
  {
    int readValue = 0;
    _valueIndex = -1;
    _lineLength = 0;
    _sectionChanged = false;
    char *buffer = NULL;
    uint8_t bufferSize = 0;
    bool isSection = false;
    ReadStep readStep = FirstANChar;

    while (true)
    {
      // Read character
      readValue = _file.read();
        
      // No more character to read?
      if (readValue < 0)
      {
        _atEnd = true;
        break;
      }

      chRead = (char) readValue;

      // End of line?
      if (chRead == '\r')
      {
        break;
      }
      // Increment the line counter on new line
      if (chRead == '\n')
      {
        _lineCounter++;
        break;
      }

      // A key, section, = or a value is currently read
      if (readStep == KeyName || readStep == SectionName || readStep == Equal || readStep == Value)
      {
        // Copy char into the buffer
        // Is the buffer full?
        if (_lineLength >= bufferSize - 1)
        { // -1 for a terminating null.
          buffer[_lineLength] = '\0';
          fileConfigDebug("Line %d too long.\n", _lineCounter);
          _atEnd = true;
          return false;
        }
        // Read a key
        if (readStep == KeyName)
        {
          if (chRead == ' ' || chRead == '\t')
          {
            // End of Key Name by blank => Now, read the equal
            buffer[_lineLength++] = '\0';
            readStep = Equal;
            continue;
          }
          else if (chRead == '=')
          {
            // End of Key Name by an Equal => Now, read the value
            buffer[_lineLength++] = '\0';
            readStep = Value;
            _valueIndex = _lineLength;
            continue;
          }
        }
        else if (readStep == SectionName && chRead == ']')
        {
          // End of Section Name
          buffer[_lineLength++] = '\0';
          readStep = SectionEnd;
          continue;
        }
        else if (readStep == Equal)
        {
          if (chRead == '=')
          {
            readStep = Value;
            _valueIndex = _lineLength;
          }
          else if (chRead != ' ' && chRead != '\t')
          {
            // Unexpected character
            readStep = BadFormat;
          }
          continue;
        }
        else if (readStep == Value)
        {
          // Nothing more to do
        }
        // Else
        buffer[_lineLength++] = chRead;
      }
      else if (readStep == SectionEnd)
      {
        if (readStep != ' ' && readStep != '\t')
        {
          fileConfigDebug("Line %d has an unexpected character after section.\n", _lineCounter);
          readStep = BadFormat;
          continue;
        }
      }
      else if (readStep == FirstANChar)
      {
        if (chRead == '#')
        {
          // Comment line.  Read until end of line or end of file.
          readStep = Comment;
          continue;
        }
        // Ignore blank text
        if (chRead == ' ' || chRead == '\t')
        {
          continue;
        }
        // A section is starting
        if (chRead == '[')
        {
          isSection = true;
          continue;
        }
        // First char found
        if (isSection)
        {
          readStep = SectionName;
          buffer = _section;
          bufferSize = _sectionSize;
        }
        else
        {
          readStep = KeyName;
          buffer = _line;
          bufferSize = _lineSize;
        }
        buffer[_lineLength++] = chRead;
      }
      else if (readStep == Comment)
      {
        continue;
      }
      else if (readStep == BadFormat)
      {
        continue;
      }
    }

    // Now a line has been read, check what we obtain
    switch (readStep)
    {
    case Value:
      buffer[_lineLength] = '\0';
      settingFound = true;
      break;
    case Equal:
    case BadFormat:
    case SectionName:
    case KeyName:
      fileConfigDebug("Line %d is badly formatted.\n", _lineCounter);
      if (!_ignoreError)
      {
        _atEnd = true;
        return false;
      }
      break;
    case SectionEnd:
      buffer[_lineLength] = '\0';
      _sectionChanged = true;
      break;
    case FirstANChar:
    case Comment:
      // loop again
      break;
    }
  }

  return settingFound;
}

/*
 * Returns the section of the most-recently-read setting.
 * Or empty string is no section has been discovered.
 */
const char *FileConfig::getSection()
{
  return _section;
}

/*
 * Returns true if the most-recently-read setting section
 * matches the given section, false otherwise.
 */
boolean FileConfig::sectionIs(const char *section)
{
  return _strcmp(section, _section) == 0;
}

/*
 * Returns true if the most-recently-read setting name
 * matches the given name, false otherwise.
 */
boolean FileConfig::nameIs(const char *name)
{
  return _strcmp(name, _line) == 0;
}

/*
 * Returns the name part of the most-recently-read setting.
 * or null if an error occurred.
 * WARNING: calling this when an error has occurred can crash your sketch.
 */
const char *FileConfig::getName()
{
  if (_lineLength <= 0 || _valueIndex <= 1)
  {
    return NULL;
  }
  return _line;
}

/*
 * Returns the value part of the most-recently-read setting,
 * or null if there was an error.
 * When trim is true, the value is first trimed.
 * Notice that once the getValue is called with trim = true,
 * the whole raw value is no longer available: it remains right-trimed.
 *  
 * WARNING: calling this when an error has occurred can crash your sketch.
 */
const char *FileConfig::getValue(bool trim)
{
  // No value => NULL
  if (_lineLength <= 0 || _valueIndex <= 1)
  {
    return NULL;
  }
  const char *s = &_line[_valueIndex];
  // Else
  // NULL => NULL
  if (!s)
  {
    return NULL;
  }
  // "" => ""
  if (!*s)
  {
    return s;
  }
  // Not trimed => s
  if (!trim)
  {
    return s;
  }
  // "   abc   " => "abc   "
  while (isspace(*s))
    s++;
  // "abc   " => "abc"
  char *e = (char *)s + strlen(s);
  while (isspace(*--e))
    ;
  *(e + 1) = '\0';
  return s;
}

/*
 * Returns the trimed value part of the most-recently-read setting,
 * or null if there was an error.
 * Notice that once this function is called (=>trim = true),
 * the whole raw value is no longer available: it remains right-trimed.
 * WARNING: calling this when an error has occurred can crash your sketch.
 */
const char *FileConfig::getValue()
{
  return getValue(true);
}

/*
 * Returns a persistent, dynamically-allocated copy of the value part
 * of the most-recently-read setting, or null if a failure occurred.
 * When trim is true, the copied value is first trimed.
 *
 * Unlike getValue(), the return value of this function
 * persists after readNextSetting() is called or end() is called.
 */
char *FileConfig::copyValue(bool trim)
{
  const char *value = NULL;
  char *result = NULL;
  int length;

  if (_lineLength <= 0 || _valueIndex <= 1)
  {
    return NULL; // begin() wasn't called, or failed.
  }

  value = getValue(trim);
  length = strlen(value);
  result = (char *)malloc(length + 1);
  if (result == NULL)
  {
    return NULL; // out of memory
  }

  strcpy(result, value);

  return result;
}

/*
 * Returns a persistent, dynamically-allocated copy of the value part
 * of the most-recently-read setting, or null if a failure occurred.
 * Trim is first applied to the value.
 *
 * Unlike getValue(), the return value of this function
 * persists after readNextSetting() is called or end() is called.
 */
char *FileConfig::copyValue()
{
  return copyValue(true);
}

/*
 * Returns the value part of the most-recently-read setting
 * as an integer, or 0 if an error occurred.
 */
int FileConfig::getIntValue()
{
  const char *str = getValue(true);
  if (!str)
  {
    return 0;
  }
  return atoi(str);
}

IPAddress FileConfig::getIPAddress()
{
  IPAddress ip(0, 0, 0, 0);
  const char *str = getValue(true);
  int len = strlen(str);
  char ipStr[len + 1];
  strncpy(ipStr, str, len); // char * strcpy ( char * destination, const char * source ); It is necessary to make a copy
  ipStr[len] = '\0';
  int i = 0;
  int tmp;
  const char *token = strtok(ipStr, ".");
  while (token != NULL)
  {
    tmp = atoi(token);
    if (tmp < 0 || tmp > 255 || i > 3)
    {
      ip = {0, 0, 0, 0};
      return ip; // IP does not have more than four octets and its values are smaller than 256
    }
    ip[i++] = (byte)tmp;
    token = strtok(NULL, ".");
  }
  return ip;
}
/*
 * Returns the value part of the most-recently-read setting
 * as a boolean.
 * The value "true" corresponds to true;
 * all other values correspond to false.
 */
boolean FileConfig::getBooleanValue()
{
  return _strcmp("true", getValue(true)) == 0;
}
