/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Mon Mar 01 2004
 copyright   : (C) 2004 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "account_p.h"
#include "banking_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_INHERIT_FUNCTIONS(AB_ACCOUNT)
GWEN_LIST_FUNCTIONS(AB_ACCOUNT, AB_Account)
GWEN_LIST2_FUNCTIONS(AB_ACCOUNT, AB_Account)



AB_ACCOUNT *AB_Account_new(AB_BANKING *ab,
                           AB_PROVIDER *pro,
                           const char *idForProvider){
  AB_ACCOUNT *a;

  assert(ab);
  assert(pro);
  GWEN_NEW_OBJECT(AB_ACCOUNT, a);
  GWEN_INHERIT_INIT(AB_ACCOUNT, a);
  GWEN_LIST_INIT(AB_ACCOUNT, a);
  a->banking=ab;
  a->provider=pro;
  a->data=GWEN_DB_Group_new("Data");
  if (idForProvider)
    GWEN_DB_SetCharValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "static/idForProvider", idForProvider);

  return a;
}



AB_ACCOUNT *AB_Account_fromDb(AB_BANKING *ab,
                              GWEN_DB_NODE *db){
  AB_ACCOUNT *a;
  GWEN_DB_NODE *dbT;
  const char *pname;
  AB_PROVIDER *pro;

  assert(ab);
  pname=GWEN_DB_GetCharValue(db, "provider", 0, 0);
  assert(pname);
  pro=AB_Banking_GetProvider(ab, pname);
  if (!pro) {
    DBG_ERROR(0, "Provider \"%s\" is not available, ignoring account",
              pname);
    return 0;
  }

  GWEN_NEW_OBJECT(AB_ACCOUNT, a);
  GWEN_INHERIT_INIT(AB_ACCOUNT, a);
  GWEN_LIST_INIT(AB_ACCOUNT, a);
  a->banking=ab;
  a->provider=pro;
  a->data=GWEN_DB_Group_new("Data");
  dbT=GWEN_DB_GetGroup(a->data, GWEN_DB_FLAGS_DEFAULT, "static");
  assert(dbT);
  GWEN_DB_AddGroupChildren(dbT, db);

  /* mark DB not-dirty */
  GWEN_DB_ModifyBranchFlagsDown(a->data, 0, GWEN_DB_NODE_FLAGS_DIRTY);

  /* let provider update account data */
  a->availability=AB_Provider_UpdateAccount(pro, a);
  if (a->availability) {
    DBG_WARN(0, "Error updating account by backend");
  }
  return a;
}



int AB_Account_toDb(const AB_ACCOUNT *a, GWEN_DB_NODE *db){
  GWEN_DB_NODE *dbT;

  assert(a);
  dbT=GWEN_DB_GetGroup(a->data, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "static");
  if (dbT)
    GWEN_DB_AddGroupChildren(db, dbT);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "provider",
                       AB_Provider_GetName(a->provider));
  return 0;
}



void AB_Account_free(AB_ACCOUNT *a){
  if (a) {
    GWEN_INHERIT_INIT(AB_ACCOUNT, a);
    GWEN_DB_Group_free(a->data);
    GWEN_LIST_INIT(AB_ACCOUNT, a);
    GWEN_FREE_OBJECT(a);
  }
}



int AB_Account_CheckAvailability(AB_ACCOUNT *a){
  assert(a);
  return a->availability;
}



GWEN_TYPE_UINT32 AB_Account_GetUniqueId(const AB_ACCOUNT *a){
  assert(a);
  return GWEN_DB_GetIntValue(a->data, "static/uniqueId", 0, 0);
}



void AB_Account_SetUniqueId(AB_ACCOUNT *a, GWEN_TYPE_UINT32 id){
  assert(a);
  GWEN_DB_SetIntValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "static/uniqueId", id);
}



AB_PROVIDER *AB_Account_GetProvider(const AB_ACCOUNT *a) {
  assert(a);
  return a->provider;
}



GWEN_DB_NODE *AB_Account_GetAppData(const AB_ACCOUNT *a){
  GWEN_DB_NODE *n;
  const char *appName;

  assert(a);
  appName=AB_Banking_GetAppName(a->banking);
  assert(appName);
  n=GWEN_DB_GetGroup(a->data, GWEN_DB_FLAGS_DEFAULT, "static/apps");
  assert(n);
  n=GWEN_DB_GetGroup(n, GWEN_DB_FLAGS_DEFAULT, appName);
  return n;
}



GWEN_DB_NODE *AB_Account_GetProviderData(const AB_ACCOUNT *a){
  GWEN_DB_NODE *n;

  assert(a);
  n=GWEN_DB_GetGroup(a->data, GWEN_DB_FLAGS_DEFAULT, "static/provider");
  return n;
}



const char *AB_Account_GetAccountNumber(const AB_ACCOUNT *a){
  assert(a);
  return GWEN_DB_GetCharValue(a->data, "static/accountNumber", 0, 0);
}



void AB_Account_SetAccountNumber(AB_ACCOUNT *a, const char *s){
  assert(a);
  if (!s)
    GWEN_DB_DeleteVar(a->data, "static/accountNumber");
  else
    GWEN_DB_SetCharValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "static/accountNumber", s);
}



const char *AB_Account_GetBankCode(const AB_ACCOUNT *a){
  assert(a);
  return GWEN_DB_GetCharValue(a->data, "static/bankCode", 0, 0);
}



void AB_Account_SetBankCode(AB_ACCOUNT *a, const char *s){
  assert(a);
  if (!s)
    GWEN_DB_DeleteVar(a->data, "static/bankCode");
  else
    GWEN_DB_SetCharValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "static/bankCode", s);
}



const char *AB_Account_GetAccountName(const AB_ACCOUNT *a){
  assert(a);
  return GWEN_DB_GetCharValue(a->data, "static/accountName", 0, 0);
}



void AB_Account_SetAccountName(AB_ACCOUNT *a, const char *s){
  assert(a);
  if (!s)
    GWEN_DB_DeleteVar(a->data, "static/accountName");
  else
    GWEN_DB_SetCharValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "static/accountName", s);
}



const char *AB_Account_GetBankName(const AB_ACCOUNT *a){
  assert(a);
  return GWEN_DB_GetCharValue(a->data, "static/bankName", 0, 0);
}



void AB_Account_SetBankName(AB_ACCOUNT *a, const char *s){
  assert(a);
  if (!s)
    GWEN_DB_DeleteVar(a->data, "static/bankName");
  else
    GWEN_DB_SetCharValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "static/bankName", s);
}



const char *AB_Account_GetOwnerName(const AB_ACCOUNT *a){
  assert(a);
  return GWEN_DB_GetCharValue(a->data, "static/ownerName", 0, 0);
}



void AB_Account_SetOwnerName(AB_ACCOUNT *a, const char *s){
  assert(a);
  if (!s)
    GWEN_DB_DeleteVar(a->data, "static/ownerName");
  else
    GWEN_DB_SetCharValue(a->data, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "static/ownerName", s);
}



AB_BANKING *AB_Account_GetBanking(const AB_ACCOUNT *acc){
  assert(acc);
  return acc->banking;
}


















