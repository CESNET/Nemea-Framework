/**
 * \file configurator.cpp
 * \author Marek Hurta <xhurta01@stud.fit.vutbr.cz>
 * \author Erik Sabik <xsabik02@stud.fit.vutr.cz>
 * \date 2015
 */

/*
 * Copyright (C) 2014 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <libxml/parser.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <stddef.h>

#include "configurator-internal.h"
#include "../include/configurator.h"

using namespace std;

const static int USERXML = 1;
const static int PATTERNXML = 2;

map<string, configStrucItem> *configStructureMap;
map<string, userConfigItem> *userConfigMap;
infoStruct moduleInfo;
int globalStructureOffset = 0;

xmlDocPtr doc;
xmlDocPtr userDoc;

// Structurer for storing information about memory allocation for
userAllocatedMemoryBlockStructure UAMBS;


/**
 * \brief Function for deallocating memory used for arrays.
 *        Using global variable `UAMBS`, which is structure
 *        holding information about allocated arrays.
 */
extern "C" void configuratorFreeUAMBS()
{
    for (unsigned int i = 0; i < UAMBS.memBlockArr.size(); i++) {
        free(UAMBS.memBlockArr[i]);
    }
    UAMBS.memBlockArr.clear();
    UAMBS.memBlockElCount.clear();
}


/**
 * \brief Function return number of elements in provided array.
 *        Using global variable `UAMBS`, which is structure
 *        holding information about allocated arrays.
 * \param arr Void pointer to array of which size we want to know.
 * \return Number of elements in provided array.
 */
extern "C" unsigned int configuratorGetArrElemCount(void *arr)
{
    for (unsigned int i = 0; i < UAMBS.memBlockArr.size(); i++) {
        if (arr == UAMBS.memBlockArr[i]) {
            return UAMBS.memBlockElCount[i];
        }
    }

    return 0;
}


/**
 * \brief Function for deallocating memory for configuration map.
 *        Function will recursively call itself to deallocate memory
 *        of child items and maps.
 * \param configMap Pointer to configuration map which needs to be
 *        destroyed.
 */
void clearConfigStructureMap(map<string, configStrucItem> *configMap)
{
    map<string, configStrucItem>::iterator it = configMap->begin();

    while (it != configMap->end()) {
        if (it->second.map != NULL) {
            clearConfigStructureMap((map<string, configStrucItem> *)it->second.map);
        }

        if (it->second.defaultValue != NULL) {
            free(it->second.defaultValue);
        }

        if (it->second.type == ARRAY && it->second.arrType == AR_ELEMENT) {
            configStrucItem *elem = it->second.arrElement;
            elem->name.clear();
            delete elem;
        }

        it++;
    }
    configMap->clear();
    delete (map<string, configStrucItem>*)configMap;
}

/**
 * \brief Function for deallocating memory for user configuration map.
 *        Function will recursively call itself to deallocate memory
 *        of child items and maps.
 * \param configMap Pointer to user configuration map which needs to be
 *        destroyed.
 */
void clearConfigStructureMap(map<string, userConfigItem> *configMap)
{
    map<string, userConfigItem>::iterator it = configMap->begin();

    while (it != configMap->end()) {
        if (it->second.type == UARRAY) {
            // Clear array
            if (it->second.arrType == AR_ELEMENT) {
                // Single element array
                vector<userArrElemStruct> *elemVect = (vector<userArrElemStruct> *) it->second.elementAr;
                elemVect->clear();
                delete elemVect;
            } else {
                // Array of structs
                vector<map<string, userConfigItem> > *elemVect = (vector<map<string, userConfigItem> > *) it->second.elementAr;
                elemVect->clear();
                delete elemVect;
            }
        }
        if (it->second.map != NULL) {
            // Clear struct
            clearConfigStructureMap((map<string, userConfigItem> *)it->second.map);
        }
        it++;
    }
    configMap->clear();
    delete (map<string, userConfigItem>*)configMap;
}


/**
 * \brief Debug function for printing item type and its default value.
 * \param item Structure containing parsed item from XML file.
 */
void printTypeAndDefaultValue(configStrucItem item) {
    switch(item.type) {
    case UINT8_T:
        cout << "UINT8_T" << endl;
        cout << "DefaultValue: " << *(uint8_t*)item.defaultValue << endl;
        break;
    case UINT16_T:
        cout << "UINT16_T" << endl;
        cout << "DefaultValue: " << *(uint16_t*)item.defaultValue << endl;
        break;
    case UINT32_T:
        cout << "UINT32_T" << endl;
        cout << "DefaultValue: " << *(uint32_t*)item.defaultValue << endl;
        break;
    case UINT64_T:
        cout << "UINT64_T" << endl;
        cout << "DefaultValue: " << *(uint64_t*)item.defaultValue << endl;
        break;
    case FLOAT:
        cout << "FLOAT" << endl;
        cout << "DefaultValue: " << *(float*)item.defaultValue << endl;
        break;
    case DOUBLE:
        cout << "DOUBLE" << endl;
        cout << "DefaultValue: " << *(double*)item.defaultValue << endl;
        break;
    case STRING:
        cout << "STRING" << endl;
        cout << "DefaultValue: " << item.defaultStringValue.c_str() << endl;
        break;
    default:
        break;
    }
}


/**
 * \brief Debug function for printing whole configuration map.
 * \param configMap Pointer to configuration map which we want to print.
 */
void printConfigMap(map<string, configStrucItem> *configMap)
{
    map<string, configStrucItem>::iterator it = configMap->begin();

    while(it != configMap->end()) {
        cout << "Element name: " << it->second.name << endl;
        if (it->second.isRequired) {
            cout << "Element is required." << endl;
        } else {
            cout << "Element is optional." << endl;
        }

        if (it->second.type == STRUCT) {
            cout << "--------------------------" << endl;
            printConfigMap(((map<string, configStrucItem> *)it->second.map));
            cout << "--------------------------" << endl;
        } else {
            cout << "Element type: "; printTypeAndDefaultValue(it->second);
            cout << "****************" << endl;
        }
        it++;
    }
}


