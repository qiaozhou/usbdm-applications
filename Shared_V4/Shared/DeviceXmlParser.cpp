/*! \file
    \brief Parses device XML files

    DeviceXmlParser.cpp

    \verbatim
    USBDM
    Copyright (C) 2009  Peter O'Donoghue

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    \endverbatim

    \verbatim
   Change History
   -============================================================================
   |    Aug 2011 | Added I/O memory type                                    - pgo
   |    Aug 2011 | Removed Boost                                            - pgo
   |    Apr 2011 | Added TCL scripts etc                                    - pgo
   +============================================================================
   \endverbatim
*/

#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <typeinfo>
#include <string>
#include <limits.h>
#include <errno.h>
#include <map>

using namespace std;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/sax/HandlerBase.hpp>
XERCES_CPP_NAMESPACE_USE

#include "Common.h"
#include "helper-classes.h"
#include "DeviceXmlParser.h"
#include "DeviceData.h"
#include "Names.h"

#include "Log.h"

char DeviceXmlParser::currentDeviceName[] = "Doing preamble";

#if 0
//! Convert a single digit to an integer
//!
//! @param s    - character to convert
//! @param base - base to use - either 8, 10 or 16
//!
//! @return <0  => invalid digit
//!         >=0 => converted digit
//!
uint8_t getDigit(char s, int base) {
   int digit = s-'0';
   if ((s>=0) && (s<=base)) {
      return digit;
   }
   if (base <= 10) {
      return -1;
   }
   digit -= 10;
   if ((s>=0) && (s<=base)) {
      return digit;
   }
   return -1;
}

//! Convert string to long integer
//!
//! @param start = ptr to string to convert
//! @param end   = updated to point to char following valid string (may be NULL)
//! @param value = value read
//!
//! @return true  => OK \n
//!         false => failed
//!
//! @note Accepts decimal, octal with '0' prefix or hex with '0x' prefix \n
//!                        '_' may be used to space number e.g. 0x1000_0000
//!
bool strToULong(const char *s, char **end, long *returnValue) {
   uint32_t value = 0;
   int base = 10;
   *returnValue = 0;
   while (isspace(*s)) {
      s++;
   }
   if (*s == '_') {
      // Don't allow '_' as first character of number
      return false;
   }
   if (*s == '0') {
      s++;
      base = 8;
      if ((s=='x')||(s=='X')) {
         s++;
         base = 16;
      }
   }
   char *startOfNum = s;
   do {
      if (*s == '_') {
         // Ignore '_'
         continue;
      }
      int digit = getDigit(*s);
      if (digit < 0) {
         break;
      }
      s++;
      value = base*value + digit;
   } while (1);
   if (startOfNum == s) {
      // No digits
      return false;
   }
   if (end != NULL) {
      // Record end of number (immediately following character)
      end = s;
   }
   else {
      // If end is not used then check if at end of string
      // Skip trailing spaces
      while (isspace(*end_t)) {
         end_t++;
      }
      if (*end_t != '\0') {
         print("strToULong() - Unexpected chars following number (%s)\n", start);
         return false;
      }
   }
   // if accepted chars its a valid number
   return true;
}
#else
//! Convert string to long integer
//!
//! @param start = ptr to string to convert
//! @param end   = updated to point to char following valid string (may be NULL)
//! @param value = value read
//!
//! @return true => OK \n
//!         false => failed
//!
//! @note Accepts decimal, octal with '0' prefix or hex with '0x' prefix
//!
bool strToULong(const char *start, char **end, long *value) {
   char *end_t;

//   while (isspace(*start)) {
//      start++;
//   }
//   errno = 0;
   unsigned long value_t = strtoul(start, &end_t, 0);

//   print("strToULong() - s=\'%s\', e='%s', val=%ld(0x%lX)\n", start, end_t, value_t, value_t);
   if (errno != 0) { // no String found
//      print("strToLong() - No number found\n");
      return false;
   }
   if (end_t == start) { // no String found
//      print("strToLong() - No number found\n");
      return false;
   }
   if ((ULONG_MAX == value_t) && ERANGE == errno) { // too large
     print("strToULong() - Number too large\n");
     return false;
   }
   if (end != NULL) {
      *end = end_t;
   }
   else {
      // If end is not used then check if at end of string
      // Skip trailing spaces
      while (isspace(*end_t)) {
         end_t++;
      }
      if (*end_t != '\0') {
         print("strToULong() - Unexpected chars following number (%s)\n", start);
         return false;
      }
   }
   *value = value_t;
   return true;
}
#endif

//! Convert string to long integer.
//!
//! @param start = ptr to string to convert
//! @param end   = updated to point to char following valid string (may be NULL)
//! @param value = value read
//!
//! @return true => OK \n
//!         false => failed
//!
//! @note Accepts decimal, octal with '0' prefix or hex with '0x' prefix
//! @note Value may have K or M suffix to indicate kilobytes(2^10) or megabytes(2^20) \n
//!       e.g. '1M' '0x1000 K' etc
//!
bool strSizeToULong(const char *start, char **end, long *value) {
   char *end_t;
   bool success = strToULong(start, &end_t, value);
   if (!success) {
      return false;
   }
   if (end != NULL) {
      *end = end_t;
   }
   // Skip trailing spaces
   while (isspace(*end_t)) {
      end_t++;
   }
   if ((*end_t == 'k') || (*end_t == 'K')) {
      *value <<= 10;
      ++end_t;
      if (end != NULL)
         *end = end_t;
   }
   if ((*end_t == 'm') || (*end_t == 'M')) {
      *value <<= 20;
      ++end_t;
      if (end != NULL)
         *end = end_t;
   }
   if (end == NULL) {
      // If end is not used then check if at end of string
      // Skip trailing spaces
      while (isspace(*end_t)) {
         end_t++;
      }
      if (*end_t != '\0') {
         print("strToULong() - Unexpected chars following number\n");
         return false;
      }
   }
   return true;
}

//! Simple utility class for traversing XML document
//!
class DOMChildIterator {

private:
   DOMNodeList *elementList;
   int          index;
   DOMElement  *currentElement;

public:
   DOMChildIterator(DOMDocument *document, const char *tagName) {
      elementList    = document->getElementsByTagName(DualString(tagName).asXMLString());
      index          = 0;
      findElement(0);
   };

   DOMChildIterator(DOMElement *element) {
      elementList    = element->getChildNodes();
      index          = 0;
      findElement(0);
   };

   DOMChildIterator(DOMElement *element, const char *tagName) {
      elementList    = element->getElementsByTagName(DualString(tagName).asXMLString());
      index          = 0;
      findElement(0);
   };

private:
   void findElement(unsigned index) {
      currentElement = NULL;
      while ((elementList != NULL) && (index < elementList->getLength())) {
         DOMNode *node = elementList->item(index);
         if (node == NULL) {
            // Should be impossible
            throw MyException("DOMChildIterator::findElement() - elementList->item(i) failed");
         }
//         print("DOMChildIterator::setIndex() examining <%s> node\n", DualString(node->getNodeName()).asCString());
         if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
//            print("DOMChildIterator::findElement() Element node type (from)  = %s\n", typeid(node).name());
//            print("DOMChildIterator::findElement() Element node type (to)    = %s\n", typeid(DOMElement *).name());
//            DOMElement *el = dynamic_cast< DOMElement* >( node );
            DOMElement *el = static_cast< DOMElement* >( node );
//            if (el == NULL) {
//               // Should be impossible
//               throw MyException("DOMChildIterator::findElement() - casting failed");
//            }
//            print("DOMChildIterator::findElement() Element node type (after) = %s\n", typeid(el).name());
            currentElement = el;
            this->index = index;
            break;
         }
//         else {
//            print("DOMChildIterator::findElement() Non-element node type = %s\n", typeid(node).name());
//         }
         index++;
      }
   }

