/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; c-file-style: "stroustrup"; -*-
 * $Id: gauss2molprop.c,v 1.26 2009/05/20 10:48:03 spoel Exp $
 * 
 *                This source code is part of
 * 
 *                 G   R   O   M   A   C   S
 * 
 *          GROningen MAchine for Chemical Simulations
 * 
 *                        VERSION 4.0.99
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2008, The GROMACS development team,
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
 * Groningen Machine for Chemical Simulation
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "typedefs.h"
#include "maths.h"
#include "macros.h"
#include "copyrite.h"
#include "bondf.h"
#include "string2.h"
#include "smalloc.h"
#include "strdb.h"
#include "sysstuff.h"
#include "confio.h"
#include "physics.h"
#include "statutil.h"
#include "vec.h"
#include "random.h"
#include "3dview.h"
#include "txtdump.h"
#include "readinp.h"
#include "names.h"
#include "filenm.h"
#include "pbc.h"
#include "pdbio.h"
#include "gpp_atomtype.h"
#include "atomprop.h"
#include "molprop.h"
#include "molprop_util.h"
#include "molprop_xml.h"
#include "poldata.h"
#include "poldata_xml.h"

typedef struct gap {
    char *element, *method,*desc;
    real temp;
    real value;
} gap_t;

typedef struct gau_atomprop
{
    int   ngap;
    gap_t *gap;
} gau_atomprop;

typedef struct gau_atomprop *gau_atomprop_t;

static int get_lib_file(const char *db,char ***strings)
{
  FILE *in;
  char **ptr=NULL;
  char buf[STRLEN];
  int  i,nstr,maxi;

  in=libopen(db);
  
  i=maxi=0;
  while (fgets2(buf,STRLEN-1,in)) {
    if (i>=maxi) {
      maxi+=50;
      srenew(ptr,maxi);
    }
    ptr[i] = strdup(buf);
    i++;
  }
  nstr=i;
  ffclose(in);
  srenew(ptr,nstr);
  *strings=ptr;
  
  return nstr;
}

/* read composite method atom data */
gau_atomprop_t read_gauss_data(const char *fn)
{
    FILE *fp;
    char **strings=NULL,**ptr;
    int nstrings,i,j=0;
    gau_atomprop_t gaps;

    snew(gaps,1);
    nstrings = get_lib_file(fn,&strings);
    
    /* First loop over strings to deduct basic stuff */
    for(i=0; (i<nstrings); i++) 
    {
        if ( strings[i][0] == '#') {
            continue;
        } 

        ptr = split('|', strings[i]);
        if ((NULL != ptr) && 
            (NULL != ptr[0]) && (NULL != ptr[1]) &&
            (NULL != ptr[2]) && (NULL != ptr[3]) &&
            (NULL != ptr[4]))
        {
            srenew(gaps->gap,j+1); 
            gaps->gap[j].element = ptr[0];
            gaps->gap[j].method = ptr[1];
            gaps->gap[j].desc = ptr[2];
            gaps->gap[j].temp = atof(ptr[3]);
            gaps->gap[j].value = atof(ptr[4]);
            j++;
        }
        sfree(strings[i]);
    }
    sfree(strings);
    
    gaps->ngap = j;
    return gaps;
}

double gau_atomprop_get_value(gau_atomprop_t gaps,char *element,char *method,char *desc,double temp)
{
    int i;
    double v = 0;
    double ttol = 0.01; /* K */
    
    for(i=0; (i<gaps->ngap); i++) 
    {
        if ((0 == strcasecmp(gaps->gap[i].element,element)) &&
            (0 == strcasecmp(gaps->gap[i].method,method)) &&
            (0 == strcasecmp(gaps->gap[i].desc,desc)) &&
            (fabs(temp - gaps->gap[i].temp) < ttol)) 
        {
            v = gaps->gap[i].value;
            break;
        }
    }
    return v;
}

