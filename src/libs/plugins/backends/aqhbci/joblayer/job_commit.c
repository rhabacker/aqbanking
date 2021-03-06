/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2018 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


/*
 * This file is included by job.c
 */



int AH_Job__GetJobGroup(GWEN_DB_NODE *dbJob, const char *groupName, GWEN_DB_NODE **pResult)
{
  GWEN_DB_NODE *dbRd;

  dbRd=GWEN_DB_GetGroup(dbJob, GWEN_PATH_FLAGS_NAMEMUSTEXIST, groupName);
  if (dbRd==NULL) {
    DBG_DEBUG(AQHBCI_LOGDOMAIN, "Group \"%s\" not found in response", groupName);
    return GWEN_ERROR_NOT_FOUND;
  }

  dbRd=GWEN_DB_GetGroup(dbRd, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "data");
  if (dbRd==NULL) {
    DBG_ERROR(AQHBCI_LOGDOMAIN, "Missing \"data\" group inside group \"%s\"", groupName);
    return GWEN_ERROR_INVALID;
  }

  dbRd=GWEN_DB_GetGroup(dbRd, GWEN_PATH_FLAGS_NAMEMUSTEXIST, groupName);
  if (dbRd==NULL) {
    DBG_ERROR(AQHBCI_LOGDOMAIN, "Missing effective group \"%s\" inside response", groupName);
    return GWEN_ERROR_INVALID;
  }

  *pResult=dbRd;
  return 0;
}



