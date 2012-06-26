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

#include "Log.h"

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
   unsigned long value_t = strtoul(start, &end_t, 0);

//   print("strToULong() - s=\'%s\', e='%s', val=%ld(0x%lX)\n", start, end_t, value_t, value_t);
   if (end_t == start) { // no String found
      print("strToLong() - No number found\n");
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

   attr_name("name"),
   attr_family("family"),
   attr_isDefault("isDefault"),
   attr_type("type"),
   attr_subtype("subtype"),
   attr_start("start"),
   attr_end("end"),
   attr_size("size"),
   attr_pageNo("pageNo"),
   attr_pageAddress("pageAddress"),
   attr_registerAddress("registerAddress"),
   attr_securityAddress("securityAddress"),
   attr_sectorSize("sectorSize"),
   attr_value("value"),
   attr_id("id"),
   attr_ref("ref"),
   attr_addressMode("addressMode"),
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
      print("DeviceXmlParser::loadFile() - document failed to load/parse\n");
      throw MyException("DeviceXmlParser::loadFile() - Unable to create document");
   }
}

// Create a TCL script from XML element
TclScript *DeviceXmlParser::parseTCLScript(DOMElement *xmlTclScript) {
   // Type of node (must be a TCL script)
   DualString sTag (xmlTclScript->getTagName());
   if (!XMLString::equals(sTag.asXMLString(), tag_tclScript.asXMLString())) {
      print("DeviceXmlParser::parseTCLScript() unexpected node \'%s\'\n", sTag.asCString());
      throw MyException("DeviceXmlParser::parseTCLScript() self check failed");
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
      print("DeviceXmlParser::parseFlashProgram() unexpected node \'%s\'\n", sTag.asCString());
      throw MyException("DeviceXmlParser::parseFlashProgram() self check failed");
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
      print("DeviceXmlParser::parseSecurity() unexpected node \'%s\'\n", sTag.asCString());
      throw MyException("DeviceXmlParser::parseSecurity() self check failed");
   }
   long size = 0;
   DualString sSize(currentProperty->getAttribute(attr_size.asXMLString()));
   if (!strToULong(sSize.asCString(), NULL, &size)) {
      throw MyException("DeviceXmlParser::parseSecurity() - Illegal size in securityInfo");
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
      print("Illegal type in securityInfo\n");
      throw MyException("DeviceXmlParser::parseSecurity() self check failed");
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
   // Iterator for all the <sharedInformation> sections
   DOMChildIterator sharedIt(document, tag_sharedInformation.asCString());
   if (sharedIt.getCurrentElement() == NULL) {
      // No <sharedInformation> section
      return;
   }
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
//            print("DeviceXmlParser::parseSharedXML(): Inserted Shared TCL Script item ID=%s\n", sId.asCString());
         }
         else if (XMLString::equals(sTag.asXMLString(), tag_flashProgram.asXMLString())) {
            // Parse <flashProgram>
            FlashProgram *flashProgram = parseFlashProgram(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), flashProgram);
//            print("DeviceXmlParser::parseSharedXML(): Inserted Shared Flash program item ID=%s\n", sId.asCString());
         }
         else if (XMLString::equals(sTag.asXMLString(), tag_securityInfo.asXMLString())) {
            // Parse <securityInfo>
            SecurityInfo *securityInfo = parseSecurity(sharedInformationElement);
            deviceDataBase->addSharedData(string(sId.asCString()), securityInfo);
//            print("DeviceXmlParser::parseSharedXML(): Inserted Shared Security Information item ID=%s\n", sId.asCString());
            }
         else {
            print("DeviceXmlParser::parseSharedXML(): Unexpected Tag=<%s>\n", sTag.asCString());
            print("\n");
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
            print("DeviceXmlParser::parseSequence() - Unable to find suitable reference \'%s\'\n", sRef.asCString());
            return TclScriptPtr();
         }
         else {
//            print("DeviceXmlParser::parseSequence() -  tclScriptRef \'%s\'\n", sRef.asCString());
            return tclScript;
         }
      }
      else if (XMLString::equals(sequenceTag.asXMLString(), tag_tclScript.asXMLString())) {
         // <tclScript>
         TclScript *tclScript = parseTCLScript(sequenceElement);
         return TclScriptPtr(tclScript);
      }
      else {
         print("DeviceXmlParser::parseSequence() - Ignoring Unknown sequence type - %s\n", sequenceTag.asCString());
         return TclScriptPtr();
      }
   }
}

