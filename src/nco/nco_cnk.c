/* $Header: /data/zender/nco_20150216/nco/src/nco/nco_cnk.c,v 1.6 2009-05-26 00:10:51 zender Exp $ */

/* Purpose: NCO utilities for chunking */

/* Copyright (C) 1995--2009 Charlie Zender
   License: GNU General Public License (GPL) Version 3
   See http://www.gnu.org/copyleft/gpl.html for full license text */

/* Usage:
   ncks -O -4 --cnk_scl=8 ~/nco/data/in.nc ~/foo.nc
   ncks -O -4 --cnk_scl=8 ${DATA}/dstmch90/dstmch90_clm.nc ~/foo.nc
   ncks -O -4 --cnk_dmn lat,64 --cnk_dmn lon,128 ${DATA}/dstmch90/dstmch90_clm.nc ~/foo.nc 
   ncks -O -4 --cnk_plc=plc_g2d --cnk_map=map_dmn_rcd_one --cnk_dmn lat,64 --cnk_dmn lon,128 ${DATA}/dstmch90/dstmch90_clm.nc ~/foo.nc */

#include "nco_cnk.h" /* Chunking */

const char * /* O [sng] Chunking map string */
nco_cnk_map_sng_get /* [fnc] Convert chunking map enum to string */
(const int nco_cnk_map) /* I [enm] Chunking map */
{
  /* Purpose: Convert chunking map enum to string */
  switch(nco_cnk_map){
  case nco_cnk_map_nil:
    return "nil";
  case nco_cnk_map_dmn:
    return "dmn";
  case nco_cnk_map_rcd_one:
    return "rcd_one";
  case nco_cnk_map_scl:
    return "scl";
  default: nco_dfl_case_cnk_map_err(); break;
  } /* end switch */
  /* Some compilers, e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_cnk_map_sng_get() */

const char * /* O [sng] Chunking policy string */
nco_cnk_plc_sng_get /* [fnc] Convert chunking policy enum to string */
(const int nco_cnk_plc) /* I [enm] Chunking policy */
{
  /* Purpose: Convert chunking policy enum to string */
  switch(nco_cnk_plc){
  case nco_cnk_plc_nil:
    return "nil";
  case nco_cnk_plc_all:
    return "all";
  case nco_cnk_plc_g2d:
    return "g2d";
  case nco_cnk_plc_g3d: 
    return "g3d";
  case nco_cnk_plc_uck:
    return "uck";
  default: nco_dfl_case_cnk_plc_err(); break;
  } /* end switch */
  /* Some compilers, e.g., SGI cc, need return statement to end non-void functions */
  return (char *)NULL;
} /* end nco_cnk_plc_sng_get() */

void 
nco_dfl_case_cnk_map_err(void) /* [fnc] Print error and exit for illegal switch(cnk_map) case */
{
  /* Purpose: Convenience routine for printing error and exiting when
     switch(cnk_map) statement receives an illegal default case

     Placing this in its own routine also has the virtue of saving many lines 
     of code since this function is used in many many switch() statements. */
  const char fnc_nm[]="nco_dfl_case_cnk_map_err()";
  (void)fprintf(stdout,"%s: ERROR switch(cnk_map) statement fell through to default case, which is unsafe. This catch-all error handler ensures all switch(cnk_map) statements are fully enumerated. Exiting...\n",fnc_nm);
  nco_err_exit(0,fnc_nm);
} /* end nco_dfl_case_cnk_map_err() */

void 
nco_dfl_case_cnk_plc_err(void) /* [fnc] Print error and exit for illegal switch(cnk_plc) case */
{
  /* Purpose: Convenience routine for printing error and exiting when
     switch(cnk_plc) statement receives an illegal default case

     Placing this in its own routine also has the virtue of saving many lines 
     of code since this function is used in many many switch() statements. */
  const char fnc_nm[]="nco_dfl_case_cnk_plc_err()";
  (void)fprintf(stdout,"%s: ERROR switch(cnk_plc) statement fell through to default case, which is unsafe. This catch-all error handler ensures all switch(cnk_plc) statements are fully enumerated. Exiting...\n",fnc_nm);
  nco_err_exit(0,fnc_nm);
} /* end nco_dfl_case_cnk_plc_err() */

cnk_sct ** /* O [sct] Structure list with user-specified chunking information */
nco_cnk_prs /* [fnc] Create chunking structures with name and chunksize elements */
(const int cnk_nbr, /* I [nbr] Number of chunksizes specified */
 CST_X_PTR_CST_PTR_CST_Y(char,cnk_arg)) /* I [sng] List of user-specified chunksizes */
{
  /* Purpose: Determine name and chunksize elements from user arguments
     Routine merely evaluates syntax of input expressions and does not 
     attempt to validate dimensions or chunksizes against input file.
     Routine based on nco_lmt_prs() */
  
  /* Valid syntax adheres to nm,cnk_sz */
  
  void nco_usg_prn(void);
  
  char **arg_lst;
  
  const char dlm_sng[]=",";
  
  cnk_sct **cnk=NULL_CEWI;
  
  int idx;
  int arg_nbr;
  
  if(cnk_nbr > 0) cnk=(cnk_sct **)nco_malloc(cnk_nbr*sizeof(cnk_sct *));
  for(idx=0;idx<cnk_nbr;idx++){
    /* Process chunksize specifications as normal text list */
    /* fxm: probably need to free arg_lst sometime... */
    arg_lst=nco_lst_prs_2D(cnk_arg[idx],dlm_sng,&arg_nbr);
    
    /* Check syntax */
    if(
       arg_nbr < 2 || /* Need more than just dimension name */
       arg_nbr > 2 || /* Too much information */
       arg_lst[0] == NULL || /* Dimension name not specified */
       False){
      (void)fprintf(stdout,"%s: ERROR in chunksize specification for dimension %s\n%s: HINT Conform request to chunksize documentation at http://nco.sf.net/nco.html#cnk\n",prg_nm_get(),cnk_arg[idx],prg_nm_get());
      nco_exit(EXIT_FAILURE);
    } /* end if */
    
    /* Initialize structure */
    /* cnk strings that are not explicitly set by user remain NULL, i.e., 
       specifying default setting will appear as if nothing at all was set.
       Hopefully, in routines that follow, branch followed when dimension has
       all default settings specified (e.g.,"-d foo,,,,") yields same answer
       as branch for which no hyperslab along that dimension was set. */
    cnk[idx]=(cnk_sct *)nco_malloc(sizeof(cnk_sct));
    cnk[idx]->nm=NULL;
    cnk[idx]->is_usr_spc_cnk=True; /* True if any part of limit is user-specified, else False */
    
    /* Fill in structure */
    cnk[idx]->nm=arg_lst[0];
    cnk[idx]->sz=strtoul(arg_lst[1],(char **)NULL,10);
    
    /* Free current pointer array to strings
       Strings themselves are untouched and will be free()'d with chunk structures 
       in nco_cnk_lst_free() */
    arg_lst=(char **)nco_free(arg_lst);
  } /* end loop over cnk structure list */
  
  return cnk;
} /* end nco_cnk_prs() */

cnk_sct ** /* O [sct] Pointer to free'd structure list */
nco_cnk_lst_free /* [fnc] Free memory associated with chunking structure list */
(cnk_sct **cnk_lst, /* I/O [sct] Chunking structure list to free */
 const int cnk_nbr) /* I [nbr] Number of chunking structures in list */
{
  /* Threads: Routine is thread safe and calls no unsafe routines */
  /* Purpose: Free all memory associated with dynamically allocated chunking structure list */
  int idx;

  for(idx=0;idx<cnk_nbr;idx++){
    cnk_lst[idx]=nco_cnk_free(cnk_lst[idx]);
  } /* end loop over idx */

  /* Free structure pointer last */
  cnk_lst=(cnk_sct **)nco_free(cnk_lst);

  return cnk_lst;
} /* end nco_cnk_lst_free() */

cnk_sct * /* O [sct] Pointer to free'd chunking structure */
nco_cnk_free /* [fnc] Free all memory associated with chunking structure */
(cnk_sct *cnk) /* I/O [sct] Chunking structure to free */
{
  /* Threads: Routine is thread safe and calls no unsafe routines */
  /* Purpose: Free all memory associated with a dynamically allocated chunking structure */
  cnk->nm=(char *)nco_free(cnk->nm);
  /* Free structure pointer last */
  cnk=(cnk_sct *)nco_free(cnk);

  return NULL;
} /* end nco_cnk_free() */

void
nco_cnk_sz_set /* [fnc] Set chunksize parameters */
(const int nc_id, /* I [id] netCDF file ID */
 const int cnk_map, /* I [enm] Chunking map */
 const int cnk_plc, /* I [enm] Chunking policy */
 const size_t cnk_sz_scl, /* I [nbr] Chunk size scalar */
 CST_X_PTR_CST_PTR_CST_Y(cnk_sct,cnk), /* I [sct] Chunking information */
 const int cnk_nbr) /* I [nbr] Number of dimensions with user-specified chunking */
{
  /* Purpose: Use chunking map and policy to determine chunksize list */
  char var_nm[NC_MAX_NAME];
  char dmn_nm[NC_MAX_NAME];

  int *dmn_id;

  int cnk_idx;
  int dmn_idx;
  int dmn_nbr; /* [nbr] Number of dimensions in variable */
  int fl_fmt; /* [enm] Input file format */
  int nbr_dmn_fl; /* [nbr] Number of dimensions in file */
  int rcd_dmn_id;
  int var_idx;
  int var_nbr;

  long dmn_sz;

  nco_bool flg_cnk=False; /* [flg] Chunking requested */

  nc_type var_typ_dsk;
  
  size_t *cnk_sz; /* [nbr] Chunksize list */
  
  if(cnk_nbr > 0 || cnk_sz_scl != 0UL || cnk_map != nco_cnk_map_nil) flg_cnk=True; /* [flg] Chunking requested */

  if(!flg_cnk) return;

  if(dbg_lvl_get() >= nco_dbg_fl) (void)fprintf(stderr,"%s: INFO Requested chunking\n",prg_nm_get());
  if(dbg_lvl_get() >= nco_dbg_scl){
    (void)fprintf(stderr,"cnk_plc: %s\n",nco_cnk_plc_sng_get(cnk_plc));
    (void)fprintf(stderr,"cnk_map: %s\n",nco_cnk_map_sng_get(cnk_map));
    (void)fprintf(stderr,"idx dmn_nm\tcnk_sz:\n");
    for(cnk_idx=0;cnk_idx<cnk_nbr;cnk_idx++) (void)fprintf(stderr,"%02d %s\t%lu\n",cnk_idx,cnk[cnk_idx]->nm,(unsigned long)cnk[cnk_idx]->sz);
  } /* endif dbg */

  /* Does output file support chunking? */
  (void)nco_inq_format(nc_id,&fl_fmt);
  if(fl_fmt != NC_FORMAT_NETCDF4){
    (void)fprintf(stderr,"%s: WARNING Output file format is %s so chunking request will be ignored\n",prg_nm_get(),nco_fmt_sng(fl_fmt));
    return;
  } /* endif dbg */

  /* Get record dimension ID */
  (void)nco_inq(nc_id,&nbr_dmn_fl,&var_nbr,(int *)NULL,&rcd_dmn_id);
  
  /* NB: Assumes variable IDs range from [0..var_nbr-1] */
  for(var_idx=0;var_idx<var_nbr;var_idx++){
    /* Get type and number of dimensions for variable */
    (void)nco_inq_var(nc_id,var_idx,var_nm,&var_typ_dsk,&dmn_nbr,(int *)NULL,(int *)NULL);
    
    /* Skip rest of loop unless policy applies to this variable */
    if(cnk_plc == nco_cnk_plc_g2d && dmn_nbr < 2) continue;
    if(cnk_plc == nco_cnk_plc_g3d && dmn_nbr < 3) continue;

    /* Allocate space to hold dimension IDs */
    dmn_id=(int *)nco_malloc(dmn_nbr*sizeof(int));
    /* Allocate space to hold chunksizes */
    cnk_sz=(size_t *)nco_malloc(dmn_nbr*sizeof(size_t));
    
    /* Get dimension IDs */
    (void)nco_inq_vardimid(nc_id,var_idx,dmn_id);
    
    for(dmn_idx=0;dmn_idx<dmn_nbr;dmn_idx++){
      /* Get dimension sizes and names */
      (void)nco_inq_dim(nc_id,dmn_id[dmn_idx],dmn_nm,&dmn_sz);
      
      /* Do we treat the record dimension specially? */
      if(cnk_map == nco_cnk_map_rcd_one){
	/* Is this a record dimension? */
	if(dmn_id[dmn_idx] == rcd_dmn_id){
	  cnk_sz[dmn_idx]=1UL;
	  /* Skip rest of loop for this dimension */
	  continue;
	} /* endif record dimension */
      } /* !nco_cnk_map_rcd_one */

      /* Non-record sizes default to cnk_sz_scl, if specified, then dimension size */
      if(cnk_sz_scl > 0UL){
	/* Propagate scalar chunksize, if specified */
	cnk_sz[dmn_idx]=cnk_sz_scl;
      }else{
	/* Dimensions not in user-specified chunksize list get default */
	cnk_sz[dmn_idx]=dmn_sz;
      } /* !cnk_sk_scl */

      /* Explicit chunk specifications override all else */
      for(cnk_idx=0;cnk_idx<cnk_nbr;cnk_idx++){
	/* Match on name not ID */
	if(!strcmp(cnk[cnk_idx]->nm,dmn_nm)){
	  cnk_sz[dmn_idx]=cnk[cnk_idx]->sz;
	  break;
	} /* end if */
      } /* end loop over cnk */

    } /* end loop over dmn */
    
    if(dbg_lvl_get() >= nco_dbg_scl){
      (void)fprintf(stderr,"idx nm\tcnk_sz:\n");
      (void)nco_inq_dim(nc_id,dmn_id[dmn_idx],dmn_nm,&dmn_sz);
      for(dmn_idx=0;dmn_idx<dmn_nbr;dmn_idx++){
	(void)nco_inq_dimname(nc_id,dmn_id[dmn_idx],dmn_nm);
	(void)fprintf(stderr,"%02d %s\t%lu\n",dmn_idx,dmn_nm,(unsigned long)cnk_sz[dmn_idx]);
      } /* end loop over dmn */
    } /* endif dbg */

    if(cnk_sz != NULL && dmn_nbr > 0) (void)nco_def_var_chunking(nc_id,var_idx,(int)NC_CHUNKED,cnk_sz);
    
  } /* end loop over var */
  
  /* fxm: free malloc's */
  return;
} /* end nco_cnk_sz_set() */

int /* O [enm] Chunking map */
nco_cnk_map_get /* [fnc] Convert user-specified chunking map to key */
(const char *nco_cnk_map_sng) /* [sng] User-specified chunking map */
{
  /* Purpose: Process ncpdq '-P' command line argument
     Convert user-specified string to chunking map
     Return nco_cnk_map_nil by default */
  const char fnc_nm[]="nco_cnk_map_get()"; /* [sng] Function name */
  char *prg_nm; /* [sng] Program name */
  prg_nm=prg_nm_get(); /* [sng] Program name */

  if(nco_cnk_map_sng == NULL){ 
    (void)fprintf(stderr,"%s: ERROR %s reports empty user-specified chunking map string %s\n",prg_nm,fnc_nm,nco_cnk_map_sng);
    nco_exit(EXIT_FAILURE);
  } /* endif */

  if(!strcmp(nco_cnk_map_sng,"nil")) return nco_cnk_map_nil;
  if(!strcmp(nco_cnk_map_sng,"cnk_map_nil")) return nco_cnk_map_nil;
  if(!strcmp(nco_cnk_map_sng,"dmn")) return nco_cnk_map_dmn;
  if(!strcmp(nco_cnk_map_sng,"cnk_map_dmn")) return nco_cnk_map_dmn;
  if(!strcmp(nco_cnk_map_sng,"rcd_one")) return nco_cnk_map_rcd_one;
  if(!strcmp(nco_cnk_map_sng,"cnk_map_rcd_one")) return nco_cnk_map_rcd_one;
  if(!strcmp(nco_cnk_map_sng,"scl")) return nco_cnk_map_scl;
  if(!strcmp(nco_cnk_map_sng,"cnk_map_scl")) return nco_cnk_map_scl;

  (void)fprintf(stderr,"%s: ERROR %s reports unknown user-specified chunking policy %s\n",prg_nm_get(),fnc_nm,nco_cnk_map_sng);
  nco_exit(EXIT_FAILURE);
  return nco_cnk_map_nil; /* Statement should not be reached */
} /* end nco_cnk_map_get() */

int /* O [enm] Chunking policy */
nco_cnk_plc_get /* [fnc] Convert user-specified chunking policy to key */
(const char *nco_cnk_plc_sng) /* [sng] User-specified chunking policy */
{
  /* Purpose: Process ncpdq '-P' command line argument
     Convert user-specified string to chunking operation type 
     Return nco_cnk_plc_nil by default */
  const char fnc_nm[]="nco_cnk_plc_get()"; /* [sng] Function name */
  char *prg_nm; /* [sng] Program name */
  prg_nm=prg_nm_get(); /* [sng] Program name */

  if(nco_cnk_plc_sng == NULL){
    if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stdout,"%s: INFO %s reports %s invoked without explicit chunking policy. Defaulting to chunking policy \"g2d\".\n",prg_nm,fnc_nm,prg_nm);
    return nco_cnk_plc_g2d;
  } /* endif */

  if(!strcmp(nco_cnk_plc_sng,"nil")) return nco_cnk_plc_nil;
  if(!strcmp(nco_cnk_plc_sng,"cnk_nil")) return nco_cnk_plc_nil;
  if(!strcmp(nco_cnk_plc_sng,"all")) return nco_cnk_plc_all;
  if(!strcmp(nco_cnk_plc_sng,"cnk_all")) return nco_cnk_plc_all;
  if(!strcmp(nco_cnk_plc_sng,"g2d")) return nco_cnk_plc_g2d;
  if(!strcmp(nco_cnk_plc_sng,"cnk_g2d")) return nco_cnk_plc_g2d;
  if(!strcmp(nco_cnk_plc_sng,"g3d")) return nco_cnk_plc_g3d;
  if(!strcmp(nco_cnk_plc_sng,"cnk_g3d")) return nco_cnk_plc_g3d;
  if(!strcmp(nco_cnk_plc_sng,"uck")) return nco_cnk_plc_uck;
  if(!strcmp(nco_cnk_plc_sng,"cnk_uck")) return nco_cnk_plc_uck;

  (void)fprintf(stderr,"%s: ERROR %s reports unknown user-specified chunking policy %s\n",prg_nm_get(),fnc_nm,nco_cnk_plc_sng);
  nco_exit(EXIT_FAILURE);
  return nco_cnk_plc_nil; /* Statement should not be reached */
} /* end nco_cnk_plc_get() */

