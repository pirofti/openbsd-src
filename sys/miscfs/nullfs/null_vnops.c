/*	$OpenBSD: null_vnops.c,v 1.7 1997/10/06 20:20:31 deraadt Exp $	*/
/*	$NetBSD: null_vnops.c,v 1.7 1996/05/10 22:51:01 jtk Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * John Heidemann of the UCLA Ficus project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)null_vnops.c	8.1 (Berkeley) 6/10/93
 *
 * Ancestors:
 *	@(#)lofs_vnops.c	1.2 (Berkeley) 6/18/92
 *	Id: lofs_vnops.c,v 1.11 1992/05/30 10:05:43 jsp Exp
 *	...and...
 *	@(#)null_vnodeops.c 1.20 92/07/07 UCLA Ficus project
 */

/*
 * Null Layer
 *
 * (See mount_null(8) for more information.)
 *
 * The null layer duplicates a portion of the file system
 * name space under a new name.  In this respect, it is
 * similar to the loopback file system.  It differs from
 * the loopback fs in two respects:  it is implemented using
 * a stackable layers techniques, and it's "null-node"s stack above
 * all lower-layer vnodes, not just over directory vnodes.
 *
 * The null layer has two purposes.  First, it serves as a demonstration
 * of layering by proving a layer which does nothing.  (It actually
 * does everything the loopback file system does, which is slightly
 * more than nothing.)  Second, the null layer can serve as a prototype
 * layer.  Since it provides all necessary layer framework,
 * new file system layers can be created very easily be starting
 * with a null layer.
 *
 * The remainder of this man page examines the null layer as a basis
 * for constructing new layers.
 *
 *
 * INSTANTIATING NEW NULL LAYERS
 *
 * New null layers are created with mount_null(8).
 * Mount_null(8) takes two arguments, the pathname
 * of the lower vfs (target-pn) and the pathname where the null
 * layer will appear in the namespace (alias-pn).  After
 * the null layer is put into place, the contents
 * of target-pn subtree will be aliased under alias-pn.
 *
 *
 * OPERATION OF A NULL LAYER
 *
 * The null layer is the minimum file system layer,
 * simply bypassing all possible operations to the lower layer
 * for processing there.  The majority of its activity centers
 * on the bypass routine, though which nearly all vnode operations
 * pass.
 *
 * The bypass routine accepts arbitrary vnode operations for
 * handling by the lower layer.  It begins by examing vnode
 * operation arguments and replacing any null-nodes by their
 * lower-layer equivlants.  It then invokes the operation
 * on the lower layer.  Finally, it replaces the null-nodes
 * in the arguments and, if a vnode is return by the operation,
 * stacks a null-node on top of the returned vnode.
 *
 * Although bypass handles most operations, 
 * vop_getattr, _inactive, _reclaim, and _print are not bypassed.
 * Vop_getattr must change the fsid being returned.
 * Vop_inactive and vop_reclaim are not bypassed so that
 * they can handle freeing null-layer specific data.
 * Vop_print is not bypassed to avoid excessive debugging
 * information.
 *
 *
 * INSTANTIATING VNODE STACKS
 *
 * Mounting associates the null layer with a lower layer,
 * effect stacking two VFSes.  Vnode stacks are instead
 * created on demand as files are accessed.
 *
 * The initial mount creates a single vnode stack for the
 * root of the new null layer.  All other vnode stacks
 * are created as a result of vnode operations on
 * this or other null vnode stacks.
 *
 * New vnode stacks come into existance as a result of
 * an operation which returns a vnode.  
 * The bypass routine stacks a null-node above the new
 * vnode before returning it to the caller.
 *
 * For example, imagine mounting a null layer with
 * "mount_null /usr/include /dev/layer/null".
 * Changing directory to /dev/layer/null will assign
 * the root null-node (which was created when the null layer was mounted).
 * Now consider opening "sys".  A vop_lookup would be
 * done on the root null-node.  This operation would bypass through
 * to the lower layer which would return a vnode representing 
 * the UFS "sys".  Null_bypass then builds a null-node
 * aliasing the UFS "sys" and returns this to the caller.
 * Later operations on the null-node "sys" will repeat this
 * process when constructing other vnode stacks.
 *
 *
 * CREATING OTHER FILE SYSTEM LAYERS
 *
 * One of the easiest ways to construct new file system layers is to make
 * a copy of the null layer, rename all files and variables, and
 * then begin modifing the copy.  Sed can be used to easily rename
 * all variables.
 *
 * The umap layer is an example of a layer descended from the 
 * null layer.
 *
 *
 * INVOKING OPERATIONS ON LOWER LAYERS
 *
 * There are two techniques to invoke operations on a lower layer 
 * when the operation cannot be completely bypassed.  Each method
 * is appropriate in different situations.  In both cases,
 * it is the responsibility of the aliasing layer to make
 * the operation arguments "correct" for the lower layer
 * by mapping an vnode arguments to the lower layer.
 *
 * The first approach is to call the aliasing layer's bypass routine.
 * This method is most suitable when you wish to invoke the operation
 * currently being hanldled on the lower layer.  It has the advantage
 * that the bypass routine already must do argument mapping.
 * An example of this is null_getattrs in the null layer.
 *
 * A second approach is to directly invoked vnode operations on
 * the lower layer with the VOP_OPERATIONNAME interface.
 * The advantage of this method is that it is easy to invoke
 * arbitrary operations on the lower layer.  The disadvantage
 * is that vnodes arguments must be manualy mapped.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <miscfs/nullfs/null.h>


int null_bug_bypass = 0;   /* for debugging: enables bypass printf'ing */