//! Create memory description from node for flash 'type' memory
//!
//! @param currentProperty - Present position in XML parse
//!
//! @return == 0 - success\n
//!         != 0 - fail
//!
MemoryRegion *DeviceXmlParser::parseFlashMemoryDetails(DOMElement *currentProperty, MemType_t memoryType, long &defaultSectorSize) {
   long ppageAddress    = 0;
   long registerAddress = 0;
   long securityAddress = 0;
   DualString sPpageAddress(currentProperty->getAttribute(attr_pageAddress.asXMLString()));
   DualString sRegisterAddress(currentProperty->getAttribute(attr_registerAddress.asXMLString()));
   DualString sSecurityAddress(currentProperty->getAttribute(attr_securityAddress.asXMLString()));
   if (!strToULong(sPpageAddress.asCString(),    NULL, &ppageAddress) ||
       !strToULong(sRegisterAddress.asCString(), NULL, &registerAddress) ||
       !strToULong(sSecurityAddress.asCString(), NULL, &securityAddress)) {
      throw MyException("DeviceXmlParser::parseMemory() - Illegal PPAGE/Register/Security field in Flash\n");
   }
   long sectorSize      = defaultSectorSize;
   DualString sSectorSize(currentProperty->getAttribute(attr_sectorSize.asXMLString()));
   if (XMLString::stringLen(sSectorSize.asXMLString()) != 0) {
      if (!strToULong(sSectorSize.asCString(), NULL, &sectorSize)) {
         throw MyException("DeviceXmlParser::parseMemory() - Illegal sectorSize in Flash");
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
         throw MyException("DeviceXmlParser::parseMemory(): Illegal addressMode\n");
      }
      if (isDefault) {
//            print("DeviceXmlParser::parseMemory() - Setting default Flash address mode = %s\n", addressMode==AddrLinear?"linear":"paged");
         defaultAddressMode = addressMode;
      }
   }
   MemoryRegion *pMemoryRegion = new MemoryRegion(memoryType, registerAddress, ppageAddress, securityAddress, sectorSize);
   pMemoryRegion->setAddressType(addressMode);
   if (isDefault) {
      defaultSectorSize = pMemoryRegion->getSectorSize();
   }
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
      static long defaultFlashSectorSize = 0;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemEEPROM, defaultFlashSectorSize);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("flash").asXMLString())) {
      // <flash>
      static long defaultFlashSectorSize = 0;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemFLASH, defaultFlashSectorSize);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("pROM").asXMLString())) {
      // <prom>
      static long defaultFlashSectorSize = 0;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemPROM, defaultFlashSectorSize);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("xROM").asXMLString())) {
      // <xrom>
      static long defaultFlashSectorSize = 0;
      pMemoryRegion = parseFlashMemoryDetails(currentProperty, MemXROM, defaultFlashSectorSize);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("ram").asXMLString())) {
      // <ram>
      pMemoryRegion = new MemoryRegion(MemRAM);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("rom").asXMLString())) {
      // <rom>
      pMemoryRegion = new MemoryRegion(MemROM);
   }
   else if (XMLString::equals(sMemoryType.asXMLString(), DualString("flexNVM").asXMLString())) {
      // <ram>
      pMemoryRegion = new MemoryRegion(MemFlexNVM);
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
         print("Unable to find reference \'%s\'\n", sRef.asCString());
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
         print("Unable to find reference \'%s\'\n", sRef.asCString());
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
   DOMChildIterator memoryRangeIt(currentProperty, tag_memoryRange.asCString());
   for (;
         memoryRangeIt.getCurrentElement() != NULL;
         memoryRangeIt.advanceElement()) {
      // <memoryRange>
      DualString sMemoryStartAddress(memoryRangeIt.getCurrentElement()->getAttribute(attr_start.asXMLString()));
      DualString sMemoryEndAddress(memoryRangeIt.getCurrentElement()->getAttribute(attr_end.asXMLString()));
      DualString sMemorySize(memoryRangeIt.getCurrentElement()->getAttribute(attr_size.asXMLString()));
      DualString sPageNo(memoryRangeIt.getCurrentElement()->getAttribute(attr_pageNo.asXMLString()));

      long memoryStartAddress;
      if (!strToULong(sMemoryStartAddress.asCString(), NULL, &memoryStartAddress)) {
         print("Current device = %s\n", DualString(currentDeviceName).asCString());
         throw MyException("memoryRangeIt() - Illegal memory start address in memory region list");
         return NULL;
      }
      long memoryEndAddress;
      long memorySize;
      if (strSizeToULong(sMemorySize.asCString(), NULL, &memorySize) &&
          (memorySize > 0)) {
         memoryEndAddress = memoryStartAddress + memorySize - 1;
      }
      else if (strToULong(sMemoryEndAddress.asCString(), NULL, &memoryEndAddress) &&
               (memoryEndAddress > memoryStartAddress)) {
         // done
      }
      else {
         print("Current device = %s\n", DualString(currentDeviceName).asCString());
         print("size=\'%s\'\n", sMemorySize.asCString());
         print("end =\'%s\'\n", sMemoryEndAddress.asCString());
         throw MyException("memoryRangeIt() - Illegal memory end/size in memory region list\n");
      }
      long pageNo;
      if (XMLString::stringLen(sPageNo.asXMLString()) == 0) {
         pageNo = MemoryRegion::DefaultPageNo;
      }
      else if (!strToULong(sPageNo.asCString(), NULL, &pageNo) ||(pageNo > 0xFF)) {
            print("Current device = %s\n", DualString(currentDeviceName).asCString());
            throw MyException("memoryRangeIt() - Illegal page Number in memory region list");
            return NULL;
      }
      pMemoryRegion->addRange(memoryStartAddress, memoryEndAddress, pageNo);
//      print("DeviceXmlParser::parseMemory((): Current device = %s, 0x%02X:[0x%06X, 0x%06X]\n", DualString(currentDeviceName).asCString(), pageNo, memoryStartAddress, memoryEndAddress);
   }
   return pMemoryRegion;
}

