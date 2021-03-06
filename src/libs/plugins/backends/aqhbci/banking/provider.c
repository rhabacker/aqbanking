/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2018 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "provider_p.h"
#include "aqhbci_l.h"
#include "account_l.h"
#include "hbci_l.h"
#include "dialog_l.h"
#include "outbox_l.h"
#include "user_l.h"
#include "control_l.h"

#include "message_l.h" /* for test4 */

#include "jobgetbalance_l.h"
#include "jobgettransactions_l.h"
#include "jobgettrans_camt_l.h"
#include "jobloadcellphone_l.h"
#include "jobsepaxfersingle_l.h"
#include "jobsepaxfermulti_l.h"
#include "jobsepadebitdatedsinglecreate_l.h"
#include "jobsepadebitdatedmulticreate_l.h"
#include "jobsepacor1datedsinglecreate_l.h"
#include "jobsepacor1datedmulticreate_l.h"

#include "jobsepastandingorderdelete_l.h"
#include "jobsepastandingordercreate_l.h"
#include "jobsepastandingordermodify_l.h"
#include "jobsepastandingorderget_l.h"
#include "jobgetestatements_l.h"

#include "jobsepadebitsingle_l.h" /* deprecated job */

/* special jobs */
#include "jobforeignxferwh_l.h"

/* admin jobs */
#include "jobgetkeys_l.h"
#include "jobsendkeys_l.h"
#include "jobchangekeys_l.h"
#include "jobgetsepainfo_l.h"
#include "jobgetsysid_l.h"
#include "jobgetbankinfo_l.h"

/*
#include "dlg_newuser_l.h"
#include "dlg_pintan_l.h"
#include "dlg_ddvcard_l.h"
#include "dlg_zkacard_l.h"
#include "dlg_newkeyfile_l.h"
#include "dlg_importkeyfile_l.h"
#include "dlg_edituserpintan_l.h"
#include "dlg_edituserddv_l.h"
#include "dlg_edituserrdh_l.h"
#include "dlg_choose_usertype_l.h"
#include "dlg_editaccount_l.h"
*/

#include "adminjobs_l.h"
#include "aqhbci/banking/user.h"

#include <aqbanking/banking_be.h>
#include <aqbanking/backendsupport/account.h>
#include <aqbanking/backendsupport/provider_be.h>
#include <aqbanking/backendsupport/userqueue.h>

#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/mdigest.h>
#include <gwenhywfar/ctplugin.h>



#ifdef OS_WIN32
# define AH_PATH_SEP "\\"
#else
# define AH_PATH_SEP "/"
#endif



GWEN_INHERIT(AB_PROVIDER, AH_PROVIDER);


AB_PROVIDER *AH_Provider_new(AB_BANKING *ab, const char *name)
{
  AB_PROVIDER *pro;
  AH_PROVIDER *hp;
  GWEN_BUFFER *pbuf;

  pbuf=0;
  pro=AB_Provider_new(ab, name);
  assert(pro);

  AB_Provider_SetInitFn(pro, AH_Provider_Init);
  AB_Provider_SetFiniFn(pro, AH_Provider_Fini);

  AB_Provider_SetGetNewUserDialogFn(pro, AH_Provider_GetNewUserDialog);
  AB_Provider_SetGetEditUserDialogFn(pro, AH_Provider_GetEditUserDialog);
  AB_Provider_SetGetUserTypeDialogFn(pro, AH_Provider_GetUserTypeDialog);
  AB_Provider_SetGetEditAccountDialogFn(pro, AH_Provider_GetEditAccountDialog);

  AB_Provider_SetSendCommandsFn(pro, AH_Provider_SendCommands);
  AB_Provider_SetCreateAccountObjectsFn(pro, AH_Provider_CreateAccountObject);
  AB_Provider_SetCreateUserObjectsFn(pro, AH_Provider_CreateUserObject);

  AB_Provider_SetUpdateAccountSpecFn(pro, AH_Provider_UpdateAccountSpec);
  AB_Provider_SetControlFn(pro, AH_Control);

  AB_Provider_AddFlags(pro,
                       AB_PROVIDER_FLAGS_HAS_NEWUSER_DIALOG |
                       AB_PROVIDER_FLAGS_HAS_EDITUSER_DIALOG |
                       AB_PROVIDER_FLAGS_HAS_EDITACCOUNT_DIALOG |
                       AB_PROVIDER_FLAGS_HAS_USERTYPE_DIALOG);

  GWEN_NEW_OBJECT(AH_PROVIDER, hp);
  GWEN_INHERIT_SETDATA(AB_PROVIDER, AH_PROVIDER, pro, hp,
                       AH_Provider_FreeData);

  hp->hbci=AH_HBCI_new(pro);
  assert(hp->hbci);
  GWEN_Buffer_free(pbuf);

  hp->dbTempConfig=GWEN_DB_Group_new("tmpConfig");

  return pro;
}