int AH_Job__Commit_Bpd(AH_JOB *j)
{
  GWEN_DB_NODE *dbJob    ;
  GWEN_DB_NODE *dbRd=NULL;
  AH_BPD *bpd;
  GWEN_DB_NODE *n;
  const char *p;
  int i;
  int rv;
  GWEN_MSGENGINE *msgEngine;

  //dbJob=GWEN_DB_GetFirstGroup(j->jobResponses);
  dbJob=j->jobResponses;

  rv=AH_Job__GetJobGroup(dbJob, "bpd", &dbRd);
  if (rv<0) {
    if (rv!=GWEN_ERROR_NOT_FOUND) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
    DBG_INFO(AQHBCI_LOGDOMAIN, "No BPD in response for job %s", AH_Job_GetName(j));
    /*GWEN_DB_Dump(j->jobResponses, 2);*/
    return 0;
  }

  DBG_NOTICE(AQHBCI_LOGDOMAIN, "Found BPD, replacing existing");

  msgEngine=AH_User_GetMsgEngine(j->user);
  assert(msgEngine);

  /* create new BPD */
  bpd=AH_Bpd_new();

  /* read version */
  AH_Bpd_SetBpdVersion(bpd, GWEN_DB_GetIntValue(dbRd, "version", 0, 0));

  /* read bank name */
  p=GWEN_DB_GetCharValue(dbRd, "name", 0, 0);
  if (p)
    AH_Bpd_SetBankName(bpd, p);

  /* read message and job limits */
  AH_Bpd_SetJobTypesPerMsg(bpd, GWEN_DB_GetIntValue(dbRd, "jobtypespermsg", 0, 0));
  AH_Bpd_SetMaxMsgSize(bpd, GWEN_DB_GetIntValue(dbRd, "maxmsgsize", 0, 0));

  /* read languages */
  n=GWEN_DB_GetGroup(dbRd, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "languages");
  if (n) {
    for (i=0;; i++) {
      int k;

      k=GWEN_DB_GetIntValue(n, "language", i, 0);
      if (k) {
        if (AH_Bpd_AddLanguage(bpd, k)) {
          DBG_ERROR(AQHBCI_LOGDOMAIN, "Too many languages (%d)", i);
          break;
        }
      }
      else
        break;
    } /* for */
  } /* if languages */

  /* read supported version */
  n=GWEN_DB_GetGroup(dbRd, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "versions");
  if (n) {
    for (i=0;; i++) {
      int k;

      k=GWEN_DB_GetIntValue(n, "version", i, 0);
      if (k) {
        if (AH_Bpd_AddHbciVersion(bpd, k)) {
          DBG_ERROR(AQHBCI_LOGDOMAIN, "Too many versions (%d)", i);
          break;
        }
      }
      else
        break;
    } /* for */
  } /* if versions */


  /* communication parameters */
  rv=AH_Job__GetJobGroup(dbJob, "ComData", &dbRd);
  if (rv==0) {
    GWEN_DB_NODE *currService;

    DBG_DEBUG(AQHBCI_LOGDOMAIN, "Found communication infos");

    currService=GWEN_DB_FindFirstGroup(dbRd, "service");
    while (currService) {
      AH_BPD_ADDR *ba;

      ba=AH_BpdAddr_FromDb(currService);
      if (ba) {
        if (1) { /* dump info */
          GWEN_BUFFER *tbuf;
          const char *s;

          tbuf=GWEN_Buffer_new(0, 256, 0, 1);

          switch (AH_BpdAddr_GetType(ba)) {
          case AH_BPD_AddrTypeTCP:
            GWEN_Buffer_AppendString(tbuf, "TCP: ");
            break;
          case AH_BPD_AddrTypeBTX:
            GWEN_Buffer_AppendString(tbuf, "BTX: ");
            break;
          case AH_BPD_AddrTypeSSL:
            GWEN_Buffer_AppendString(tbuf, "SSL: ");
            break;
          default:
            GWEN_Buffer_AppendString(tbuf, "<UNK>: ");
            break;
          }

          s=AH_BpdAddr_GetAddr(ba);
          if (s && *s)
            GWEN_Buffer_AppendString(tbuf, s);
          else
            GWEN_Buffer_AppendString(tbuf, "<empty>");

          s=AH_BpdAddr_GetSuffix(ba);
          if (s && *s) {
            GWEN_Buffer_AppendString(tbuf, ", ");
            GWEN_Buffer_AppendString(tbuf, s);
          }

          GWEN_Buffer_AppendString(tbuf, ", ");
          switch (AH_BpdAddr_GetFType(ba)) {
          case AH_BPD_FilterTypeNone:
            GWEN_Buffer_AppendString(tbuf, "none");
            break;
          case AH_BPD_FilterTypeBase64:
            GWEN_Buffer_AppendString(tbuf, "base64");
            break;
          case AH_BPD_FilterTypeUUE:
            GWEN_Buffer_AppendString(tbuf, "uue");
            break;
          default:
            GWEN_Buffer_AppendString(tbuf, "<unk>");
            break;
          }

          DBG_DEBUG(AQHBCI_LOGDOMAIN, "Server address found: %s", GWEN_Buffer_GetStart(tbuf));
          GWEN_Gui_ProgressLog2(0,
                                GWEN_LoggerLevel_Info,
                                I18N("Server address found: %s"),
                                GWEN_Buffer_GetStart(tbuf));
          GWEN_Buffer_free(tbuf);
        }

        /* add service */
        AH_Bpd_AddAddr(bpd, ba);
      }
      currService=GWEN_DB_FindNextGroup(currService, "service");
    }
  } /* if ComData found */


  /* special extension of BPD for PIN/TAN mode */
  rv=AH_Job__GetJobGroup(dbJob, "PinTanBPD", &dbRd);
  if (rv==0) {
    GWEN_DB_NODE *bn;
    GWEN_DB_NODE *currJob;

    bn=AH_Bpd_GetBpdJobs(bpd, GWEN_MsgEngine_GetProtocolVersion(msgEngine));
    assert(bn);

    currJob=GWEN_DB_FindFirstGroup(dbRd, "job");
    while (currJob) {
      const char *jobName;
      int needTAN;
      GWEN_DB_NODE *dbJob;

      jobName=GWEN_DB_GetCharValue(currJob, "job", 0, 0);
      assert(jobName);
      dbJob=GWEN_DB_GetGroup(bn, GWEN_DB_FLAGS_DEFAULT, jobName);
      assert(dbJob);
      needTAN=strcasecmp(GWEN_DB_GetCharValue(currJob, "needTan", 0, "N"), "J")==0;
      GWEN_DB_SetIntValue(dbJob, GWEN_DB_FLAGS_OVERWRITE_VARS, "needTan", needTAN);
      currJob=GWEN_DB_FindNextGroup(currJob, "job");
    } /* while */
  } /* if PIN/TAN extension found */


  /* check for BPD jobs */
  n=GWEN_DB_GetFirstGroup(dbJob);
  while (n) {
    dbRd=GWEN_DB_GetGroup(n, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "data");
    if (dbRd)
      dbRd=GWEN_DB_GetFirstGroup(dbRd);
    if (dbRd) {
      GWEN_XMLNODE *bpdn;
      int segver;
      /* check for BPD job */

      DBG_DEBUG(AQHBCI_LOGDOMAIN, "Checking whether \"%s\" is a BPD job", GWEN_DB_GroupName(dbRd));
      segver=GWEN_DB_GetIntValue(dbRd, "head/version", 0, 0);
      /* get segment description (first try id, then code) */
      bpdn=GWEN_MsgEngine_FindNodeByProperty(msgEngine, "SEG", "id", segver, GWEN_DB_GroupName(dbRd));
      if (!bpdn)
        bpdn=GWEN_MsgEngine_FindNodeByProperty(msgEngine, "SEG", "code", segver, GWEN_DB_GroupName(dbRd));
      if (bpdn) {
        DBG_DEBUG(AQHBCI_LOGDOMAIN, "Found a candidate");
        if (atoi(GWEN_XMLNode_GetProperty(bpdn, "isbpdjob", "0"))) {
          /* segment contains a BPD job, move contents */
          GWEN_DB_NODE *bn;
          char numbuffer[32];

          DBG_NOTICE(AQHBCI_LOGDOMAIN, "Found BPD job \"%s\"", GWEN_DB_GroupName(dbRd));
          bn=AH_Bpd_GetBpdJobs(bpd, GWEN_MsgEngine_GetProtocolVersion(msgEngine));
          assert(bn);
          bn=GWEN_DB_GetGroup(bn, GWEN_DB_FLAGS_DEFAULT,
                              GWEN_DB_GroupName(dbRd));
          assert(bn);

          if (GWEN_Text_NumToString(segver, numbuffer, sizeof(numbuffer)-1, 0)<1) {
            DBG_NOTICE(AQHBCI_LOGDOMAIN, "Buffer too small");
            abort();
          }
          bn=GWEN_DB_GetGroup(bn, GWEN_DB_FLAGS_OVERWRITE_GROUPS, numbuffer);
          assert(bn);

          GWEN_DB_AddGroupChildren(bn, dbRd);
          /* remove "head" and "segment" group */
          GWEN_DB_DeleteGroup(bn, "head");
          GWEN_DB_DeleteGroup(bn, "segment");
          DBG_DEBUG(AQHBCI_LOGDOMAIN, "Added BPD Job %s:%d", GWEN_DB_GroupName(dbRd), segver);
        } /* if isbpdjob */
        else {
          DBG_DEBUG(AQHBCI_LOGDOMAIN,
                    "Segment \"%s\" is known but not as a BPD job",
                    GWEN_DB_GroupName(dbRd));
        }
      } /* if segment found */
      else {
        DBG_WARN(AQHBCI_LOGDOMAIN, "Did not find segment \"%s\" (%d) ignoring",
                 GWEN_DB_GroupName(dbRd), segver);
      }
    }
    n=GWEN_DB_GetNextGroup(n);
  } /* while */


  /* set BPD */
  AH_User_SetBpd(j->user, bpd);
  return 0;
}