/**
 * \brief Debug function for printing user configuration map.
 * \param configMap Pointer to user configuration map which we want to print.
 */
void printUserMap(map<string, userConfigItem> *configMap)
{
    map<string, userConfigItem>::iterator it = configMap->begin();

    while (it != configMap->end()) {
        if (it->second.type == USTRUCT) {
            cout << "Struct name: " << it->second.name << endl;
            printUserMap((map<string, userConfigItem>*)it->second.map);
        } else {
            cout << "Variable name: " << it->second.name << endl;
            cout << "Variable value: " << it->second.value << endl;
        }
        it++;
    }
}

/**
 * \brief Function for setting type of item. It will convert string
 *        value to corresponding enum value.
 * \param item Pointer to item structure, which type will be set.
 * \param type String value of item type.
 * \return False if unknown type encountered, true otherwise.
 */
bool setType(configStrucItem *item, string type)
{
    // Convert type to lower case for easy comparation
    string tmpString = type;
    transform(type.begin(), type.end(), type.begin(), ::tolower);

    if (type.compare("uint8_t") == 0) {
        item->type = UINT8_T;
    } else if (type.compare("int8_t") == 0) {
        item->type = INT8_T;
    } else if (type.compare("uint16_t") == 0) {
        item->type = UINT16_T;
    } else if (type.compare("int16_t") == 0) {
        item->type = INT16_T;
    } else if (type.compare("uint32_t") == 0) {
        item->type = UINT32_T;
    } else if (type.compare("int32_t") == 0) {
        item->type = INT32_T;
    } else if (type.compare("uint64_t") == 0) {
        item->type = UINT64_T;
    } else if (type.compare("int64_t") == 0) {
        item->type = INT64_T;
    } else if (type.compare("float") == 0) {
        item->type = FLOAT;
    } else if (type.compare("double") == 0) {
        item->type = DOUBLE;
    } else if (type.compare("string") == 0) {
        item->type = STRING;
    } else {
        cerr << "Error: Element '"
             << item->name
             << "' has unknown type '"
             << tmpString
             << "'. Could not continue, aborting."
             << endl;
        return false;
    }

    return true;
}


/**
 * \brief Function which convert string representation of item to structure
 *        and add it to configuration map.
 *        BEWARE: Function uses and changes global variable `globalStructureOffset`.
 * \param name Name of the element.
 * \param type Type of the element.
 * \param defValue Default value of the element.
 * \param charArraySize Maximum allowed size for string type item.
 * \param requiredFlag Item will be required if set, otherwise it is optional.
 * \param structMap Configuration map to which will be this item added.
 * \param setGlobalOffset If set, global variable `globalStructureOffset`
 *                        will be changed. Otherwise it will NOT be changed.
 * \return True on success, false otherwise.
 */
bool addElement(string name,
                string type,
                string defValue,
                string charArraySize,
                bool requiredFlag,
                map<string, configStrucItem> *structMap,
                bool setGlobalOffset)
{
    configStrucItem tmpItem;

    tmpItem.name = name;
    tmpItem.map = NULL;
    tmpItem.defaultValue = NULL;
    // Check and set type of element
    if (!setType(&tmpItem, type)) {
        return false;
    }

    tmpItem.isRequired = requiredFlag;

    tmpItem.offset = globalStructureOffset;

    switch(tmpItem.type) {
    case UINT8_T:
        tmpItem.defaultValue = malloc(sizeof(uint8_t*));
        *(uint8_t*)tmpItem.defaultValue = (uint8_t)atoi(defValue.c_str());
        globalStructureOffset += sizeof(uint8_t) * setGlobalOffset;
        break;
    case INT8_T:
        tmpItem.defaultValue = malloc(sizeof(int8_t*));
        *(int8_t*)tmpItem.defaultValue = (int8_t)atoi(defValue.c_str());
        globalStructureOffset += sizeof(int8_t) * setGlobalOffset;
        break;
    case UINT16_T:
        tmpItem.defaultValue = malloc(sizeof(uint16_t*));
        *(uint16_t*)tmpItem.defaultValue = (uint16_t)atoi(defValue.c_str());
        globalStructureOffset += sizeof(uint16_t) * setGlobalOffset;
        break;
    case INT16_T:
        tmpItem.defaultValue = malloc(sizeof(int16_t*));
        *(int16_t*)tmpItem.defaultValue = (int16_t)atoi(defValue.c_str());
        globalStructureOffset += sizeof(int16_t) * setGlobalOffset;
        break;
    case UINT32_T:
        tmpItem.defaultValue = malloc(sizeof(uint32_t*));
        *(uint32_t*)tmpItem.defaultValue = (uint32_t)atoi(defValue.c_str());
        globalStructureOffset += sizeof(uint32_t) * setGlobalOffset;
        break;
    case INT32_T:
        tmpItem.defaultValue = malloc(sizeof(int32_t*));
        *(int32_t*)tmpItem.defaultValue = (int32_t)atoi(defValue.c_str());
        globalStructureOffset += sizeof(int32_t) * setGlobalOffset;
        break;
    case UINT64_T:
        tmpItem.defaultValue = malloc(sizeof(uint64_t*));
        *(uint64_t*)tmpItem.defaultValue = (uint64_t)atoll(defValue.c_str());
        globalStructureOffset += sizeof(uint64_t) * setGlobalOffset;
        break;
    case INT64_T:
        tmpItem.defaultValue = malloc(sizeof(int64_t*));
        *(int64_t*)tmpItem.defaultValue = (int64_t)atoll(defValue.c_str());
        globalStructureOffset += sizeof(int64_t) * setGlobalOffset;
        break;
    case FLOAT:
        tmpItem.defaultValue = malloc(sizeof(float*));
        *(float*)tmpItem.defaultValue = (float)atof(defValue.c_str());
        globalStructureOffset += sizeof(float) * setGlobalOffset;
        break;
    case DOUBLE:
        tmpItem.defaultValue = malloc(sizeof(double*));
        *(double*)tmpItem.defaultValue = (double)atof(defValue.c_str());
        globalStructureOffset += sizeof(double) * setGlobalOffset;
        break;
    case STRING:
        tmpItem.defaultStringValue = defValue;
        globalStructureOffset += atoi(charArraySize.c_str()) * setGlobalOffset;
        tmpItem.stringMaxSize = atoi(charArraySize.c_str()) - 1;
        break;
    default:
        break;
    }

    if (structMap == NULL) {
        configStructureMap->insert(pair<string, configStrucItem>(tmpItem.name, tmpItem));
    } else {
        structMap->insert(pair<string, configStrucItem>(tmpItem.name, tmpItem));
    }

    return true;
}