#if 0
size_t * /* O [nbr] Chunksize array for variable */
nco_cnk_sz_get /* [fnc] Determine chunksize array */
(const int nc_id, /* I [id] netCDF file ID */
 const char * const var_nm, /* I [sng] Variable name */
 const int cnk_map, /* I [enm] Chunking map */
 const int cnk_plc, /* I [enm] Chunking policy */
 const size_t cnk_sz_scl, /* I [nbr] Chunk size scalar */
 CST_X_PTR_CST_PTR_CST_Y(cnk_sct,cnk), /* I [sct] Chunking information */
 const int cnk_nbr) /* I [nbr] Number of dimensions with user-specified chunking */
{
  /* Purpose: Use chunking map and policy to determine chunksize list */
  int dmn_nbr; /* [nbr] Number of dimensions in variable */
  int dmn_idx;
  int idx;
  int rec_dmn_id;
  
  size_t *cnk_sz; /* [nbr] Chunksize list */
  
  /* Get record dimension ID */
  (void)nco_inq(nc_id,(int *)NULL,(int *)NULL,(int *)NULL,&rec_dmn_id);

  /* Get type and number of dimensions and attributes for variable */

  cnk_sz=(size_t *)nco_malloc(dmn_nbr*sizeof(size_t));
  
  return cnk_sz;
} /* end nco_cnk_sz_get() */