//! Create device list from xerces document
//!
//! @return == 0 - success\n
//!         != 0 - fail
//!
void DeviceXmlParser::parseDeviceXML(void) {
//   print("DeviceXmlParser::parseDeviceXML()\n");

   // Default device characteristics
   // These are initialised from the default device (if any) in the XML
   TclScriptPtr defaultTCLScript;
   FlashProgramPtr defFlashProgram;
   uint32_t   defSDIDAddress;
   uint32_t   defSDIDMask;
#if (TARGET == HC12)||(TARGET == MC56F80xx)
   uint32_t   defCOPCTLAddress;
#else
   uint32_t   defSOPTAddress;
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
   defSDIDAddress       = 0x00; // Not used
   defSDIDMask          = 0xFFFFFFFF;
#endif

   DOMChildIterator deviceIt(document, tag_device.asCString());
   if (deviceIt.getCurrentElement() == NULL) {
      throw MyException("DeviceXmlParser::parseDeviceXML() - Device file has no device tag");
   }
   // Process each <device> element
   for (;
         deviceIt.getCurrentElement() != NULL;
         deviceIt.advanceElement()) {

      DOMElement *deviceEl = deviceIt.getCurrentElement();

      if (!XMLString::equals(deviceEl->getTagName(), tag_device.asXMLString())) {
         print("DeviceXmlParser::parseDeviceXML(): Unexpected element %s\n", DualString(deviceEl->getTagName()).asCString());
         throw MyException("DeviceXmlParser::parseDeviceXML() - <device> tag expected");
      }
      currentDeviceName = deviceEl->getAttribute(attr_name.asXMLString());
      DualString targetName(currentDeviceName);
      DualString familyValue(deviceEl->getAttribute(attr_family.asXMLString()));
      isDefault = false;
      if (deviceEl->hasAttribute(attr_isDefault.asXMLString())) {
         DualString value(deviceEl->getAttribute(attr_isDefault.asXMLString()));
         isDefault = XMLString::equals(value.asCString(), "true");
      }

//      cerr << "Element: <device";
//      cerr << " name=" << targetName;
//      cerr << ", family=" << familyValue;
//      cerr << ">\n";

      // Create new device
      DeviceData *itDev = new DeviceData();

      // Note - Assumes ASCII string
      char buff[50];
      strncpy(buff, targetName.asCString(), sizeof(buff));
      buff[sizeof(buff)-1] = '\0';
      strupr(buff);
      itDev->setTargetName(buff);

      // Tags to handle
      //  <!ELEMENT device (clock?,
      //                    memory+,soptAddress?,
      //                    sdidAddress?,sdidMask?,sdid+,
      //                    securityAddress?,
      //                    (foptAddress?|fsecAddress?),
      //                    copctlAddress?,
      //                    flashScripts?,
      //                    (flashProgram|flashProgramRef)?,flashProgramData?,note*)> clock;

#if TARGET == HCS08
      itDev->setSOPTAddress(defSOPTAddress);
      itDev->setSDIDAddress(defSDIDAddress);
      itDev->setSDIDMask(defSDIDMask);
#elif TARGET == RS08
      itDev->setSDIDMask(defSDIDMask);
#elif TARGET == CFV1
      itDev->setSDIDMask(defSDIDMask);
#elif TARGET == CFVx
      itDev->setSDIDAddress(defSDIDAddress);
      itDev->setSDIDMask(defSDIDMask);
#elif TARGET == HC12
      itDev->setCOPCTLAddress(defCOPCTLAddress);
      itDev->setSDIDAddress(defSDIDAddress);
      itDev->setSDIDMask(defSDIDMask);
#elif TARGET == ARM
      itDev->setSDIDMask(defSDIDMask);
#elif TARGET == MC56F80xx
      itDev->setSDIDAddress(defSDIDAddress);
      itDev->setSDIDMask(defSDIDMask);
#endif

      itDev->setFlashScripts(defaultTCLScript);
      itDev->setFlashProgram(defFlashProgram);
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
               throw MyException("DeviceXmlParser::parseDeviceXML() - Illegal Clock register address");
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
//               if (!itDev->getFlashProgram()) {
//                  // Save as default Flash program if not explicitly set (yet)
//                  itDev->setFlashProgram(pMemoryRegion->getFlashprogram());
//               }
            }
         }
         else if (XMLString::equals(propertyTag.asXMLString(), tag_sdid.asXMLString())) {
            // <sdid>
            DualString sSdidValue(currentProperty->getAttribute(attr_value.asXMLString()));
            long sdidValue;
            if (!strToULong(sSdidValue.asCString(), NULL, &sdidValue)) {
               throw MyException("DeviceXmlParser::parseDeviceXML() - Illegal Clock SDID address \'%s\'\n");
            }
            itDev->addSDID(sdidValue);
         }
         else if (XMLString::equals(propertyTag.asXMLString(), tag_sdidAddress.asXMLString())) {
            // <sdidAddress>
            DualString sSdidAddress(currentProperty->getAttribute(attr_value.asXMLString()));
            uint32_t sdidAddress  = strtoul(sSdidAddress.asCString(), NULL, 16);
            itDev->setSDIDAddress(sdidAddress);
            if (isDefault) {
               defSDIDAddress = sdidAddress;
            }
         }
         else if (XMLString::equals(propertyTag.asXMLString(), tag_sdidMask.asXMLString())) {
            // <sdidMask>
            DualString sSdidMask(currentProperty->getAttribute(attr_value.asXMLString()));
            long sdidMask;
            if (!strToULong(sSdidMask.asCString(), NULL, &sdidMask)) {
               throw MyException("DeviceXmlParser::parseDeviceXML() - Illegal SDID mask");
            }
            itDev->setSDIDMask(sdidMask);
            if (isDefault) {
               defSDIDMask = sdidMask;
            }
         }