void GWENHYWFAR_CB AH_Provider_FreeData(void *bp, void *p)
{
  AH_PROVIDER *hp;

  DBG_INFO(AQHBCI_LOGDOMAIN, "Destroying AH_PROVIDER");
  hp=(AH_PROVIDER *)p;

  GWEN_DB_Group_free(hp->dbTempConfig);
  AH_HBCI_free(hp->hbci);

  GWEN_FREE_OBJECT(hp);
}



int AH_Provider_Init(AB_PROVIDER *pro, GWEN_DB_NODE *dbData)
{
  AH_PROVIDER *hp;
  int rv;
  const char *logLevelName;
  uint32_t currentVersion;
  uint32_t lastVersion;

  if (!GWEN_Logger_IsOpen(AQHBCI_LOGDOMAIN)) {
    GWEN_Logger_Open(AQHBCI_LOGDOMAIN, "aqhbci", 0, GWEN_LoggerType_Console, GWEN_LoggerFacility_User);
  }

  logLevelName=getenv("AQHBCI_LOGLEVEL");
  if (logLevelName) {
    GWEN_LOGGER_LEVEL ll;

    ll=GWEN_Logger_Name2Level(logLevelName);
    if (ll!=GWEN_LoggerLevel_Unknown) {
      GWEN_Logger_SetLevel(AQHBCI_LOGDOMAIN, ll);
      DBG_WARN(AQHBCI_LOGDOMAIN, "Overriding loglevel for AqHBCI with \"%s\"", logLevelName);
    }
    else {
      DBG_ERROR(AQHBCI_LOGDOMAIN, "Unknown loglevel \"%s\"", logLevelName);
    }
  }

  DBG_NOTICE(AQHBCI_LOGDOMAIN, "Initializing AqHBCI backend");
  assert(pro);

  hp=GWEN_INHERIT_GETDATA(AB_PROVIDER, AH_PROVIDER, pro);
  assert(hp);

  GWEN_DB_ClearGroup(hp->dbTempConfig, 0);

  /* check whether we need to update */
  currentVersion=
    (AQBANKING_VERSION_MAJOR<<24) |
    (AQBANKING_VERSION_MINOR<<16) |
    (AQBANKING_VERSION_PATCHLEVEL<<8) |
    AQBANKING_VERSION_BUILD;
  lastVersion=GWEN_DB_GetIntValue(dbData, "lastVersion", 0, 0);

  if (lastVersion<currentVersion) {
    DBG_WARN(AQHBCI_LOGDOMAIN, "Updating configuration for AqHBCI (before init)");
    rv=AH_Provider_UpdatePreInit(pro, lastVersion, currentVersion);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
  }

  /* HBCI init */
  rv=AH_HBCI_Init(hp->hbci, dbData);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
  }

  if (lastVersion<currentVersion) {
    DBG_WARN(AQHBCI_LOGDOMAIN, "Updating configuration for AqHBCI (after init)");
    rv=AH_Provider_UpdatePostInit(pro, lastVersion, currentVersion);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
  }

  return rv;
}



