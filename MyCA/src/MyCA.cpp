#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>

MSIHANDLE hInstall = 0;

static void printLog(MSIHANDLE hInstall, const char *format, ...) {
   UINT er;
   char buff[1000];
   va_list list;

   if (hInstall == 0)
      return;
   va_start(list, format);
   vsprintf(buff, format, list);
   va_end(list);

   PMSIHANDLE hRecord = ::MsiCreateRecord(1);
   if (hRecord) {
       er = ::MsiRecordSetString(hRecord, 0, TEXT(buff));
       if (er == ERROR_SUCCESS) {
           er = ::MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRecord);
       }
   }
}

static bool fileExists(const char *path) {
   struct stat stFileInfo;
   int intStat;
   bool found;

   intStat = stat(path, &stFileInfo);
   found = ((intStat == 0) && S_ISREG(stFileInfo.st_mode));

   printLog(hInstall, "fileExists(%s) => %s\n", path, found?"Found":"Not Found");
   return found;
}

static bool directoryExists(char *path) {
   struct stat stFileInfo;
   int intStat;
   bool found;

   intStat = stat(path, &stFileInfo);
   found = ((intStat == 0) && S_ISDIR(stFileInfo.st_mode));

   printLog(hInstall, "dirExists(%s) => %s\n", path, found?"Found":"Not Found");
   return found;
}

typedef struct  {
   const char *directory;    // Property containing the Feature install path
   const char *feature;      // Feature
   const char *subdir;       // Sub-directory to probe for
   const char *description;  // Description of feature to set for error message
} DirectoryQuad;

//! Check if feature paths are valid
//!
extern "C" UINT __stdcall CheckCodewarriorPaths(MSIHANDLE hInstall) {
   bool found = true;
   char path[MAX_PATH];
   DWORD filePathLength;
   DirectoryQuad pathsToCheck[] = {
         // directory                         feature                            subdir         description
         {"D.CW_FOR_MCU_V10_3",              "F.CW_FOR_MCU_V10_3",              "MCU",          "Codewarrior for MCU V10.3"},
         {"D.CW_FOR_MCU_V10_2",              "F.CW_FOR_MCU_V10_2",              "MCU",          "Codewarrior for MCU V10.2"},
         {"D.CW_FOR_MCU_V10_1",              "F.CW_FOR_MCU_V10_1",              "MCU",          "Codewarrior for MCU V10.1"},
         {"D.CODEWARRIOR_COLDFIRE_V7_2",     "F.CODEWARRIOR_COLDFIRE_V7_2",     "bin",          "Codewarrior for Coldfire V7.2"},
         {"D.CODEWARRIOR_COLDFIRE_V7_1",     "F.CODEWARRIOR_COLDFIRE_V7_1",     "bin",          "Codewarrior for Coldfire V7.1"},
         {"D.CODEWARRIOR_COLDFIRE_V7",       "F.CODEWARRIOR_COLDFIRE_V7",       "bin",          "Codewarrior for Coldfire V7.x"},
         {"D.CODEWARRIOR_DSC_V8_3",          "F.CODEWARRIOR_DSC_V8_3",          "bin",          "Codewarrior for DSC56800"},
         {"D.CW_FOR_MICROCONTROLLERS_V6_3",  "F.CW_FOR_MICROCONTROLLERS_V6_3",  "bin",          "Codewarrior for Microcontrollers V6.3"},
         {"D.CW_FOR_MICROCONTROLLERS_V6_2",  "F.CW_FOR_MICROCONTROLLERS_V6_2",  "bin",          "Codewarrior for Microcontrollers V6.2"},
         {"D.CW_FOR_MICROCONTROLLERS_V6",    "F.CW_FOR_MICROCONTROLLERS_V6",    "bin",          "Codewarrior for Microcontrollers V6.x"},
         {"D.CWS12_X_V5_1",                  "F.CWS12_X_V5_1",                  "bin",          "Codewarrior for S12(X)"},
         {"D.FREEMASTER_V1_3",               "F.FREEMASTER_V1_3",               "plugins",      "FreeMASTER V1.3"},
         {"D.CFFLASHER_V3_1",                "F.CFFLASHER_V3_1",                "BDM Protocol", "CF Flasher V3.1"},
         {"D.CODESOURCERY_CFV1",             "F.CODESOURCERY_CFV1",             "bin",          "Sourcery Codebench Lite for Coldfire ELF"},
         {"D.CODESOURCERY_KINETIS",          "F.CODESOURCERY_KINETIS",          "bin",          "Sourcery Codebench Lite for Kinetis EABI"},
         {"D.CODESOURCERY_ARM",              "F.CODESOURCERY_ARM",              "bin",          "Sourcery Codebench Lite for ARM EABI"},
         {NULL,                              NULL,                              NULL,      NULL}
   };
   ::hInstall = hInstall;
   printLog(hInstall,  "CheckCodewarriorPaths()\n");

   int pathNum;
   for (pathNum=0; pathsToCheck[pathNum].directory != NULL; pathNum++) {
      UINT rc;
      filePathLength = MAX_PATH;
      INSTALLSTATE installedState;
      INSTALLSTATE actionState;

      // Check if feature is being installed - otherwise no path check needed
      rc = MsiGetFeatureState(hInstall,pathsToCheck[pathNum].feature, &installedState, &actionState);
      if (actionState != INSTALLSTATE_LOCAL) {
         continue;
      }
      // Get path & validate
      rc = MsiGetProperty (hInstall, pathsToCheck[pathNum].directory, path, &filePathLength);
      if (rc != ERROR_SUCCESS) {
         return rc;
      }
      strcat(path, pathsToCheck[pathNum].subdir);
      found = directoryExists(path);
      printLog(hInstall, "CheckCodewarriorPaths(%s) => %s\n", path, found?"Found":"Not Found");
      if (!found) {
         MsiSetProperty (hInstall, "CODEWARRIOR_MISSING_PATH", path);
         MsiSetProperty (hInstall, "CODEWARRIOR_MISSING_APP",  pathsToCheck[pathNum].description);
         break;
      }
   }
   UINT rc = MsiSetProperty(hInstall, "CODEWARRIOR_PATHS_EXIST", found?"1":"0");
   return rc;
}

extern "C" UINT __stdcall FileExists(MSIHANDLE hInstall) {
   bool found;
   char path[MAX_PATH];
   DWORD filePathLength = MAX_PATH;

   ::hInstall = hInstall;

   MsiGetProperty (hInstall, "MYCA_PATH", path, &filePathLength);

   found = fileExists(path);
   MsiSetProperty (hInstall, "MYCA_EXISTS", found?"1":"0");

   printLog(hInstall, "FileExists(%s) => %s\n", path, found?"Found":"Not Found");
   return ERROR_SUCCESS;
}

extern "C" UINT __stdcall  DirectoryExists(MSIHANDLE hInstall) {
   bool found;

   ::hInstall = hInstall;

   char path[MAX_PATH];
   DWORD filePathLength = MAX_PATH;

   MsiGetProperty (hInstall, "MYCA_PATH", path, &filePathLength);

   // Attempt to get the subdir attributes
   found = directoryExists(path);
   MsiSetProperty (hInstall, "MYCA_EXISTS", found?"1":"0");

   printLog(hInstall, "DirectoryExists(%s) => %s\n", path, found?"Found":"Not Found");
   return ERROR_SUCCESS;
}