void gmx_molprop_get_atomic_energy(gmx_molprop_t mpt,gau_atomprop_t gaps,
                                   char *method,double temp,real *atom_ener,real *temp_corr)
{
    char   *atomname;
    char   desc[128];
    int    natom;
    double vm,ve,vdh;
    
    vm = ve = vdh = 0;
    sprintf(desc,"%s(0K)",method);
    while (0  < gmx_molprop_get_composition_atom(mpt,"bosque",
                                                 &atomname,&natom))
    {
        /* There are natom atoms with name atomname */
        vm  += natom*gau_atomprop_get_value(gaps,atomname,method,desc,0);
        ve  += natom*gau_atomprop_get_value(gaps,atomname,"exp","DHf(0K)",0);
        vdh += natom*gau_atomprop_get_value(gaps,atomname,"exp","H(0K)-H(298.15K)",298.15);
    }
    /* Make sure units match! */
    *atom_ener = convert2gmx(ve-vm,eg2cHartree);
    *temp_corr = convert2gmx(vdh,eg2cHartree);
}

/* Read a line from a G03/G09 composite method (G3, G4, etc) record */
int gau_comp_meth_read_line(char *line,real *temp,real *pres)
{
    char **ptr1,**ptr2;
    gmx_bool bThermResults=FALSE;
    
    ptr1 = split('=', line);
    if (NULL != ptr1[1]) 
    {
        ptr2 = split(' ',ptr1[1]);
        if (NULL != ptr2[0] && NULL != ptr1[2]) {
            *temp = atof(ptr2[0]);
            *pres = atof(ptr1[2]);
        }
    }

    return TRUE;
}