int	null_bypass __P((void *));
int	null_getattr __P((void *));
int	null_inactive __P((void *));
int	null_reclaim __P((void *));
int	null_print __P((void *));
int	null_strategy __P((void *));
int	null_bwrite __P((void *));
int	null_lock __P((void *));
int	null_unlock __P((void *));
int	null_islocked __P((void *));
int	null_lookup __P((void *));

/*
 * This is the 10-Apr-92 bypass routine.
 *    This version has been optimized for speed, throwing away some
 * safety checks.  It should still always work, but it's not as
 * robust to programmer errors.
 *    Define SAFETY to include some error checking code.
 *
 * In general, we map all vnodes going down and unmap them on the way back.
 * As an exception to this, vnodes can be marked "unmapped" by setting
 * the Nth bit in operation's vdesc_flags.
 *
 * Also, some BSD vnode operations have the side effect of vrele'ing
 * their arguments.  With stacking, the reference counts are held
 * by the upper node, not the lower one, so we must handle these
 * side-effects here.  This is not of concern in Sun-derived systems
 * since there are no such side-effects.
 *
 * This makes the following assumptions:
 * - only one returned vpp
 * - no INOUT vpp's (Sun's vop_open has one of these)
 * - the vnode operation vector of the first vnode should be used
 *   to determine what implementation of the op should be invoked
 * - all mapped vnodes are of our vnode-type (NEEDSWORK:
 *   problems on rmdir'ing mount points and renaming?)
 */ 
