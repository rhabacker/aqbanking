/***************************************************************************
 begin       : Sun Jul 28 2019
 copyright   : (C) 2019 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/


#ifndef AQFINTS_H
#define AQFINTS_H



#define AQFINTS_LOGDOMAIN "aqfints"



typedef enum {
  AQFINTS_LimitType_Unknown=-1,
  AQFINTS_LimitType_None=0,
  AQFINTS_LimitType_JobLimit=1,
  AQFINTS_LimitType_DayLimit,
  AQFINTS_LimitType_WeekLimit,
  AQFINTS_LimitType_MonthLimit,
  AQFINTS_LimitType_TimeLimit

} AQFINTS_LIMIT_TYPE;





#endif