public:
   DOMElement *getCurrentElement() {
      return currentElement;
   }

public:
   DOMElement *advanceElement() {
      findElement(index+1);
      return currentElement;
   }
};

//! XML parser for device files
//!
DeviceXmlParser::DeviceXmlParser(DeviceDataBase *deviceDataBase)
   :
   tag_deviceList("deviceList"),
   tag_device("device"),
   tag_clock("clock"),
   tag_memory("memory"),
   tag_memoryRef("memoryRef"),
   tag_memoryRange("memoryRange"),
   tag_sdid("sdid"),
   tag_sdidMask("sdidMask"),
   tag_sdidAddress("sdidAddress"),
   tag_soptAddress("soptAddress"),
   tag_securityAddress("securityAddress"),
   tag_copctlAddress("copctlAddress"),
//   tag_foptAddress("foptAddress"),
//   tag_fsecAddress("fsecAddress"),
   tag_note("note"),
   tag_tclScriptRef("tclScriptRef"),
   tag_sharedInformation("sharedInformation"),
   tag_tclScript("tclScript"),
   tag_preSequence("preSequence"),
   tag_postSequence("postSequence"),
   tag_flashScripts("flashScripts"),
   tag_flashProgram("flashProgram"),
   tag_flashProgramRef("flashProgramRef"),
   tag_securityInfo("securityInfo"),
   tag_securityInfoRef("securityInfoRef"),
   tag_flexNvmInfo("flexNVMInfo"),
   tag_flexNvmInfoRef("flexNVMInfoRef"),
   tag_eeepromEntry("eeepromEntry"),
   tag_partitionEntry("partitionEntry"),

   attr_name("name"),
   attr_isDefault("isDefault"),
   attr_alias("alias"),
   attr_hidden("hidden"),
   attr_family("family"),
   attr_subFamily("subFamily"),
   attr_speed("speed"),
   attr_type("type"),
   attr_subType("subType"),
   attr_start("start"),
   attr_middle("middle"),
   attr_end("end"),
   attr_size("size"),
   attr_pageNo("pageNo"),
   attr_pageStart("pageStart"),
   attr_pageEnd("pageEnd"),
   attr_pageReset("pageReset"),
   attr_pages("pages"),
   attr_pageAddress("pageAddress"),
   attr_registerAddress("registerAddress"),
   attr_securityAddress("securityAddress"),
   attr_sectorSize("sectorSize"),
   attr_value("value"),
   attr_id("id"),
   attr_ref("ref"),
   attr_addressMode("addressMode"),
   attr_description("description"),
   attr_eeeSize("eeeSize"),
   attr_eeSize("eeSize"),
   attr_alignment("alignment"),

   isDefault(false),
   deviceDataBase(deviceDataBase),

   parser(NULL),
   errHandler(NULL),
   document(NULL) {
//   print("DeviceXmlParser::DeviceXmlParser()\n");
}

DeviceXmlParser::~DeviceXmlParser() {
   // Don't delete document as owned by parser!
//   print("DeviceXmlParser::~DeviceXmlParser()\n");
   delete parser;
   delete errHandler;
}

//! Load a device XML file as xerces document
//!
//! @param xmlFile - Device XML file to load
//!
//! @throws MyException() - on any error
//!
void DeviceXmlParser::loadFile(const string &xmlFile) {
//   print("DeviceXmlParser::loadFile()\n");
   parser = new XercesDOMParser();
   parser->setDoNamespaces( true );
   parser->setDoSchema( false ) ;
   parser->setLoadExternalDTD( false ) ;
   parser->setValidationScheme(XercesDOMParser::Val_Auto);
   parser->setValidationSchemaFullChecking(false);
//   print("DeviceXmlParser::loadFile() #1\n");
   errHandler = new HandlerBase();
//   print("DeviceXmlParser::loadFile() #2\n");
   parser->setErrorHandler(errHandler);
//   print("DeviceXmlParser::loadFile() #3\n");
   parser->parse(xmlFile.c_str()); //xx
//   print("DeviceXmlParser::loadFile() #4\n");
   document = parser->getDocument();
//   print("DeviceXmlParser::loadFile() #5\n");
   if (document == NULL) {
      throw MyException("DeviceXmlParser::loadFile() - Unable to create document or load/parse file");
   }
}

// Create a TCL script from XML element
TclScript *DeviceXmlParser::parseTCLScript(DOMElement *xmlTclScript) {
   // Type of node (must be a TCL script)
   DualString sTag (xmlTclScript->getTagName());
   if (!XMLString::equals(sTag.asXMLString(), tag_tclScript.asXMLString())) {
      throw MyException(string("DeviceXmlParser::parseTCLScript() - Unexpected tag = ")+sTag.asCString());
   }
   DualString text(xmlTclScript->getTextContent());
   TclScript *tclScript =  new TclScript(text.asCString());
//   print("DeviceXmlParser::parseTCLScript():\n");
//   print(tclScript->toString().c_str());
   return tclScript;
}

// Create a flashProgram from XML element
FlashProgram *DeviceXmlParser::parseFlashProgram(DOMElement *xmlFlashProgram) {
   // Type of node (must be a flashProgram)
   DualString sTag (xmlFlashProgram->getTagName());
   if (!XMLString::equals(sTag.asXMLString(), tag_flashProgram.asXMLString())) {
      throw MyException(string("DeviceXmlParser::parseFlashProgram() - Unexpected tag = ")+sTag.asCString());
   }
   DualString text(xmlFlashProgram->getTextContent());
   FlashProgram *flashProgram =  new FlashProgram(text.asCString());
//   print("DeviceXmlParser::parseFlashProgram():\n");
//   print(flashProgram->toString().c_str());
   return flashProgram;
}

//! Create security description from node
//!
SecurityInfo *DeviceXmlParser::parseSecurity(DOMElement *currentProperty) {

   // Type of node (must be a flashProgram)
   DualString sTag (currentProperty->getTagName());
   if (!XMLString::equals(sTag.asXMLString(), tag_securityInfo.asXMLString())) {
      throw MyException(string("DeviceXmlParser::parseSecurity() - Unexpected tag = ")+sTag.asCString());
   }
   long size = 0;
   DualString sSize(currentProperty->getAttribute(attr_size.asXMLString()));
   if (!strToULong(sSize.asCString(), NULL, &size)) {
      throw MyException(string("DeviceXmlParser::parseSecurity() - Illegal size in securityInfo = ")+sSize.asCString());
   }
   bool type;
   DualString sType(currentProperty->getAttribute(attr_type.asXMLString()));
   if (strcmp(sType.asCString(), "on")==0) {
      type = true;
   }
   else if (strcmp(sType.asCString(), "off")==0) {
      type = false;
   }
   else {
      throw MyException(string("DeviceXmlParser::parseSecurity() - Illegal type in securityInfo = ")+sType.asCString());
   }
   DualString text(currentProperty->getTextContent());
   SecurityInfo *securityInfo =  new SecurityInfo(size, type, text.asCString());
//   print("DeviceXmlParser::parseSecurity():\n");
//   print(securityInfo->toString().c_str());
   return securityInfo;
}

