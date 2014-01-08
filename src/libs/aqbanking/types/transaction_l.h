/* This file is auto-generated from "transaction.xml" by the typemaker
   tool of Gwenhywfar. 
   Do not edit this file -- all changes will be lost! */
#ifndef TRANSACTION_L_H
#define TRANSACTION_L_H

/** @page P_AB_TRANSACTION_LIB AB_Transaction (lib)
This page describes the properties of AB_TRANSACTION
This type contains all important information about transactions. All text fields are in UTF-8.<h3>Local Account Info</h3>
<p>
This group contains information about the local account.</p>
<h3>Remote Account Info</h3>
<p>
This group contains information about the remote account.</p>
<h3>Dates</h3>
<p>
</p>
<h3>Value</h3>
<p>
</p>
<h3>Info Which Is Not Supported by All Backends</h3>
<p>
<p>This group contains information which differ between backends.</p>
<p>Some of this information might not even be<b>supported</b>by every backends.</p></p>
<h3>Additional Information for Standing Orders</h3>
<p>
<p>This group contains information which is used with standing orders. It is not needed for other usage of this type.</p></p>
<h3>Additional Information for Transfers</h3>
<p>
<p>This group contains information which is used with all kinds of transfers. It is setup by the function @ref AB_Banking_GatherResponses for transfers but not used by AqBanking otherwise.</p></p>
<h3>Additional Information for Foreign Transfers</h3>
<p>
<p>This group contains information which is used with transfers to other countries in the world. It is used by backends and applications but not by AqBanking itself.</p></p>
<h3>Additional Information for Investment Transfers</h3>
<p>
<p>This group contains information which is used with investment/stock transfers. It is used by backends and applications but not by AqBanking itself.</p></p>
<h3>Additional Information for SEPA Direct Debits</h3>
<p>
<p>This group contains information which is used with SEPA transfers within the European Community. It is used by backends and applications but not by AqBanking itself.</p></p>
*/
#include "transaction.h"

#ifdef __cplusplus
extern "C" {
#endif





/** @name Local Account Info
 *
This group contains information about the local account.*/
/*@{*/









/*@}*/

/** @name Remote Account Info
 *
This group contains information about the remote account.*/
/*@{*/











/*@}*/




/** @name Dates
*/
/*@{*/


/*@}*/

/** @name Value
 *
*/
/*@{*/


/*@}*/


/** @name Info Which Is Not Supported by All Backends
 *
<p>This group contains information which differ between backends.</p>
<p>Some of this information might not even be<b>supported</b>by every backends.</p>*/
/*@{*/
















/*@}*/

/** @name Additional Information for Standing Orders
 *
<p>This group contains information which is used with standing orders. It is not needed for other usage of this type.</p>*/
/*@{*/







/*@}*/

/** @name Additional Information for Transfers
 *
<p>This group contains information which is used with all kinds of transfers. It is setup by the function @ref AB_Banking_GatherResponses for transfers but not used by AqBanking otherwise.</p>*/
/*@{*/





/*@}*/

/** @name Additional Information for Foreign Transfers
 *
<p>This group contains information which is used with transfers to other countries in the world. It is used by backends and applications but not by AqBanking itself.</p>*/
/*@{*/





/*@}*/

/** @name Additional Information for Investment Transfers
 *
<p>This group contains information which is used with investment/stock transfers. It is used by backends and applications but not by AqBanking itself.</p>*/
/*@{*/






/*@}*/

/** @name Additional Information for SEPA Direct Debits
 *
<p>This group contains information which is used with SEPA transfers within the European Community. It is used by backends and applications but not by AqBanking itself.</p>*/
/*@{*/









/*@}*/


#ifdef __cplusplus
} /* __cplusplus */
#endif


#endif /* TRANSACTION_L_H */