int AH_Provider_Fini(AB_PROVIDER *pro, GWEN_DB_NODE *dbData)
{
  AH_PROVIDER *hp;
  uint32_t currentVersion;
  int rv;

  DBG_NOTICE(AQHBCI_LOGDOMAIN, "Deinitializing AqHBCI backend");

  assert(pro);
  hp=GWEN_INHERIT_GETDATA(AB_PROVIDER, AH_PROVIDER, pro);
  assert(hp);

  currentVersion=
    (AQBANKING_VERSION_MAJOR<<24) |
    (AQBANKING_VERSION_MINOR<<16) |
    (AQBANKING_VERSION_PATCHLEVEL<<8) |
    AQBANKING_VERSION_BUILD;

  /* save version */
  DBG_NOTICE(AQHBCI_LOGDOMAIN, "Setting version %08x", currentVersion);
  GWEN_DB_SetIntValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS, "lastVersion", currentVersion);

  /* save configuration */
  rv=AH_HBCI_Fini(hp->hbci, dbData);
  GWEN_DB_ClearGroup(hp->dbTempConfig, 0);

  return rv;
}



const char *AH_Provider_GetProductName(const AB_PROVIDER *pro)
{
  AH_HBCI *h;

  assert(pro);
  h=AH_Provider_GetHbci(pro);
  assert(h);
  return AH_HBCI_GetProductName(h);
}



const char *AH_Provider_GetProductVersion(const AB_PROVIDER *pro)
{
  AH_HBCI *h;

  assert(pro);
  h=AH_Provider_GetHbci(pro);
  assert(h);
  return AH_HBCI_GetProductVersion(h);
}