//! Create shared elements from document
//!
void DeviceXmlParser::parseSharedXML(void) {
//   print("DeviceXmlParser::parseSharedXML()\n");

   strncpy(currentDeviceName, "Shared Data", sizeof(currentDeviceName));

   // Iterator for all the <sharedInformation> sections
   DOMChildIterator sharedIt(document, tag_sharedInformation.asCString());
   // Process each <sharedInformation> section (usually only one)
   for (;
         sharedIt.getCurrentElement() != NULL;
         sharedIt.advanceElement()) {
      // Point to <sharedInformation> section
      DOMElement *shareInformationIt(sharedIt.getCurrentElement());

      // Iterator for the contents of <sharedInformation> section
      // Each is a shared element
      DOMChildIterator sharedInformationElementIt(shareInformationIt);
      // Process each element of <sharedInformation>
      for (;
            sharedInformationElementIt.getCurrentElement() != NULL;
            sharedInformationElementIt.advanceElement()) {
         // Handle to <sharedInformation> element
         DOMElement *sharedInformationElement = sharedInformationElementIt.getCurrentElement();
         // ID for element - this allows the element to be accessed
         DualString sId(sharedInformationElement->getAttribute(attr_id.asXMLString()));
         // Type of element
         DualString sTag (sharedInformationElement->getTagName());
         if (XMLString::equals(sTag.asXMLString(), tag_tclScript.asXMLString())) {
            // Parse <tclScript>
            TclScript *tclScript = parseTCLScript(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), tclScript);
            print("DeviceXmlParser::parseSharedXML(): Inserted shared TclScript item ID=%s\n", sId.asCString());
         }
         else if (XMLString::equals(sTag.asXMLString(), tag_flashProgram.asXMLString())) {
            // Parse <flashProgram>
            FlashProgram *flashProgram = parseFlashProgram(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), flashProgram);
            print("DeviceXmlParser::parseSharedXML(): Inserted shared FlashProgram item ID=%s\n", sId.asCString());
         }
         else if (XMLString::equals(sTag.asXMLString(), tag_securityInfo.asXMLString())) {
            // Parse <securityInfo>
            SecurityInfo *securityInfo = parseSecurity(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), securityInfo);
            print("DeviceXmlParser::parseSharedXML(): Inserted shared SecurityInfo item ID=%s\n", sId.asCString());
         }
         else if (XMLString::equals(sTag.asXMLString(), tag_flexNvmInfo.asXMLString())) {
            // Parse <flexNVMInfo>
            FlexNVMInfo *flexNVMInfo = parseFlexNVMInfo(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), flexNVMInfo);
            print("DeviceXmlParser::parseSharedXML(): Inserted shared FlexNVMInfo item ID=%s\n", sId.asCString());
         }
         else if (XMLString::equals(sTag.asXMLString(), tag_memory.asXMLString())) {
            // Parse <memory>
            MemoryRegion *memoryRegion = parseMemory(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), memoryRegion);
            print("DeviceXmlParser::parseSharedXML(): Inserted shared MemoryRegion item ID=%s\n", sId.asCString());
         }
         else {
            throw MyException(string("DeviceXmlParser::parseSharedXML() - Unexpected Tag = ")+sTag.asCString());
         }
      }
   }
}

TclScriptPtr DeviceXmlParser::parseSequence(DOMElement *sequence) {
   // Iterator for <preSequence/postSequence/unsecureSequence> children
   DOMChildIterator sequenceElementIt(sequence);
   DOMElement *sequenceElement = sequenceElementIt.getCurrentElement();
   if (sequenceElement == NULL) {
      // No <preSequence/postSequence> children
      print("DeviceXmlParser::parseSequence() - empty <preSequence/postSequence> ignored\n");
      return TclScriptPtr();
   }
   else {
      // Process each <tclStriptRef> child (should be only one)
      DualString sequenceTag(sequenceElement->getTagName());
      if (XMLString::equals(sequenceTag.asXMLString(), tag_tclScriptRef.asXMLString())) {
         // <tclScriptRef>
         DualString sRef(sequenceElement->getAttribute(attr_ref.asXMLString()));
         SharedInformationItemPtr itemPtr = deviceDataBase->getSharedData(sRef.asCString());
         TclScriptPtr tclScript = std::tr1::dynamic_pointer_cast<TclScript>(itemPtr);
         if (tclScript == NULL) {
            throw MyException(string("DeviceXmlParser::parseSequence() - Unable to find reference - ")+sRef.asCString());
         }
//         print("DeviceXmlParser::parseSequence() -  tclScriptRef \'%s\'\n", sRef.asCString());
         return tclScript;
      }
      else if (XMLString::equals(sequenceTag.asXMLString(), tag_tclScript.asXMLString())) {
         // <tclScript>
         TclScript *tclScript = parseTCLScript(sequenceElement);
         return TclScriptPtr(tclScript);
      }
      else {
         throw MyException(string("DeviceXmlParser::parseSequence() - Unexpected Tag = ")+sequenceTag.asCString());
      }
   }
}

FlexNVMInfo::EeepromSizeValue DeviceXmlParser::parseEeepromEntry(DOMElement *eeepromElement) {
   long eeeSize;
   long value;
   DualString sDescription(eeepromElement->getAttribute(attr_description.asXMLString()));
   DualString sEeeSize(eeepromElement->getAttribute(attr_eeeSize.asXMLString()));
   DualString sValue(eeepromElement->getAttribute(attr_value.asXMLString()));
   if ((strlen(sDescription.asCString()) == 0) ||
       !strSizeToULong(sEeeSize.asCString(), NULL, &eeeSize) ||
       !strToULong(sValue.asCString(), NULL, &value)) {
      throw MyException("DeviceXmlParser::parseEeepromEntry() - Illegal description/eeeSize/value attributes");
   }
   return (FlexNVMInfo::EeepromSizeValue(sDescription.asCString(), value, eeeSize));
}

FlexNVMInfo::FlexNvmPartitionValue DeviceXmlParser::parsePartitionEntry(DOMElement *partitionElement) {
   long eeSize;
   long value;
   DualString sDescription(partitionElement->getAttribute(attr_description.asXMLString()));
   DualString sEeSize(partitionElement->getAttribute(attr_eeSize.asXMLString()));
   DualString sValue(partitionElement->getAttribute(attr_value.asXMLString()));
   if ((strlen(sDescription.asCString()) == 0) ||
       !strSizeToULong(sEeSize.asCString(), NULL, &eeSize) ||
       !strToULong(sValue.asCString(), NULL, &value)) {
      throw MyException("DeviceXmlParser::parsePartitionEntry() - Illegal description/eeeSize/value attributes");
   }
   return (FlexNVMInfo::FlexNvmPartitionValue(sDescription.asCString(), value, eeSize));
}

//! Create flexNVMInfo description from flexNVMInfo node
//!
//! @param flexNVMInfoElement - Present position in XML parse
//!
//! @return FlexNVMInfo node created
//!
FlexNVMInfo *DeviceXmlParser::parseFlexNVMInfo(DOMElement *flexNVMInfoElement) {
   static unsigned defaultBackingRatio = 16;

   FlexNVMInfo *pFlexNVMInfo = new FlexNVMInfo(defaultBackingRatio);
   DOMChildIterator eeepromEntryIt(flexNVMInfoElement, tag_eeepromEntry.asCString());
   for (;
         eeepromEntryIt.getCurrentElement() != NULL;
         eeepromEntryIt.advanceElement()) {
      // <eeepromEntry>
      FlexNVMInfo::EeepromSizeValue eeepromSizeValue = parseEeepromEntry(eeepromEntryIt.getCurrentElement());
      pFlexNVMInfo->addEeepromSizeValues(eeepromSizeValue);
   }
   DOMChildIterator partitionEntryIt(flexNVMInfoElement, tag_partitionEntry.asCString());
   for (;
         partitionEntryIt.getCurrentElement() != NULL;
         partitionEntryIt.advanceElement()) {
      // <partitionEntry>
      FlexNVMInfo::FlexNvmPartitionValue flexNvmPartitionValue = parsePartitionEntry(partitionEntryIt.getCurrentElement());
      pFlexNVMInfo->addFlexNvmPartitionValues(flexNvmPartitionValue);
   }
   return pFlexNVMInfo;
}

