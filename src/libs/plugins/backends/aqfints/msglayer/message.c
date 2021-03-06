/***************************************************************************
 begin       : Fri Jul 19 2019
 copyright   : (C) 2019 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "message_p.h"

#include "aqfints.h"
#include "parser/parser_xml.h"
#include "parser/parser_hbci.h"
#include "parser/parser_normalize.h"
#include "parser/parser_dbwrite.h"

#include <gwenhywfar/debug.h>

#include <inttypes.h>




AQFINTS_MESSAGE *AQFINTS_Message_new(void)
{
  AQFINTS_MESSAGE *msg;

  GWEN_NEW_OBJECT(AQFINTS_MESSAGE, msg);
  msg->signerList=AQFINTS_KeyName_List_new();
  msg->segmentList=AQFINTS_Segment_List_new();

  return msg;
}



void AQFINTS_Message_free(AQFINTS_MESSAGE *msg)
{
  if (msg) {
    free(msg->tanJobCode);
    AQFINTS_Segment_List_free(msg->segmentList);
    AQFINTS_KeyName_List_free(msg->signerList);
    AQFINTS_KeyName_free(msg->crypter);
    GWEN_FREE_OBJECT(msg);
  }
}



int AQFINTS_Message_GetMessageNumber(const AQFINTS_MESSAGE *msg)
{
  assert(msg);
  return msg->messageNumber;
}



void AQFINTS_Message_SetMessageNumber(AQFINTS_MESSAGE *msg, int v)
{
  assert(msg);
  msg->messageNumber=v;
}



int AQFINTS_Message_GetRefMessageNumber(const AQFINTS_MESSAGE *msg)
{
  assert(msg);
  return msg->refMessageNumber;
}



void AQFINTS_Message_SetRefMessageNumber(AQFINTS_MESSAGE *msg, int v)
{
  assert(msg);
  msg->refMessageNumber=v;
}



const char *AQFINTS_Message_GetTanJobCode(const AQFINTS_MESSAGE *msg)
{
  assert(msg);
  return msg->tanJobCode;
}



void AQFINTS_Message_SetTanJobCode(AQFINTS_MESSAGE *msg, const char *s)
{
  assert(msg);
  if (msg->tanJobCode)
    free(msg->tanJobCode);
  if (s)
    msg->tanJobCode=strdup(s);
  else
    msg->tanJobCode=NULL;
}




AQFINTS_KEYNAME_LIST *AQFINTS_Message_GetSignerList(const AQFINTS_MESSAGE *msg)
{
  assert(msg);
  return msg->signerList;
}



void AQFINTS_Message_AddSigner(AQFINTS_MESSAGE *msg, AQFINTS_KEYNAME *keyName)
{
  uint32_t uid;

  assert(msg);
  uid=AQFINTS_KeyName_GetUniqueUserId(keyName);

  if (AQFINTS_Message_FindSigner(msg, uid)) {
    DBG_WARN(AQFINTS_LOGDOMAIN, "Signer %" PRIx32 "already exists, not adding", uid);
  }
  else
    AQFINTS_KeyName_List_Add(keyName, msg->signerList);
}



AQFINTS_KEYNAME *AQFINTS_Message_FindSigner(AQFINTS_MESSAGE *msg, uint32_t uniqueUserId)
{
  if (msg->signerList) {
    AQFINTS_KEYNAME *keyName;

    keyName=AQFINTS_KeyName_List_First(msg->signerList);
    while (keyName) {
      if (AQFINTS_KeyName_GetUniqueUserId(keyName)==uniqueUserId)
        return keyName;

      keyName=AQFINTS_KeyName_List_Next(keyName);
    }
  }

  return NULL;
}



AQFINTS_KEYNAME *AQFINTS_Message_GetCrypter(const AQFINTS_MESSAGE *msg)
{
  assert(msg);
  return msg->crypter;
}



void AQFINTS_Message_SetCrypter(AQFINTS_MESSAGE *msg, AQFINTS_KEYNAME *keyName)
{
  assert(msg);
  if (msg->crypter)
    AQFINTS_KeyName_free(msg->crypter);
  msg->crypter=keyName;
}



AQFINTS_SEGMENT_LIST *AQFINTS_Message_GetSegmentList(const AQFINTS_MESSAGE *msg)
{
  assert(msg);
  return msg->segmentList;
}



void AQFINTS_Message_AddSegment(AQFINTS_MESSAGE *msg, AQFINTS_SEGMENT *segment)
{
  assert(msg);
  assert(segment);
  AQFINTS_Segment_List_Add(segment, msg->segmentList);
}



void AQFINTS_Message_Reenumerate(AQFINTS_MESSAGE *msg)
{
  int segNum;
  AQFINTS_SEGMENT *segment;

  assert(msg);

  segNum=AQFINTS_KeyName_List_GetCount(msg->signerList)+1;
  segment=AQFINTS_Segment_List_First(msg->segmentList);
  while (segment) {
    AQFINTS_Segment_SetSegmentNumber(segment, segNum++);
    segment=AQFINTS_Segment_List_Next(segment);
  }
}



int AQFINTS_Message_GetFirstSegNum(const AQFINTS_MESSAGE *msg)
{
  AQFINTS_SEGMENT *segment;

  assert(msg);

  segment=AQFINTS_Segment_List_First(msg->segmentList);
  if (segment)
    return AQFINTS_Segment_GetSegmentNumber(segment);
  return 0;
}



int AQFINTS_Message_GetLastSegNum(const AQFINTS_MESSAGE *msg)
{
  AQFINTS_SEGMENT *segment;

  assert(msg);

  segment=AQFINTS_Segment_List_Last(msg->segmentList);
  if (segment)
    return AQFINTS_Segment_GetSegmentNumber(segment);
  return 0;
}



void AQFINTS_Message_WriteSegments(const AQFINTS_MESSAGE *msg, GWEN_BUFFER *msgBuffer)
{
  AQFINTS_SEGMENT *segment;

  assert(msg);

  while (segment) {
    AQFINTS_Parser_Hbci_WriteSegment(segment, msgBuffer);
    segment=AQFINTS_Segment_List_Next(segment);
  }
}




#if 0
int AQFINTS_Message_AddSegment(AQFINTS_MESSAGE *msg,
                               AQFINTS_SEGMENT *defSegment)
{
  AQFINTS_SEGMENT *segmentOut;
  int rv;

  segmentOut=AQFINTS_Segment_new();
  rv=AQFINTS_Parser_Db_WriteSegment(defSegment, segmentOut, dbData);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    AQFINTS_Segment_free(segmentOut);
    return rv;
  }
  AQFINTS_Parser_Segment_RemoveTrailingEmptyElements(segmentOut);

  AQFINTS_Parser_Hbci_WriteSegment(segmentOut, msg->buffer);

  AQFINTS_Segment_free(segmentOut);
  return 0;
}
#endif