#if (TARGET == HC12)||(TARGET == MC56F80xx)
         else if (XMLString::equals(propertyTag.asXMLString(), tag_copctlAddress.asXMLString())) {
            // <copctlAddress>
            DualString sCopctlAddress(currentProperty->getAttribute(attr_value.asXMLString()));
            long copctlAddress=0;
            if (!strToULong(sCopctlAddress.asCString(), NULL, &copctlAddress)) {
               throw MyException("DeviceXmlParser::parseDeviceXML() - Illegal COPCTL address");
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
               print("Illegal SOPT address \'%s\'\n", sSoptAddress.asCString());
               continue;
            }
            itDev->setSOPTAddress(soptAddress);
            if (isDefault) {
               defSOPTAddress = soptAddress;
            }
         }
#endif
         else if (XMLString::equals(propertyTag.asXMLString(), tag_flashScripts.asXMLString())) {
            // <flashScripts>
            TclScriptPtr pTclScript = parseSequence(currentProperty);
            if (pTclScript) {
               itDev->setFlashScripts(pTclScript);
               if (isDefault) {
                  defaultTCLScript = pTclScript;
               }
            }
         }
         else if (XMLString::equals(propertyTag.asXMLString(), tag_flashProgramRef.asXMLString())) {
            // <flashProgramRef>
            DualString sRef(currentProperty->getAttribute(attr_ref.asXMLString()));
            SharedInformationItemPtr pFlashProgram(deviceDataBase->getSharedData(sRef.asCString()));
            if (!pFlashProgram) {
               print("Unable to find reference \'%s\'\n", sRef.asCString());
            }
            else {
               itDev->setFlashProgram(std::tr1::dynamic_pointer_cast<FlashProgram>(pFlashProgram));
               if (isDefault) {
//                  print("DeviceXmlParser::parseDeviceXML() - Setting default flash program (shared reference)\"%s\" \n", sRef.asCString());
                  defFlashProgram = itDev->getFlashProgram();
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
//               print("DeviceXmlParser::parseDeviceXML() - Setting default flash program (non-shared)\n");
               defFlashProgram = itDev->getFlashProgram();
            }
         }
         else {
            print("Ignoring Unknown tag \'%s\'\n", propertyTag.asCString());
         }
      }
      if (itDev->getTargetName().compare("_DEFAULT") == 0) {
         // Add device as default
         deviceDataBase->setDefaultDevice(itDev);
      }
      else {
         // Add general device
         deviceDataBase->addDevice(itDev);
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
   bool exceptionHandled = false;
   try {
      xercesc::XMLPlatformUtils::Initialize();
   }
//   catch (const XMLException& toCatch) {
//      char* message = XMLString::transcode(toCatch.getMessage());
//      XMLString::release(&message);
//      throw MyException("Error during XML Initialisation");
//   }
   catch (...) {
      throw MyException("DeviceXmlParser::loadDeviceData((): Exception in XMLPlatformUtils::Initialise()");
   }
//   static volatile xercesc_3_1::MemoryManager* xx = xercesc_3_1::XMLPlatformUtils::fgMemoryManager;
//   const char *cString_   = xercesc::XMLString::replicate( "Hello" );
//   DualString s("Hello");
   DeviceXmlParser* deviceXmlParser = new DeviceXmlParser(deviceDataBase);
   try {
      // Load the XML
      deviceXmlParser->loadFile(deviceFilePath);
      // Parse shared information
      deviceXmlParser->parseSharedXML();
      // Parse device information
      deviceXmlParser->parseDeviceXML();
   }
   catch (const xercesc::XMLException& toCatch) {
#ifdef LOG
      DualString message(toCatch.getMessage());
      print("DeviceXmlParser::loadDeviceData() - Exception, reason = %s\n", message.asCString());
#endif
      exceptionHandled = true;
      throw MyException("DeviceXmlParser::loadDeviceData() - XML Exception");
   }
   catch (const xercesc::DOMException& toCatch) {
#ifdef LOG
      DualString message(toCatch.getMessage());
      print("DeviceXmlParser::loadDeviceData() - Exception, reason = %s\n", message.asCString());
#endif
      exceptionHandled = true;
      throw MyException("DeviceXmlParser::loadDeviceData() - DOM Exception");
   }
   catch (std::runtime_error &exception) {
      print("DeviceXmlParser::loadDeviceData() - Exception, reason = %s\n", exception.what());
      exceptionHandled = true;
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