int AH_Provider__CreateHbciJob(AB_PROVIDER *pro, AB_USER *mu, AB_ACCOUNT *ma, int cmd, AH_JOB **pHbciJob)
{
  AH_PROVIDER *hp;
  AH_JOB *mj;
  uint32_t aFlags;

  assert(pro);
  hp=GWEN_INHERIT_GETDATA(AB_PROVIDER, AH_PROVIDER, pro);
  assert(hp);

  aFlags=AH_Account_GetFlags(ma);

  mj=0;
  switch (cmd) {

  case AB_Transaction_CommandGetBalance:
    mj=AH_Job_GetBalance_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;

  case AB_Transaction_CommandGetTransactions:
    if (aFlags & AH_BANK_FLAGS_PREFER_CAMT_DOWNLOAD) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Customer prefers CAMT download");
      mj=AH_Job_GetTransactionsCAMT_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "CAMT download job not supported with this account, falling back to SWIFT");
      }
    }
    if (!mj) {
      mj=AH_Job_GetTransactions_new(pro, mu, ma);
      if (!mj) {
        DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
        return GWEN_ERROR_NOT_AVAILABLE;
      }
    }
    break;

  case AB_Transaction_CommandLoadCellPhone:
    mj=AH_Job_LoadCellPhone_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;

  case AB_Transaction_CommandSepaTransfer:
    if (!(aFlags & AH_BANK_FLAGS_SEPA_PREFER_SINGLE_TRANSFER)) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Customer prefers multi jobs");

      /* try multi transfer first */
      mj=AH_Job_SepaTransferMulti_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "Job \"SepaTransferMulti\" not supported with this account");

        /* try single transfer */
        mj=AH_Job_SepaTransferSingle_new(pro, mu, ma);
        if (!mj) {
          DBG_WARN(AQHBCI_LOGDOMAIN, "Job \"SepaTransferSingle\" not supported with this account");
          return GWEN_ERROR_NOT_AVAILABLE;
        }
      }
    }
    else {
      /* try single job first */
      mj=AH_Job_SepaTransferSingle_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "Job \"SepaTransferSingle\" not supported with this account");

        /* try multi transfer next */
        mj=AH_Job_SepaTransferMulti_new(pro, mu, ma);
        if (!mj) {
          DBG_INFO(AQHBCI_LOGDOMAIN, "Job \"SepaTransferMulti\" not supported with this account");
          return GWEN_ERROR_NOT_AVAILABLE;
        }
      }
    }
    break;

  case AB_Transaction_CommandSepaDebitNote:
    if (!(aFlags & AH_BANK_FLAGS_SEPA_PREFER_SINGLE_DEBITNOTE)) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Customer prefers multi jobs");

      /* try multi transfer first */
      mj=AH_Job_SepaDebitDatedMultiCreate_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "SepaDebitDatedMultiCreate not supported with this account");

        /* try single transfer */
        mj=AH_Job_SepaDebitDatedSingleCreate_new(pro, mu, ma);
        if (!mj) {
          DBG_WARN(AQHBCI_LOGDOMAIN,
                   "Job \"SepaDebitDatedSingleCreate\" not supported with this account, trying old single debit");

          /* try old singleDebit job next */
          mj=AH_Job_SepaDebitSingle_new(pro, mu, ma);
          if (!mj) {
            DBG_INFO(AQHBCI_LOGDOMAIN, "Job \"SepaDebitSingle\" not supported with this account");
            return GWEN_ERROR_NOT_AVAILABLE;
          }
        }
      }
    }
    else {
      /* try single job first */
      mj=AH_Job_SepaDebitDatedSingleCreate_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "SepaDebitDatedSingleCreate not supported with this account");

        /* try old singleDebit job next */
        mj=AH_Job_SepaDebitSingle_new(pro, mu, ma);
        if (!mj) {
          DBG_INFO(AQHBCI_LOGDOMAIN, "Job \"SepaDebitSingle\" not supported with this account");

          /* try multi transfer next */
          mj=AH_Job_SepaDebitDatedMultiCreate_new(pro, mu, ma);
          if (!mj) {
            DBG_INFO(AQHBCI_LOGDOMAIN, "SepaDebitDatedMultiCreate not supported with this account");
            return GWEN_ERROR_NOT_AVAILABLE;
          }
        }
      }
    }
    break;

  case AB_Transaction_CommandSepaFlashDebitNote:
    if (!(aFlags & AH_BANK_FLAGS_SEPA_PREFER_SINGLE_DEBITNOTE)) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Customer prefers multi jobs");

      /* try multi transfer first */
      mj=AH_Job_SepaCor1DebitDatedMultiCreate_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "SepaCor1DebitDatedMultiCreate not supported with this account");

        /* try single transfer */
        mj=AH_Job_SepaCor1DebitDatedSingleCreate_new(pro, mu, ma);
        if (!mj) {
          DBG_WARN(AQHBCI_LOGDOMAIN, "Job \"SepaCor1DebitDatedSingleCreate\" not supported with this account");
          return GWEN_ERROR_NOT_AVAILABLE;
        }
      }
    }
    else {
      /* try single job first */
      mj=AH_Job_SepaCor1DebitDatedSingleCreate_new(pro, mu, ma);
      if (!mj) {
        DBG_WARN(AQHBCI_LOGDOMAIN, "SepaCor1DebitDatedSingleCreate not supported with this account");

        /* try multi transfer next */
        mj=AH_Job_SepaCor1DebitDatedMultiCreate_new(pro, mu, ma);
        if (!mj) {
          DBG_INFO(AQHBCI_LOGDOMAIN, "SepaCor1DebitDatedMultiCreate not supported with this account");
          return GWEN_ERROR_NOT_AVAILABLE;
        }
      }
    }
    break;

  case AB_Transaction_CommandSepaCreateStandingOrder:
    mj=AH_Job_SepaStandingOrderCreate_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;
  case AB_Transaction_CommandSepaModifyStandingOrder:
    mj=AH_Job_SepaStandingOrderModify_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;
  case AB_Transaction_CommandSepaDeleteStandingOrder:
    mj=AH_Job_SepaStandingOrderDelete_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;

  case AB_Transaction_CommandSepaGetStandingOrders:
    mj=AH_Job_SepaStandingOrderGet_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;

  case AB_Transaction_CommandGetEStatements:
    mj=AH_Job_GetEStatements_new(pro, mu, ma);
    if (!mj) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "Job not supported with this account");
      return GWEN_ERROR_NOT_AVAILABLE;
    }
    break;

  default:
    DBG_INFO(AQHBCI_LOGDOMAIN,
             "Job not supported by AqHBCI");
    return GWEN_ERROR_NOT_AVAILABLE;
  } /* switch */
  assert(mj);

  DBG_INFO(AQHBCI_LOGDOMAIN, "Job successfully created");
  *pHbciJob=mj;
  return 0;
}



