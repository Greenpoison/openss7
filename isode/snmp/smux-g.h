/*****************************************************************************

 @(#) $Id$

 -----------------------------------------------------------------------------

 Copyright (c) 2001-2007  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; version 3 of the License.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program.  If not, see <http://www.gnu.org/licenses/>, or write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 -----------------------------------------------------------------------------

 Last Modified $Date$ by $Author$

 -----------------------------------------------------------------------------

 $Log$
 *****************************************************************************/

#ifndef __SNMP_SMUX_G_H__
#define __SNMP_SMUX_G_H__

#ident "@(#) $RCSfile$ $Name$($Revision$) Copyright (c) 2001-2007 OpenSS7 Corporation."

/* smux-g.h - SMUX group */

/* 
 * Header: /xtel/isode/isode/snmp/RCS/smux-g.h,v 9.0 1992/06/16 12:38:11 isode Rel
 *
 *
 * Log: smux-g.h,v
 * Revision 9.0  1992/06/16  12:38:11  isode
 * Release 8.0
 *
 */

/*
 *				  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

#include "internet.h"
#include "psap.h"

struct smuxPeer {
	struct smuxPeer *pb_forw;	/* doubly-linked list */
	struct smuxPeer *pb_back;	/* .. */

	int pb_index;			/* smuxPindex */

	int pb_fd;
	struct sockaddr_in pb_address;
	char pb_source[30];

	OID pb_identity;		/* smuxPidentity */
	char *pb_description;		/* smuxPdescription */

	PS pb_ps;
	int pb_sendCoR;

	int pb_priority;		/* minimum allowed priority */

	int pb_newstatus;		/* for setting smuxPstatus */
	int pb_invalid;
};

extern struct smuxPeer *PHead;

struct smuxTree {
	struct smuxTree *tb_forw;	/* doubly-linked list */
	struct smuxTree *tb_back;	/* .. */

#define	TB_SIZE	30			/* object instance */
	unsigned int tb_instance[TB_SIZE + 1];
	int tb_insize;

	OT tb_subtree;			/* smuxTsubtree */
	int tb_priority;		/* smuxTpriority */
	struct smuxPeer *tb_peer;	/* smuxTindex */

	struct smuxTree *tb_next;	/* linked list for ot_smux */

	int tb_newstatus;		/* for setting smuxPstatus */
	int tb_invalid;
};

extern struct smuxTree *THead;

#endif				/* __SNMP_SMUX_G_H__ */