/**
 * \brief Function parses given XML node and extracts element parameters
 *        from it.
 * \param structNode XML node with element to be parsed.
 * \param elementName This variable will be set to element name.
 * \param elementType This variable will be set to element type.
 * \param elementDefValue This variable will be set to element default value.
 * \param charArraySize This variable will be set to maximum allowed size
 *                      of string.
 * \param requiredFlag This variable will be set if element is required.
 */
void parseElement(xmlNodePtr structNode,
                  string &elementName,
                  string &elementType,
                  string &elementDefValue,
                  string &charArraySize,
                  bool &requiredFlag)
{
    xmlChar * tmpXmlChar;

    // Get 'required' attribute
    xmlAttrPtr tmpXmlAttr = structNode->properties;
    requiredFlag = false;
    while (tmpXmlAttr != NULL) {
        xmlChar * attXmlChar = xmlGetProp(structNode, tmpXmlAttr->name);
        if (xmlStrcmp(attXmlChar, (const xmlChar *) "required") == 0) {
            requiredFlag = true;
        }
        xmlFree(attXmlChar);
        tmpXmlAttr = tmpXmlAttr->next;
    }

    xmlNodePtr elementContent = structNode->xmlChildrenNode;
    bool stringSizeNotPresentFlag = true;

    // Get name, type and default-value attribute
    while (elementContent != NULL) {
        if (xmlStrcmp(elementContent->name, (const xmlChar *) "name") == 0) {
            tmpXmlChar = xmlNodeGetContent(elementContent);
            elementName.assign((const char *)tmpXmlChar);
            xmlFree(tmpXmlChar);
        } else if (xmlStrcmp(elementContent->name, (const xmlChar *) "type") == 0) {
            tmpXmlChar = xmlNodeGetContent(elementContent);
            if (xmlStrcmp(tmpXmlChar, (const xmlChar *) "string") == 0) {
                xmlAttrPtr stringAttr = elementContent->properties;
                while (stringAttr != NULL) {
                    if (xmlStrcmp(stringAttr->name, (const xmlChar *) "size") == 0) {
                        xmlChar *stringAttChar = xmlGetProp(elementContent, stringAttr->name);
                        charArraySize.assign((const char *)stringAttChar);
                        xmlFree(stringAttChar);
                        stringSizeNotPresentFlag = false;
                        break;
                    }
                    stringAttr = stringAttr->next;
                }
                if (stringSizeNotPresentFlag) {
                    // Set default size
                    cerr << "Warning: Element '"
                         << elementName
                         << "' is missing size attribute, using default size ("
                         << DEFAULT_STRING_MAX_SIZE
                         << ")!"
                         << endl;
                    cerr << "         Please check definition of your structure if string size matches this default size."
                         << endl;

                    charArraySize.assign(DEFAULT_STRING_MAX_SIZE);
                }
            }
            elementType.assign((const char *)tmpXmlChar);
            xmlFree(tmpXmlChar);
        } else if (xmlStrcmp(elementContent->name, (const xmlChar *) "default-value") == 0) {
            tmpXmlChar = xmlNodeGetContent(elementContent);
            elementDefValue.assign((const char *)tmpXmlChar);
            xmlFree(tmpXmlChar);
        }
        elementContent = elementContent->next;
    }
}


/**
 * \brief Function parses XML node containing 'struct' element and every
 *        element it finds will be added to configuration map.
 * \param parentNode XML node containing 'struct' element.
 * \param structMap Configuration map to which will be added all found
 *                  elements.
 * \return True on success, false otherwise.
 */
