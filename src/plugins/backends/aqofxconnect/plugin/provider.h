/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2018 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifndef AO_PROVIDER_H
#define AO_PROVIDER_H

#include <aqofxconnect/aqofxconnect.h>
#include <aqbanking/banking_be.h>
#include <aqbanking/provider_be.h>

#define AQOFXCONNECT_BACKENDNAME "aqofxconnect"

#define AQOFXCONNECT_LOGDOMAIN "aqofxconnect"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AO_PROVIDER AO_PROVIDER;


typedef struct {
  const char *appName;
  const char *appId;
  const char *appVer;
} AO_APPINFO;


AQOFXCONNECT_API AB_PROVIDER *AO_Provider_new(AB_BANKING *ab);

AQOFXCONNECT_API const AO_APPINFO *AO_Provider_GetAppInfos(AB_PROVIDER *pro);

AQOFXCONNECT_API int AO_Provider_GetCert(AB_PROVIDER *pro, AB_USER *u);

AQOFXCONNECT_API int AO_Provider_RequestAccounts(AB_PROVIDER *pro, AB_USER *u, int keepOpen);


AQOFXCONNECT_API int AO_Provider_AddAccount(AB_PROVIDER *pro, AB_USER *u, AB_ACCOUNT *a);
AQOFXCONNECT_API int AO_Provider_DeleteAccount(AB_PROVIDER *pro, uint32_t uid);


AQOFXCONNECT_API int AO_Provider_AddUser(AB_PROVIDER *pro, AB_USER *u);
AQOFXCONNECT_API int AO_Provider_DeleteUser(AB_PROVIDER *pro, uint32_t uid);


#ifdef __cplusplus
}
#endif


#endif

