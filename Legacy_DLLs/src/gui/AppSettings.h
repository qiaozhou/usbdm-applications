/*
 * AppSettings.h
 *
 *  Created on: 01/03/2010
 *      Author: podonoghue
 */
#ifndef APPSETTINGS_HPP_
#define APPSETTINGS_HPP_

#include <wx/wx.h>
#include <map>
#include "USBDM_API.h"

using namespace std;

class AppSettings {
private:
   enum ValueType {intType, stringType};
   //! Class to encapsulate the attribute & type
   class Value {
      private:
         ValueType type;
         int       intValue;
         wxString  stringValue;
      public:
         //! Create a integer attribute
         //!
         //! @param value - value to create
         Value(int value) {
            intValue = value;
            type     = intType;
         }
         //! Create a wxString attribute
         //!
         //! @param value - value to create
         Value(const wxString &value) {
            stringValue = value;
            type        = stringType;
         }
         //! Create a wxString attribute
         //!
         //! @param value - value to create
         Value(const char *value) {
            if (value == NULL)
               value = " ";
            stringValue = wxString(value, wxConvUTF8);
            type        = stringType;
         }
         //! Obtain the integer value
         //!
         int getIntValue() const {
            return intValue;
         }
         //! Obtain the wxString value
         //!
         const wxString &getStringValue() const {
            return stringValue;
         }
         //! Obtain the type of the attribute
         //!
         ValueType getType() const {
            return type;
         }
   };
   //! Container for key/attribute pairs
   map<wxString,Value *> mymap;

   void loadFromFile(FILE *fp);
   void writeToFile(FILE *fp, const wxString &comment) const;

public:
   //! Add a integer attribute
   //!
   //! @param key - key to use to save/retrieve the attribute
   //! @param value - value to save
   void addValue(wxString const &key, int value) {
      mymap[key] = new Value(value);
   }
   //! Add a wxString attribute
   //!
   //! @param key   - key to use to save the attribute
   //! @param value - value to save
   void addValue(wxString const &key, wxString const &value) {
      mymap[key] = new Value(value);
   }
   //! Add a wxString attribute
   //!
   //! @param key   - key to use to save the attribute
   //! @param value - value to save
   void addValue(wxString const &key, const char *value) {
      mymap[key] = new Value(value);
   }
   //! Retrieve an integer attribute
   //!
   //! @param key      - key to use to retrieve the attribute
   //! @param defValue - default value to use if not found
   int getValue(wxString const &key, int defValue) const {
      map<wxString,Value *>::const_iterator it;
      it = mymap.find(key);
      if (it != mymap.end())
         return it->second->getIntValue();
      else
         return defValue;
   }
   //! Retrieve an wxString attribute
   //!
   //! @param key      - key to use to retrieve the attribute
   //! @param defValue - default value to use if not found
   wxString getValue(wxString const &key, wxString const &defValue) const {
      map<wxString,Value *>::const_iterator it;
      it = mymap.find(key);
      if (it != mymap.end())
         return it->second->getStringValue();
      else
         return defValue;
   }
   static wxString getSettingsFilename(const wxString &rootFilename, TargetType_t targetType);

   bool loadFromFile(const wxString &fileName);
   bool writeToFile( const wxString &fileName, wxString const &comment = wxEmptyString) const;

   bool loadFromAppDirFile(const wxString &fileName);
   bool writeToAppDirFile( const wxString &fileName, const wxString &comment) const;

   void printToLog() const;
};

#endif /* APPSETTINGS_HPP_ */
