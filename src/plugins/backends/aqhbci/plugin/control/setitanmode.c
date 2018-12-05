/***************************************************************************
 begin       : Tue May 03 2005
 copyright   : (C) 2018 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "globals_l.h"

#include <gwenhywfar/text.h>

#include <aqhbci/user.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


int setItanMode(AB_PROVIDER *pro,
		GWEN_DB_NODE *dbArgs,
		int argc,
		char **argv) {
  GWEN_DB_NODE *db;
  uint32_t uid;
  AB_USER *u=NULL;
  int itanMethod;
  int itanMethodFunction;
  int rv;
  const GWEN_ARGS args[]={
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT, /* flags */
    GWEN_ArgsType_Int,            /* type */
    "userId",                     /* name */
    0,                            /* minnum */
    1,                            /* maxnum */
    "u",                          /* short option */
    "user",                       /* long option */
    "Specify the unique user id",    /* short description */
    "Specify the unique user id"     /* long description */
  },
  {
    GWEN_ARGS_FLAGS_HAS_ARGUMENT, 
    GWEN_ArgsType_Int,            
    "itanMethod",                 
    1,           
    1,           
    "m",         
    "method",    
    "Specify the iTAN method to use",
    "Specify the iTAN method to use"
  },
  {
    GWEN_ARGS_FLAGS_HELP | GWEN_ARGS_FLAGS_LAST, /* flags */
    GWEN_ArgsType_Int,            /* type */
    "help",                       /* name */
    0,                            /* minnum */
    0,                            /* maxnum */
    "h",                          /* short option */
    "help",                       /* long option */
    "Show this help screen",      /* short description */
    "Show this help screen"       /* long description */
  }
  };

  db=GWEN_DB_GetGroup(dbArgs, GWEN_DB_FLAGS_DEFAULT, "local");
  rv=GWEN_Args_Check(argc, argv, 1,
                     0 /*GWEN_ARGS_MODE_ALLOW_FREEPARAM*/,
                     args,
                     db);
  if (rv==GWEN_ARGS_RESULT_ERROR) {
    fprintf(stderr, "ERROR: Could not parse arguments\n");
    return 1;
  }
  else if (rv==GWEN_ARGS_RESULT_HELP) {
    GWEN_BUFFER *ubuf;

    ubuf=GWEN_Buffer_new(0, 1024, 0, 1);
    if (GWEN_Args_Usage(args, ubuf, GWEN_ArgsOutType_Txt)) {
      fprintf(stderr, "ERROR: Could not create help string\n");
      return 1;
    }
    fprintf(stderr, "%s\n", GWEN_Buffer_GetStart(ubuf));
    GWEN_Buffer_free(ubuf);
    return 0;
  }

  /* get and check params */
  itanMethod=GWEN_DB_GetIntValue(db, "itanMethod", 0, -1);
  itanMethodFunction=itanMethod % 1000;
  if (itanMethodFunction<900 || itanMethodFunction>999) {
    fprintf(stderr, "Only methods between x900 and x999 allowed\n");
    return 1;
  }


  /* doit */
  uid=(uint32_t) GWEN_DB_GetIntValue(db, "userId", 0, 0);
  if (uid==0) {
    fprintf(stderr, "ERROR: Invalid or missing unique user id\n");
    return 1;
  }

  rv=AB_Provider_GetUser(pro, uid, 1, 0, &u); /* don't unlock to allow for AH_Provider_EndExclUseUser */
  if (rv<0) {
    fprintf(stderr, "ERROR: User with id %lu not found\n", (unsigned long int) uid);
    return 2;
  }
  else {
    /* modify */
    if (!AH_User_HasTanMethod(u, itanMethodFunction)) {
      fprintf(stderr, "ERROR: iTAN method not allowed for this user (try \"getitanmodes\" first)\n");
      AB_Provider_EndExclUseUser(pro, u, 1); /* abort */
      AB_User_free(u);
      return 3;
    }
    AH_User_SetSelectedTanMethod(u, itanMethod);

    /* unlock user */
    rv=AB_Provider_EndExclUseUser(pro, u, 0);
    if (rv<0) {
      fprintf(stderr, "ERROR: Could not unlock user (%d)\n", rv);
      AB_Provider_EndExclUseUser(pro, u, 1); /* abort */
      AB_User_free(u);
      return 4;
    }
  }
  AB_User_free(u);

  return 0;
}