//! Create memory description from node for flash 'type' memory
//!
//! @param currentProperty - Present position in XML parse
//!
//! @return == 0 - success\n
//!         != 0 - fail
//!
MemoryRegion *DeviceXmlParser::parseFlashMemoryDetails(DOMElement *currentProperty, MemType_t memoryType, uint32_t &defaultSectorSize, uint8_t &defaultAlignment) {
   long ppageAddress    = 0;
   long registerAddress = 0;
   long securityAddress = 0;
   DualString sPpageAddress(currentProperty->getAttribute(attr_pageAddress.asXMLString()));
   DualString sRegisterAddress(currentProperty->getAttribute(attr_registerAddress.asXMLString()));
   DualString sSecurityAddress(currentProperty->getAttribute(attr_securityAddress.asXMLString()));
   if (!strToULong(sPpageAddress.asCString(),    NULL, &ppageAddress) ||
       !strToULong(sRegisterAddress.asCString(), NULL, &registerAddress) ||
       !strToULong(sSecurityAddress.asCString(), NULL, &securityAddress)) {
      throw MyException("DeviceXmlParser::parseFlashMemoryDetails() - Illegal PPAGE/Register/Security field in Flash");
   }
   long sectorSize      = defaultSectorSize;
   if (currentProperty->hasAttribute(attr_sectorSize.asXMLString())) {
      DualString sSectorSize(currentProperty->getAttribute(attr_sectorSize.asXMLString()));
      if (!strToULong(sSectorSize.asCString(), NULL, &sectorSize)) {
         throw MyException(string("DeviceXmlParser::parseFlashMemoryDetails() - Illegal sectorSize in Flash - ")+sSectorSize.asCString());
      }
      if (isDefault) {
         print("DeviceXmlParser::parseFlashMemoryDetails() - Setting %s default sector size 0x%04X\n",
               MemoryRegion::getMemoryTypeName(memoryType), sectorSize);
         defaultSectorSize = sectorSize;
      }
   }
   static AddressType defaultAddressMode = AddrPaged;
   AddressType        addressMode        = defaultAddressMode;
   if (currentProperty->hasAttribute(attr_addressMode.asXMLString())) {
      DualString sAddressMode(currentProperty->getAttribute(attr_addressMode.asXMLString()));
      if (XMLString::equals(sAddressMode.asXMLString(), DualString("linear").asXMLString())) {
         addressMode = AddrLinear;
      }
      else if (XMLString::equals(sAddressMode.asXMLString(), DualString("paged").asXMLString())) {
         addressMode = AddrPaged;
      }
      else {
         throw MyException(string("DeviceXmlParser::parseFlashMemoryDetails() - Illegal addressMode in Flash - ")+sAddressMode.asCString());
      }
      if (isDefault) {
         print("DeviceXmlParser::parseFlashMemoryDetails() - Setting default address mode = %s\n",
               addressMode==AddrLinear?"linear":"paged");
         defaultAddressMode = addressMode;
      }
   }
   uint8_t alignment = defaultAlignment;
   if (currentProperty->hasAttribute(attr_alignment.asXMLString())) {
      DualString sAlignment(currentProperty->getAttribute(attr_alignment.asXMLString()));
      long temp;
      if (!strToULong(sAlignment.asCString(), NULL, &temp) ||
          ((temp != 1) && (temp != 2) && (temp != 4) && (temp != 8) && (temp != 16) && (temp != 32))) {
         throw MyException(string("DeviceXmlParser::parseFlashMemoryDetails() - Illegal alignment ")+sAlignment.asCString());
      }
      alignment = (uint8_t) temp;
      if (isDefault) {
         print("DeviceXmlParser::parseFlashMemoryDetails() - Setting %s default alignment = %d\n",
               MemoryRegion::getMemoryTypeName(memoryType), alignment);
         defaultAlignment = alignment;
      }
   }
   MemoryRegion *pMemoryRegion = new MemoryRegion(memoryType, registerAddress, ppageAddress, securityAddress, sectorSize, alignment);
   pMemoryRegion->setAddressType(addressMode);
   return pMemoryRegion;
}

