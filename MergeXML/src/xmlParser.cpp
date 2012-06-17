// xmlParser.cpp
//
// Modifies the Codewarrior New project Wizard xml file to add USBM entries
//

#include <iostream>
using namespace std;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMComment.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

XERCES_CPP_NAMESPACE_USE

#include "helper-classes.h"
#include "xmlParser.h"

//const XMLByte XmlParser::MyResolver::data[] = "SHORTCUTS;    ";
bool    XmlParser::verbose = false;

class DOMChildIterator {

private:
   DOMNodeList *elementList;
   int          index;
   DOMElement  *currentElement;

public:
   DOMChildIterator(DOMElement *parent) {

      elementList    = parent->getChildNodes();
      index          = 0;
      findElement(0);
   };
   DOMChildIterator(DOMDocument *document, const char *tagName) {
      elementList    = document->getElementsByTagName(DualString(tagName).asXMLString());
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
            throw new runtime_error("DOMChildIterator::setIndex() - elementList->item(i) failed");
         }
         if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMElement *el = dynamic_cast< xercesc::DOMElement* >( node );
            if (el == NULL) {
               // Should be impossible
               throw new runtime_error("DOMChildIterator::setIndex() - casting failed");
            }
            currentElement = el;
            this->index = index;
            break;
         }
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

//InputSource *XmlParser::MyResolver::resolveEntity(const XMLCh*, const XMLCh* systemId) {
//   if (XMLString::compareString(systemId, DualString("SHORTCUTS").asXMLString())) {
//      cerr << "Found SHORTCUTS\n";
//      return new MemBufInputSource(data, 12, id.asXMLString(), false);
//   } else {
//      return NULL;
//   }
//}

void XmlParser::load(XercesDOMParser* &parser, DOMDocument* &document, const char* xmlFile) {

   parser = new XercesDOMParser();
   parser->setValidationScheme(XercesDOMParser::Val_Never);
   parser->setCreateEntityReferenceNodes(true);
//   parser->setExternalParameterEntities(false);
   parser->setDoNamespaces(false);
   parser->setDoSchema( false ) ;
   parser->setLoadExternalDTD( false ) ;
//   parser->setEntityResolver(new MyResolver());

   if (errHandler == NULL)
      errHandler = new HandlerBase();
   parser->setErrorHandler(errHandler);
   try {
       parser->parse(xmlFile);
   }
   catch (const XMLException& toCatch) {
       char* message = xercesc::XMLString::transcode(toCatch.getMessage());
       cout << "Exception message is: \n"
            << message << "\n";
       throw new runtime_error("XML Exception");
   }
   catch (const xercesc::DOMException& toCatch) {
       char* message = xercesc::XMLString::transcode(toCatch.msg);
       cout << "Exception message is: \n"
            << message << "\n";
       xercesc::XMLString::release(&message);
       throw new runtime_error("DOM Exception");
   }
   catch (...) {
       cout << "Unexpected Exception \n" ;
       throw new runtime_error("Unexpected Exception");
   }
   document = parser->getDocument();
   if (document == NULL) {
      cerr << "parser->getDocument() failed";
      throw new runtime_error("Unable to create document");
   }
}

DOMComment *XmlParser::getCommentNode(DOMElement *element) {
   DOMNode *node = element;
   do {
      node = node->getPreviousSibling();
      if (node == NULL) {
//         cerr << "getCommentNode() - no prior comment sibling\n";
         return NULL;
      }
   } while (node->getNodeType() == DOMNode::TEXT_NODE);

   DOMComment *el = NULL;
   if (node->getNodeType() == DOMNode::COMMENT_NODE) {
      el = dynamic_cast< xercesc::DOMComment* >( node );
      if (el == NULL) {
         cerr << "getCommentNode() - cast failed\n";
      }
   }
   else {
//      cerr << "getCommentNode() - no comment found - type :" << node->getNodeType() << endl;
   }
   return el;
}

