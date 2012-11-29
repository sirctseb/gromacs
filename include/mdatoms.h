/*
 * 
 *                This source code is part of
 * 
 *                 G   R   O   M   A   C   S
 * 
 *          GROningen MAchine for Chemical Simulations
 * 
 *                        VERSION 3.2.0
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2004, The GROMACS development team,
 * check out http://www.gromacs.org for more information.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 * 
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 * 
 * For more info, check our website at http://www.gromacs.org
 * 
 * And Hey:
 * Gromacs Runs On Most of All Computer Systems
 */

#ifndef _mdatoms_h
#define _mdatoms_h
#include "visibility.h"
#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

GMX_LIBMD_EXPORT
t_mdatoms *init_mdatoms(FILE *fp,gmx_mtop_t *mtop,gmx_bool bFreeEnergy);

GMX_LIBMD_EXPORT
void atoms2md(gmx_mtop_t *mtop,t_inputrec *ir,		     
		     int nindex,int *index,
		     int start,int homenr,
		     t_mdatoms *md);
/* This routine copies the atoms->atom struct into md.
 * If index!=NULL only the indexed atoms are copied.
 */

GMX_LIBMD_EXPORT
void update_mdatoms(t_mdatoms *md,real lambda);
/* (Re)set all the mass parameters */

#ifdef __cplusplus
}
#endif

#endif