/*****************************************************************************

 @(#) src/include/sys/os7/bufpool.h

 -----------------------------------------------------------------------------

 Copyright (c) 2008-2015  Monavacon Limited <http://www.monavacon.com/>
 Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Affero General Public License as published by the Free
 Software Foundation; version 3 of the License.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
 details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>, or
 write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA
 02139, USA.

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

 *****************************************************************************/

#ifndef __OS7_BUFPOOL_H__
#define __OS7_BUFPOOL_H__

/*
 *  -------------------------------------------------------------------------
 *
 *  Fast BUFPOOL buffer allocation/deallocation for ISRs.
 *
 *  -------------------------------------------------------------------------
 */

typedef struct ss7_bufpool {
	int initialized;
	mblk_t *head;
	atomic_t count;
	atomic_t reserve;
	spinlock_t lock;
} ss7_bufpool_t;

/*
 *  BUFPOOL ALLOCB
 *  -------------------------------------------------------------------------
 *  This is a fast allocation mechanism for FASTBUF sized buffers for the
 *  receive ISRs.  This increases FISU/LSSU compression performance.  If there
 *  are blocks in the pool we use one of those, otherwise we allocate as
 *  normal.
 */
/*
   assumes bufpool is locked 
 */
static __inline__ streamscall mblk_t *
__ss7_fast_allocb(struct ss7_bufpool *pool, size_t size, int prior)
{
	mblk_t *mp = NULL;

	if (size <= FASTBUF && prior == BPRI_HI) {
		if ((mp = pool->head)) {
			pool->head = mp->b_cont;
			atomic_dec(&pool->count);
			mp->b_cont = NULL;
			mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
			mp->b_datap->db_type = M_DATA;
		}
	}
	if (!mp)
		mp = allocb(size, prior);
	return (mp);
}

/* if you need local irq or bottom half suppression, do it yourself */
static __inline__ streamscall mblk_t *
ss7_fast_allocb(struct ss7_bufpool *pool, size_t size, int prior)
{
	mblk_t *mp = NULL;

	if (size <= FASTBUF && prior == BPRI_HI) {
		spin_lock(&pool->lock);
		if ((mp = pool->head)) {
			pool->head = mp->b_cont;
			spin_unlock(&pool->lock);
			atomic_dec(&pool->count);
			mp->b_cont = NULL;
			mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
			mp->b_datap->db_type = M_DATA;
		} else
			spin_unlock(&pool->lock);
	}
	if (!mp)
		mp = allocb(size, prior);
	return (mp);
}

/*
 *  BUFPOOL FREEB
 *  -------------------------------------------------------------------------
 *  This is a fast deallocation mechanism for FASTBUF sized buffers for the
 *  receive ISRs.  This increases FISU/LSSU compression performance.  If there
 *  is only one reference to the message block, we just place the block in the
 *  pool.  If there is more than one reference, we free as normal.
 */
/*
   assumes bufpool is locked 
 */
static __inline__ streamscall void
__ss7_fast_freeb(struct ss7_bufpool *pool, mblk_t *mp)
{
	if (mp->b_datap->db_ref == 1 && mp->b_datap->db_size == FASTBUF &&
	    atomic_read(&pool->count) < atomic_read(&pool->reserve)) {
		mp->b_cont = xchg(&pool->head, mp);
		atomic_inc(&pool->count);
	} else {
		/* 
		   other references, use normal free mechanism */
		freeb(mp);
	}
}

/* if you need local irq or bottom half suppression, do it yourself */
static __inline__ streamscall void
ss7_fast_freeb(struct ss7_bufpool *pool, mblk_t *mp)
{
	if (mp->b_datap->db_ref == 1 && mp->b_datap->db_size == FASTBUF &&
	    atomic_read(&pool->count) < atomic_read(&pool->reserve)) {
		spin_lock(&pool->lock);
		mp->b_cont = xchg(&pool->head, mp);
		spin_unlock(&pool->lock);
		atomic_inc(&pool->count);
	} else {
		/* 
		   other references, use normal free mechanism */
		freeb(mp);
	}
}

