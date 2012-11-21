/*
 * Note: this file was generated by the Gromacs sse2_single kernel generator.
 *
 *                This source code is part of
 *
 *                 G   R   O   M   A   C   S
 *
 * Copyright (c) 2001-2012, The GROMACS Development Team
 *
 * Gromacs is a library for molecular simulation and trajectory analysis,
 * written by Erik Lindahl, David van der Spoel, Berk Hess, and others - for
 * a full list of developers and information, check out http://www.gromacs.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * To help fund GROMACS development, we humbly ask that you cite
 * the papers people have written on it - you can find them on the website.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include "../nb_kernel.h"
#include "types/simple.h"
#include "vec.h"
#include "nrnb.h"

#include "gmx_math_x86_sse2_single.h"
#include "kernelutil_x86_sse2_single.h"

/*
 * Gromacs nonbonded kernel:   nb_kernel_ElecRF_VdwCSTab_GeomP1P1_VF_sse2_single
 * Electrostatics interaction: ReactionField
 * VdW interaction:            CubicSplineTable
 * Geometry:                   Particle-Particle
 * Calculate force/pot:        PotentialAndForce
 */
void
nb_kernel_ElecRF_VdwCSTab_GeomP1P1_VF_sse2_single
                    (t_nblist * gmx_restrict                nlist,
                     rvec * gmx_restrict                    xx,
                     rvec * gmx_restrict                    ff,
                     t_forcerec * gmx_restrict              fr,
                     t_mdatoms * gmx_restrict               mdatoms,
                     nb_kernel_data_t * gmx_restrict        kernel_data,
                     t_nrnb * gmx_restrict                  nrnb)
{
    /* Suffixes 0,1,2,3 refer to particle indices for waters in the inner or outer loop, or 
     * just 0 for non-waters.
     * Suffixes A,B,C,D refer to j loop unrolling done with SSE, e.g. for the four different
     * jnr indices corresponding to data put in the four positions in the SIMD register.
     */
    int              i_shift_offset,i_coord_offset,outeriter,inneriter;
    int              j_index_start,j_index_end,jidx,nri,inr,ggid,iidx;
    int              jnrA,jnrB,jnrC,jnrD;
    int              j_coord_offsetA,j_coord_offsetB,j_coord_offsetC,j_coord_offsetD;
    int              *iinr,*jindex,*jjnr,*shiftidx,*gid;
    real             shX,shY,shZ,rcutoff_scalar;
    real             *shiftvec,*fshift,*x,*f;
    __m128           tx,ty,tz,fscal,rcutoff,rcutoff2,jidxall;
    int              vdwioffset0;
    __m128           ix0,iy0,iz0,fix0,fiy0,fiz0,iq0,isai0;
    int              vdwjidx0A,vdwjidx0B,vdwjidx0C,vdwjidx0D;
    __m128           jx0,jy0,jz0,fjx0,fjy0,fjz0,jq0,isaj0;
    __m128           dx00,dy00,dz00,rsq00,rinv00,rinvsq00,r00,qq00,c6_00,c12_00;
    __m128           velec,felec,velecsum,facel,crf,krf,krf2;
    real             *charge;
    int              nvdwtype;
    __m128           rinvsix,rvdw,vvdw,vvdw6,vvdw12,fvdw,fvdw6,fvdw12,vvdwsum,sh_vdw_invrcut6;
    int              *vdwtype;
    real             *vdwparam;
    __m128           one_sixth   = _mm_set1_ps(1.0/6.0);
    __m128           one_twelfth = _mm_set1_ps(1.0/12.0);
    __m128i          vfitab;
    __m128i          ifour       = _mm_set1_epi32(4);
    __m128           rt,vfeps,vftabscale,Y,F,G,H,Heps,Fp,VV,FF;
    real             *vftab;
    __m128           dummy_mask,cutoff_mask;
    __m128           signbit = _mm_castsi128_ps( _mm_set1_epi32(0x80000000) );
    __m128           one     = _mm_set1_ps(1.0);
    __m128           two     = _mm_set1_ps(2.0);
    x                = xx[0];
    f                = ff[0];

    nri              = nlist->nri;
    iinr             = nlist->iinr;
    jindex           = nlist->jindex;
    jjnr             = nlist->jjnr;
    shiftidx         = nlist->shift;
    gid              = nlist->gid;
    shiftvec         = fr->shift_vec[0];
    fshift           = fr->fshift[0];
    facel            = _mm_set1_ps(fr->epsfac);
    charge           = mdatoms->chargeA;
    krf              = _mm_set1_ps(fr->ic->k_rf);
    krf2             = _mm_set1_ps(fr->ic->k_rf*2.0);
    crf              = _mm_set1_ps(fr->ic->c_rf);
    nvdwtype         = fr->ntype;
    vdwparam         = fr->nbfp;
    vdwtype          = mdatoms->typeA;

    vftab            = kernel_data->table_vdw->data;
    vftabscale       = _mm_set1_ps(kernel_data->table_vdw->scale);

    /* Avoid stupid compiler warnings */
    jnrA = jnrB = jnrC = jnrD = 0;
    j_coord_offsetA = 0;
    j_coord_offsetB = 0;
    j_coord_offsetC = 0;
    j_coord_offsetD = 0;

    outeriter        = 0;
    inneriter        = 0;

    /* Start outer loop over neighborlists */
    for(iidx=0; iidx<nri; iidx++)
    {
        /* Load shift vector for this list */
        i_shift_offset   = DIM*shiftidx[iidx];
        shX              = shiftvec[i_shift_offset+XX];
        shY              = shiftvec[i_shift_offset+YY];
        shZ              = shiftvec[i_shift_offset+ZZ];

        /* Load limits for loop over neighbors */
        j_index_start    = jindex[iidx];
        j_index_end      = jindex[iidx+1];

        /* Get outer coordinate index */
        inr              = iinr[iidx];
        i_coord_offset   = DIM*inr;

        /* Load i particle coords and add shift vector */
        ix0              = _mm_set1_ps(shX + x[i_coord_offset+DIM*0+XX]);
        iy0              = _mm_set1_ps(shY + x[i_coord_offset+DIM*0+YY]);
        iz0              = _mm_set1_ps(shZ + x[i_coord_offset+DIM*0+ZZ]);

        fix0             = _mm_setzero_ps();
        fiy0             = _mm_setzero_ps();
        fiz0             = _mm_setzero_ps();

        /* Load parameters for i particles */
        iq0              = _mm_mul_ps(facel,_mm_load1_ps(charge+inr+0));
        vdwioffset0      = 2*nvdwtype*vdwtype[inr+0];

        /* Reset potential sums */
        velecsum         = _mm_setzero_ps();
        vvdwsum          = _mm_setzero_ps();

        /* Start inner kernel loop */
        for(jidx=j_index_start; jidx<j_index_end && jjnr[jidx+3]>=0; jidx+=4)
        {

            /* Get j neighbor index, and coordinate index */
            jnrA             = jjnr[jidx];
            jnrB             = jjnr[jidx+1];
            jnrC             = jjnr[jidx+2];
            jnrD             = jjnr[jidx+3];

            j_coord_offsetA  = DIM*jnrA;
            j_coord_offsetB  = DIM*jnrB;
            j_coord_offsetC  = DIM*jnrC;
            j_coord_offsetD  = DIM*jnrD;

            /* load j atom coordinates */
            gmx_mm_load_1rvec_4ptr_swizzle_ps(x+j_coord_offsetA,x+j_coord_offsetB,
                                              x+j_coord_offsetC,x+j_coord_offsetD,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _mm_sub_ps(ix0,jx0);
            dy00             = _mm_sub_ps(iy0,jy0);
            dz00             = _mm_sub_ps(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_mm_calc_rsq_ps(dx00,dy00,dz00);

            rinv00           = gmx_mm_invsqrt_ps(rsq00);

            rinvsq00         = _mm_mul_ps(rinv00,rinv00);

            /* Load parameters for j particles */
            jq0              = gmx_mm_load_4real_swizzle_ps(charge+jnrA+0,charge+jnrB+0,
                                                              charge+jnrC+0,charge+jnrD+0);
            vdwjidx0A        = 2*vdwtype[jnrA+0];
            vdwjidx0B        = 2*vdwtype[jnrB+0];
            vdwjidx0C        = 2*vdwtype[jnrC+0];
            vdwjidx0D        = 2*vdwtype[jnrD+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            r00              = _mm_mul_ps(rsq00,rinv00);

            /* Compute parameters for interactions between i and j atoms */
            qq00             = _mm_mul_ps(iq0,jq0);
            gmx_mm_load_4pair_swizzle_ps(vdwparam+vdwioffset0+vdwjidx0A,
                                         vdwparam+vdwioffset0+vdwjidx0B,
                                         vdwparam+vdwioffset0+vdwjidx0C,
                                         vdwparam+vdwioffset0+vdwjidx0D,
                                         &c6_00,&c12_00);

            /* Calculate table index by multiplying r with table scale and truncate to integer */
            rt               = _mm_mul_ps(r00,vftabscale);
            vfitab           = _mm_cvttps_epi32(rt);
            vfeps            = _mm_sub_ps(rt,_mm_cvtepi32_ps(vfitab));
            vfitab           = _mm_slli_epi32(vfitab,3);

            /* REACTION-FIELD ELECTROSTATICS */
            velec            = _mm_mul_ps(qq00,_mm_sub_ps(_mm_add_ps(rinv00,_mm_mul_ps(krf,rsq00)),crf));
            felec            = _mm_mul_ps(qq00,_mm_sub_ps(_mm_mul_ps(rinv00,rinvsq00),krf2));

            /* CUBIC SPLINE TABLE DISPERSION */
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            VV               = _mm_add_ps(Y,_mm_mul_ps(vfeps,Fp));
            vvdw6            = _mm_mul_ps(c6_00,VV);
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw6            = _mm_mul_ps(c6_00,FF);

            /* CUBIC SPLINE TABLE REPULSION */
            vfitab           = _mm_add_epi32(vfitab,ifour);
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            VV               = _mm_add_ps(Y,_mm_mul_ps(vfeps,Fp));
            vvdw12           = _mm_mul_ps(c12_00,VV);
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw12           = _mm_mul_ps(c12_00,FF);
            vvdw             = _mm_add_ps(vvdw12,vvdw6);
            fvdw             = _mm_xor_ps(signbit,_mm_mul_ps(_mm_add_ps(fvdw6,fvdw12),_mm_mul_ps(vftabscale,rinv00)));

            /* Update potential sum for this i atom from the interaction with this j atom. */
            velecsum         = _mm_add_ps(velecsum,velec);
            vvdwsum          = _mm_add_ps(vvdwsum,vvdw);

            fscal            = _mm_add_ps(felec,fvdw);

            /* Calculate temporary vectorial force */
            tx               = _mm_mul_ps(fscal,dx00);
            ty               = _mm_mul_ps(fscal,dy00);
            tz               = _mm_mul_ps(fscal,dz00);

            /* Update vectorial force */
            fix0             = _mm_add_ps(fix0,tx);
            fiy0             = _mm_add_ps(fiy0,ty);
            fiz0             = _mm_add_ps(fiz0,tz);

            gmx_mm_decrement_1rvec_4ptr_swizzle_ps(f+j_coord_offsetA,f+j_coord_offsetB,
                                                   f+j_coord_offsetC,f+j_coord_offsetD,
                                                   tx,ty,tz);

            /* Inner loop uses 67 flops */
        }

        if(jidx<j_index_end)
        {

            /* Get j neighbor index, and coordinate index */
            jnrA             = jjnr[jidx];
            jnrB             = jjnr[jidx+1];
            jnrC             = jjnr[jidx+2];
            jnrD             = jjnr[jidx+3];

            /* Sign of each element will be negative for non-real atoms.
             * This mask will be 0xFFFFFFFF for dummy entries and 0x0 for real ones,
             * so use it as val = _mm_andnot_ps(mask,val) to clear dummy entries.
             */
            dummy_mask = gmx_mm_castsi128_ps(_mm_cmplt_epi32(_mm_loadu_si128((const __m128i *)(jjnr+jidx)),_mm_setzero_si128()));
            jnrA       = (jnrA>=0) ? jnrA : 0;
            jnrB       = (jnrB>=0) ? jnrB : 0;
            jnrC       = (jnrC>=0) ? jnrC : 0;
            jnrD       = (jnrD>=0) ? jnrD : 0;

            j_coord_offsetA  = DIM*jnrA;
            j_coord_offsetB  = DIM*jnrB;
            j_coord_offsetC  = DIM*jnrC;
            j_coord_offsetD  = DIM*jnrD;

            /* load j atom coordinates */
            gmx_mm_load_1rvec_4ptr_swizzle_ps(x+j_coord_offsetA,x+j_coord_offsetB,
                                              x+j_coord_offsetC,x+j_coord_offsetD,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _mm_sub_ps(ix0,jx0);
            dy00             = _mm_sub_ps(iy0,jy0);
            dz00             = _mm_sub_ps(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_mm_calc_rsq_ps(dx00,dy00,dz00);

            rinv00           = gmx_mm_invsqrt_ps(rsq00);

            rinvsq00         = _mm_mul_ps(rinv00,rinv00);

            /* Load parameters for j particles */
            jq0              = gmx_mm_load_4real_swizzle_ps(charge+jnrA+0,charge+jnrB+0,
                                                              charge+jnrC+0,charge+jnrD+0);
            vdwjidx0A        = 2*vdwtype[jnrA+0];
            vdwjidx0B        = 2*vdwtype[jnrB+0];
            vdwjidx0C        = 2*vdwtype[jnrC+0];
            vdwjidx0D        = 2*vdwtype[jnrD+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            r00              = _mm_mul_ps(rsq00,rinv00);
            r00              = _mm_andnot_ps(dummy_mask,r00);

            /* Compute parameters for interactions between i and j atoms */
            qq00             = _mm_mul_ps(iq0,jq0);
            gmx_mm_load_4pair_swizzle_ps(vdwparam+vdwioffset0+vdwjidx0A,
                                         vdwparam+vdwioffset0+vdwjidx0B,
                                         vdwparam+vdwioffset0+vdwjidx0C,
                                         vdwparam+vdwioffset0+vdwjidx0D,
                                         &c6_00,&c12_00);

            /* Calculate table index by multiplying r with table scale and truncate to integer */
            rt               = _mm_mul_ps(r00,vftabscale);
            vfitab           = _mm_cvttps_epi32(rt);
            vfeps            = _mm_sub_ps(rt,_mm_cvtepi32_ps(vfitab));
            vfitab           = _mm_slli_epi32(vfitab,3);

            /* REACTION-FIELD ELECTROSTATICS */
            velec            = _mm_mul_ps(qq00,_mm_sub_ps(_mm_add_ps(rinv00,_mm_mul_ps(krf,rsq00)),crf));
            felec            = _mm_mul_ps(qq00,_mm_sub_ps(_mm_mul_ps(rinv00,rinvsq00),krf2));

            /* CUBIC SPLINE TABLE DISPERSION */
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            VV               = _mm_add_ps(Y,_mm_mul_ps(vfeps,Fp));
            vvdw6            = _mm_mul_ps(c6_00,VV);
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw6            = _mm_mul_ps(c6_00,FF);

            /* CUBIC SPLINE TABLE REPULSION */
            vfitab           = _mm_add_epi32(vfitab,ifour);
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            VV               = _mm_add_ps(Y,_mm_mul_ps(vfeps,Fp));
            vvdw12           = _mm_mul_ps(c12_00,VV);
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw12           = _mm_mul_ps(c12_00,FF);
            vvdw             = _mm_add_ps(vvdw12,vvdw6);
            fvdw             = _mm_xor_ps(signbit,_mm_mul_ps(_mm_add_ps(fvdw6,fvdw12),_mm_mul_ps(vftabscale,rinv00)));

            /* Update potential sum for this i atom from the interaction with this j atom. */
            velec            = _mm_andnot_ps(dummy_mask,velec);
            velecsum         = _mm_add_ps(velecsum,velec);
            vvdw             = _mm_andnot_ps(dummy_mask,vvdw);
            vvdwsum          = _mm_add_ps(vvdwsum,vvdw);

            fscal            = _mm_add_ps(felec,fvdw);

            fscal            = _mm_andnot_ps(dummy_mask,fscal);

            /* Calculate temporary vectorial force */
            tx               = _mm_mul_ps(fscal,dx00);
            ty               = _mm_mul_ps(fscal,dy00);
            tz               = _mm_mul_ps(fscal,dz00);

            /* Update vectorial force */
            fix0             = _mm_add_ps(fix0,tx);
            fiy0             = _mm_add_ps(fiy0,ty);
            fiz0             = _mm_add_ps(fiz0,tz);

            gmx_mm_decrement_1rvec_4ptr_swizzle_ps(f+j_coord_offsetA,f+j_coord_offsetB,
                                                   f+j_coord_offsetC,f+j_coord_offsetD,
                                                   tx,ty,tz);

            /* Inner loop uses 68 flops */
        }

        /* End of innermost loop */

        gmx_mm_update_iforce_1atom_swizzle_ps(fix0,fiy0,fiz0,
                                              f+i_coord_offset,fshift+i_shift_offset);

        ggid                        = gid[iidx];
        /* Update potential energies */
        gmx_mm_update_1pot_ps(velecsum,kernel_data->energygrp_elec+ggid);
        gmx_mm_update_1pot_ps(vvdwsum,kernel_data->energygrp_vdw+ggid);

        /* Increment number of inner iterations */
        inneriter                  += j_index_end - j_index_start;

        /* Outer loop uses 12 flops */
    }

    /* Increment number of outer iterations */
    outeriter        += nri;

    /* Update outer/inner flops */

    inc_nrnb(nrnb,eNR_NBKERNEL_ELEC_VDW_VF,outeriter*12 + inneriter*68);
}
/*
 * Gromacs nonbonded kernel:   nb_kernel_ElecRF_VdwCSTab_GeomP1P1_F_sse2_single
 * Electrostatics interaction: ReactionField
 * VdW interaction:            CubicSplineTable
 * Geometry:                   Particle-Particle
 * Calculate force/pot:        Force
 */
void
nb_kernel_ElecRF_VdwCSTab_GeomP1P1_F_sse2_single
                    (t_nblist * gmx_restrict                nlist,
                     rvec * gmx_restrict                    xx,
                     rvec * gmx_restrict                    ff,
                     t_forcerec * gmx_restrict              fr,
                     t_mdatoms * gmx_restrict               mdatoms,
                     nb_kernel_data_t * gmx_restrict        kernel_data,
                     t_nrnb * gmx_restrict                  nrnb)
{
    /* Suffixes 0,1,2,3 refer to particle indices for waters in the inner or outer loop, or 
     * just 0 for non-waters.
     * Suffixes A,B,C,D refer to j loop unrolling done with SSE, e.g. for the four different
     * jnr indices corresponding to data put in the four positions in the SIMD register.
     */
    int              i_shift_offset,i_coord_offset,outeriter,inneriter;
    int              j_index_start,j_index_end,jidx,nri,inr,ggid,iidx;
    int              jnrA,jnrB,jnrC,jnrD;
    int              j_coord_offsetA,j_coord_offsetB,j_coord_offsetC,j_coord_offsetD;
    int              *iinr,*jindex,*jjnr,*shiftidx,*gid;
    real             shX,shY,shZ,rcutoff_scalar;
    real             *shiftvec,*fshift,*x,*f;
    __m128           tx,ty,tz,fscal,rcutoff,rcutoff2,jidxall;
    int              vdwioffset0;
    __m128           ix0,iy0,iz0,fix0,fiy0,fiz0,iq0,isai0;
    int              vdwjidx0A,vdwjidx0B,vdwjidx0C,vdwjidx0D;
    __m128           jx0,jy0,jz0,fjx0,fjy0,fjz0,jq0,isaj0;
    __m128           dx00,dy00,dz00,rsq00,rinv00,rinvsq00,r00,qq00,c6_00,c12_00;
    __m128           velec,felec,velecsum,facel,crf,krf,krf2;
    real             *charge;
    int              nvdwtype;
    __m128           rinvsix,rvdw,vvdw,vvdw6,vvdw12,fvdw,fvdw6,fvdw12,vvdwsum,sh_vdw_invrcut6;
    int              *vdwtype;
    real             *vdwparam;
    __m128           one_sixth   = _mm_set1_ps(1.0/6.0);
    __m128           one_twelfth = _mm_set1_ps(1.0/12.0);
    __m128i          vfitab;
    __m128i          ifour       = _mm_set1_epi32(4);
    __m128           rt,vfeps,vftabscale,Y,F,G,H,Heps,Fp,VV,FF;
    real             *vftab;
    __m128           dummy_mask,cutoff_mask;
    __m128           signbit = _mm_castsi128_ps( _mm_set1_epi32(0x80000000) );
    __m128           one     = _mm_set1_ps(1.0);
    __m128           two     = _mm_set1_ps(2.0);
    x                = xx[0];
    f                = ff[0];

    nri              = nlist->nri;
    iinr             = nlist->iinr;
    jindex           = nlist->jindex;
    jjnr             = nlist->jjnr;
    shiftidx         = nlist->shift;
    gid              = nlist->gid;
    shiftvec         = fr->shift_vec[0];
    fshift           = fr->fshift[0];
    facel            = _mm_set1_ps(fr->epsfac);
    charge           = mdatoms->chargeA;
    krf              = _mm_set1_ps(fr->ic->k_rf);
    krf2             = _mm_set1_ps(fr->ic->k_rf*2.0);
    crf              = _mm_set1_ps(fr->ic->c_rf);
    nvdwtype         = fr->ntype;
    vdwparam         = fr->nbfp;
    vdwtype          = mdatoms->typeA;

    vftab            = kernel_data->table_vdw->data;
    vftabscale       = _mm_set1_ps(kernel_data->table_vdw->scale);

    /* Avoid stupid compiler warnings */
    jnrA = jnrB = jnrC = jnrD = 0;
    j_coord_offsetA = 0;
    j_coord_offsetB = 0;
    j_coord_offsetC = 0;
    j_coord_offsetD = 0;

    outeriter        = 0;
    inneriter        = 0;

    /* Start outer loop over neighborlists */
    for(iidx=0; iidx<nri; iidx++)
    {
        /* Load shift vector for this list */
        i_shift_offset   = DIM*shiftidx[iidx];
        shX              = shiftvec[i_shift_offset+XX];
        shY              = shiftvec[i_shift_offset+YY];
        shZ              = shiftvec[i_shift_offset+ZZ];

        /* Load limits for loop over neighbors */
        j_index_start    = jindex[iidx];
        j_index_end      = jindex[iidx+1];

        /* Get outer coordinate index */
        inr              = iinr[iidx];
        i_coord_offset   = DIM*inr;

        /* Load i particle coords and add shift vector */
        ix0              = _mm_set1_ps(shX + x[i_coord_offset+DIM*0+XX]);
        iy0              = _mm_set1_ps(shY + x[i_coord_offset+DIM*0+YY]);
        iz0              = _mm_set1_ps(shZ + x[i_coord_offset+DIM*0+ZZ]);

        fix0             = _mm_setzero_ps();
        fiy0             = _mm_setzero_ps();
        fiz0             = _mm_setzero_ps();

        /* Load parameters for i particles */
        iq0              = _mm_mul_ps(facel,_mm_load1_ps(charge+inr+0));
        vdwioffset0      = 2*nvdwtype*vdwtype[inr+0];

        /* Start inner kernel loop */
        for(jidx=j_index_start; jidx<j_index_end && jjnr[jidx+3]>=0; jidx+=4)
        {

            /* Get j neighbor index, and coordinate index */
            jnrA             = jjnr[jidx];
            jnrB             = jjnr[jidx+1];
            jnrC             = jjnr[jidx+2];
            jnrD             = jjnr[jidx+3];

            j_coord_offsetA  = DIM*jnrA;
            j_coord_offsetB  = DIM*jnrB;
            j_coord_offsetC  = DIM*jnrC;
            j_coord_offsetD  = DIM*jnrD;

            /* load j atom coordinates */
            gmx_mm_load_1rvec_4ptr_swizzle_ps(x+j_coord_offsetA,x+j_coord_offsetB,
                                              x+j_coord_offsetC,x+j_coord_offsetD,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _mm_sub_ps(ix0,jx0);
            dy00             = _mm_sub_ps(iy0,jy0);
            dz00             = _mm_sub_ps(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_mm_calc_rsq_ps(dx00,dy00,dz00);

            rinv00           = gmx_mm_invsqrt_ps(rsq00);

            rinvsq00         = _mm_mul_ps(rinv00,rinv00);

            /* Load parameters for j particles */
            jq0              = gmx_mm_load_4real_swizzle_ps(charge+jnrA+0,charge+jnrB+0,
                                                              charge+jnrC+0,charge+jnrD+0);
            vdwjidx0A        = 2*vdwtype[jnrA+0];
            vdwjidx0B        = 2*vdwtype[jnrB+0];
            vdwjidx0C        = 2*vdwtype[jnrC+0];
            vdwjidx0D        = 2*vdwtype[jnrD+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            r00              = _mm_mul_ps(rsq00,rinv00);

            /* Compute parameters for interactions between i and j atoms */
            qq00             = _mm_mul_ps(iq0,jq0);
            gmx_mm_load_4pair_swizzle_ps(vdwparam+vdwioffset0+vdwjidx0A,
                                         vdwparam+vdwioffset0+vdwjidx0B,
                                         vdwparam+vdwioffset0+vdwjidx0C,
                                         vdwparam+vdwioffset0+vdwjidx0D,
                                         &c6_00,&c12_00);

            /* Calculate table index by multiplying r with table scale and truncate to integer */
            rt               = _mm_mul_ps(r00,vftabscale);
            vfitab           = _mm_cvttps_epi32(rt);
            vfeps            = _mm_sub_ps(rt,_mm_cvtepi32_ps(vfitab));
            vfitab           = _mm_slli_epi32(vfitab,3);

            /* REACTION-FIELD ELECTROSTATICS */
            felec            = _mm_mul_ps(qq00,_mm_sub_ps(_mm_mul_ps(rinv00,rinvsq00),krf2));

            /* CUBIC SPLINE TABLE DISPERSION */
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw6            = _mm_mul_ps(c6_00,FF);

            /* CUBIC SPLINE TABLE REPULSION */
            vfitab           = _mm_add_epi32(vfitab,ifour);
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw12           = _mm_mul_ps(c12_00,FF);
            fvdw             = _mm_xor_ps(signbit,_mm_mul_ps(_mm_add_ps(fvdw6,fvdw12),_mm_mul_ps(vftabscale,rinv00)));

            fscal            = _mm_add_ps(felec,fvdw);

            /* Calculate temporary vectorial force */
            tx               = _mm_mul_ps(fscal,dx00);
            ty               = _mm_mul_ps(fscal,dy00);
            tz               = _mm_mul_ps(fscal,dz00);

            /* Update vectorial force */
            fix0             = _mm_add_ps(fix0,tx);
            fiy0             = _mm_add_ps(fiy0,ty);
            fiz0             = _mm_add_ps(fiz0,tz);

            gmx_mm_decrement_1rvec_4ptr_swizzle_ps(f+j_coord_offsetA,f+j_coord_offsetB,
                                                   f+j_coord_offsetC,f+j_coord_offsetD,
                                                   tx,ty,tz);

            /* Inner loop uses 54 flops */
        }

        if(jidx<j_index_end)
        {

            /* Get j neighbor index, and coordinate index */
            jnrA             = jjnr[jidx];
            jnrB             = jjnr[jidx+1];
            jnrC             = jjnr[jidx+2];
            jnrD             = jjnr[jidx+3];

            /* Sign of each element will be negative for non-real atoms.
             * This mask will be 0xFFFFFFFF for dummy entries and 0x0 for real ones,
             * so use it as val = _mm_andnot_ps(mask,val) to clear dummy entries.
             */
            dummy_mask = gmx_mm_castsi128_ps(_mm_cmplt_epi32(_mm_loadu_si128((const __m128i *)(jjnr+jidx)),_mm_setzero_si128()));
            jnrA       = (jnrA>=0) ? jnrA : 0;
            jnrB       = (jnrB>=0) ? jnrB : 0;
            jnrC       = (jnrC>=0) ? jnrC : 0;
            jnrD       = (jnrD>=0) ? jnrD : 0;

            j_coord_offsetA  = DIM*jnrA;
            j_coord_offsetB  = DIM*jnrB;
            j_coord_offsetC  = DIM*jnrC;
            j_coord_offsetD  = DIM*jnrD;

            /* load j atom coordinates */
            gmx_mm_load_1rvec_4ptr_swizzle_ps(x+j_coord_offsetA,x+j_coord_offsetB,
                                              x+j_coord_offsetC,x+j_coord_offsetD,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _mm_sub_ps(ix0,jx0);
            dy00             = _mm_sub_ps(iy0,jy0);
            dz00             = _mm_sub_ps(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_mm_calc_rsq_ps(dx00,dy00,dz00);

            rinv00           = gmx_mm_invsqrt_ps(rsq00);

            rinvsq00         = _mm_mul_ps(rinv00,rinv00);

            /* Load parameters for j particles */
            jq0              = gmx_mm_load_4real_swizzle_ps(charge+jnrA+0,charge+jnrB+0,
                                                              charge+jnrC+0,charge+jnrD+0);
            vdwjidx0A        = 2*vdwtype[jnrA+0];
            vdwjidx0B        = 2*vdwtype[jnrB+0];
            vdwjidx0C        = 2*vdwtype[jnrC+0];
            vdwjidx0D        = 2*vdwtype[jnrD+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            r00              = _mm_mul_ps(rsq00,rinv00);
            r00              = _mm_andnot_ps(dummy_mask,r00);

            /* Compute parameters for interactions between i and j atoms */
            qq00             = _mm_mul_ps(iq0,jq0);
            gmx_mm_load_4pair_swizzle_ps(vdwparam+vdwioffset0+vdwjidx0A,
                                         vdwparam+vdwioffset0+vdwjidx0B,
                                         vdwparam+vdwioffset0+vdwjidx0C,
                                         vdwparam+vdwioffset0+vdwjidx0D,
                                         &c6_00,&c12_00);

            /* Calculate table index by multiplying r with table scale and truncate to integer */
            rt               = _mm_mul_ps(r00,vftabscale);
            vfitab           = _mm_cvttps_epi32(rt);
            vfeps            = _mm_sub_ps(rt,_mm_cvtepi32_ps(vfitab));
            vfitab           = _mm_slli_epi32(vfitab,3);

            /* REACTION-FIELD ELECTROSTATICS */
            felec            = _mm_mul_ps(qq00,_mm_sub_ps(_mm_mul_ps(rinv00,rinvsq00),krf2));

            /* CUBIC SPLINE TABLE DISPERSION */
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw6            = _mm_mul_ps(c6_00,FF);

            /* CUBIC SPLINE TABLE REPULSION */
            vfitab           = _mm_add_epi32(vfitab,ifour);
            Y                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,0) );
            F                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,1) );
            G                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,2) );
            H                = _mm_load_ps( vftab + gmx_mm_extract_epi32(vfitab,3) );
            _MM_TRANSPOSE4_PS(Y,F,G,H);
            Heps             = _mm_mul_ps(vfeps,H);
            Fp               = _mm_add_ps(F,_mm_mul_ps(vfeps,_mm_add_ps(G,Heps)));
            FF               = _mm_add_ps(Fp,_mm_mul_ps(vfeps,_mm_add_ps(G,_mm_add_ps(Heps,Heps))));
            fvdw12           = _mm_mul_ps(c12_00,FF);
            fvdw             = _mm_xor_ps(signbit,_mm_mul_ps(_mm_add_ps(fvdw6,fvdw12),_mm_mul_ps(vftabscale,rinv00)));

            fscal            = _mm_add_ps(felec,fvdw);

            fscal            = _mm_andnot_ps(dummy_mask,fscal);

            /* Calculate temporary vectorial force */
            tx               = _mm_mul_ps(fscal,dx00);
            ty               = _mm_mul_ps(fscal,dy00);
            tz               = _mm_mul_ps(fscal,dz00);

            /* Update vectorial force */
            fix0             = _mm_add_ps(fix0,tx);
            fiy0             = _mm_add_ps(fiy0,ty);
            fiz0             = _mm_add_ps(fiz0,tz);

            gmx_mm_decrement_1rvec_4ptr_swizzle_ps(f+j_coord_offsetA,f+j_coord_offsetB,
                                                   f+j_coord_offsetC,f+j_coord_offsetD,
                                                   tx,ty,tz);

            /* Inner loop uses 55 flops */
        }

        /* End of innermost loop */

        gmx_mm_update_iforce_1atom_swizzle_ps(fix0,fiy0,fiz0,
                                              f+i_coord_offset,fshift+i_shift_offset);

        /* Increment number of inner iterations */
        inneriter                  += j_index_end - j_index_start;

        /* Outer loop uses 10 flops */
    }

    /* Increment number of outer iterations */
    outeriter        += nri;

    /* Update outer/inner flops */

    inc_nrnb(nrnb,eNR_NBKERNEL_ELEC_VDW_F,outeriter*10 + inneriter*55);
}