gmx_molprop_t gmx_molprop_read_log(gmx_atomprop_t aps,gmx_poldata_t pd,
                                   const char *fn,char *molnm,char *iupac,
                                   gau_atomprop_t gaps,
                                   real th_toler,real ph_toler)
{
  /* Read a gaussian log file */
  char **strings=NULL;
  char sbuf[STRLEN];
  int  nstrings,atomiccenter=-1,atomicnumber=-1;
  double ee,x,y,z,V,mass=0;
  int i,j,k,kk,zz,anumber,natom,nesp,nelprop,charge,nfitpoints=-1;
  int calcref=NOTSET,atomref=NOTSET,len,iMP;
  gmx_bool bWarnESP=FALSE;
  gmx_bool bAtomicCenter;
  gmx_molprop_t mpt;
  real mm;
  char *atomname,*ginc,*hfener,*mp2ener,*g3ener,*g4ener,*cbsener;
  char *conformation = "unknown";
  char *reference = "This Work";
  char *program=NULL,*method=NULL,*basis=NULL;
  char **ptr,**qtr;
  rvec xatom;
  rvec *esp=NULL;
  real *pot=NULL;
  char **ptr2;
  gmx_bool bThermResults=FALSE;
  real temp,pres,ezpe,etherm,comp_0K,comp_energy,comp_enthalpy,comp_free_energy,atom_ener,temp_corr;
  int status;

  nstrings = get_file(fn,&strings);
    
  /* Create new calculation */ 
  mpt = gmx_molprop_init();

  natom   = 0;
  nesp    = 0;
  nelprop = NOTSET;
  charge  = NOTSET;
  hfener  = NULL;
  mp2ener = NULL;
  g3ener  = NULL;
  g4ener  = NULL;
  cbsener = NULL;
  
  /* First loop over strings to deduct basic stuff */
  for(i=0; (i<nstrings); i++) 
  {
      if (NULL != strstr(strings[i],"Revision")) 
      {
          program = strdup(strings[i]);
          trim(program);
          len = strlen(program);
          if ((len > 0) && (program[len-1] == ',')) 
          {
              program[len-1] = '\0';
          }
      }
      else if ((NULL != strstr(strings[i],"Standard basis:")) || 
               (NULL != strstr(strings[i],"Basis read from chk"))) 
      {
          ptr = split(' ',strings[i]);
          if (NULL != ptr[2]) 
          {
              basis = strdup(ptr[2]);
          }
      }
      else if (NULL != strstr(strings[i],"Temperature=")) 
      {
          status = gau_comp_meth_read_line(strings[i],&temp,&pres);
/*          fprintf(stdout, "na gau_(): %f %f\n", temp, pres); */

          bThermResults = TRUE;
      }
      else if (NULL != strstr(strings[i],"E(ZPE)=")) 
      {
          status = gau_comp_meth_read_line(strings[i],&ezpe,&etherm);
          fprintf(stdout, "na gau_(): %f %f\n", ezpe, etherm);

          bThermResults = TRUE;
      }
      else if (NULL != strstr(strings[i],"G3(0 K)=")) 
      {
          status = gau_comp_meth_read_line(strings[i],&comp_0K,&comp_energy);
          fprintf(stdout, "na gau_(): %f %f\n", comp_0K,comp_energy);
      }
      else if (NULL != strstr(strings[i],"G3 Enthalpy=")) 
      {
          status = gau_comp_meth_read_line(strings[i],&comp_enthalpy,&comp_free_energy);
          fprintf(stdout, "na gau_(): %f %f\n", comp_enthalpy, comp_free_energy);
      }
      else if (NULL != strstr(strings[i],"GINC")) 
      {
          j = 0;
          while ((i+j<nstrings) && (strlen(strings[i+j]) > 0))
              j++;
          snew(ginc,80*(j+1));
          for(k=i; (k<i+j); k++) 
          {
              trim(strings[k]);
              strncat(ginc,strings[k],80);
          }
          i = k-1;
          ptr = split('\\',ginc);
          for(j=0; (NULL != ptr[j]); j++) 
          {
              if (NULL != strstr(ptr[j],"HF=")) 
              {
                  hfener = strdup(ptr[j]+3);
              }
              if (NULL != strstr(ptr[j],"MP2=")) 
              {
                  mp2ener = strdup(ptr[j]+4);
              }
              if (NULL != strstr(ptr[j],"G3=")) 
              {
                  g3ener = strdup(ptr[j]+3);
              }
              if (NULL != strstr(ptr[j],"G4=")) 
              {
                  g4ener = strdup(ptr[j]+3);
              }
              if (NULL != strstr(ptr[j],"CBSQB3=")) 
              {
                  cbsener = strdup(ptr[j]+7);
              }
          }
      }
      else if (NULL != strstr(strings[i],"#P")) 
      {
          ptr = split(' ',strings[i]);
          if (NULL != ptr[1]) {
              qtr = split('/',ptr[1]);
              if (NULL != qtr[0]) 
              {
                  method = strdup(qtr[0]);
              }
          }
      }
  }
  
  /* Add the calculation */
  if ((calcref == NOTSET) && (NULL != program) && (NULL != method) && (NULL != basis))
  {
      gmx_molprop_add_calculation(mpt,program,method,basis,reference,conformation,&calcref);
    
      for(i=0; (i<nstrings); i++) 
      {
          bAtomicCenter = (NULL != strstr(strings[i],"Atomic Center"));
          
          if (NULL != strstr(strings[i],"fitting atomic charges"))
          {
              if (1 == sscanf(strings[i],"%d",&kk))
              {
                  if (nfitpoints == -1)
                  {
                      nfitpoints = kk;
                  }
                  else
                  {
                      if (!bWarnESP)
                      {
                          /*   gmx_resp_warning(fn,i+1);*/
                          bWarnESP = TRUE;
                      }
                      if (kk != nfitpoints)
                          fprintf(stderr,"nfitpoints was %d, now is %d\n",
                                  nfitpoints,kk);
                      nfitpoints = kk;
                  }
              }
          }
          else if (NULL != strstr(strings[i],"Stoichiometry"))
          {
              if (1 == sscanf(strings[i],"%*s%s",sbuf))
              {
                  gmx_molprop_set_formula(mpt,sbuf);
                  if (NULL == molnm)
                      gmx_molprop_set_molname(mpt,sbuf);
                  else
                      gmx_molprop_set_molname(mpt,molnm);
                  if (NULL == iupac)
                      gmx_molprop_set_iupac(mpt,sbuf);
                  else
                      gmx_molprop_set_iupac(mpt,iupac);
              }
          }
          else if (NULL != strstr(strings[i],"Coordinates (Angstroms)"))
          {
              atomicnumber = i;
          }
          else if ((atomicnumber >= 0) && (i >= atomicnumber+3))
          {
              if (6 == sscanf(strings[i],"%d%d%d%lf%lf%lf",
                              &k,&anumber,&kk,&x,&y,&z))
              {
                  if (natom+1 != k)
                  {
                      if (!bWarnESP)
                      {
                          /* gmx_resp_warning(fn,i+1);*/
                          bWarnESP = TRUE;
                      }
                  }
                  else
                  {
                      natom++;
                      atomname = gmx_atomprop_element(aps,anumber);
                      gmx_molprop_calc_add_atom(mpt,calcref,atomname,natom,&atomref);
                      if (TRUE == gmx_atomprop_query(aps,epropMass,"",atomname,&mm)) {
                          mass += mm;
                      }
                      gmx_molprop_calc_set_atomcoords(mpt,calcref,atomref,"pm",
                                                      100*x,100*y,100*z);
                      
                  }
              }
              else
                atomicnumber = -1;
          }
          else if (NULL != strstr(strings[i],"Charge ="))
          {
              if (1 != sscanf(strings[i],"%*s%*s%d",&charge))
                  fprintf(stderr,"Can not read the charge on line %d of file %s\n",i+1,fn);
          }
          else if (bAtomicCenter || (NULL != strstr(strings[i],"ESP Fit Center")))
          {
              if ((bAtomicCenter  && (4 != sscanf(strings[i],"%*s%*s%d%*s%*s%lf%lf%lf",&k,&x,&y,&z))) ||
                  (!bAtomicCenter && (4 != sscanf(strings[i],"%*s%*s%*s%d%*s%*s%lf%lf%lf",&k,&x,&y,&z))))
                  fprintf(stderr,"Warning something fishy on line %d of file %s\n",i+1,fn);
              else
              {
                  if (bAtomicCenter)
                  {
                      xatom[XX] = 100*x;
                      xatom[YY] = 100*y;
                      xatom[ZZ] = 100*z;
                      if (NULL != debug)
                          fprintf(debug,"Coordinates for atom %d found on line %d %8.3f  %8.3f  %8.3f\n",k,i,x,y,z);
                  }
                  if (k > nesp)
                  {
                      nesp++;
                      srenew(esp,nesp);
                  }
                  esp[k-1][XX] = 100*x;
                  esp[k-1][YY] = 100*y;
                  esp[k-1][ZZ] = 100*z;
                  if (NULL != debug)
                      fprintf(debug,"Coordinates for fit %d found on line %d\n",k,i);
              }
          }
          else if (NULL != strstr(strings[i],"Electrostatic Properties (Atomic Units)"))
          {
              if ((NOTSET != nelprop) && (!bWarnESP))
              {
                  /*gmx_resp_warning(fn,i+1);*/
                  bWarnESP = TRUE;
              }
              nelprop = i;
              snew(pot,nesp);
          }
          else if ((NOTSET != nelprop) && (i >= nelprop+6) && (i < nelprop+6+natom+nfitpoints))
          {
              if (2 != sscanf(strings[i],"%d%*s%lf",&k,&V))
                  fprintf(stderr,"Warning something fishy on line %d of file %s\n",i+1,fn);
              if ((k-1) != (i-nelprop-6))
                  fprintf(stderr,"Warning something fishy with fit center number on line %d of file %s\n",i+1,fn);
              if (k-1 >= nesp)
                  fprintf(stderr,"More potential points (%d) than fit centers (%d). Check line %d of file %s\n",
                          k,nesp,i+1,fn);
              pot[k-1] = V;
              if (NULL != debug)
                  fprintf(debug,"Potential %d found on line %d\n",k,i);
          }
          sfree(strings[i]);
      }
      if (debug)
          fprintf(debug,"Found %d atoms, %d esp points, and %d fitpoints\n",natom,nesp,nfitpoints);
      sfree(strings);
      
      if ((charge == NOTSET) || (natom == 0)) 
          gmx_fatal(FARGS,"Error reading Gaussian log file.");
      gmx_molprop_set_charge(mpt,charge);
      gmx_molprop_set_mass(mpt,mass);
      
      for(i=0; (i<nesp); i++) 
      {
          gmx_molprop_add_potential(mpt,calcref,"pm","Hartree/e",i,
                                    esp[i][XX],esp[i][YY],esp[i][ZZ],pot[i]);
      }
      sfree(pot);
      sfree(esp);
      
      /* Generate atomic composition, needed for energetics */
      generate_composition(1,&mpt,pd,aps,TRUE,th_toler,ph_toler);

      /* Add energies */
      if (NULL != g4ener)
      {
          ee = convert2gmx(atof(g4ener),eg2cHartree);
          gmx_molprop_add_energy(mpt,calcref,"G4","kJ/mol",ee,0);
      }		
      else if (NULL != g3ener)
      {
          gmx_molprop_get_atomic_energy(mpt,gaps,"G3",temp,&atom_ener,&temp_corr);
          ee = convert2gmx(atof(g3ener),eg2cHartree) + atom_ener;
          gmx_molprop_add_energy(mpt,calcref,"G3 DHf(0K)","kJ/mol",ee,0);
          ee = convert2gmx(atof(g3ener)-ezpe+etherm,eg2cHartree) + atom_ener - temp_corr;
          gmx_molprop_add_energy(mpt,calcref,"G3 DHf(298.15K)","kJ/mol",ee,0);
      }		
      else if (NULL != cbsener)
      {
          ee = convert2gmx(atof(cbsener),eg2cHartree);
          gmx_molprop_add_energy(mpt,calcref,"CBS-BQ3","kJ/mol",ee,0);
      }		
      else if (NULL != mp2ener)
      {
          ee = convert2gmx(atof(mp2ener),eg2cHartree);
          gmx_molprop_add_energy(mpt,calcref,"MP2","kJ/mol",ee,0);
      }
      else if (NULL != hfener)
      {
          ee = convert2gmx(atof(hfener),eg2cHartree);
          gmx_molprop_add_energy(mpt,calcref,"HF","kJ/mol",ee,0);
      }
      
      
      
      return mpt;
  }
  else {
    fprintf(stderr,"Error reading %s\n",fn);
    for(i=0; (i<nstrings); i++) {
      sfree(strings[i]);
    }
    sfree(strings);
    return NULL;
  }

}