bool parseStruct(xmlNodePtr parentNode, map<string, configStrucItem> *structMap)
{
    xmlNodePtr structNode = parentNode->xmlChildrenNode;
    xmlAttrPtr structXmlAttr = parentNode->properties;

    while (structXmlAttr != NULL) {
        xmlChar * structXmlChar = xmlGetProp(parentNode, structXmlAttr->name);
        xmlFree(structXmlChar);
        structXmlAttr = structXmlAttr->next;
    }

    while (structNode != NULL) {
        if (xmlStrcmp(structNode->name, (const xmlChar *) "element") == 0) {
            // Parse single element
            string elementName;
            string elementType;
            string elementDefValue;
            string charArraySize;
            bool requiredFlag = false;

            parseElement(structNode, elementName, elementType, elementDefValue, charArraySize, requiredFlag);
            // Check and add element
            if (!addElement(elementName, elementType, elementDefValue, charArraySize, requiredFlag, structMap, true)) {
                return false;
            }
        } else if (xmlStrcmp(structNode->name, (const xmlChar *) "struct") == 0) {
            // Parse structure
            configStrucItem tmpItem;
            string structureName;
            xmlAttrPtr structXmlAttr2 = parentNode->properties;

            while (structXmlAttr2 != NULL) {
                xmlChar * attXmlStructChar = xmlGetProp(structNode, structXmlAttr2->name);
                structureName.assign((const char *)attXmlStructChar);
                xmlFree(attXmlStructChar);
                structXmlAttr2 = structXmlAttr2->next;
            }

            tmpItem.type = STRUCT;
            tmpItem.name = structureName;
            tmpItem.defaultValue = NULL;
            tmpItem.arrElement = NULL;
            map<string, configStrucItem> *newStructMap = new map<string, configStrucItem>;
            if (newStructMap == NULL) {
                cerr << "parser error, cannot allocate enought space for sub-map." << endl;
                return false;
            }
            tmpItem.map = (void*)newStructMap;

            if (structMap == NULL) {
                configStructureMap->insert(pair<string, configStrucItem>(tmpItem.name, tmpItem));
            } else {
                structMap->insert(pair<string, configStrucItem>(tmpItem.name, tmpItem));
            }


            parseStruct(structNode, newStructMap);
        } else if (xmlStrcmp(structNode->name, (const xmlChar *) "array") == 0) {
            // Parse array
            configStrucItem tmpItem;
            string arrayName;
            xmlAttrPtr arrayXmlAttr2 = parentNode->properties;

            // Get name
            while (arrayXmlAttr2 != NULL) {
                xmlChar * attXmlArrayChar = xmlGetProp(structNode, arrayXmlAttr2->name);
                arrayName.assign((const char *)attXmlArrayChar);
                xmlFree(attXmlArrayChar);
                arrayXmlAttr2 = arrayXmlAttr2->next;
            }

            // Determine if array item is variable or struct
            uint8_t arrChildType = 0;
            xmlNodePtr arrChildNode = structNode->xmlChildrenNode;
            while (arrChildNode != NULL) {
                if (xmlStrcmp(arrChildNode->name, (const xmlChar *) "element") == 0) {
                    // Single element
                    string elementName, elementType, elementDefValue, charArraySize;
                    bool requiredFlag;

                    parseElement(arrChildNode, elementName, elementType, elementDefValue, charArraySize, requiredFlag);

                    // Allocate memory for element information
                    tmpItem.arrElement = new configStrucItem;
                    if (tmpItem.arrElement == NULL) {
                        cerr << "Error: Could not allocate enought space for array element." << endl;
                        return false;
                    }

                    // Set values for array element
                    tmpItem.arrElement->name = elementName;
                    tmpItem.arrElement->defaultValue = NULL;
                    tmpItem.arrElement->stringMaxSize = atoi(charArraySize.c_str()) - 1;
                    setType(tmpItem.arrElement, elementType);
                    tmpItem.arrElement->arrElement = NULL;
                    tmpItem.map = NULL;

                    arrChildType = 1;
                } else if (xmlStrcmp(arrChildNode->name, (const xmlChar *) "struct") == 0) {
                    // Struct
                    // Create map for elements in structure
                    map<string, configStrucItem> *newStructMap = new map<string, configStrucItem>;
                    if (newStructMap == NULL) {
                        cerr << "parser error, cannot allocate enought space for sub-map." << endl;
                        return false;
                    }
                    tmpItem.map = (void*)newStructMap;
                    tmpItem.arrayElementSize = 0;
                    tmpItem.arrElement = NULL;
                    unsigned structOffset = 0;

                    // Parse elements of structure
                    xmlNodePtr arrStructElems = arrChildNode->xmlChildrenNode;
                    while (arrStructElems != NULL) {
                        // Array support only simple elements
                        if (xmlStrcmp(arrStructElems->name, (const xmlChar *) "element") != 0) {
                            ;//cerr << "WARNING: Not an element, skipping!";
                        } else {
                            string elementName, elementType, elementDefValue, charArraySize;
                            bool requiredFlag;

                            parseElement(arrStructElems, elementName, elementType, elementDefValue, charArraySize, requiredFlag);
                            // Check and add element
                            if (!addElement(elementName, elementType, elementDefValue, charArraySize, requiredFlag, (map<string, configStrucItem> *)tmpItem.map, false)) {
                                return false;
                            }

                            // Change element offset based on its location in structure
                            configStrucItem *element = &(*(map<string, configStrucItem> *)tmpItem.map)[elementName];
                            element->offset = structOffset;

                            // Increment size of current structure based on parsed element
                            if (element->type == STRING) {
                                tmpItem.arrayElementSize += atoi(charArraySize.c_str());
                                structOffset += atoi(charArraySize.c_str());
                            } else {
                                tmpItem.arrayElementSize += varTypeSize[element->type];
                                structOffset += varTypeSize[element->type];
                            }
                        }
                        arrStructElems = arrStructElems->next;
                    }
                    arrChildType = 2;
                }
                arrChildNode = arrChildNode->next;
            }

            switch (arrChildType) {
            case '0':
                cerr << "Warning: No elements found!";
                break;
            case '1':
            case '2':
            default:
                break;
            }

            // Set array params
            tmpItem.type = ARRAY;
            tmpItem.name = arrayName;
            tmpItem.defaultValue = NULL;
            tmpItem.offset = globalStructureOffset;
            globalStructureOffset += sizeof(void*);

            // Add array to config map
            if (structMap == NULL) {
                configStructureMap->insert(pair<string, configStrucItem>(tmpItem.name, tmpItem));
            } else {
                structMap->insert(pair<string, configStrucItem>(tmpItem.name, tmpItem));
            }
        }

        structNode = structNode->next;
    }
    xmlFree(structNode);

    return true;
}


/**
 * \brief Function for adding parsed element to user configuration map.
 * \param name Name of parsed element.
 * \param value Value of parsed element.
 * \param userMap User configuration map, to which the element will be added.
 */
void addElementToUserMap(string name, string value, map<string, userConfigItem> *userMap)
{
    userConfigItem tmpItem;
    tmpItem.name = name;
    tmpItem.value = value;
    tmpItem.type = VARIABLE;
    tmpItem.map = NULL;

    if (userMap == NULL) {
        userConfigMap->insert(pair<string, userConfigItem>(tmpItem.name, tmpItem));
    } else {
        userMap->insert(pair<string, userConfigItem>(tmpItem.name, tmpItem));
    }
}

/**
 * \brief Function will trim whitespaces from both ends of a string.
 * \param s String to trim.
 * \param t Optional paramater, which specify characters to remove.
 *          (default are whitespace characters)
 */
void trim(std::string &s, const char *t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    s.erase(s.find_last_not_of(t) + 1);
}