//! Create memory description from node
//!
//! @param currentProperty - Present position in XML parse
//!
//! @return == 0 - success\n
//!         != 0 - fail
//!
MemoryRegion *DeviceXmlParser::parseMemory(DOMElement *currentProperty) {

   // <memory>
   DualString sMemoryType(currentProperty->getAttribute(attr_type.asXMLString()));
   MemoryRegion *pMemoryRegion = NULL;

   if (XMLString::equals(sMemoryType.asXMLString(), DualString("eeprom").asXMLString())) {
      // <eeprom>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemEEPROM, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("flash").asXMLString())) {
      // <flash>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemFLASH, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("pFlash").asXMLString())) {
      // <flash>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemPFlash, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("dFlash").asXMLString())) {
      // <flash>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemDFlash, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("pROM").asXMLString())) {
      // <prom>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemPROM, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("xROM").asXMLString())) {
      // <xrom>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemXROM, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("flexNVM").asXMLString())) {
      // <flexNVM>
      static uint32_t defaultFlashSectorSize = 0;
      static uint8_t  defaultAlignment = 1;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemFlexNVM, defaultFlashSectorSize, defaultAlignment);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("ram").asXMLString())) {
      // <ram>
      pMemoryRegion = new MemoryRegion(MemRAM);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("rom").asXMLString())) {
      // <rom>
      pMemoryRegion = new MemoryRegion(MemROM);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("flexRAM").asXMLString())) {
      // <ram>
      pMemoryRegion = new MemoryRegion(MemFlexRAM);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("io").asXMLString())) {
      // <io>
      pMemoryRegion = new MemoryRegion(MemIO);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("pRAM").asXMLString())) {
      // <pram>
      pMemoryRegion = new MemoryRegion(MemPRAM);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("xRAM").asXMLString())) {
      // <xram>
      pMemoryRegion = new MemoryRegion(MemXRAM);
   }
   else {
      print("DeviceXmlParser::parseDeviceXML() - invalid memory range type\n");
      return NULL;
   }
   DOMChildIterator flashProgramIt(currentProperty, tag_flashProgram.asCString());
   if (flashProgramIt.getCurrentElement() != NULL) {
      // <flashProgram>
      FlashProgram *flashProgram = parseFlashProgram(flashProgramIt.getCurrentElement());
//      print("Flash program =\n%s", flashProgram->toString().c_str());
      FlashProgramPtr pFlashProgram(flashProgram);
      pMemoryRegion->setFlashProgram(pFlashProgram);
   }
   DOMChildIterator flashProgramRefIt(currentProperty, tag_flashProgramRef.asCString());
   if (flashProgramRefIt.getCurrentElement() != NULL) {
      // <flashProgramRef>
      DualString sRef(flashProgramRefIt.getCurrentElement()->getAttribute(attr_ref.asXMLString()));
      SharedInformationItemPtr flashProgram(deviceDataBase->getSharedData(sRef.asCString()));
      if (!flashProgram) {
         throw MyException(string("DeviceXmlParser::parseMemory() - Unable to find reference - ")+sRef.asCString());
      }
      else {
         FlashProgramPtr pFlashProgram = std::tr1::dynamic_pointer_cast<FlashProgram>(flashProgram);
//         print("Referenced Flash program =%s\n", sRef.asCString());
         pMemoryRegion->setFlashProgram(pFlashProgram);
      }
   }
   DOMChildIterator securityIt(currentProperty, tag_securityInfo.asCString());
   for (;
         securityIt.getCurrentElement() != NULL;
         securityIt.advanceElement()) {
      // <securityInfo>
      SecurityInfo *securityInfo = parseSecurity(securityIt.getCurrentElement());
//      print("Flash security (%s)\n", securityInfo->toString().c_str());
      SecurityInfoPtr pSecurityInfoPtr(securityInfo);
      if (pSecurityInfoPtr->getMode() == true) {
         pMemoryRegion->setSecureInfo(pSecurityInfoPtr);
      }
      else {
         pMemoryRegion->setUnsecureInfo(pSecurityInfoPtr);
      }
   }
   DOMChildIterator securityRefIt(currentProperty, tag_securityInfoRef.asCString());
   for (;
         securityRefIt.getCurrentElement() != NULL;
         securityRefIt.advanceElement()) {
      // <securityInfoRef>
      DualString sRef(securityRefIt.getCurrentElement()->getAttribute(attr_ref.asXMLString()));
      SharedInformationItemPtr securityInfo(deviceDataBase->getSharedData(sRef.asCString()));
      if (!securityInfo) {
         throw MyException(string("DeviceXmlParser::parseMemory() - Unable to find reference - ")+sRef.asCString());
      }
      else {
         SecurityInfoPtr pSecurityInfoPtr = std::tr1::dynamic_pointer_cast<SecurityInfo>(securityInfo);
         if (pSecurityInfoPtr->getMode() == true) {
            pMemoryRegion->setSecureInfo(pSecurityInfoPtr);
         }
         else {
            pMemoryRegion->setUnsecureInfo(pSecurityInfoPtr);
         }
      }
   }
   DOMChildIterator flexNvmInfoIt(currentProperty, tag_flexNvmInfo.asCString());
   if (flexNvmInfoIt.getCurrentElement() != NULL) {
      // <flexNVMInfo>
      FlexNVMInfo *flexNVMInfo = parseFlexNVMInfo(flexNvmInfoIt.getCurrentElement());
      FlexNVMInfoPtr pFlexNVMInfo(flexNVMInfo);
      pMemoryRegion->setflexNVMInfo(pFlexNVMInfo);
   }
   DOMChildIterator flexNvmInfoRefIt(currentProperty, tag_flexNvmInfoRef.asCString());
   if (flexNvmInfoRefIt.getCurrentElement() != NULL) {
      // <flexNvmInfoRef>
      DualString sRef(flexNvmInfoRefIt.getCurrentElement()->getAttribute(attr_ref.asXMLString()));
      SharedInformationItemPtr pFlexNvmInfo(deviceDataBase->getSharedData(sRef.asCString()));
      if (!pFlexNvmInfo) {
         throw MyException(string("DeviceXmlParser::parseMemory() - Unable to find reference - ")+sRef.asCString());
      }
      else {
         print("DeviceXmlParser::parseDeviceXML() - using shared flexNVMInfo()\n");
         pMemoryRegion->setflexNVMInfo(std::tr1::dynamic_pointer_cast<FlexNVMInfo>(pFlexNvmInfo));
      }
   }
   DOMChildIterator memoryRangeIt(currentProperty, tag_memoryRange.asCString());
   for (;
         memoryRangeIt.getCurrentElement() != NULL;
         memoryRangeIt.advanceElement()) {
      // <memoryRange>

      //====================================================
      // Memory addresses
      //
      bool startGiven  = memoryRangeIt.getCurrentElement()->hasAttribute(attr_start.asXMLString());
      bool middleGiven = memoryRangeIt.getCurrentElement()->hasAttribute(attr_middle.asXMLString());
      bool endGiven    = memoryRangeIt.getCurrentElement()->hasAttribute(attr_end.asXMLString());
      bool sizeGiven   = memoryRangeIt.getCurrentElement()->hasAttribute(attr_size.asXMLString());

      DualString sMemoryStartAddress(memoryRangeIt.getCurrentElement()->getAttribute(attr_start.asXMLString()));
      DualString sMemoryMiddleAddress(memoryRangeIt.getCurrentElement()->getAttribute(attr_middle.asXMLString()));
      DualString sMemoryEndAddress(memoryRangeIt.getCurrentElement()->getAttribute(attr_end.asXMLString()));
      DualString sMemorySize(memoryRangeIt.getCurrentElement()->getAttribute(attr_size.asXMLString()));

      long memoryStartAddress;
      long memoryMiddleAddress;
      long memoryEndAddress;
      long memorySize;

      bool addressOK = true;
      if (startGiven && endGiven) {
         addressOK = strToULong(sMemoryStartAddress.asCString(), NULL, &memoryStartAddress) &&
               strToULong(sMemoryEndAddress.asCString(), NULL, &memoryEndAddress) &&
               !middleGiven && !sizeGiven && (memoryStartAddress <= memoryEndAddress);
      }
      else if (startGiven && sizeGiven) {
         addressOK = strToULong(sMemoryStartAddress.asCString(), NULL, &memoryStartAddress) &&
               strSizeToULong(sMemorySize.asCString(), NULL, &memorySize) &&
               !middleGiven && !endGiven && (memorySize > 0);
         memoryEndAddress = memoryStartAddress + memorySize - 1;
      }
      else if (middleGiven && sizeGiven) {
         addressOK = strToULong(sMemoryMiddleAddress.asCString(), NULL, &memoryMiddleAddress) &&
               strSizeToULong(sMemorySize.asCString(), NULL, &memorySize) &&
               (memorySize >= 2) && ((memorySize & 0x01) == 0);
         memoryStartAddress = memoryMiddleAddress - (memorySize/2);
         memoryEndAddress   = memoryMiddleAddress + (memorySize/2)-1;
      }
      else if (endGiven && sizeGiven) {
         addressOK = strToULong(sMemoryEndAddress.asCString(), NULL, &memoryEndAddress) &&
               strSizeToULong(sMemorySize.asCString(), NULL, &memorySize) &&
               !startGiven && !middleGiven && (memorySize > 0);
         memoryStartAddress = memoryEndAddress - (memorySize - 1);
      }
      if (!addressOK) {
         print("memoryRangeIt() startGiven=%s, middleGiven=%s, endGiven=%s, sizeGiven=%s, \n",
               startGiven?"Y":"N", middleGiven?"Y":"N", endGiven?"Y":"N", sizeGiven?"Y":"N");
         print("memoryRangeIt() memoryStartAddress=0x%X, memoryMiddleAddress=0x%X, memoryEndAddress=0x%X, memorySize=0x%X, \n",
               memoryStartAddress, memoryMiddleAddress, memoryEndAddress, memorySize);
         throw MyException("memoryRangeIt() - Illegal memory start/middle/end/size in memory region list");
      }
      //====================================================
      // Page numbers
      //
      bool pageResetGiven =  memoryRangeIt.getCurrentElement()->hasAttribute(attr_pageReset.asXMLString());
      bool pageNoGiven    =  memoryRangeIt.getCurrentElement()->hasAttribute(attr_pageNo.asXMLString());
      bool pageStartGiven =  memoryRangeIt.getCurrentElement()->hasAttribute(attr_pageStart.asXMLString());
      bool pageEndGiven   =  memoryRangeIt.getCurrentElement()->hasAttribute(attr_pageEnd.asXMLString());
      bool pagesGiven     =  memoryRangeIt.getCurrentElement()->hasAttribute(attr_pages.asXMLString());

      DualString sPageReset(memoryRangeIt.getCurrentElement()->getAttribute(attr_pageReset.asXMLString()));
      DualString sPageNo(memoryRangeIt.getCurrentElement()->getAttribute(attr_pageNo.asXMLString()));
      DualString sPageStart(memoryRangeIt.getCurrentElement()->getAttribute(attr_pageStart.asXMLString()));
      DualString sPageEnd(memoryRangeIt.getCurrentElement()->getAttribute(attr_pageEnd.asXMLString()));
      DualString sPages(memoryRangeIt.getCurrentElement()->getAttribute(attr_pages.asXMLString()));

      long pageStart;
      long pageEnd;
      long pages;
      bool pageOK = true;
      // If paged, assume page size is memory range
      const long pageSize = memoryEndAddress - memoryStartAddress +1;

      if (!pageNoGiven && !pageStartGiven && !pageEndGiven && !pagesGiven && !pageResetGiven) {
         pMemoryRegion->addRange(memoryStartAddress, memoryEndAddress);
         continue;
      }
      else if (pageNoGiven) {
         pageOK = pageOK &&
               strToULong(sPageNo.asCString(), NULL, &pageStart) &&
               !pageStartGiven && !pageEndGiven && !pagesGiven && !pageResetGiven;
         if (pageOK) {
            pMemoryRegion->addRange(memoryStartAddress, memoryEndAddress, pageStart);
            continue;
         }
      }
      else if (pageStartGiven && pageEndGiven) {
         pageOK = pageOK &&
               strToULong(sPageStart.asCString(), NULL, &pageStart) &&
               strToULong(sPageEnd.asCString(), NULL, &pageEnd) &&
               !pagesGiven;
         pages = pageEnd - pageStart + 1;
//         print("Start/End, pageOK=%s\n", pageOK?"True":"False");
      }
      else if (pageStartGiven && pagesGiven) {
         pageOK = pageOK &&
               strToULong(sPageStart.asCString(), NULL, &pageStart) &&
               strSizeToULong(sPages.asCString(), NULL, &pages) &&
               !pageEndGiven;
         if (pages >= 1024) {
            // Assume size of paged area rather than number of pages
            pageOK = pageOK && ((pages % pageSize) == 0);
            pages /= pageSize;
         }
         pageEnd = pageStart + pages - 1;
//         print("Start/Pages, pageOK=%s\n", pageOK?"True":"False");
      }
      else if (pageEndGiven && pagesGiven) {
         pageOK = pageOK &&
               strToULong(sPageEnd.asCString(), NULL, &pageEnd) &&
               strSizeToULong(sPages.asCString(), NULL, &pages) &&
               !pageStartGiven;
         if (pages >= 1024) {
            // Assume size of paged area rather than number of pages
            pageOK = pageOK && ((pages % pageSize) == 0);
            pages /= pageSize;
         }
         pageStart = pageEnd - pages + 1;
//         print("End/Pages, pageOK=%s\n", pageOK?"True":"False");
      }
      long pageReset;
      if (pageResetGiven) {
         pageOK = pageOK && strToULong(sPageReset.asCString(), NULL, &pageReset) && (pageReset >= pageStart) && (pageReset <= pageEnd);
         if (pageOK) {
            // This page is visible on reset (from reset PPAGE value)
            if (pageReset == 0) {
               // Ignore reset page zero since it will created as part of set anyway
               pageResetGiven = false;
            }
            else {
               pMemoryRegion->addRange(memoryStartAddress, memoryEndAddress, pageReset);
//            print("Adding reset page (0x%06X, 0x%06X, P:0x%02X) pageOK=%s\n",
//                  memoryStartAddress, memoryEndAddress, pageReset, pageOK?"True":"False");
            }
         }
      }
      if (!pageOK ||
          ((pageStart < 0) || (pageStart > 0xFF)) ||
          ((pageEnd < 0)   || (pageEnd > 0xFF)) ||
          (pageResetGiven && (pageStart <= 0x00) && ( 0x00 <= pageEnd)) ) {
         print("memoryRangeIt() pageStartGiven=%s, pagesGiven=%s, pageEndGiven=%s, pageNoGiven=%s, pageResetGiven=%s \n",
               pageStartGiven?"Y":"N", pagesGiven?"Y":"N", pageEndGiven?"Y":"N", pageNoGiven?"Y":"N", pageResetGiven?"Y":"N");
         print("memoryRangeIt() pageStart=0x%06X, pages=0x%06X, pageEnd=0x%06X\n",
               pageStart, pages, pageEnd);
         throw MyException("memoryRangeIt() - Illegal memory pageStart/pages/pageEnd/pageNo/pageReset in memory region list");
      }
      for (int pageNo = pageStart; pageNo <= pageEnd; pageNo++) {
         // These pages use default page no from 24-bit address
         uint32_t pageValue = (pageNo<<16) & 0xFF0000;
         uint32_t start = memoryStartAddress|pageValue;
         uint32_t end   = memoryEndAddress|pageValue;
         pMemoryRegion->addRange(start, end);
//         print("Adding page (0x%06X, 0x%06X)\n", start, end);
      }
      //      print("DeviceXmlParser::parseMemory((): Current device = %s, 0x%02X:[0x%06X, 0x%06X]\n", currentDeviceName, pageNo, memoryStartAddress, memoryEndAddress);
   }
   return pMemoryRegion;
}

