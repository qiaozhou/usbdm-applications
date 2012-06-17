/*
 * Application.h
 *
 *  Created on: 15/11/2009
 *      Author: Peter
 */

#ifndef _APPLICATIONFILES_H_
#define _APPLICATIONFILES_H_

#include "wx/wx.h"

extern FILE *openApplicationFile(const wxString &filename, const wxString &attributes);
extern int checkExistsApplicationFile(const wxString &filename);

#endif /* _APPLICATIONFILES_H_ */