int
null_bypass(v)
	void *v;
{
	struct vop_generic_args /* {
		struct vnodeop_desc *a_desc;
		<other random data follows, presumably>
	} */ *ap = v;
	register struct vnode **this_vp_p;
	int error;
	struct vnode *old_vps[VDESC_MAX_VPS];
	struct vnode **vps_p[VDESC_MAX_VPS];
	struct vnode ***vppp;
	struct vnodeop_desc *descp = ap->a_desc;
	int reles, i;

	if (null_bug_bypass)
		printf ("null_bypass: %s\n", descp->vdesc_name);

#ifdef SAFETY
	/*
	 * We require at least one vp.
	 */
	if (descp->vdesc_vp_offsets == NULL ||
	    descp->vdesc_vp_offsets[0] == VDESC_NO_OFFSET)
		panic ("null_bypass: no vp's in map.\n");
#endif

	/*
	 * Map the vnodes going in.
	 * Later, we'll invoke the operation based on
	 * the first mapped vnode's operation vector.
	 */
	reles = descp->vdesc_flags;
	for (i = 0; i < VDESC_MAX_VPS; reles >>= 1, i++) {
		if (descp->vdesc_vp_offsets[i] == VDESC_NO_OFFSET)
			break;   /* bail out at end of list */
		vps_p[i] = this_vp_p = 
			VOPARG_OFFSETTO(struct vnode**,descp->vdesc_vp_offsets[i],ap);
		/*
		 * We're not guaranteed that any but the first vnode
		 * are of our type.  Check for and don't map any
		 * that aren't.  (We must always map first vp or vclean fails.)
		 */
		if (i && (*this_vp_p == NULLVP ||
		    (*this_vp_p)->v_op != null_vnodeop_p)) {
			old_vps[i] = NULLVP;
		} else {
			old_vps[i] = *this_vp_p;
			*(vps_p[i]) = NULLVPTOLOWERVP(*this_vp_p);
			/*
			 * XXX - Several operations have the side effect
			 * of vrele'ing their vp's.  We must account for
			 * that.  (This should go away in the future.)
			 */
			if (reles & 1)
				VREF(*this_vp_p);
		}
			
	}

	/*
	 * Call the operation on the lower layer
	 * with the modified argument structure.
	 */
	error = VCALL(*(vps_p[0]), descp->vdesc_offset, ap);

	/*
	 * Maintain the illusion of call-by-value
	 * by restoring vnodes in the argument structure
	 * to their original value.
	 */
	reles = descp->vdesc_flags;
	for (i = 0; i < VDESC_MAX_VPS; reles >>= 1, i++) {
		if (descp->vdesc_vp_offsets[i] == VDESC_NO_OFFSET)
			break;   /* bail out at end of list */
		if (old_vps[i] != NULLVP) {
			*(vps_p[i]) = old_vps[i];
			if (reles & 1) {
				/* they really vput them, so we must drop
				   our locks (but mark underneath as
				   unlocked first).
				   Beware of vnode duplication--put it once,
				   and rele the rest.  Check this 
				   by looking at our upper flag. */
			    if (VTONULL(*(vps_p[i]))->null_flags & NULL_LOCKED) {
				    VTONULL(*(vps_p[i]))->null_flags &= ~NULL_LLOCK;
				    vput(*(vps_p[i]));
			    } else
				    vrele(*(vps_p[i]));
			}
		}
	}

	/*
	 * Map the possible out-going vpp
	 * (Assumes that the lower layer always returns
	 * a VREF'ed vpp unless it gets an error.)
	 */
	if (descp->vdesc_vpp_offset != VDESC_NO_OFFSET &&
	    !(descp->vdesc_flags & VDESC_NOMAP_VPP) &&
	    !error) {
		/*
		 * XXX - even though some ops have vpp returned vp's,
		 * several ops actually vrele this before returning.
		 * We must avoid these ops.
		 * (This should go away when these ops are regularized.)
		 */
		if (descp->vdesc_flags & VDESC_VPP_WILLRELE)
			goto out;
		vppp = VOPARG_OFFSETTO(struct vnode***,
				 descp->vdesc_vpp_offset,ap);
		/*
		 * This assumes that **vppp is a locked vnode (it is always
		 * so as of this writing, NetBSD-current 1995/02/16)
		 */
		/*
		 * (don't want to lock it if being called on behalf
		 * of lookup--it plays weird locking games depending
		 * on whether or not it's looking up ".", "..", etc.
		 */
		error = null_node_create(old_vps[0]->v_mount, **vppp, *vppp,
					 descp == &vop_lookup_desc ? 0 : 1);
	}

 out:
	return (error);
}


/*
 *  We handle getattr only to change the fsid.
 */
int
null_getattr(v)
	void *v;
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	int error;
	if ((error = null_bypass(ap)) != NULL)
		return (error);
	/* Requires that arguments be restored. */
	ap->a_vap->va_fsid = ap->a_vp->v_mount->mnt_stat.f_fsid.val[0];
	return (0);
}