//! Create device description from node
//!
//!      <!ELEMENT device ((clock?,(memory|memoryRef)+,
//!                       (soptAddress|copctlAddress)?,
//!                       sdidAddress?,sdidMask?,sdid+,
//!                       flashScripts?,
//!                       (tclScript|tclScriptRef)?,
//!                       (flashProgram|flashProgramRef)?,flashProgramData?,
//!                       (flexNVMInfo|flexNVMInfoRef)?,
//!                       note*))?>
//! @param currentProperty - Present position in XML parse
//!
//! @return == 0 - success\n
//!         != 0 - fail
//!
DeviceData *DeviceXmlParser::parseDevice(DOMElement *deviceEl) {

   // Default device characteristics
   // These are initialised from the default device (if any) in the XML
   static TclScriptPtr     defaultTCLScript;
   static FlashProgramPtr  defFlashProgram;
   static FlexNVMInfoPtr   defaultFlexNVMInfo;
   static uint32_t         defSDIDAddress = 0x00000000;
   static uint32_t         defSDIDMask    = 0x00000000;
#if (TARGET == HC12)||(TARGET == MC56F80xx)
   static uint32_t         defCOPCTLAddress = 0x00000000;
#else
   static uint32_t         defSOPTAddress = 0x00000000;
#endif

#if TARGET == HCS08
   defSOPTAddress       = 0x1802;
   defSDIDAddress       = 0x1806;
   defSDIDMask          = 0xFFF;
#elif TARGET == RS08
   defSDIDMask          = 0xFFF;
#elif TARGET == CFV1
   defSDIDMask          = 0xFFF;
#elif TARGET == CFVx
   defSDIDMask          = 0xFFC0;
   defSDIDAddress       = 0x4011000A;
#elif TARGET == HC12
   defCOPCTLAddress     = 0x003C;
   defSDIDAddress       = 0x001A;  // actually partid
   defSDIDMask          = 0xFFFF;
#elif TARGET == ARM
   defSDIDMask          = 0x00000FF0;
#elif TARGET == MC56F80xx
   defSDIDMask          = 0xFFFFFFFF; // SDID Not used
#endif

   // Create new device
   DeviceData *itDev = new DeviceData();

   // Set default values
#if (TARGET == HC12)||(TARGET == MC56F80xx)
   itDev->setCOPCTLAddress(defCOPCTLAddress);
#else
   itDev->setSOPTAddress(defSOPTAddress);
#endif

   itDev->setFlashScripts(defaultTCLScript);
   itDev->setFlashProgram(defFlashProgram);
   itDev->setSDIDAddress(defSDIDAddress);
   itDev->setSDIDMask(defSDIDMask);

   DOMChildIterator propertyIt(deviceEl);
   for (;
         propertyIt.getCurrentElement() != NULL;
         propertyIt.advanceElement()) {

      DOMElement *currentProperty = propertyIt.getCurrentElement();
      DualString propertyTag(currentProperty->getTagName());
      if (XMLString::equals(propertyTag.asXMLString(), tag_note.asXMLString())) {
         // <note>
//            DualString sNoteText(currentProperty->getTextContent());
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_clock.asXMLString())) {
         // <clock>
         DualString sClockType(currentProperty->getAttribute(attr_type.asXMLString()));
         DualString sClockRegisterAddress(currentProperty->getAttribute(attr_registerAddress.asXMLString()));
         long clockRegisterAddress;
         if (!strToULong(sClockRegisterAddress.asCString(), NULL, &clockRegisterAddress)) {
            throw MyException("DeviceXmlParser::parseDevice() - Illegal Clock register address");
         }
         itDev->setClockType(ClockTypes::getClockType(sClockType.asCString()));
         itDev->setClockAddress(clockRegisterAddress);
         itDev->setClockTrimNVAddress(itDev->getDefaultClockTrimNVAddress());
         itDev->setClockTrimFreq(itDev->getDefaultClockTrimFreq());
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_memory.asXMLString())) {
         // <memory>
         MemoryRegion *memoryRegion = parseMemory(currentProperty);
         if (memoryRegion != NULL) {
            MemoryRegionPtr pMemoryRegion(memoryRegion);
            itDev->addMemoryRegion(pMemoryRegion);
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_memoryRef.asXMLString())) {
         // <memoryRef>
         DualString sRef(currentProperty->getAttribute(attr_ref.asXMLString()));
         SharedInformationItemPtr pMemoryRegion(deviceDataBase->getSharedData(sRef.asCString()));
         if (!pMemoryRegion) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Unable to find reference - ")+sRef.asCString());
         }
         else {
            itDev->addMemoryRegion(std::tr1::dynamic_pointer_cast<MemoryRegion>(pMemoryRegion));
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_sdid.asXMLString())) {
         // <sdid>
         DualString sSdidValue(currentProperty->getAttribute(attr_value.asXMLString()));
         long sdidValue;
         if (!strToULong(sSdidValue.asCString(), NULL, &sdidValue)) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Illegal Clock SDID address ")+sSdidValue.asCString());
         }
         itDev->addSDID(sdidValue);
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_sdidAddress.asXMLString())) {
         // <sdidAddress>
         DualString sSdidAddress(currentProperty->getAttribute(attr_value.asXMLString()));
         long sdidAddress;
         if (!strToULong(sSdidAddress.asCString(), NULL, &sdidAddress)) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Illegal Clock SDID value ")+sSdidAddress.asCString());
         }
         itDev->setSDIDAddress(sdidAddress);
         if (isDefault) {
            print("DeviceXmlParser::parseDevice() - Setting default SDID Address 0x%06X \n", sdidAddress);
            defSDIDAddress = sdidAddress;
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_sdidMask.asXMLString())) {
         // <sdidMask>
         DualString sSdidMask(currentProperty->getAttribute(attr_value.asXMLString()));
         long sdidMask;
         if (!strToULong(sSdidMask.asCString(), NULL, &sdidMask)) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Illegal SDID mask ")+sSdidMask.asCString());
         }
         itDev->setSDIDMask(sdidMask);
         if (isDefault) {
            print("DeviceXmlParser::parseDevice() - Setting default SDID mask 0x%04X \n", sdidMask);
            defSDIDMask = sdidMask;
         }
      }