bool XmlParser::nodesMatch(DOMElement *mergeEl, DOMElement *patchEl) {

   DualString mergeTagName(mergeEl->getTagName());
   DualString patchTagName(patchEl->getTagName());
   if (verbose)
      cerr << "Comparing " << mergeTagName.asCString() << " to " << patchTagName.asCString();
   if (!XMLString::equals(mergeTagName.asXMLString(), patchTagName.asXMLString())) {
      if (verbose)
         cerr << " - false\n";
      return false;
   }
   if (verbose)
      cerr << " - equal\n";

   DOMNamedNodeMap *patchAttributes = patchEl->getAttributes();

   unsigned attributeCount = patchAttributes->getLength();
   for (unsigned attrIndex = 0; attrIndex < attributeCount; attrIndex++) {
      DOMNode *attribute = patchAttributes->item(attrIndex);
      DualString attributeName(attribute->getNodeName());
      if (XMLString::equals(attributeName.asXMLString(), attr_merge_actions.asXMLString())) {
         if (verbose)
            cerr << "Skipping attribute " << attributeName.asCString();
         continue;
      }
      if (verbose)
         cerr << "Checking for attribute " << attributeName.asCString();
      if (!mergeEl->hasAttribute(attributeName.asXMLString())) {
         if (verbose)
            cerr << " - Not present\n";
         return false;
      }
      if (verbose)
         cerr << " - Present";
      DualString patchAttributeValue(attribute->getNodeValue());
      DualString mergeAttributeValue(mergeEl->getAttribute(attributeName.asXMLString()));
      if (!XMLString::equals(patchAttributeValue.asXMLString(), mergeAttributeValue.asXMLString())) {
         if (verbose)
            cerr << " - Not equal\n";
         return false;
      }
      if (verbose)
         cerr << " - Equal (" << patchAttributeValue.asCString() << ")\n";
   }
   return true;
}

XmlParser::Actions XmlParser::getAction(DOMElement *patchEl) {
   Actions currentAction = scan;

   if (patchEl->hasAttribute(attr_merge_actions.asXMLString())) {
      DualString actionAttributeValue(patchEl->getAttribute(attr_merge_actions.asXMLString()));
      if (XMLString::equalsN(actionAttributeValue.asXMLString(), DualString("replace").asXMLString(), 7)) {
         currentAction = replace;
      }
      else if (XMLString::equalsN(actionAttributeValue.asXMLString(), DualString("insert").asXMLString(), 6)) {
         currentAction = insert;
      }
      else if (XMLString::equalsN(actionAttributeValue.asXMLString(), DualString("mergeAttrs").asXMLString(), 6)) {
         currentAction = mergeAttrs;
      }
      else if (XMLString::equalsN(actionAttributeValue.asXMLString(), DualString("scan").asXMLString(), 4)) {
         currentAction = scan;
      }
   }
   return currentAction;
}

DOMElement *XmlParser::removeActionAttributes(DOMElement *element) {

   DOMChildIterator elementIter(element);
   for(;;) {
      DOMElement *childElement = elementIter.getCurrentElement();
      if (childElement == NULL)
         break;
      removeActionAttributes(childElement);
      elementIter.advanceElement();
   }
   while (element->hasAttribute(attr_merge_actions.asXMLString())) {
      DualString actionAttributeValue(element->getAttribute(attr_merge_actions.asXMLString()));
      element->removeAttribute(attr_merge_actions.asXMLString());
      string attributeValue(actionAttributeValue.asCString());
      unsigned index = 0;
      for (;;) {
         if ((index = attributeValue.find("add-attr:", index)) != string::npos) {
         }
         else if ((index = attributeValue.find("set-attr:", index)) != string::npos) {
         }
         else if ((index = attributeValue.find("del-attr:", index)) != string::npos) {
         }
         else
            break;
         unsigned colonIndex     = attributeValue.find(':',index);
         if (colonIndex == string::npos)
            throw invalid_argument("Illegal merge-action");

         unsigned equalIndex     = attributeValue.find('=',colonIndex);
         unsigned semiColonIndex = attributeValue.find(';',colonIndex);

         if (semiColonIndex == string::npos)
            semiColonIndex = attributeValue.length();

         if (equalIndex > semiColonIndex) {
            equalIndex = semiColonIndex;
         }

         if (equalIndex <= colonIndex+1)
            throw invalid_argument("Illegal merge-action attribute name");
         string attrName(attributeValue.substr(colonIndex+1, equalIndex-(colonIndex+1)));

         string attrValue;
         if (equalIndex == semiColonIndex-1)
            throw invalid_argument("Illegal merge-action attribute value");
         else if (equalIndex == semiColonIndex)
            attrValue = "";
         else
            attrValue = attributeValue.substr(equalIndex+1, semiColonIndex-(equalIndex+1));
         if (verbose)
            cerr << "Adding attribute " << attrName << "=\"" << attrValue <<"\"\n";
         element->setAttribute(DualString(attrName.c_str()).asXMLString(), DualString(attrValue.c_str()).asXMLString());
         index = semiColonIndex-1;
      }
   }
   return element;
}