/**
 * \brief Function parses XML node and extract element name and its value.
 * \param node XML node to parse.
 * \param varName This variable will be set to element name.
 * \param varValue This variable will be set to element value.
 */
void parseUserElement(xmlNodePtr node, string &varName, string &varValue)
{
    xmlAttrPtr tmpXmlAttr = node->properties;

    // Get name of element
    while (tmpXmlAttr != NULL) {
        xmlChar * attXmlChar = xmlGetProp(node, tmpXmlAttr->name);
        varName.assign((const char *)attXmlChar);
        xmlFree(attXmlChar);
        tmpXmlAttr = tmpXmlAttr->next;
    }

    // Get value and strip whitespaces
    xmlChar *tmpXmlChar = xmlNodeGetContent(node);
    varValue.assign((const char *)tmpXmlChar);
    trim(varValue);

    xmlFree(tmpXmlChar);
}

/**
 * \brief Function for parsing user XML 'struct' element and storing it in
 *        user configuration map.
 * \param parentNode XML node containg 'struct' element.
 * \param structMap Pointer to user configuration map.
 * \return True on success, false otherwise.
 */
bool parseUserStruct(xmlNodePtr parentNode, map<string, userConfigItem> *structMap)
{
    xmlNodePtr structNode = parentNode->xmlChildrenNode;

    while (structNode != NULL) {
        if (xmlStrcmp(structNode->name, (const xmlChar *) "element") == 0) {
            //xmlAttrPtr tmpXmlAttr = structNode->properties;
            string variableName;
            string variableValue;

            parseUserElement(structNode, variableName, variableValue);
            addElementToUserMap(variableName, variableValue, structMap);
        } else if (xmlStrcmp(structNode->name, (const xmlChar *) "struct") == 0) {
            xmlAttrPtr structXmlAttr = structNode->properties;
            string structName;

            while (structXmlAttr != NULL) {
                xmlChar * attXmlChar = xmlGetProp(structNode, structXmlAttr->name);
                structName.assign((const char *) attXmlChar);
                xmlFree(attXmlChar);
                structXmlAttr = structXmlAttr->next;
            }

            userConfigItem tmpItem;
            tmpItem.name = structName;
            tmpItem.type = USTRUCT;

            map<string, userConfigItem> *userStructSubMap = new map<string, userConfigItem>;
            if (userStructSubMap == NULL) {
                cerr << "parser error, cannot allocate enought space for sub-map." << endl;
                return false;
            }

            tmpItem.map = (void*)userStructSubMap;

            if (structMap == NULL) {
                userConfigMap->insert(pair<string, userConfigItem>(tmpItem.name, tmpItem));
            } else {
                structMap->insert(pair<string, userConfigItem>(tmpItem.name, tmpItem));
            }

            parseUserStruct(structNode, userStructSubMap);
        } else if (xmlStrcmp(structNode->name, (const xmlChar *) "array") == 0) {
            // Parse array
            userConfigItem tmpItem;
            string arrayName;
            xmlAttrPtr arrayXmlAttr2 = structNode->properties;

            // Get name
            while (arrayXmlAttr2 != NULL) {
                xmlChar * attXmlArrayChar = xmlGetProp(structNode, arrayXmlAttr2->name);
                arrayName.assign((const char *)attXmlArrayChar);
                xmlFree(attXmlArrayChar);
                arrayXmlAttr2 = arrayXmlAttr2->next;
            }


            // Determine if array item is variable or struct
            uint8_t arrChildType = 0;
            xmlNodePtr arrChildNode = structNode->xmlChildrenNode;
            while (arrChildNode != NULL) {
                if (xmlStrcmp(arrChildNode->name, (const xmlChar *) "element") == 0) {
                    // Single element
                    string variableName;
                    string variableValue;

                    // Create vector for elements in array
                    vector<userArrElemStruct> *userArrVector = new vector<userArrElemStruct>;
                    if (userArrVector == NULL) {
                        cerr << "parser error, cannot allocate enought space for vector of elements." << endl;
                        return false;
                    }
                    tmpItem.elementAr = (void*)userArrVector;

                    // Cycle through every element
                    while (arrChildNode != NULL) {
                        // Array support only simple elements
                        if (xmlStrcmp(arrChildNode->name, (const xmlChar *) "element") != 0) {
                            ;//cerr << "WARNING: Not an element, skipping!";
                        } else {
                            string variableName, variableValue;
                            userArrElemStruct tmpStruct;

                            parseUserElement(arrChildNode, variableName, variableValue);
                            tmpStruct.name = variableName;
                            tmpStruct.value = variableValue;
                            ((vector<userArrElemStruct> *)tmpItem.elementAr)->push_back(tmpStruct);
                        }
                        arrChildNode = arrChildNode->next;
                    }
                    tmpItem.arrType = AR_ELEMENT;
                    arrChildType = 1;
                    break;
                } else if (xmlStrcmp(arrChildNode->name, (const xmlChar *) "struct") == 0) {
                    // Struct
                    // Create vector for maps of elements in structure
                    vector<map<string, userConfigItem> > *userConfigVector = new vector<map<string, userConfigItem> >;
                    if (userConfigVector == NULL) {
                        cerr << "parser error, cannot allocate enought space for vector." << endl;
                        return false;
                    }
                    tmpItem.elementAr = (void*)userConfigVector;

                    // Parse every struct
                    while (arrChildNode != NULL) {
                        // Parse only struct nodes
                        if (xmlStrcmp(arrChildNode->name, (const xmlChar *) "struct") != 0) {
                            arrChildNode = arrChildNode->next;
                            continue;
                        }

                        // Create map for elements in structure
                        map<string, userConfigItem> userStructSubMap;

                        // Parse elements of struct node
                        xmlNodePtr arrStructElems = arrChildNode->xmlChildrenNode;
                        while (arrStructElems != NULL) {
                            // Array support only simple elements
                            if (xmlStrcmp(arrStructElems->name, (const xmlChar *) "element") != 0) {
                                ;//cerr << "WARNING: Not an element, skipping!";
                            } else {
                                string variableName, variableValue;

                                parseUserElement(arrStructElems, variableName, variableValue);
                                addElementToUserMap(variableName, variableValue, &userStructSubMap);
                            }
                            arrStructElems = arrStructElems->next;
                        }
                        // Push it to array
                        ((vector<map<string, userConfigItem> > *)tmpItem.elementAr)->push_back(userStructSubMap);

                        arrChildNode = arrChildNode->next;
                    }
                    tmpItem.arrType = AR_STRUCT;
                    arrChildType = 2;
                    break;
                }
                arrChildNode = arrChildNode->next;
            }

            switch (arrChildType) {
                case 0: cerr << "Warning: No elements found!";
                    break;
                case 1:
                case 2:
                default:
                    break;
            }

            // Set array params
            tmpItem.type = UARRAY;
            tmpItem.name = arrayName;
            tmpItem.map = NULL;

            // Add array to config map
            if (structMap == NULL) {
                userConfigMap->insert(pair<string, userConfigItem>(tmpItem.name, tmpItem));
            } else {
                structMap->insert(pair<string, userConfigItem>(tmpItem.name, tmpItem));
            }
        }
        structNode = structNode->next;
    }
    xmlFree(structNode);

    return true;
}