#if (TARGET == HC12)||(TARGET == MC56F80xx)
      else if (XMLString::equals(propertyTag.asXMLString(), tag_copctlAddress.asXMLString())) {
         // <copctlAddress>
         DualString sCopctlAddress(currentProperty->getAttribute(attr_value.asXMLString()));
         long copctlAddress=0;
         if (!strToULong(sCopctlAddress.asCString(), NULL, &copctlAddress)) {
            throw MyException("DeviceXmlParser::parseDevice() - Illegal COPCTL address");
         }
         itDev->setCOPCTLAddress(copctlAddress);
         if (isDefault)
            defCOPCTLAddress = copctlAddress;
      }
#else
      else if (XMLString::equals(propertyTag.asXMLString(), tag_soptAddress.asXMLString())) {
         // <soptAddress>
         DualString sSoptAddress(currentProperty->getAttribute(attr_value.asXMLString()));
         long soptAddress = 0x00;
         if (!strToULong(sSoptAddress.asCString(), NULL, &soptAddress)) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Illegal SPOT address ")+sSoptAddress.asCString());
         }
         itDev->setSOPTAddress(soptAddress);
         if (isDefault) {
            print("DeviceXmlParser::parseDevice() - Setting default SOPT Address 0x%06X \n", soptAddress);
            defSOPTAddress = soptAddress;
         }
      }
#endif
      else if (XMLString::equals(propertyTag.asXMLString(), tag_tclScriptRef.asXMLString())) {
         // <tclScriptRef>
         DualString sRef(currentProperty->getAttribute(attr_ref.asXMLString()));
         SharedInformationItemPtr pTclScript = deviceDataBase->getSharedData(sRef.asCString());
         if (pTclScript == NULL) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Unable to find reference - ")+sRef.asCString());
         }
         else {
            itDev->setFlashScripts(std::tr1::dynamic_pointer_cast<TclScript>(pTclScript));
            if (isDefault) {
               print("DeviceXmlParser::parseDevice() - Setting default flash script \"%s\" \n", sRef.asCString());
               defaultTCLScript = itDev->getFlashScripts();
            }
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_tclScript.asXMLString())) {
         // <tclScript>
         TclScript *tclScript = parseTCLScript(currentProperty);
         TclScriptPtr pTclScript(tclScript);
         itDev->setFlashScripts(pTclScript);
         if (isDefault) {
            defaultTCLScript = itDev->getFlashScripts();
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_flashScripts.asXMLString())) {
         // <flashScripts>
         TclScriptPtr pTclScript = parseSequence(currentProperty);
         if (pTclScript) {
            itDev->setFlashScripts(pTclScript);
            if (isDefault) {
               print("DeviceXmlParser::parseDevice() - Setting default flash script (non-shared)\n");
               defaultTCLScript = pTclScript;
            }
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_flashProgram.asXMLString())) {
         // <flashProgram>
         FlashProgram *flashProgram = parseFlashProgram(currentProperty);
         print("Flash program =\n%s", flashProgram->toString().c_str());
         FlashProgramPtr pFlashProgram(flashProgram);
         itDev->setFlashProgram(pFlashProgram);
         if (isDefault) {
            print("DeviceXmlParser::parseDevice() - Setting default flash program (non-shared)\n");
            defFlashProgram = itDev->getFlashProgram();
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_flashProgramRef.asXMLString())) {
         // <flashProgramRef>
         DualString sRef(currentProperty->getAttribute(attr_ref.asXMLString()));
         SharedInformationItemPtr pFlashProgram(deviceDataBase->getSharedData(sRef.asCString()));
         if (!pFlashProgram) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Unable to find reference - ")+sRef.asCString());
         }
         else {
            itDev->setFlashProgram(std::tr1::dynamic_pointer_cast<FlashProgram>(pFlashProgram));
            if (isDefault) {
               print("DeviceXmlParser::parseDevice() - Setting default flash program: \"%s\" \n", sRef.asCString());
               defFlashProgram = itDev->getFlashProgram();
            }
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_flexNvmInfo.asXMLString())) {
         // <flexNVMInfo>
         FlexNVMInfo *flexNVMInfo = parseFlexNVMInfo(currentProperty);
         print("DeviceXmlParser::parseDevice() - adding flexNVMInfo()\n");
         FlexNVMInfoPtr pFlexNVMInfo(flexNVMInfo);
         itDev->setflexNVMInfo(pFlexNVMInfo);
         if (isDefault) {
            print("DeviceXmlParser::parseDevice() - Setting default flexNVMInfo (non-shared)\n");
            defaultFlexNVMInfo = pFlexNVMInfo;
         }
      }
      else if (XMLString::equals(propertyTag.asXMLString(), tag_flexNvmInfoRef.asXMLString())) {
         // <flexNvmInfoRef>
         DualString sRef(currentProperty->getAttribute(attr_ref.asXMLString()));
         SharedInformationItemPtr pFlexNvmInfo(deviceDataBase->getSharedData(sRef.asCString()));
         if (!pFlexNvmInfo) {
            throw MyException(string("DeviceXmlParser::parseDevice() - Unable to find reference - ")+sRef.asCString());
         }
         else {
            //               print("DeviceXmlParser::parseDevice() - using shared flexNVMInfo()\n");
            itDev->setflexNVMInfo(std::tr1::dynamic_pointer_cast<FlexNVMInfo>(pFlexNvmInfo));
            if (isDefault) {
               print("DeviceXmlParser::parseDevice() - Setting default flash program: \"%s\" \n", sRef.asCString());
               defFlashProgram = itDev->getFlashProgram();
            }
         }
      }
      else {
         throw MyException(string("DeviceXmlParser::parseDevice() - Unknown tag - ")+propertyTag.asCString());
      }
   }
   return itDev;
}