bool XmlParser::mergeNodes(DOMElement *mergeEl, DOMElement *patchEl) {

//   cerr << "mergeNodes()\n";

   if (!nodesMatch(mergeEl, patchEl)) {
//      cerr << "mergeNodes() - nodes don't match\n";
      return false;
   }
   DOMChildIterator patchIter(patchEl);
   if (patchIter.getCurrentElement() == NULL) {
      // No more children
//      cerr << "mergeNodes() - nodes match\n";
      return true;
   }
   DOMChildIterator mergeIter(mergeEl);
   if (mergeIter.getCurrentElement() == NULL) {
      // Missing required children
//      cerr << "mergeNodes() - No children in merge node\n";
      return false;
   }
   // Match each child
   for(;;) {
      DOMElement *mergeChildEl = mergeIter.getCurrentElement();
      DOMElement *patchChildEl = patchIter.getCurrentElement();

      if (patchChildEl == NULL) {
         // No more required children
         if (verbose)
            cerr << "mergeNodes() - Patch list end\n";
         return true;
      }
      Actions currentAction = getAction(patchChildEl);
      if (mergeChildEl == NULL) {
         // Reached end of existing children
         if (currentAction == insert) {
            // We've tried matching all the children so we have to insert
            DualString newNodeName(patchChildEl->getNodeName());
            if (verbose)
               cerr << "mergeNodes() - Appending node <" << newNodeName.asCString() << ">\n";
            DOMComment *commentNode = getCommentNode(patchChildEl);
            if (commentNode != NULL) {
               DOMNode *copyOfNode;
               copyOfNode = mergeDocument->importNode(commentNode, true);
               mergeEl->insertBefore(copyOfNode, mergeChildEl);
               copyOfNode = mergeDocument->createTextNode(DualString("\n").asXMLString());
               mergeEl->insertBefore(copyOfNode, mergeChildEl);
            }
            DOMElement *copyOfNode = dynamic_cast< DOMElement* >(mergeDocument->importNode(patchChildEl, true));
            mergeEl->insertBefore(removeActionAttributes(copyOfNode), mergeChildEl);
            patchIter.advanceElement();
            continue;
         }
         else
            return false;
      }
      if (mergeNodes(mergeChildEl, patchChildEl)) {
         if (currentAction == replace) {
            // We're replacing this node
            DualString newNodeName(patchChildEl->getNodeName());
            if (verbose)
               cerr << "mergeNodes() - Replacing node <" << newNodeName.asCString() << ">\n";
            DOMComment *commentNode = getCommentNode(patchChildEl);
            if (commentNode != NULL) {
               DOMNode *copyOfNode;
               copyOfNode = mergeDocument->importNode(commentNode, true);
               mergeEl->insertBefore(copyOfNode, mergeChildEl);
               copyOfNode = mergeDocument->createTextNode(DualString("\n").asXMLString());
               mergeEl->insertBefore(copyOfNode, mergeChildEl);
            }
            DOMElement *copyOfNode = dynamic_cast< DOMElement* >(mergeDocument->importNode(patchChildEl, true));
            mergeEl->replaceChild(removeActionAttributes(copyOfNode), mergeChildEl);
            patchIter.advanceElement();
            continue;
         }
         // Successfully merged/matched patch node - advance patch
//         cerr << "mergeNodes() - Advancing patch\n";
         patchIter.advanceElement();
      }
      // Advance original only
//      cerr << "mergeNodes() - Advancing merge\n";
      mergeIter.advanceElement();
   }
   return 0;
}