/**
 * \brief Function for checking XML configuration file head.
 * \param cur XML root node.
 * \return True on success, false otherwise.
 */
bool checkHeader(xmlNodePtr cur)
{
    if (cur == NULL) {
        cerr << "parser error, input user configuration is empty." << endl;
        return false;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) "configuration")) {
        cerr << "parser error, root element is not valid, <" << cur->name <<"> found, must be <configuration>." << endl;
        return false;
    }

    return true;
}


/**
 * \brief Function for parsing XML configuration file.
 * \param doc Pointer to XML document (configuration file).
 * \param typeOfParsing Could be either 'PATTERXML' for configuration file
 *                      or 'USERXML' for user configuration file.
 * \return True on success, false otherwise.
 */
bool parseStruct(xmlDocPtr *doc, int typeOfParsing)
{
    xmlNodePtr cur;
    xmlChar *tmpXmlChar;
    bool retValue = true;

    cur = xmlDocGetRootElement(*doc);
    bool isHeaderOk = checkHeader(cur);
    if (!isHeaderOk) {
        return false;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (xmlStrcmp(cur->name, (const xmlChar *) "module-name") == 0) {
            tmpXmlChar = xmlNodeGetContent(cur);
            moduleInfo.patternModuleName.assign((const char *)tmpXmlChar);
            //cout << "Module name: " << tmpXmlChar << endl;
            xmlFree(tmpXmlChar);
        }

        if (xmlStrcmp(cur->name, (const xmlChar *) "module-author") == 0) {
            tmpXmlChar = xmlNodeGetContent(cur);
            moduleInfo.patternModuleAuthor.assign((const char *)tmpXmlChar);
            //cout << "Module author: " << tmpXmlChar << endl;
            xmlFree(tmpXmlChar);
        }

        if (xmlStrcmp(cur->name, (const xmlChar *) "struct") == 0) {
            if (typeOfParsing == PATTERNXML) {
                retValue = parseStruct(cur,NULL);
            } else {
                retValue = parseUserStruct(cur,NULL);
            }

            if (!retValue){
                break;
            }
        }
        cur = cur->next;
    }

    return retValue;
}


/**
 * \brief Function for converting string value to corresponding type
 *        based on element type.
 * \param configStruct Pointer to item which value will be set.
 * \param userStruct Pointer to item containing string value of element.
 */
void setUserValueToConfig(configStrucItem *configStruct, userConfigItem *userStruct)
{
    switch(configStruct->type) {
    case UINT8_T:
        *(uint8_t*)configStruct->defaultValue = (uint8_t)atoi(userStruct->value.c_str());
        break;
    case INT8_T:
        *(int8_t*)configStruct->defaultValue = (int8_t)atoi(userStruct->value.c_str());
        break;
    case UINT16_T:
        *(uint16_t*)configStruct->defaultValue = (uint16_t)atoi(userStruct->value.c_str());
        break;
    case INT16_T:
        *(int16_t*)configStruct->defaultValue = (int16_t)atoi(userStruct->value.c_str());
        break;
    case UINT32_T:
        *(uint32_t*)configStruct->defaultValue = (uint32_t)atoi(userStruct->value.c_str());
        break;
    case INT32_T:
        *(int32_t*)configStruct->defaultValue = (int32_t)atoi(userStruct->value.c_str());
        break;
    case UINT64_T:
        *(uint64_t*)configStruct->defaultValue = (uint64_t)atoll(userStruct->value.c_str());
        break;
    case INT64_T:
        *(int64_t*)configStruct->defaultValue = (int64_t)atoll(userStruct->value.c_str());
        break;
    case FLOAT:
        *(float*)configStruct->defaultValue = (float)atof(userStruct->value.c_str());
        break;
    case DOUBLE:
        *(double*)configStruct->defaultValue = (double)atof(userStruct->value.c_str());
        break;
    case STRING:
        configStruct->defaultStringValue = userStruct->value;
    default:
        break;
    }
}


/**
 * \brief Function will set user configured values to configuration map.
 * \param patternMap Pointer to configuration map
 * \param userMap Pointer to user configuration map with user filled values.
 * \return True on success, false otherwise.
 */
bool fillConfigStruct(map<string, configStrucItem> *patternMap, map<string, userConfigItem> *userMap)
{
    map<string, configStrucItem>::iterator it = patternMap->begin();

    while (it != patternMap->end()) {
        map<string, userConfigItem>::iterator userMapIt = userMap->find(it->first);
        if (userMapIt == userMap->end()) {
            if (it->second.isRequired) {
                cerr << "validation failed, variable (" << it->second.name << ") is required, but not found in user configuration file." << endl;
                return false;
            } else {
                it++;
                continue;
            }
        }

        if (userMapIt->second.type == USTRUCT) {
            bool retVal = fillConfigStruct((map<string, configStrucItem>*)it->second.map, (map<string, userConfigItem>*)userMapIt->second.map);
            if (!retVal) {
                return false;
            }
        }

        if (userMapIt->second.type == UARRAY) {
            // Copy elements from array
                it->second.elementAr = userMapIt->second.elementAr;
                it->second.arrType = userMapIt->second.arrType;
        }

        setUserValueToConfig(&it->second, &userMapIt->second);
        it++;
    }
    return true;
}