//! Create device list from device nodes
//!
//!      <!ELEMENT device ((clock?,(memory|memoryRef)+,
//!                       (soptAddress|copctlAddress)?,
//!                       sdidAddress?,sdidMask?,sdid+,
//!                       flashScripts?,
//!                       (tclScript|tclScriptRef)?,
//!                       (flashProgram|flashProgramRef)?,flashProgramData?,
//!                       (flexNVMInfo|flexNVMInfoRef)?,
//!                       note*))?>
//!           <!ATTLIST device name ID #REQUIRED>
//!           <!ATTLIST device isDefault (true|false) #IMPLIED>
//!           <!ATTLIST device alias IDREF #IMPLIED>
//!           <!ATTLIST device hidden (true|false) #IMPLIED>
//!           <!ATTLIST device family (RS08|HCS08|HCS12|CFV1|CFV1Plus|CFVx|ARM|DSC) #IMPLIED>
//!           <!ATTLIST device subFamily CDATA #IMPLIED>
//!           <!ATTLIST device frequency CDATA #IMPLIED>
//!
//! @return == 0 - success\n
//!         != 0 - fail
//!
void DeviceXmlParser::parseDeviceXML(void) {
//   print("DeviceXmlParser::parseDeviceXML()\n");

   DOMChildIterator deviceIt(document, tag_device.asCString());
   if (deviceIt.getCurrentElement() == NULL) {
      throw MyException("DeviceXmlParser::parseDeviceXML() - Device file has no device tag");
   }
   // Process each <device> node
   for (;
         deviceIt.getCurrentElement() != NULL;
         deviceIt.advanceElement()) {

      isDefault = false; // Assume non-default device

      DOMElement *deviceEl = deviceIt.getCurrentElement();

      // Process <device> element attributes

#if defined FAMILY
#define MAKE_STRING(x) #x

      DualString familyValue(deviceEl->getAttribute(attr_family.asXMLString()));
      if (strcmp(FAMILY,familyValue.asCString()) != 0) {
//         print("Discarding %s not matching %s\n", (const char *)familyValue.asCString(), FAMILY);
         continue;
      }
#endif

      // Get device name. Note - Assumes ASCII string
      DualString targetName(deviceEl->getAttribute(attr_name.asXMLString()));
      strncpy(currentDeviceName, targetName.asCString(), sizeof(currentDeviceName));
      currentDeviceName[sizeof(currentDeviceName)-1] = '\0';
      strupr(currentDeviceName);
      if (strlen(currentDeviceName) == 0) {
         throw MyException(string("DeviceXmlParser::parseDeviceXML() - Device name missing or invalid"));
      }
      if (deviceEl->hasAttribute(attr_alias.asXMLString())) {
         // Alias device

         // Get real device name. Note - Assumes ASCII string
         DualString aliasValue(deviceEl->getAttribute(attr_alias.asXMLString()));
         char buff[50];
         strncpy(buff, aliasValue.asCString(), sizeof(buff));
         buff[sizeof(buff)-1] = '\0';
         strupr(buff);
         if (strlen(buff) == 0) {
            throw MyException(string("DeviceXmlParser::parseDeviceXML() - Alias name missing or invalid"));
         }
         DeviceData *itDev = parseDevice(deviceEl);
         itDev->setTargetName(currentDeviceName);
         itDev->setAliasName(buff);
         if (deviceEl->hasAttribute(attr_hidden.asXMLString())) {
            itDev->setHidden();
         }
         deviceDataBase->addDevice(itDev);
      }
      else {
         // Real device
         if (deviceEl->hasAttribute(attr_isDefault.asXMLString())) {
            DualString value(deviceEl->getAttribute(attr_isDefault.asXMLString()));
            isDefault = XMLString::equals(value.asCString(), "true");
         }
         DualString subFamilyValue(deviceEl->getAttribute(attr_subFamily.asXMLString()));
         DualString speedValue(deviceEl->getAttribute(attr_speed.asXMLString()));
         DeviceData *itDev = parseDevice(deviceEl);
         itDev->setTargetName(currentDeviceName);
         if (deviceEl->hasAttribute(attr_hidden.asXMLString())) {
            itDev->setHidden();
         }
         if (isDefault) {
            // Add device as default
            deviceDataBase->setDefaultDevice(itDev);
         }
         else {
            // Add general device
            deviceDataBase->addDevice(itDev);
         }
      }
   }
}

//! Load device file into device list
//!
//! @param deviceFilePath     - Path to XML file to load from
//! @param deviceDataBase   - Device database to update
//!
//! @return == 0 => success\n
//!         != 0 => fail
//!
void DeviceXmlParser::loadDeviceData(const std::string &deviceFilePath, DeviceDataBase *deviceDataBase) {

//   print("DeviceXmlParser::loadDeviceData()\n");
   try {
      xercesc::XMLPlatformUtils::Initialize();
   }
   catch (...) {
      throw MyException("DeviceXmlParser::loadDeviceData((): Exception in XMLPlatformUtils::Initialise()");
   }
   DeviceXmlParser* deviceXmlParser = new DeviceXmlParser(deviceDataBase);
   try {
      try {
         // Load the XML
         deviceXmlParser->loadFile(deviceFilePath);
         // Parse shared information
         deviceXmlParser->parseSharedXML();
         // Parse device information
         deviceXmlParser->parseDeviceXML();
      }
      catch (const xercesc::XMLException& exception) {
         DualString message(exception.getMessage());
         throw MyException(string("DeviceXmlParser::loadDeviceData() - XML Exception")+message.asCString());
      }
      catch (const xercesc::DOMException& exception) {
         DualString message(exception.getMessage());
         throw MyException(string("DeviceXmlParser::loadDeviceData() - DOM Exception")+message.asCString());
      }
      catch (MyException &exception) {
         throw MyException(string(exception.what())+"\n   Current Device = "+deviceXmlParser->currentDeviceName);
      }
      catch (std::runtime_error &exception) {
         throw MyException(string(exception.what())+"\n   Current Device = "+deviceXmlParser->currentDeviceName);
      }
   }
   catch (MyException &exception) {
      XMLPlatformUtils::Terminate();
      throw exception;
   }
   catch (...) {
      print("Unknown Exception\n");
      XMLPlatformUtils::Terminate();
      // Translate other exceptions
      throw MyException("DeviceXmlParser::loadDeviceData() - Exception in loadFile(), parseSharedXML() or parseDeviceXML()\n");
   }
   delete deviceXmlParser;

   XMLPlatformUtils::Terminate();
}