int AH_Provider__GetMultiHbciJob(AB_PROVIDER *pro, AH_OUTBOX *outbox, AB_USER *mu, AB_ACCOUNT *ma, int cmd,
                                 AH_JOB **pHbciJob)
{
  AH_PROVIDER *hp;
  AH_JOB *mj=NULL;

  assert(pro);
  hp=GWEN_INHERIT_GETDATA(AB_PROVIDER, AH_PROVIDER, pro);
  assert(hp);

  assert(mu);

  switch (cmd) {
  case AB_Transaction_CommandSepaTransfer:
    mj=AH_Outbox_FindTransferJob(outbox, mu, ma, "JobSepaTransferMulti");
    break;

  case AB_Transaction_CommandSepaDebitNote:
    mj=AH_Outbox_FindTransferJob(outbox, mu, ma, "JobSepaDebitDatedMultiCreate");
    break;

  default:
    DBG_INFO(AQHBCI_LOGDOMAIN,
             "No Multi jobs defined for this job type");
    break;
  } /* switch */

  if (mj) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "Multi job found");
    *pHbciJob=mj;
    return 0;
  }
  else {
    DBG_INFO(AQHBCI_LOGDOMAIN, "No multi job found");
    return GWEN_ERROR_NOT_FOUND;
  }
}



AH_HBCI *AH_Provider_GetHbci(const AB_PROVIDER *pro)
{
  AH_PROVIDER *hp;

  assert(pro);
  hp=GWEN_INHERIT_GETDATA(AB_PROVIDER, AH_PROVIDER, pro);
  assert(hp);

  return hp->hbci;
}



int AH_Provider__HashRmd160(const uint8_t *p, unsigned int l, uint8_t *buf)
{
  GWEN_MDIGEST *md;
  int rv;

  md=GWEN_MDigest_Rmd160_new();
  assert(md);
  rv=GWEN_MDigest_Begin(md);
  if (rv<0) {
    GWEN_MDigest_free(md);
    return rv;
  }
  rv=GWEN_MDigest_Update(md, p, l);
  if (rv<0) {
    GWEN_MDigest_free(md);
    return rv;
  }
  rv=GWEN_MDigest_End(md);
  if (rv<0) {
    GWEN_MDigest_free(md);
    return rv;
  }

  memmove(buf, GWEN_MDigest_GetDigestPtr(md), GWEN_MDigest_GetDigestSize(md));
  GWEN_MDigest_free(md);
  return 0;
}



int AH_Provider__HashSha256(const uint8_t *p, unsigned int l, uint8_t *buf)
{
  GWEN_MDIGEST *md;
  int rv;

  md=GWEN_MDigest_Sha256_new();
  assert(md);
  rv=GWEN_MDigest_Begin(md);
  if (rv<0) {
    GWEN_MDigest_free(md);
    return rv;
  }
  rv=GWEN_MDigest_Update(md, p, l);
  if (rv<0) {
    GWEN_MDigest_free(md);
    return rv;
  }
  rv=GWEN_MDigest_End(md);
  if (rv<0) {
    GWEN_MDigest_free(md);
    return rv;
  }

  memmove(buf, GWEN_MDigest_GetDigestPtr(md), GWEN_MDigest_GetDigestSize(md));
  GWEN_MDigest_free(md);
  return 0;
}








