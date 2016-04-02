#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <inttypes.h>
#include <algorithm>

#include "../include/configurator.h"
#include "configurator-internal.h"

using namespace std;

#define KEYVAL_DELIMITER "="

extern map<string, configStrucItem> *configStructureMap;
extern map<string, userConfigItem> *userConfigMap;
extern int globalStructureOffset;

/**
 * \brief Parse configuration lines and add elements into user configuration map.
 * \param line Line to parse.
 * \return True on success, false otherwise.
 */
bool parseLine(const string &line)
{
   string key, value;

   // Skip comments and blank lines.
   if (line.length() == 0 || line[0] == '#') {
      return true;
   }

   // Look for key-value delimiter.
   size_t pos = line.find_first_of(KEYVAL_DELIMITER);
   if (pos == string::npos) {
      cerr << "Configurator: Invalid line format '" << line << "'" << endl;
      return false;
   }

   key = line.substr(0, pos);
   value = line.substr(pos + 1);
   trim(key);
   trim(value);

   // Check if element was specified in configuration map.
   if (configStructureMap->find(key) == configStructureMap->end()) {
      cerr << "Configurator: Unknown element \"" << key << "\"" << endl;
      return false;
   }

   // Insert element into user configuration map.
   addElementToUserMap(key, value, userConfigMap);
   return true;
}

/**
 * \brief Load plain configuration from file.
 * \param filePath Path to the configuration file.
 * \return True on success, false otherwise.
 */
bool loadFilePlain(const char *filePath)
{
   ifstream file(filePath);
   string line;

   if (file.good()) {
      while (file.good()) {
         getline(file, line);
         trim(line);

         if (!parseLine(line)) {
            return false;
         }
      }

      file.close();
      return true;
   }

   cerr << "Configurator: Unable to open configuration file '" << filePath << "'" << endl;
   return false;
}

/**
 * \brief Initializes configuration maps.
 * \return 0 on success, EXIT_FAILURE otherwise.
 */
extern "C" int confPlainCreateContext()
{
   configStructureMap  = new map<string, configStrucItem>;
   if (configStructureMap == NULL) {
     cerr << "Configurator: ERROR: Cannot allocate enought space for configuration map." << endl;
      return EXIT_FAILURE;
   }

   userConfigMap = new map<string, userConfigItem>;
   if (userConfigMap == NULL) {
      cerr << "Configurator: ERROR: Cannot allocate enought space for configuration map." << endl;
      delete configStructureMap;
      configStructureMap = NULL;

      return EXIT_FAILURE;
   }

   globalStructureOffset = 0;
   return 0;
}

/**
 * \brief Used to clean configuration maps.
 */
extern "C" void confPlainClearContext()
{
   // Clear configuration maps.
   if (configStructureMap) {
      clearConfigStructureMap(configStructureMap);
   }

   if (userConfigMap) {
      clearConfigStructureMap(userConfigMap);
   }

   configStructureMap = NULL;
   userConfigMap = NULL;
}

/**
 * \brief Wrapper for addElement function. WARNING: Configuration maps must be initialized
 *        with confPlainCreateContext function before first use.
 * \param name Name of the element.
 * \param type Type of the element.
 * \param defValue Default value of the element.
 * \param charArraySize Maximum allowed size for string type item.
 * \param requiredFlag Item will be required if set, otherwise it is optional.
 *                        will be changed. Otherwise it will NOT be changed.
 * \return 0 on success, EXIT_FAILURE otherwise.
 */
extern "C" int confPlainAddElement(const char *name, const char *type, const char *defValue, int charArraySize, int requiredFlag)
{
   if (configStructureMap == NULL || userConfigMap == NULL) {
      cerr << "Configurator: No context created, failed to add element." << endl;
      return EXIT_FAILURE;
   }

   char buffer[20];
   snprintf(buffer, sizeof(buffer), "%d", charArraySize);
   return addElement(name, type, defValue, buffer, requiredFlag, NULL, true) ? 0 : EXIT_FAILURE;
}

/**
 * \brief Load configuration from file to user defined struct.
 *        WARNING: Configuration items must be specified using confPlainAddElement function.
 * \param filePath Configuration file path.
 * \param userStruct Pointer to memory where configuration structure will
 *                   be created.
 * \return 0 on success, EXIT_FAILURE otherwise.
 */
extern "C" int confPlainLoadConfiguration(const char *filePath, void *userStruct)
{
   if (configStructureMap == NULL || userConfigMap == NULL) {
      cerr << "Configurator: No context created, failed to load configuration." << endl;
      return EXIT_FAILURE;
   }

   // Load configuration from file.
   if (!loadFilePlain(filePath)) {
      cerr << "Configurator: Parsing failed." << endl;
      return EXIT_FAILURE;
   }

   //printConfigMap(configStructureMap);
   //printUserMap(userConfigMap);

   bool validationRetVal = fillConfigStruct(configStructureMap, userConfigMap);
   if (!validationRetVal) {
      cerr << "Configurator: Validation failed." << endl;
      return EXIT_FAILURE;
   }

   // If ptr is specified, copy configuration into user defined structure.
   if (userStruct) {
      // Load configuration into user defined struct.
      getConfiguration(userStruct, configStructureMap);
   }
   userConfigMap->clear();

   return 0;
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" uint8_t confPlainGetUint8(const char *name, uint8_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((uint8_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" int8_t confPlainGetInt8(const char *name, int8_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((int8_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" uint16_t confPlainGetUint16(const char *name, uint16_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((uint16_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" int16_t confPlainGetInt16(const char *name, int16_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((int16_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" uint32_t confPlainGetUint32(const char *name, uint32_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((uint32_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" int32_t confPlainGetInt32(const char *name, int32_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((int32_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" uint64_t confPlainGetUint64(const char *name, uint64_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((uint64_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" int64_t confPlainGetInt64(const char *name, int64_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((int64_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" int32_t confPlainGetBool(const char *name, int32_t defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((int32_t *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" float confPlainGetFloat(const char *name, float defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((float *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" double confPlainGetDouble(const char *name, double defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return *((double *)(it->second).defaultValue);
}

/**
 * \brief Get configuration item from configuration map.
 * \param name Name of the configuration item.
 * \param defValue Default value to return if item does not exits.
 * \return Value of the configuration item or defValue.
 */
extern "C" const char *confPlainGetString(const char *name, const char *defValue)
{
   if (!configStructureMap) {
      return defValue;
   }

   map<string, configStrucItem>::iterator it = configStructureMap->find(name);
   if (it == configStructureMap->end()) {
      return defValue;
   }

   return (it->second).defaultStringValue.c_str();
}