int main(int argc, char *argv[])
{
  static const char *desc[] = {
    "gauss2molprop reads a series of Gaussian output files, and collects",
    "useful information, and saves it to molprop file."
  };
    
  t_filenm fnm[] = {
    { efLOG, "-g03",  "gauss",  ffRDMULT },
    { efDAT, "-f",    "gaussian.dat",  ffREAD },
    { efDAT, "-o",    "molprop", ffWRITE }
  };
#define NFILE asize(fnm)
  static gmx_bool bVerbose = TRUE;
  static char *molnm=NULL,*iupac=NULL;
  static real th_toler=170,ph_toler=5;
  static gmx_bool compress=FALSE;
  t_pargs pa[] = {
    { "-v",      FALSE, etBOOL, {&bVerbose},
      "Generate verbose output in the top file and on terminal." },
    { "-th_toler", FALSE, etREAL, {&th_toler},
      "HIDDENMinimum angle to be considered a linear A-B-C bond" },
    { "-ph_toler", FALSE, etREAL, {&ph_toler},
      "HIDDENMaximum angle to be considered a planar A-B-C/B-C-D torsion" },
    { "-compress", FALSE, etBOOL, {&compress},
      "Compress output XML files" },
    { "-molnm", FALSE, etSTR, {&molnm},
      "Name of the molecule in *all* input files. Do not use if you have different molecules in the input files." },
    { "-iupac", FALSE, etSTR, {&iupac},
      "IUPAC name of the molecule in *all* input files. Do not use if you have different molecules in the input files." }
  };
  output_env_t oenv;
  gmx_atomprop_t aps;
  gmx_poldata_t  pd;
  gmx_molprop_t mp,*mps=NULL;
  gau_atomprop_t gp;
  char **fns=NULL;
  int i,nmp,nfn;
  FILE *fp;
  gau_atomprop_t gaps;

  CopyRight(stdout,argv[0]);

  parse_common_args(&argc,argv,0,NFILE,fnm,asize(pa),pa,
                    asize(desc),desc,0,NULL,&oenv);
  
  /* Read standard atom properties */
  aps = gmx_atomprop_init();
  
  /* Read polarization stuff */
  if ((pd = gmx_poldata_read(NULL,aps)) == NULL)
    gmx_fatal(FARGS,"Can not read the force field information. File missing or incorrect.");
    
  gaps = read_gauss_data(opt2fn_null("-f",NFILE,fnm));

  nfn = ftp2fns(&fns,efLOG,NFILE,fnm);
  nmp = 0;
  for(i=0; (i<nfn); i++) 
  {
      mp = gmx_molprop_read_log(aps,pd,fns[i],molnm,iupac,gaps,
                                th_toler,ph_toler);
      if (NULL != mp) 
      {
          srenew(mps,++nmp);
          mps[nmp-1] = mp;
      }
  }
  printf("Succesfully read %d molprops from %d Gaussian files.\n",nmp,nfn);
  if (nmp > 0)
  {
      //     gmx_molprops_write(ftp2fn(efDAT,NFILE,fnm),nmp,mps,(int)compress);
      gmx_molprops_write(opt2fn("-o",NFILE,fnm),nmp,mps,(int)compress);
  }
      
  thanx(stderr);
  
  return 0;
}