int
null_inactive(v)
	void *v;
{
	/*
	 * Do nothing (and _don't_ bypass).
	 * Wait to vrele lowervp until reclaim,
	 * so that until then our null_node is in the
	 * cache and reusable.
	 *
	 * NEEDSWORK: Someday, consider inactive'ing
	 * the lowervp and then trying to reactivate it
	 * with capabilities (v_id)
	 * like they do in the name lookup cache code.
	 * That's too much work for now.
	 */
	return (0);
}

int
null_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct null_node *xp = VTONULL(vp);
	struct vnode *lowervp = xp->null_lowervp;

	/*
	 * Note: in vop_reclaim, vp->v_op == dead_vnodeop_p,
	 * so we can't call VOPs on ourself.
	 */
	/* After this assignment, this node will not be re-used. */
	xp->null_lowervp = NULL;
	LIST_REMOVE(xp, null_hash);
	FREE(vp->v_data, M_TEMP);
	vp->v_data = NULL;
	vrele (lowervp);
	return (0);
}


int
null_print(v)
	void *v;
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	register struct vnode *vp = ap->a_vp;
	register struct null_node *nn = VTONULL(vp);

	printf ("\ttag VT_NULLFS, vp=%p, lowervp=%p\n", vp, NULLVPTOLOWERVP(vp));
#ifdef DIAGNOSTIC
	printf("%s%s owner pid %d retpc %p retret %p\n",
	       (nn->null_flags & NULL_LOCKED) ? "(LOCKED) " : "",
	       (nn->null_flags & NULL_LLOCK) ? "(LLOCK) " : "",
	       nn->null_pid, nn->null_lockpc, nn->null_lockpc2);
#else
	printf("%s%s\n",
	       (nn->null_flags & NULL_LOCKED) ? "(LOCKED) " : "",
	       (nn->null_flags & NULL_LLOCK) ? "(LLOCK) " : "");
#endif
	vprint("nullfs lowervp", NULLVPTOLOWERVP(vp));
	return (0);
}


/*
 * XXX - vop_strategy must be hand coded because it has no
 * vnode in its arguments.
 * This goes away with a merged VM/buffer cache.
 */
int
null_strategy(v)
	void *v;
{
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	struct buf *bp = ap->a_bp;
	int error;
	struct vnode *savedvp;

	savedvp = bp->b_vp;
	bp->b_vp = NULLVPTOLOWERVP(bp->b_vp);

	error = VOP_STRATEGY(bp);

	bp->b_vp = savedvp;

	return (error);
}


/*
 * XXX - like vop_strategy, vop_bwrite must be hand coded because it has no
 * vnode in its arguments.
 * This goes away with a merged VM/buffer cache.
 */
int
null_bwrite(v)
	void *v;
{
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	struct buf *bp = ap->a_bp;
	int error;
	struct vnode *savedvp;

	savedvp = bp->b_vp;
	bp->b_vp = NULLVPTOLOWERVP(bp->b_vp);

	error = VOP_BWRITE(bp);

	bp->b_vp = savedvp;

	return (error);
}

/*
 * We need a separate null lock routine, to avoid deadlocks at reclaim time.
 * If a process holds the lower-vnode locked when it tries to reclaim
 * the null upper-vnode, _and_ null_bypass is used as the locking operation,
 * then a process can end up locking against itself.
 * This has been observed when a null mount is set up to "tunnel" beneath a
 * union mount (that setup is useful if you still wish to be able to access
 * the non-union version of either the above or below union layer)
 */