/*
 *  BUFPOOL FREEMSG
 *  -------------------------------------------------------------------------
 *  This is a fast deallocation mechansim for FASTBUF sized buffers for the
 *  receive ISRs.  This increases FISU/LSSU compression performance.  If there
 *  is only one reference to the message block we just place the block in the
 *  pool.  If there is more than one reference, we free as normal.
 */
/*
   assumes bufpool is locked 
 */
static __inline__ streamscall void
__ss7_fast_freemsg(struct ss7_bufpool *pool, mblk_t *mp)
{
	mblk_t *bp, *bp_next = mp;

	while ((bp = bp_next)) {
		bp_next = bp->b_cont;
		__ss7_fast_freeb(pool, bp);
	}
}

static __inline__ streamscall void
ss7_fast_freemsg(struct ss7_bufpool *pool, mblk_t *mp)
{
	mblk_t *bp, *bp_next = mp;

	while ((bp = bp_next)) {
		bp_next = bp->b_cont;
		ss7_fast_freeb(pool, bp);
	}
}

/*
 *  BUFPOOL INIT and TERM
 *  -------------------------------------------------------------------------
 *  These initialization, allocation, deallocation and termination routines
 *  must be called outside of interrupts.
 */
/*
 *  BUFPOOL INIT
 *  -----------------------------------
 *  Initialized the buffer pool for operation.
 */
/* if you need local irq or bottom half suppression, do it yourself */
static __inline__ streamscall void
ss7_bufpool_init(struct ss7_bufpool *pool)
{
	if (!pool->initialized) {
		pool->head = NULL;
		atomic_set(&pool->count, 0);
		atomic_set(&pool->reserve, 0);
#ifdef SPIN_LOCK_UNLOCKED
		pool->lock = SPIN_LOCK_UNLOCKED;
#elif defined spin_lock_init
                spin_lock_init(&pool->lock);
#else
#error cannot initialize spin lock
#endif
		pool->initialized = 1;
	} else
		swerr();
}

/*
 *  BUFPOOL RESERVE
 *  -----------------------------------
 *  Reserves n more FASTBUF sized blocks in the buffer pool and precharges
 *  those n blocks into the buffer pool.
 */
static __inline__ streamscall void
ss7_bufpool_reserve(struct ss7_bufpool *pool, int n)
{
	mblk_t *mp;

	atomic_add(n, &pool->reserve);
	/* 
	   precharge the pool */
	while (n--) {
		if (!(mp = allocb(FASTBUF, BPRI_LO)))
			break;
		ss7_fast_freemsg(pool, mp);
	}
}

/*
 *  BUFPOOL RELEASE
 *  -----------------------------------
 *  Releases reservation of n FASTBUF sized blocks.  We do not release them
 *  here, we wait for them to be used normally, or freed on termination.
 */
static __inline__ streamscall void
ss7_bufpool_release(struct ss7_bufpool *pool, int n)
{
	atomic_sub(n, &pool->reserve);
}

/*
 *  BUFPOOL TERM
 *  -----------------------------------
 *  Terminate the buffer pool and free any blocks in the pool.
 */
/* if you need local irq or bottom half suppression, do it yourself */
static __inline__ streamscall void
ss7_bufpool_term(struct ss7_bufpool *pool)
{
	spin_lock(&pool->lock);
	if (pool->initialized) {
		mblk_t *bp;

		while ((bp = pool->head)) {
			pool->head = bp->b_cont;
			freeb(bp);
		}
		atomic_set(&pool->count, 0);
		atomic_set(&pool->reserve, 0);
		pool->initialized = 0;
	}
	spin_unlock(&pool->lock);
}

#endif				/* __OS7_BUFPOOL_H__ */