nco_bool /* O [flg] NCO will attempt to chunk variable */
nco_is_chunkable /* [fnc] Will NCO attempt to chunk variable? */
(const nc_type nc_typ_in) /* I [enm] Type of input variable */
{
  /* Purpose: Determine whether NCO should attempt to chunk a given type
     Chunking certain variable types is not recommended, e.g., chunking NC_CHAR
     and NC_BYTE makes no sense, because precision would needlessly be lost.
     Routine should be consistent with nco_cnk_plc_typ_get()
     NB: Routine is deprecated in favor of more flexible nco_cnk_plc_typ_get() */
  const char fnc_nm[]="nco_is_chunkable()"; /* [sng] Function name */

  (void)fprintf(stdout,"%s: ERROR deprecated routine %s should not be called\n",prg_nm_get(),fnc_nm);
  nco_exit(EXIT_FAILURE);

  switch(nc_typ_in){ 
  case NC_FLOAT: 
  case NC_DOUBLE: 
  case NC_INT64: 
  case NC_UINT64: 
  case NC_INT: 
  case NC_UINT: 
    return True;
    break;
  case NC_SHORT: 
  case NC_USHORT: 
  case NC_CHAR: 
  case NC_BYTE: 
  case NC_UBYTE: 
  case NC_STRING:
    return False;
    break;
  default: nco_dfl_case_nc_type_err(); break;
  } /* end switch */ 

  /* Some compilers, e.g., SGI cc, need return statement to end non-void functions */
  return False;
} /* end nco_is_chunkable() */

