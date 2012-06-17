/*
 * xmlParser.h
 *
 *  Created on: 22/12/2010
 *      Author: podonoghue
 */

#ifndef XMLPARSER_H_
#define XMLPARSER_H_

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMComment.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>

#include "helper-classes.h"

class XmlParser {

private:
   DualString tag_extension;
   DualString tag_pages;
   DualString tag_page;
   DualString tag_uiElement;
   DualString tag_elementChoice;
   DualString tag_conditions;
   DualString tag_condition;
   DualString tag_conditionRef;
   DualString tag_or;
   DualString tag_wizard;
   DualString tag_projectGeneration;
   DualString tag_configurations;
   DualString tag_configuration;
   DualString tag_launchSettings;
   DualString tag_launchSettingsRef;
   DualString tag_configurationLaunchSettings;
   DualString tag_settingsGroup;
   DualString tag_settingsGroupRef;
   DualString attr_point;
   DualString attr_id;
   DualString attr_contentHelp;
   DualString attr_name;
   DualString attr_conditionId;
   DualString attr_uiExtensionID;
   DualString attr_defaultValue;
   DualString attr_merge_actions;

   DualString value_newProjectWizard;
   DualString value_wizardUI;
   DualString value_connections_s08;
   DualString value_connections_cf;
   DualString value_connections_list_s08;
   DualString value_connections_list_cf;
   DualString value_projectwizard_ui;

   xercesc_3_1::ErrorHandler*    errHandler;

   xercesc_3_1::XercesDOMParser* mergeParser;
   xercesc_3_1::DOMDocument*     mergeDocument;

   xercesc_3_1::XercesDOMParser* patchParser;
   xercesc_3_1::DOMDocument*     patchDocument;
   static bool      verbose;

private:
//   class MyResolver : public EntityResolver {
//
//   private:
//      static const XMLByte data[];
//      DualString id;
//
//   public:
//      MyResolver() :
//         id("XX")
//      {
//      }
//      InputSource *resolveEntity(const XMLCh*, const XMLCh*);
//};

private:
   XmlParser() :
      tag_extension("extension"),
      tag_pages("pages"),
      tag_page("page"),
      tag_uiElement("uiElement"),
      tag_elementChoice("elementChoice"),
      tag_conditions("conditions"),
      tag_condition("condition"),
      tag_conditionRef("conditionRef"),
      tag_or("or"),
      tag_wizard("wizard"),
      tag_projectGeneration("projectGeneration"),
      tag_configurations("configurations"),
      tag_configuration("configuration"),
      tag_launchSettings("launchSettings"),
      tag_launchSettingsRef("launchSettingsRef"),
      tag_configurationLaunchSettings("configurationLaunchSettings"),
      tag_settingsGroup("settingsGroup"),
      tag_settingsGroupRef("settingsGroupRef"),
      attr_point("point"),
      attr_id("id"),
      attr_contentHelp("contentHelp"),
      attr_name("name"),
      attr_conditionId("conditionId"),
      attr_uiExtensionID("uiExtensionID"),
      attr_defaultValue("defaultValue"),
      attr_merge_actions("merge-actions"),
      value_newProjectWizard("com.freescale.core.ide.newprojectwizard.newProjectWizard"),
      value_wizardUI("com.freescale.core.ide.wizard.ui.wizardUI"),
      value_connections_s08("mcu.projectwizard.page.connections.s08"),
      value_connections_cf("mcu.projectwizard.page.connections.cf"),
      value_connections_list_s08("mcu.projectwizard.connections.list.s08"),
      value_connections_list_cf("mcu.projectwizard.connections.list.cf"),
      value_projectwizard_ui("com.freescale.mcu.projectwizard.ui"),

      errHandler(NULL),

      mergeParser(NULL),
      mergeDocument(NULL),

      patchParser(NULL),
      patchDocument(NULL)
   {
   }

   ~XmlParser() {
//      cerr << "~XmlParser()\n";
      delete mergeParser;
      delete patchParser;
      delete errHandler;
   }

private:
   typedef enum {scan, insert, replace, mergeAttrs} Actions;

   int modifyNewProjectWizardXML(xercesc_3_1::DOMElement *el);
   int modifyWizardUIXML(xercesc_3_1::DOMElement *el);
   int modifyWizardUIXMLPage(xercesc_3_1::DOMElement *el);
   int modifyWizardUIXMLCondition(xercesc_3_1::DOMNode *el);
   int addLaunchSetting( xercesc_3_1::DOMElement *el,
                         const char *conditionId,
                         const char *configurationId,
                         const char *name,
                         const char *projectType);
   int addLaunchSettings(xercesc_3_1::DOMElement *el);
   int addLaunchSettingsRefs(xercesc_3_1::DOMElement *el);
   int addLaunchSettingsRef( xercesc_3_1::DOMElement *configuration,
                             const char *id);
   int addcheckBoxItem(xercesc_3_1::DOMElement *uiElement, const char *id );

   void  load(xercesc_3_1::XercesDOMParser* &parser, xercesc_3_1::DOMDocument* &document, const char* xmlFile);
   int   mergePatchfile();
   int   openSourcefile(const char *sourcePath);
   int   openPatchfile(const char *patchPath);
   int   commit(const char* xmlFile);
   bool mergeNodes(xercesc_3_1::DOMElement *mergeEl, xercesc_3_1::DOMElement *patchEl);
   bool  nodesMatch(xercesc_3_1::DOMElement *mergeEl, xercesc_3_1::DOMElement *patchEl);
   Actions getAction(xercesc_3_1::DOMElement *mergeEl);
   xercesc_3_1::DOMElement *removeActionAttributes(xercesc_3_1::DOMElement *patchEl);
   xercesc_3_1::DOMComment *getCommentNode(xercesc_3_1::DOMElement *element);

public:
   static int addUsbdmWizard(const char *sourcePath,
                             const char *destinationPath,
                             const char *patchPath);
};

#endif /* XMLPARSER_H_ */