int XmlParser::mergePatchfile() {

//   cout << "mergePatchfile()\n";

   DOMElement *mergeRoot = mergeDocument->getDocumentElement();
   if (mergeRoot == NULL) {
      cerr << "mergePatchfile() - No merge root";
      return -1;
   }
   DOMElement *patchRoot = patchDocument->getDocumentElement();
   if (patchRoot == NULL) {
      cerr << "mergePatchfile() - No patch root";
      return -1;
   }

   mergeNodes(mergeRoot, patchRoot);
   if (verbose)
      cout << "mergePatchfile() - Completed patching XML file \n";
   return 0;
}

int XmlParser::commit(const char* xmlFile) {
   try {
      // Obtain DOM implementation supporting Load/Save
      DOMImplementationLS* pImplementation = dynamic_cast<DOMImplementationLS *>(DOMImplementationRegistry::getDOMImplementation(DualString("LS").asXMLString()));
      if (pImplementation == NULL){
         throw( std::runtime_error( "Unable to obtain suitable DOMImplementation!" ) ) ;
      }

      DOMLSSerializer *pSerializer = pImplementation->createLSSerializer();

      DOMLSOutput *pOutput = pImplementation->createLSOutput();
#if 1
      // Change output format to be pretty (but it isn't)
      DOMConfiguration *pConfiguration = pSerializer->getDomConfig();
      if (pConfiguration->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
         pConfiguration->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
#if 0
      // Overrides above but seems to have little effect!
      if (pConfiguration->canSetParameter(XMLUni::fgDOMWRTCanonicalForm, true))
         pConfiguration->setParameter(XMLUni::fgDOMWRTCanonicalForm, true);
#endif
#if 1
      //
      if (pConfiguration->canSetParameter(XMLUni::fgDOMWRTEntities, true))
         pConfiguration->setParameter(XMLUni::fgDOMWRTEntities, true);
#endif
#endif
      LocalFileFormatTarget *pTarget = new LocalFileFormatTarget(DualString( xmlFile ).asXMLString());
      pOutput->setByteStream(pTarget);

//      mergeDocument->normalizeDocument(); // Needed?
      pSerializer->write(mergeDocument, pOutput);

      delete pTarget;
      pOutput->release();
      pSerializer->release();

   } catch( const xercesc::XMLException& e ){
      return -1;
   }

   return 0;
}

int XmlParser::openSourcefile(const char *sourcePath) {
   load(mergeParser, mergeDocument, sourcePath);
   return 0;
}

int XmlParser::openPatchfile(const char *patchPath) {
   load(patchParser, patchDocument, patchPath);
   return 0;
}

//!======================================================================
//! @param sourcePath
//! @param destinationPath
//! @param patchPath
//!
//! @return 0 => OK
//!
int XmlParser::addUsbdmWizard(const char *sourcePath, const char *destinationPath, const char *patchPath) {
int rc = 0;

   if (verbose) {
      cout << "Applying patches: " << patchPath <<"\n"
              " <= " << sourcePath << "\n"
              " => " << destinationPath << "\n";
   }
   try {
      XMLPlatformUtils::Initialize();
   }
   catch (const XMLException& toCatch) {
      char* message = XMLString::transcode(toCatch.getMessage());
      cerr << "Error during XML Initialisation! :\n"
           << message << "\n";
      XMLString::release(&message);
      return -1;
   }

   try {
      XmlParser parser;
      if (verbose)
         cerr << "Opening source: '" << sourcePath << "'" << endl;
      parser.openSourcefile(sourcePath);
      if (verbose)
         cerr << "Opening patch: '" << patchPath << "'" << endl;
      parser.openPatchfile(patchPath);
      if (verbose)
         cerr << "Parsing XML file\n";
      parser.mergePatchfile();
      if (verbose)
         cerr << "Saving result: '" << destinationPath << "'" << endl;
      parser.commit(destinationPath);
   }
   catch (runtime_error *ex) {
      cerr << "Exception while parsing, reason: " << ex->what();
      rc = -1;
   }

   try {
      xercesc::XMLPlatformUtils::Terminate();
   }
   catch( xercesc::XMLException& e ){

      cerr<< " in catch()\n";
      char* message = xercesc::XMLString::transcode( e.getMessage() ) ;

      cerr << "XML toolkit tear-down error: "
           << message
           << std::endl;
      XMLString::release( &message ) ;
      rc = -1;
   }
   if (rc == 0)
      cout << "OK\n";
   return rc;
}

