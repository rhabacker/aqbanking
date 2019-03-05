/***************************************************************************
 begin       : Mon Jan 07 2008
 copyright   : (C) 2019 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "g_acctinfotrnrs_p.h"
#include "ofxxmlctx_l.h"

#include "g_generic_l.h"
#include "g_ignore_l.h"
#include "g_status_l.h"
#include "g_acctinfors_l.h"

#include "aqbanking/i18n_l.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>




AIO_OFX_GROUP *AIO_OfxGroup_ACCTINFOTRNRS_new(const char *groupName,
                                              AIO_OFX_GROUP *parent,
                                              GWEN_XML_CONTEXT *ctx)
{
  AIO_OFX_GROUP *g;

  /* create base group */
  g=AIO_OfxGroup_Generic_new(groupName, parent, ctx);
  assert(g);

  /* set virtual functions */
  AIO_OfxGroup_SetStartTagFn(g, AIO_OfxGroup_ACCTINFOTRNRS_StartTag);

  return g;
}



int AIO_OfxGroup_ACCTINFOTRNRS_StartTag(AIO_OFX_GROUP *g,
                                        const char *tagName)
{
  AIO_OFX_GROUP *gNew=NULL;
  GWEN_XML_CONTEXT *ctx;

  assert(g);

  ctx=AIO_OfxGroup_GetXmlContext(g);

  if (strcasecmp(tagName, "STATUS")==0) {
    gNew=AIO_OfxGroup_STATUS_new(tagName, g, ctx,
                                 I18N("Status for account info request"));
  }
  else if (strcasecmp(tagName, "TRNUID")==0 ||
           strcasecmp(tagName, "CLTCOOKIE")==0) {
    /* some tags, just ignore them here */
  }
  else if (strcasecmp(tagName, "ACCTINFORS")==0) {
    gNew=AIO_OfxGroup_ACCTINFORS_new(tagName, g, ctx);
  }
  else {
    DBG_WARN(AQBANKING_LOGDOMAIN,
             "Ignoring group [%s]", tagName);
    gNew=AIO_OfxGroup_Ignore_new(tagName, g, ctx);
  }

  if (gNew) {
    AIO_OfxXmlCtx_SetCurrentGroup(ctx, gNew);
    GWEN_XmlCtx_IncDepth(ctx);
  }

  return 0;
}