nco_bool /* O [flg] Variable is chunked on disk */
nco_cnk_dsk_inq /* [fnc] Check whether variable is chunked on disk */
(const int nc_id, /* I [idx] netCDF file ID */
 var_sct * const var) /* I/O [sct] Variable */
{
  /* Purpose: Check whether variable is chunked on disk and set variable members 
     cnk_dsk, has_scl_fct, has_add_fst, and typ_uck accordingly
     nco_cnk_dsk_inq() should be called early in application, e.g., in nco_var_fll() 
     Call nco_cnk_dsk_inq() before copying input list to output list 
     Multi-file operators which handle chunking must call this routine prior
     to each read of a variable, in case that variable has been unchunked. */
  /* ncea -O -D 3 -v cnk ~/nco/data/in.nc ~/nco/data/foo.nc */
  
  const char add_fst_sng[]="add_offset"; /* [sng] Unidata standard string for add offset */
  const char scl_fct_sng[]="scale_factor"; /* [sng] Unidata standard string for scale factor */
  
  int rcd; /* [rcd] Return success code */
  
  long add_fst_lng; /* [idx] Number of elements in add_offset attribute */
  long scl_fct_lng; /* [idx] Number of elements in scale_factor attribute */

  nc_type add_fst_typ; /* [idx] Type of add_offset attribute */
  nc_type scl_fct_typ; /* [idx] Type of scale_factor attribute */

  /* Set some defaults in variable structure for safety in case of early return
     Flags for variables without valid scaling information should appear 
     same as flags for variables with _no_ scaling information
     Set has_scl_fct, has_add_fst in var_dfl_set()
     typ_uck:
     1. is required by ncra nco_cnv_mss_val_typ() 
     2. depends on var->type and so should not be set in var_dfl_set()
     3. is therefore set to default here */
  var->typ_uck=var->type; /* [enm] Type of variable when unchunked (expanded) (in memory) */

  /* Vet scale_factor */
  rcd=nco_inq_flg(nc_id,var->id,scl_fct_sng,&scl_fct_typ,&scl_fct_lng);
  if(rcd != NC_ENOTATT){
    if(scl_fct_typ == NC_BYTE || scl_fct_typ == NC_CHAR){
      if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stdout,"%s: WARNING nco_cnk_dsk_inq() reports scale_factor for %s is NC_BYTE or NC_CHAR. Will not attempt to unchunk using scale_factor.\n",prg_nm_get(),var->nm); 
      return False;
    } /* endif */
    if(scl_fct_lng != 1){
      if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stdout,"%s: WARNING nco_cnk_dsk_inq() reports %s has scale_factor of length %li. Will not attempt to unchunk using scale_factor\n",prg_nm_get(),var->nm,scl_fct_lng); 
      return False;
    } /* endif */
    var->has_scl_fct=True; /* [flg] Valid scale_factor attribute exists */
    var->typ_uck=scl_fct_typ; /* [enm] Type of variable when unchunked (expanded) (in memory) */
  } /* endif */

  /* Vet add_offset */
  rcd=nco_inq_flg(nc_id,var->id,add_fst_sng,&add_fst_typ,&add_fst_lng);
  if(rcd != NC_ENOTATT){
    if(add_fst_typ == NC_BYTE || add_fst_typ == NC_CHAR){
      if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stdout,"%s: WARNING nco_cnk_dsk_inq() reports add_offset for %s is NC_BYTE or NC_CHAR. Will not attempt to unchunk using add_offset.\n",prg_nm_get(),var->nm); 
      return False;
    } /* endif */
    if(add_fst_lng != 1){
      if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stdout,"%s: WARNING nco_cnk_dsk_inq() reports %s has add_offset of length %li. Will not attempt to unchunk.\n",prg_nm_get(),var->nm,add_fst_lng); 
      return False;
    } /* endif */
    var->has_add_fst=True; /* [flg] Valid add_offset attribute exists */
    var->typ_uck=add_fst_typ; /* [enm] Type of variable when unchunked (expanded) (in memory) */
  } /* endif */

  if(var->has_scl_fct && var->has_add_fst){
    if(scl_fct_typ != add_fst_typ){
      if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stdout,"%s: WARNING nco_cnk_dsk_inq() reports type of scale_factor does not equal type of add_offset. Will not attempt to unchunk.\n",prg_nm_get());
      return False;
    } /* endif */
  } /* endif */

  if(var->has_scl_fct || var->has_add_fst){
    /* Variable is considered chunked iff either or both valid scale_factor or add_offset exist */
    var->cnk_dsk=True; /* [flg] Variable is chunked on disk */
    /* If variable is chunked on disk and is in memory then variable is chunked in memory */
    var->cnk_ram=True; /* [flg] Variable is chunked in memory */
    var->typ_uck=(var->has_scl_fct) ? scl_fct_typ : add_fst_typ; /* [enm] Type of variable when unchunked (expanded) (in memory) */
    if(nco_is_rth_opr(prg_get()) && dbg_lvl_get() >= nco_dbg_var){
      (void)fprintf(stdout,"%s: CHUNKING Variable %s is type %s chunked into type %s\n",prg_nm_get(),var->nm,nco_typ_sng(var->typ_uck),nco_typ_sng(var->typ_dsk));
      (void)fprintf(stdout,"%s: DEBUG Chunked variables processed by all arithmetic operators are unchunked automatically, and then stored unchunked in the output file. If you wish to rechunk them in the output file, use, e.g., ncap -O -s \"foo=chunk(foo);\" out.nc out.nc. If you wish to chunk all variables in a file, use, e.g., ncpdq -P all in.nc out.nc.\n",prg_nm_get());
    } /* endif print chunking information */
  }else{
    /* Variable is not chunked since neither scale factor nor add_offset exist
       Insert hooks which depend on variable not being chunked here
       Currently this is no-op */
    ;
  } /* end else */

  return var->cnk_dsk; /* [flg] Variable is chunked on disk (valid scale_factor, add_offset, or both attributes exist) */
  
} /* end nco_cnk_dsk_inq() */

#endif /* endif 0 */