/**
 * \brief Function for converting string value element to its type and
 *        storing its value in memory.
 * \param element String value of element.
 * \param dest Destination memory address to which will be element value
 *             stored.
 * \param type Type of the element.
 */
void storeElementToMemory(string &element, void *dest, unsigned type)
{
    switch(type) {
    case UINT8_T:
        *(uint8_t*)dest = (uint8_t)atoi(element.c_str());
        break;
    case INT8_T:
        *(int8_t*)dest = (int8_t)atoi(element.c_str());
        break;
    case UINT16_T:
        *(uint16_t*)dest = (uint16_t)atoi(element.c_str());
        break;
    case INT16_T:
        *(int16_t*)dest = (int16_t)atoi(element.c_str());
        break;
    case UINT32_T:
        *(uint32_t*)dest = (uint32_t)atoi(element.c_str());
        break;
    case INT32_T:
        *(int32_t*)dest = (int32_t)atoi(element.c_str());
        break;
    case UINT64_T:
        *(uint64_t*)dest = (uint64_t)atoll(element.c_str());
        break;
    case INT64_T:
        *(int64_t*)dest = (int64_t)atoll(element.c_str());
        break;
    case FLOAT:
        *(float*)dest = (float)atof(element.c_str());
        break;
    case DOUBLE:
        *(double*)dest = (double)atof(element.c_str());
        break;
    case STRING:
        memcpy(dest, element.c_str(), element.length() + 1); // Copy with terminating byte
        break;
    default:
        break;
    }
}



/**
 * \brief Function will allocate memory for user specified array and
 *        then fill this memory with user filled values.
 * \param item Item containg specification of array and user filled values.
 * \return Memory address of allocated array on success, NULL otherwise.
 */
void *createUserArray(configStrucItem &item)
{
    if (item.arrType == AR_ELEMENT) {
        // Single element array
        unsigned int elemCnt = ((vector<userArrElemStruct> *)item.elementAr)->size();
        unsigned int elemType = ((configStrucItem *)item.arrElement)->type;
        unsigned int elemSize;
        if (elemType == STRING) {
            elemSize = ((configStrucItem *)item.arrElement)->stringMaxSize;
        } else {
            elemSize = varTypeSize[elemType];
        }
        void *arrayData;

        // Allocate space for array
        arrayData = malloc(elemCnt * elemSize);

        if (arrayData == NULL) {
            cerr << "Error: Could not allocate memory for user array!\n";
            return NULL;
        }
        UAMBS.memBlockArr.push_back(arrayData);
        UAMBS.memBlockElCount.push_back(elemCnt);

        // Fill allocated memory with user values
        // Iterate through every array element and stare them on corresponding offset
        unsigned elemOffset = 0;
        vector<userArrElemStruct> *elementAr = (vector<userArrElemStruct> *) item.elementAr;
        for (unsigned i = 0; i < elemCnt; i++) {
            string elemValue = (*elementAr)[i].value;

            if (elemType == STRING && elemValue.length() > elemSize) {
                // Cut string to maximum allowed size
                elemValue.resize(elemSize);
            }

            storeElementToMemory(elemValue, (char*)arrayData + elemOffset, elemType);
            elemOffset += elemSize + (elemType == STRING); // Plus null terminating byte if STRING
        }

        return arrayData;
    } else {
        // Array of structures
        unsigned int structCnt = ((vector<map<string, userConfigItem> > *)item.elementAr)->size();
        unsigned int structSize = item.arrayElementSize;
        void *arrayData;

        // Allocate space for array
        arrayData = malloc(structCnt * structSize);
        if (arrayData == NULL) {
            cerr << "Error: Could not allocate memory for user array!\n";
            return NULL;
        }
        UAMBS.memBlockArr.push_back(arrayData);
        UAMBS.memBlockElCount.push_back(structCnt);

        // Fill allocated memory with user values
        // Iterate through every array element (every structure)
        // Then iterate through elements in structure and store them on corresponding offset
        unsigned base_address = 0;
        vector<map<string, userConfigItem> > *elementAr = (vector<map<string, userConfigItem> > *) item.elementAr;
        for (unsigned i = 0; i < structCnt; i++) {
            map<string, userConfigItem> structMap = (*elementAr)[i];
            for (map<string, userConfigItem>::iterator it = structMap.begin(); it != structMap.end(); it++) {
                unsigned elementOffset = (*(map<string, configStrucItem>*)item.map)[it->first].offset;
                unsigned elementType = (*(map<string, configStrucItem>*)item.map)[it->first].type;
                string elementValue = it->second.value;

                if (elementType == STRING) {
                    // Cut string to maximum allowed size
                    elementValue.resize((*(map<string, configStrucItem>*)item.map)[it->first].stringMaxSize);
                }

                storeElementToMemory(elementValue, (char*)arrayData + base_address + elementOffset, elementType);
            }
            base_address += structSize;
        }
        return arrayData;
    }

    return NULL;
}

/**
 * \brief Function for creating C structure in memory based on parsed XML
 *        configuration.
 * \param inputStruct Pointer to memory where to create C structure.
 * \param configureMap Parsed XML configuration in form of C++ map.
 */