int AH_Provider_WriteValueToDb(const AB_VALUE *v, GWEN_DB_NODE *dbV)
{
  if (v) {
    GWEN_BUFFER *nbuf;
    char *p;
    const char *s;
    int l;

    nbuf=GWEN_Buffer_new(0, 32, 0, 1);
    AH_Job_ValueToChallengeString(v, nbuf);
    l=GWEN_Buffer_GetUsedBytes(nbuf);
    if (!l) {
      DBG_ERROR(AQHBCI_LOGDOMAIN, "Error in conversion");
      GWEN_Buffer_free(nbuf);
      abort();
    }

    /* replace "C" comma with "DE" comma, remove thousand's comma */
    p=GWEN_Buffer_GetStart(nbuf);
    s=p;
    while (*s) {
      if (*s=='.') {
        *p=',';
        p++;
      }
      else if (*s!=',') {
        *p=*s;
        p++;
      }
      s++;
    } /* while */
    *p=0;

    if (strchr(GWEN_Buffer_GetStart(nbuf), ',')) {
      /* kill all trailing '0' behind the comma */
      p=GWEN_Buffer_GetStart(nbuf)+l;
      while (l--) {
        --p;
        if (*p=='0')
          *p=0;
        else
          break;
      }
    }
    else
      GWEN_Buffer_AppendString(nbuf, ",");

    /* store value */
    GWEN_DB_SetCharValue(dbV, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "value",
                         GWEN_Buffer_GetStart(nbuf));
    GWEN_Buffer_free(nbuf);

    s=AB_Value_GetCurrency(v);
    if (!s)
      s="EUR";
    GWEN_DB_SetCharValue(dbV, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "currency", s);

    return 0;
  } /* if value */
  else {
    DBG_ERROR(AQHBCI_LOGDOMAIN, "No value");
    return GWEN_ERROR_NO_DATA;
  }
}



static int AH_Provider_Test4(AB_PROVIDER *pro)
{
#if 0
  AB_BANKING *ab;
  AB_USER *u;
  AH_DIALOG *dlg;
  AH_MSG *msg;
  GWEN_BUFFER *tbuf;

  ab=AB_Provider_GetBanking(pro);
  assert(ab);

  tbuf=GWEN_Buffer_new(0, 1024, 0, 1);
  GWEN_Buffer_AppendString(tbuf, "(Test-String)");
  GWEN_Buffer_Rewind(tbuf);

  u=AB_Banking_FindUser(ab, "aqhbci", "de", "20090500", "*", "*");
  assert(u);
  dlg=AH_Dialog_new(u);
  assert(dlg);
  AH_Dialog_AddFlags(dlg, AH_DIALOG_FLAGS_INITIATOR);
  msg=AH_Msg_new(dlg);
  assert(msg);
  AH_Msg_SetBuffer(msg, tbuf);
  AH_Msg_SetHbciVersion(msg, 220);

  AH_Msg_AddSignerId(msg, AB_User_GetUserId(u));
  AH_Msg_SetCrypterId(msg, AB_User_GetUserId(u));

  if (AH_Msg_EncodeMsg(msg)) {
    DBG_ERROR(AQHBCI_LOGDOMAIN, "Could not decode this message:");
    AH_Msg_Dump(msg, 2);
    return -1;
  }

  fprintf(stderr, "Message is:\n");
  AH_Msg_Dump(msg, 2);
#endif
  return 0;
}



int AH_Provider_Test(AB_PROVIDER *pro)
{
  return AH_Provider_Test4(pro);
}





#include "provider_sendcmd.c"
#include "provider_dtazv.c"
#include "provider_iniletter.c"
#include "provider_keys.c"
#include "provider_online.c"
#include "provider_dialogs.c"
#include "provider_accspec.c"
#include "provider_update.c"
#include "provider_tan.c"