int
null_lock(v)
	void *v;
{
	struct vop_lock_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct null_node *nn;

#ifdef NULLFS_DIAGNOSTIC
	vprint("null_lock_e", ap->a_vp);
	printf("retpc=%p, retretpc=%p\n",
	       RETURN_PC(0),
	       RETURN_PC(1));
#endif
start:
	while (vp->v_flag & VXLOCK) {
		vp->v_flag |= VXWANT;
		tsleep((caddr_t)vp, PINOD, "nulllock1", 0);
	}

	nn = VTONULL(vp);

	if ((nn->null_flags & NULL_LLOCK) == 0 &&
	    (vp->v_usecount != 0)) {
		/*
		 * only lock underlying node if we haven't locked it yet
		 * for null ops, and our refcount is nonzero.  If usecount
		 * is zero, we are probably being reclaimed so we need to
		 * keep our hands off the lower node.
		 */
		VOP_LOCK(nn->null_lowervp);
		nn->null_flags |= NULL_LLOCK;
	}

	if (nn->null_flags & NULL_LOCKED) {
#ifdef DIAGNOSTIC
		if (curproc && nn->null_pid == curproc->p_pid &&
		    nn->null_pid > -1 && curproc->p_pid > -1) {
			vprint("self-lock", vp);
			panic("null: locking against myself");
		}
#endif
		nn->null_flags |= NULL_WANTED;
		tsleep((caddr_t)nn, PINOD, "nulllock2", 0);
		goto start;
	}

#ifdef DIAGNOSTIC
	if (curproc)
		nn->null_pid = curproc->p_pid;
	else
		nn->null_pid = -1;
	nn->null_lockpc = RETURN_PC(0);
	nn->null_lockpc2 = RETURN_PC(1);
#endif

	nn->null_flags |= NULL_LOCKED;
	return (0);
}

int
null_unlock(v)
	void *v;
{
	struct vop_lock_args *ap = v;
	struct null_node *nn = VTONULL(ap->a_vp);

#ifdef NULLFS_DIAGNOSTIC
	vprint("null_unlock_e", ap->a_vp);
#endif
#ifdef DIAGNOSTIC
	if ((nn->null_flags & NULL_LOCKED) == 0) {
		vprint("null_unlock", ap->a_vp);
		panic("null: unlocking unlocked node");
	}
	if (curproc && nn->null_pid != curproc->p_pid &&
	    curproc->p_pid > -1 && nn->null_pid > -1) {
		vprint("null_unlock", ap->a_vp);
		panic("null: unlocking other process's null node");
	}
#endif
	nn->null_flags &= ~NULL_LOCKED;

	if ((nn->null_flags & NULL_LLOCK) != 0)
		VOP_UNLOCK(nn->null_lowervp);

	nn->null_flags &= ~NULL_LLOCK;
    
	if (nn->null_flags & NULL_WANTED) {
		nn->null_flags &= ~NULL_WANTED;
		wakeup((caddr_t)nn);
	}
#ifdef DIAGNOSTIC
	nn->null_pid = 0;
	nn->null_lockpc = nn->null_lockpc2 = 0;
#endif
	return (0);
}

int
null_islocked(v)
	void *v;
{
	struct vop_islocked_args *ap = v;
	return ((VTONULL(ap->a_vp)->null_flags & NULL_LOCKED) ? 1 : 0);
}