int AH_Job__VerifiyInitialKey(AB_USER *u,
                              AH_HBCI *h,
                              GWEN_CRYPT_KEY *key,
                              uint16_t sentModl,
                              const char *keyName)
{

  GWEN_CRYPT_TOKEN *ct;
  const GWEN_CRYPT_TOKEN_CONTEXT *ctx;
  int rv;

  /* get crypt token of signer */
  rv=AB_Banking_GetCryptToken(AH_HBCI_GetBankingApi(h),
                              AH_User_GetTokenType(u),
                              AH_User_GetTokenName(u),
                              &ct);
  if (rv) {
    DBG_INFO(AQHBCI_LOGDOMAIN,
             "Could not get crypt token for user \"%s\" (%d)",
             AB_User_GetUserId(u), rv);
    return rv;
  }

  /* open CryptToken if necessary */
  if (!GWEN_Crypt_Token_IsOpen(ct)) {
    GWEN_Crypt_Token_AddModes(ct, GWEN_CRYPT_TOKEN_MODE_DIRECT_SIGN);
    rv=GWEN_Crypt_Token_Open(ct, 0, 0);
    if (rv) {
      DBG_INFO(AQHBCI_LOGDOMAIN,
               "Could not open crypt token for user \"%s\" (%d)",
               AB_User_GetUserId(u), rv);
      return rv;
    }
  }

  /* get context and key info */
  ctx=GWEN_Crypt_Token_GetContext(ct, AH_User_GetTokenContextId(u), 0);
  if (ctx==NULL) {
    DBG_INFO(AQHBCI_LOGDOMAIN,
             "Context %d not found on crypt token [%s:%s]",
             AH_User_GetTokenContextId(u),
             GWEN_Crypt_Token_GetTypeName(ct),
             GWEN_Crypt_Token_GetTokenName(ct));
    return GWEN_ERROR_NOT_FOUND;
  }

  return AH_User_VerifyInitialKey(ct, ctx, u, key, sentModl, keyName);

}


