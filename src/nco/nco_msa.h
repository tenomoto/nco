/* $Header: /data/zender/nco_20150216/nco/src/nco/nco_msa.h,v 1.7 2003-06-16 16:37:27 zender Exp $ */

/* Purpose: Multi-slabbing algorithm */

/* Copyright (C) 1995--2003 Charlie Zender and Henry Butowsky
   This software is distributed under the terms of the GNU General Public License
   See http://www.gnu.ai.mit.edu/copyleft/gpl.html for full license text */

/* Usage:
   #include "nco_var_utl.h" *//* Multi-slabbing algorithm */

#ifndef NCO_VAR_UTL_H
#define NCO_VAR_UTL_H

#ifdef HAVE_CONFIG_H
#include <config.h> /* Autotools tokens */
#endif /* !HAVE_CONFIG_H */

/* Standard header files */
#include <stdio.h> /* stderr, FILE, NULL, printf */
#include <limits.h> /* need LONG_MAX */

/* 3rd party vendors */
#include <netcdf.h> /* netCDF definitions */
#include "nco_netcdf.h" /* netCDF3.0 wrapper functions */

/* Personal headers */
#include "nco.h" /* NCO definitions */
#include "nco_bnr.h" /* Binary write utilities */
#include "nco_ctl.h" /* Program flow control functions */
#include "nco_mmr.h" /* Memory management */
#include "nco_prn.h" /* print format functions */
#include "nco_sng_utl.h" /* sng_ascii_trn */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

 void
 nco_cpy_var_val_mlt_lmt /* [fnc] Copy variable data from input to output file */
 (const int in_id, /* I [id] netCDF input file ID */
  const int out_id, /* I [id] netCDF output file ID */
  FILE * const fp_bnr, /* I [fl] Unformatted binary output file handle */
  const bool NCO_BNR_WRT, /* I [flg] Write binary file */
  char *var_nm, /* I [sng] Variable name */
  lmt_all_sct * const lmt_lst, /* I multi-hyperslab limits */
  int nbr_dmn_fl); /* I [nbr] Number of multi-hyperslab limits */
  
  bool /* if false then we are at the end of the slab */

  nco_msa_clc_idx
  (bool NORMALIZE,         /* Return indices of slab within the slab */
   lmt_all_sct *lmt_a,         /* I list of lmts for each dimension  */
   long *indices,          /* I/O so routine can keep track of where its at */
   lmt_sct* lmt_out,      /* O  output hyperslab */
   int *slb );             /* slab which the above limit refers to */ 
  
  void 
  nco_msa_prn_idx    /* Print multiple hyperslab indices  */
  (lmt_all_sct * lmt_lst); 
  
  void 
  nco_msa_clc_cnt    /* Calculate size of  multiple hyperslab */ 
  (lmt_all_sct *lmt_lst); 

  void
  nco_msa_wrp_splt /* Split wrapped dimensions */
  (lmt_all_sct *lmt_lst);
  
  void *
  nco_msa_rec_clc /* Multi slab algorithm (recursive routine, returns a single slab pointer */
  (int i,             /* current depth, we start at 0 */
   int imax,          /* maximium depth (i.e the number of dims in variable (does not change)*/		 
   lmt_sct **lmt,    /* limits of the current hyperslab these change as we recurse */
   lmt_all_sct **lmt_lst, /* list of limits in each dimension (this remains STATIC as we recurse */
   var_sct *var1);    /* Infor for routine to read var (should not change */
  
  long
  nco_msa_min_idx /* find min values in current and return the min value*/
  (long *current,   /* current indices */
   bool *min,       /* element true if a minimum */
   int size);       /* size of current and min */

  void             /* convert hyperlsab indices into indices relative to disk */ 
  nco_msa_ram_2_dsk( 
  long *dmn_sbs_ram,   /* Input indices */
  lmt_all_sct** lmt_mult,   /* input hyperlab limits     */
  int nbr_dmn,         /* number of dimensions */    
  long *dmn_sbs_dsk,  /* Output - indices relative to disk */
  bool FREE);        /* Free static space on last call */

  void
  nco_msa_prn_var_val   /* [fnc] Print variable data */
  (const int in_id, /* I [id] netCDF input file ID */
   const char * const var_nm, /* I [sng] Variable name */
    lmt_all_sct *  const lmt_lst, /* I [sct] Dimension limits */
   const int lmt_nbr, /* I [nbr] number of dimensions with user-specified limits */
   char * const dlm_sng, /* I [sng] User-specified delimiter string, if any */
   const bool FORTRAN_STYLE, /* I [flg] Hyperslab indices obey Fortran convention */
   const bool PRINT_DIMENSIONAL_UNITS, /* I [flg] Print units attribute, if any */
   const bool PRN_DMN_IDX_CRD_VAL); /* I [flg] Print dimension/coordinate indices/values */
  
#ifdef __cplusplus
} /* end extern "C" */
#endif /* __cplusplus */

#endif /* NCO_VAR_UTL_H */