void getConfiguration(void *inputStruct, map<string, configStrucItem> *configureMap)
{
    map<string, configStrucItem>::iterator it = configureMap->begin();

    while (it != configureMap->end()) {
        switch(it->second.type) {
        case UINT8_T:
            *(uint8_t*)((char*)inputStruct + it->second.offset) = *((uint8_t*)(it->second).defaultValue);
            break;
        case INT8_T:
            *(int8_t*)((char*)inputStruct + it->second.offset) = *((int8_t*)(it->second).defaultValue);
            break;
        case UINT16_T:
            *(uint16_t*)((char*)inputStruct + it->second.offset) = *((uint16_t*)(it->second).defaultValue);
            break;
        case INT16_T:
            *(int16_t*)((char*)inputStruct + it->second.offset) = *((int16_t*)(it->second).defaultValue);
            break;
        case UINT32_T:
            *(uint32_t*)((char*)inputStruct + it->second.offset) = *((uint32_t*)(it->second).defaultValue);
            break;
        case INT32_T:
            *(int32_t*)((char*)inputStruct + it->second.offset) = *((int32_t*)(it->second).defaultValue);
            break;
        case UINT64_T:
            *(uint64_t*)((char*)inputStruct + it->second.offset) = *((uint64_t*)(it->second).defaultValue);
            break;
        case INT64_T:
            *(int64_t*)((char*)inputStruct + it->second.offset) = *((int64_t*)(it->second).defaultValue);
            break;
        case FLOAT:
            *(float*)((char*)inputStruct + it->second.offset) = *((float*)(it->second).defaultValue);
            break;
        case DOUBLE:
            *(double*)((char*)inputStruct + it->second.offset) = *((double*)(it->second).defaultValue);
            break;
        case STRING:
            {
                unsigned stringLen;
                // Choose smaller value between actual string size and maximum allowed size
                if ((unsigned) (it->second).stringMaxSize > (it->second).defaultStringValue.length()) {
                    stringLen = (it->second).defaultStringValue.length() + 1;
                } else {
                    stringLen = (it->second).stringMaxSize;
                }

                memcpy(((char*)inputStruct + it->second.offset), (it->second).defaultStringValue.c_str(), stringLen);
                *(((char*)inputStruct + it->second.offset) + (it->second).stringMaxSize) = 0; // Null byte to terminate string
                break;
             }
        case STRUCT:
            getConfiguration(inputStruct, (map<string, configStrucItem>*)(it->second).map);
            break;
        case ARRAY:
            *(void**)((char*)inputStruct + it->second.offset) = createUserArray(it->second);
            break;
        default:
            break;
        }
        it++;
    }
}


/**
 * \brief Initializing function for XML parser. Checks for
 *        successfull memory allocation of configuration maps.
 * \return True on success, false otherwise.
 */
bool initXmlParser()
{
    configStructureMap  = new map<string, configStrucItem>;
    userConfigMap = new map<string, userConfigItem>;

    if (configStructureMap == NULL) {
        cerr << "parser error, cannot allocate enought space for configuration Map." << endl;
        xmlCleanupParser();
        return EXIT_FAILURE;
    }

    if (userConfigMap == NULL) {
        cerr << "parser error, cannot allocate enought space for User Configuration Map." << endl;
        delete configStructureMap;
        xmlCleanupParser();
        return EXIT_FAILURE;
    }

    return 0;
}

/**
 * \brief Function loads two configuration files, parses them and fill
 *        configuration structure with values from these files.
 * \param patternFile XML file with specification of structure.
 * \param userFile XML file with user filled values.
 * \param userStruct Pointer to memory where configuration structure will
 *                   be created.
 * \param patternType Specify type of `patternFile` config. Could be either
 *                    CONF_PATTERN_FILE, if configuration is read from XML file
 *                    or CONF_PATTERN_STRING, if configuration is read from
 *                    string.
 * \return 0 on success, EXIT_FAILURE otherwise.
 */
extern "C" int loadConfiguration(char *patternFile, char *userFile, void *userStruct, int patternType)
{
    string structurePatternFile = patternFile;
    string structureUserConfigFile = userFile;

    // Clear offset from previous run
    globalStructureOffset = 0;

    if (initXmlParser()) {
       return EXIT_FAILURE;
    }

    // Read from file or string based on patternType
    switch (patternType) {
    case CONF_PATTERN_FILE:
        doc = xmlParseFile(structurePatternFile.c_str());
        break;
    case CONF_PATTERN_STRING:
        doc = xmlParseDoc((const xmlChar*)structurePatternFile.c_str());
        break;
    default:
        cerr << "Error: Configurator: Unknown pattern type." << endl;
        return EXIT_FAILURE;
    }

    if (doc == NULL) {
        cerr << "Configurator: Cannot parse configuration file." << endl;
        return EXIT_FAILURE;
    }


    userDoc = xmlParseFile(structureUserConfigFile.c_str());
    if (userDoc == NULL) {
        cerr << "Configurator: Cannot parse user configuration file." << endl;
        return EXIT_FAILURE;
    }


    //cout << "Started parsing pattern structure..." << endl;
    bool patternRetVal = parseStruct(&doc, PATTERNXML);
    if (!patternRetVal) {
        cerr << "Configurator: Parsing failed." << endl;
        return EXIT_FAILURE;
    } /*else {
        cout << "Parsing compete..." << endl;
        //printConfigMap(configStructureMap);
    }*/

    //cout << "Started parsing user config structure..." << endl;
    bool userRetVal = parseStruct(&userDoc, USERXML);
    if (!userRetVal) {
        cerr << "Configurator: Parsing failed." << endl;
        return EXIT_FAILURE;
    } /*else {
        cout << "Parsing complete..." << endl;
        //printUserMap(userConfigMap);
    }*/


    //cout << "Starting validation..." << endl;
    bool validationRetVal = fillConfigStruct(configStructureMap, userConfigMap);
    if (!validationRetVal) {
        cerr << "Configurator: Validation failed." << endl;
        //printConfigMap(configStructureMap);
    }



    getConfiguration(userStruct, configStructureMap);

    xmlFreeDoc(doc);
    xmlFreeDoc(userDoc);
    xmlCleanupParser();
    clearConfigStructureMap(configStructureMap);
    clearConfigStructureMap(userConfigMap);

   return 0;
}