int AH_Job__CommitSystemData(AH_JOB *j, int doLock)
{
  GWEN_DB_NODE *dbCurr;
  AB_USER *u;
  AB_BANKING *ab;
  AH_HBCI *h;
  GWEN_MSGENGINE *e;
  int rv;

  DBG_NOTICE(AQHBCI_LOGDOMAIN, "Committing data");
  assert(j);
  assert(j->usage);

  u=j->user;
  assert(u);
  h=AH_Job_GetHbci(j);
  assert(h);
  ab=AH_Job_GetBankingApi(j);
  assert(ab);
  e=AH_User_GetMsgEngine(j->user);
  assert(e);

  /* GWEN_DB_Dump(j->jobResponses, 2); */

  dbCurr=GWEN_DB_GetFirstGroup(j->jobResponses);
  while (dbCurr) {
    GWEN_DB_NODE *dbRd;

    dbRd=GWEN_DB_GetGroup(dbCurr, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "data");
    if (dbRd) {
      dbRd=GWEN_DB_GetFirstGroup(dbRd);
    }
    if (dbRd) {
      DBG_NOTICE(AQHBCI_LOGDOMAIN, "Checking group \"%s\"", GWEN_DB_GroupName(dbRd));
      if (strcasecmp(GWEN_DB_GroupName(dbRd), "SegResult")==0) {
        GWEN_DB_NODE *dbRes;

        dbRes=GWEN_DB_GetFirstGroup(dbRd);
        while (dbRes) {
          if (strcasecmp(GWEN_DB_GroupName(dbRes), "result")==0) {
            int code;
            const char *text;

            code=GWEN_DB_GetIntValue(dbRes, "resultcode", 0, 0);
            text=GWEN_DB_GetCharValue(dbRes, "text", 0, 0);
            DBG_NOTICE(AQHBCI_LOGDOMAIN, "Segment result: %d (%s)", code, text?text:"<none>");
            if (code==3920) {
              int i;

              AH_User_ClearTanMethodList(u);
              for (i=0; ; i++) {
                int j;

                j=GWEN_DB_GetIntValue(dbRes, "param", i, 0);
                if (j==0)
                  break;
                DBG_NOTICE(AQHBCI_LOGDOMAIN, "Adding allowed TAN method %d", j);
                AH_User_AddTanMethod(u, j);
              } /* for */
              if (i==0) {
                /* add single step if empty list */
                DBG_INFO(AQHBCI_LOGDOMAIN, "No allowed TAN method reported, assuming 999");
                AH_User_AddTanMethod(u, 999);
              }
            }
          } /* if result */
          dbRes=GWEN_DB_GetNextGroup(dbRes);
        } /* while */
      }
      else if (strcasecmp(GWEN_DB_GroupName(dbRd), "GetKeyResponse")==0) {
        const char *keytype;

        keytype=GWEN_DB_GetCharValue(dbRd, "keyname/keytype",  0, NULL);
        if (keytype && *keytype) {
          GWEN_CRYPT_KEY *bpk;
          uint8_t *expp, *modp;
          unsigned int expl, modl;
          int keynum, keyver;
          uint16_t sentModulusLength;
          uint16_t nbits;



          int keySize;
          int verified=0;
          GWEN_CRYPT_KEY *bpsk;

          /* process received keys */
          keynum=GWEN_DB_GetIntValue(dbRd, "keyname/keynum",  0, -1);
          keyver=GWEN_DB_GetIntValue(dbRd, "keyname/keyversion",  0, -1);
          modp=(uint8_t *)GWEN_DB_GetBinValue(dbRd, "key/modulus",  0, NULL, 0, &modl);
          sentModulusLength=modl;
          DBG_DEBUG(AQHBCI_LOGDOMAIN, "Got Key with modulus length %d.", modl);
          /* skip zero bytes if any */
          while (modl && *modp==0) {
            modp++;
            modl--;
          }
          /* calc real length in bits for information purposes */
          nbits=modl*8;
          if (modl) {
            uint8_t b=*modp;
            int i;
            uint8_t mask=0x80;

            for (i=0; i<8; i++) {
              if (b & mask)
                break;
              nbits--;
              mask>>=1;
            }
          }

          /* calculate key size in bytes */
          if (modl<=96) /* could only be for RDH1, in this case we have to pad to 768 bits */
            keySize=96;
          else {
            keySize=modl;
          }
          DBG_INFO(AQHBCI_LOGDOMAIN, "Key has real modulus length %d bytes (%d bits) after skipping leading zero bits.", modl,
                   nbits);
          expp=(uint8_t *)GWEN_DB_GetBinValue(dbRd, "key/exponent", 0, NULL, 0, &expl);
          bpk=GWEN_Crypt_KeyRsa_fromModExp(keySize, modp, modl, expp, expl);
          GWEN_Crypt_Key_SetKeyNumber(bpk, keynum);
          GWEN_Crypt_Key_SetKeyVersion(bpk, keyver);

          /* check if it was already verified and saved at the signature verification stage
           * (this is implemented for RDH7 and RDH9 only at the moment) */
          bpsk=AH_User_GetBankPubSignKey(u);
          if (bpsk) {
            int hasVerifiedFlag = GWEN_Crypt_KeyRsa_GetFlags(bpsk) & GWEN_CRYPT_KEYRSA_FLAGS_ISVERIFIED ;
            if (hasVerifiedFlag == GWEN_CRYPT_KEYRSA_FLAGS_ISVERIFIED)
              verified=1;
          }

          /* commit the new key */
          if (strcasecmp(keytype, "S")==0) {



            if (verified == 0) {
              verified = AH_Job__VerifiyInitialKey(u, h, bpk, sentModulusLength, "sign");
            }

            if (verified == 1) {
              DBG_ERROR(AQHBCI_LOGDOMAIN, "Imported sign key.");
              GWEN_Crypt_KeyRsa_AddFlags(bpk, GWEN_CRYPT_KEYRSA_FLAGS_ISVERIFIED);
              AH_User_SetBankPubSignKey(u, bpk);
            }
            else {
              DBG_ERROR(AQHBCI_LOGDOMAIN, "Crypt key not imported.");
            }


          }
          else if (strcasecmp(keytype, "V")==0) {

            if (verified == 0) {
              verified = AH_Job__VerifiyInitialKey(u, h, bpk, sentModulusLength, "crypt");
            }

            if (verified == 1) {
              DBG_ERROR(AQHBCI_LOGDOMAIN, "Imported crypt key.");
              GWEN_Crypt_KeyRsa_AddFlags(bpk, GWEN_CRYPT_KEYRSA_FLAGS_ISVERIFIED);
              AH_User_SetBankPubCryptKey(u, bpk);
            }
            else {
              DBG_ERROR(AQHBCI_LOGDOMAIN, "Crypt key not imported.");
            }

          }
          else {
            char hashString[1024];
            int expPadBytes=keySize-expl;
            uint8_t *mdPtr;
            unsigned int mdSize;
            /* pad exponent to length of modulus */
            GWEN_BUFFER *keyBuffer;
            GWEN_MDIGEST *md;
            uint16_t i;
            keyBuffer=GWEN_Buffer_new(NULL, 2*keySize, 0, 0);
            GWEN_Buffer_FillWithBytes(keyBuffer, 0x0, expPadBytes);
            GWEN_Buffer_AppendBytes(keyBuffer, (const char *)expp, expl);
            GWEN_Buffer_AppendBytes(keyBuffer, (const char *)modp, keySize);
            /*SHA256*/
            md=GWEN_MDigest_Sha256_new();
            GWEN_MDigest_Begin(md);
            GWEN_MDigest_Update(md, (uint8_t *)GWEN_Buffer_GetStart(keyBuffer), 2*keySize);
            GWEN_MDigest_End(md);
            mdPtr=GWEN_MDigest_GetDigestPtr(md);
            mdSize=GWEN_MDigest_GetDigestSize(md);
            memset(hashString, 0, 1024);
            for (i=0; i<mdSize; i++)
              sprintf(hashString+3*i, "%02x ", *(mdPtr+i));
            GWEN_MDigest_free(md);
            DBG_ERROR(AQHBCI_LOGDOMAIN, "Received unknown server key: type=%s, num=%d, version=%d, hash=%s", keytype, keynum,
                      keyver, hashString);
            GWEN_Gui_ProgressLog2(0,
                                  GWEN_LoggerLevel_Warning,
                                  I18N("Received unknown server key: type=%s, num=%d, version=%d, hash=%s"),
                                  keytype, keynum, keyver, hashString);
          }
          if (bpk)
            GWEN_Crypt_Key_free(bpk);
        }
      }

      else if (strcasecmp(GWEN_DB_GroupName(dbRd), "SecurityMethods")==0) {
        GWEN_DB_NODE *dbT;

        dbT=GWEN_DB_FindFirstGroup(dbRd, "SecProfile");
        while (dbT) {
          const char *code;
          int version;

          code=GWEN_DB_GetCharValue(dbT, "code", 0, NULL);
          version=GWEN_DB_GetIntValue(dbT, "version", 0, -1);
          if (code && (version>0)) {
            DBG_ERROR(AQHBCI_LOGDOMAIN,
                      "Bank supports mode %s %d",
                      code, version);
            /* TODO: store possible modes */
          }
          dbT=GWEN_DB_FindNextGroup(dbT, "SecProfile");
        } /* while */
      }

      else if (strcasecmp(GWEN_DB_GroupName(dbRd), "UserData")==0) {
        /* UserData found */
        DBG_NOTICE(AQHBCI_LOGDOMAIN, "Found UserData");
        AH_User_SetUpdVersion(j->user, GWEN_DB_GetIntValue(dbRd, "version", 0, 0));
      }

      else if (strcasecmp(GWEN_DB_GroupName(dbRd), "BankMsg")==0) {
        const char *subject;
        const char *text;

        DBG_NOTICE(AQHBCI_LOGDOMAIN, "Found a bank message");
        GWEN_Gui_ProgressLog(0,
                             GWEN_LoggerLevel_Notice,
                             I18N("Bank message received"));
        subject=GWEN_DB_GetCharValue(dbRd, "subject", 0, "(Kein Betreff)");
        text=GWEN_DB_GetCharValue(dbRd, "text", 0, 0);
        if (subject && text) {
          AB_MESSAGE *amsg;
          GWEN_TIME *ti;

          ti=GWEN_CurrentTime();
          amsg=AB_Message_new();
          AB_Message_SetSource(amsg, AB_Message_SourceBank);
          AB_Message_SetSubject(amsg, subject);
          AB_Message_SetText(amsg, text);
          AB_Message_SetDateReceived(amsg, ti);
          GWEN_Time_free(ti);
          AB_Message_SetUserId(amsg, AB_User_GetUniqueId(u));
          AB_Message_List_Add(amsg, j->messages);

          if (1) {
            GWEN_DB_NODE *dbTmp;

            /* save message, later this will no longer be necessary */
            dbTmp=GWEN_DB_Group_new("bank message");
            GWEN_DB_SetCharValue(dbTmp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                                 "subject", subject);
            GWEN_DB_SetCharValue(dbTmp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                                 "text", text);
            if (AH_HBCI_SaveMessage(h, j->user, dbTmp)) {
              DBG_ERROR(AQHBCI_LOGDOMAIN, "Could not save this message:");
              GWEN_DB_Dump(dbTmp, 2);
            }
            GWEN_DB_Group_free(dbTmp);
          }

        } /* if subject and text given */
      } /* if bank msg */


    } /* if response data found */
    dbCurr=GWEN_DB_GetNextGroup(dbCurr);
  } /* while */

  /* try to extract bank parameter data */
  DBG_DEBUG(AQHBCI_LOGDOMAIN, "Committing BPD");
  rv=AH_Job__Commit_Bpd(j);
  if (rv<0) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* try to extract accounts */
  if (AH_Job_GetFlags(j) & AH_JOB_FLAGS_IGNOREACCOUNTS) {
    DBG_INFO(AQHBCI_LOGDOMAIN, "Ignoring possibly received accounts");
  }
  else {
    DBG_INFO(AQHBCI_LOGDOMAIN, "Committing accounts");
    rv=AH_Job__Commit_Accounts(j);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
  }



  DBG_NOTICE(AQHBCI_LOGDOMAIN, "Finished.");
  return 0;
}