int
null_lookup(v)
	void *v;
{
	register struct vop_lookup_args /* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;
	register int error;
	register struct vnode *dvp;
	int flags = ap->a_cnp->cn_flags;

#ifdef NULLFS_DIAGNOSTIC
	printf("null_lookup: dvp=%p, name='%s'\n",
	       ap->a_dvp, ap->a_cnp->cn_nameptr);
#endif
	/*
	 * the starting dir (ap->a_dvp) comes in locked.
	 */

	/* set LOCKPARENT to hold on to it until done below */
	ap->a_cnp->cn_flags |= LOCKPARENT;
	error = null_bypass(ap);
	if (!(flags & LOCKPARENT))
		ap->a_cnp->cn_flags &= ~LOCKPARENT;

	if (error)
		/*
		 * starting dir is still locked/has been relocked
		 * on error return.
		 */
		return error;

	if (ap->a_dvp != *ap->a_vpp) {
		/*
		 * Lookup returns node locked; we mark both lower and
		 * upper nodes as locked by setting the lower lock
		 * flag (it came back locked), and then call lock to
		 * set upper lock flag & record pid, etc.  see
		 * null_node_create()
		 */
		VTONULL(*ap->a_vpp)->null_flags |= NULL_LLOCK;

		dvp = ap->a_dvp;
		if (flags & ISDOTDOT) {
			/*
			 * If we're looking up `..' and this isn't the
			 * last component, then the starting directory
			 * ("parent") is _unlocked_ as a side-effect
			 * of lookups.  This is to avoid deadlocks:
			 * lock order is always parent, child, so
			 * looking up `..'  requires dropping the lock
			 * on the starting directory.
			 */
			/* see ufs_lookup() for hairy ugly locking protocol
			   examples */
			/*
			 * underlying starting dir comes back locked if flags &
			 * LOCKPARENT (which we artificially set above) and
			 * ISLASTCN.
			 */
			if (flags & ISLASTCN) {
				VTONULL(dvp)->null_flags |= NULL_LLOCK;	/* no-op, right? */
#ifdef NULLFS_DIAGNOSTIC
				if (!VOP_ISLOCKED(VTONULL(dvp)->null_lowervp)) {
					vprint("lowerdvp not locked after lookup\n", dvp);
					panic("null_lookup not locked");
				}
#endif
			} else {
				VTONULL(dvp)->null_flags &= ~NULL_LLOCK;
#ifdef NULLFS_DIAGNOSTIC
				if (VOP_ISLOCKED(VTONULL(dvp)->null_lowervp)) {
					vprint("lowerdvp locked after lookup?\n", dvp);
					panic("null_lookup locked");
				}
#endif
			}
			/*
			 * locking order: drop lock on lower-in-tree
			 * element, then get lock on higher-in-tree
			 * element, then (if needed) re-fetch lower
			 * lock.  No need for vget() since we hold a
			 * refcount to the starting directory
			 */
			VOP_UNLOCK(dvp);
			VOP_LOCK(*ap->a_vpp);
			/*
			 * we should return our directory locked if
			 * (flags & LOCKPARENT) and (flags & ISLASTCN)
			 */
			if ((flags & LOCKPARENT) && (flags & ISLASTCN))
				VOP_LOCK(dvp);
		} else {
			/*
			 * Normal directory locking order: we hold the starting
			 * directory locked; now lock our layer of the target.
			 */
			VOP_LOCK(*ap->a_vpp);
			/*
			 * underlying starting dir comes back locked
			 * if lockparent (we set it) and no error
			 * (this leg) and ISLASTCN
			 */
			if (flags & ISLASTCN) {
				VTONULL(dvp)->null_flags |= NULL_LLOCK;	/* no op, right? */
#ifdef NULLFS_DIAGNOSTIC
				if (!VOP_ISLOCKED(VTONULL(dvp)->null_lowervp)) {
					vprint("lowerdvp not locked after lookup\n", dvp);
					panic("null_lookup not locked");
				}
#endif
			} else {
				VTONULL(dvp)->null_flags &= ~NULL_LLOCK;
#ifdef NULLFS_DIAGNOSTIC
				if (VOP_ISLOCKED(VTONULL(dvp)->null_lowervp)) {
					vprint("lowerdvp locked after lookup?\n", dvp);
					panic("null_lookup locked");
				}
#endif
			}
			/*
			 * we should return our directory unlocked if
			 * our caller didn't want the parent locked,
			 * !(flags & LOCKPARENT), or we're not at the
			 * end yet, !(flags & ISLASTCN)
			 */
			if (!(flags & LOCKPARENT) || !(flags & ISLASTCN))
				VOP_UNLOCK(dvp);
		}
	}
	return error;
}

/*
 * Global vfs data structures
 */
int (**null_vnodeop_p) __P((void *));
struct vnodeopv_entry_desc null_vnodeop_entries[] = {
	{ &vop_default_desc,	null_bypass },

	{ &vop_getattr_desc,	null_getattr },
	{ &vop_inactive_desc,	null_inactive },
	{ &vop_reclaim_desc,	null_reclaim },
	{ &vop_print_desc,	null_print },

	{ &vop_lock_desc,	null_lock },
	{ &vop_unlock_desc,	null_unlock },
	{ &vop_islocked_desc,	null_islocked },
	{ &vop_lookup_desc,	null_lookup }, /* special locking frob */

	{ &vop_strategy_desc,	null_strategy },
	{ &vop_bwrite_desc,	null_bwrite },

	{ (struct vnodeop_desc*)NULL,	(int(*) __P((void *)))NULL }
};
struct vnodeopv_desc null_vnodeop_opv_desc =
	{ &null_vnodeop_p, null_vnodeop_entries };