int AH_Job_CommitSystemData(AH_JOB *j, int doLock)
{
  AB_USER *u;
  AB_BANKING *ab;
  AB_PROVIDER *pro;
  int rv, rv2;

  u=j->user;
  assert(u);

  ab=AH_Job_GetBankingApi(j);
  assert(ab);
  pro=AH_Job_GetProvider(j);
  assert(pro);

  /* lock user */
  if (doLock) {
    rv=AB_Provider_BeginExclUseUser(pro, u);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
  }

  /* commit data */
  rv2=AH_Job__CommitSystemData(j, doLock);

  if (doLock) {
    /* unlock user */
    rv=AB_Provider_EndExclUseUser(pro, u, 0);
    if (rv<0) {
      DBG_INFO(AQHBCI_LOGDOMAIN, "here (%d)", rv);
      AB_Provider_EndExclUseUser(pro, u, 1); /* abandon */
      return rv;
    }
  }

  return rv2;
}



void AH_Job_ReadAccountDataSeg(AB_ACCOUNT *acc, GWEN_DB_NODE *dbAccountData)
{
  int t;

  assert(acc);

  AB_Account_SetBankCode(acc, GWEN_DB_GetCharValue(dbAccountData, "bankCode", 0, 0));
  AB_Account_SetAccountNumber(acc, GWEN_DB_GetCharValue(dbAccountData, "accountId", 0, 0));
  AB_Account_SetIban(acc, GWEN_DB_GetCharValue(dbAccountData, "iban", 0, 0));
  AB_Account_SetAccountName(acc, GWEN_DB_GetCharValue(dbAccountData, "account/name", 0, 0));
  AB_Account_SetSubAccountId(acc, GWEN_DB_GetCharValue(dbAccountData, "accountsubid", 0, 0));
  AB_Account_SetOwnerName(acc, GWEN_DB_GetCharValue(dbAccountData, "name1", 0, 0));

  if (GWEN_DB_GetIntValue(dbAccountData, "head/version", 0, 1)>=4)
    /* KTV in version 2 available */
    AH_Account_AddFlags(acc, AH_BANK_FLAGS_KTV2);
  else
    AH_Account_SubFlags(acc, AH_BANK_FLAGS_KTV2);

  /* account type (from FinTS_3.0_Formals) */
  t=GWEN_DB_GetIntValue(dbAccountData, "type", 0, 1);
  if (t>=1 && t<=9)          /* Kontokorrent-/Girokonto */
    AB_Account_SetAccountType(acc, AB_AccountType_Bank);
  else if (t>=10 && t<=19)   /* Sparkonto */
    AB_Account_SetAccountType(acc, AB_AccountType_Savings);
  else if (t>=20 && t<=29)   /* Festgeldkonto/Termineinlagen */
    AB_Account_SetAccountType(acc, AB_AccountType_MoneyMarket);
  else if (t>=30 && t<=39)   /* Wertpapierdepot */
    AB_Account_SetAccountType(acc, AB_AccountType_Investment);
  else if (t>=40 && t<=49)   /* Kredit-/Darlehenskonto */
    AB_Account_SetAccountType(acc, AB_AccountType_Credit);
  else if (t>=50 && t<=59)   /* Kreditkartenkonto */
    AB_Account_SetAccountType(acc, AB_AccountType_CreditCard);
  else if (t>=60 && t<=69)   /* Fonds-Depot bei einer Kapitalanlagengesellschaft */
    AB_Account_SetAccountType(acc, AB_AccountType_Investment);
  else if (t>=70 && t<=79)   /* Bausparvertrag */
    AB_Account_SetAccountType(acc, AB_AccountType_Savings);
  else if (t>=80 && t<=89)   /* Versicherungsvertrag */
    AB_Account_SetAccountType(acc, AB_AccountType_Savings);
  else if (t>=90 && t<=99)   /* sonstige */
    AB_Account_SetAccountType(acc, AB_AccountType_Unspecified);
  else
    AB_Account_SetAccountType(acc, AB_AccountType_Unspecified);
}


