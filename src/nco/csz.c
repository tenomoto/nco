/* $Header: /data/zender/nco_20150216/nco/src/nco/csz.c,v 1.11 1999-01-13 21:46:23 zender Exp $ */

/* (c) Copyright 1995--1999 University Corporation for Atmospheric Research 
   The file LICENSE contains the full copyright notice 
   Contact NSF/UCAR/NCAR/CGD/CMS for copyright assistance */

/* Purpose: Standalone utilities for C programs (no netCDF required) */ 

/* Standard header files */
#include <math.h>               /* sin cos cos sin 3.14159 */
#include <stdio.h>              /* stderr, FILE, NULL, etc. */
#include <stdlib.h>             /* atof, atoi, malloc, getopt */ 
#include <string.h>             /* strcmp. . . */
#include <sys/stat.h>           /* stat() */
#include <time.h>               /* machine time */
#include <unistd.h>             /* all sorts of POSIX stuff */ 
/* #include <errno.h> */             /* errno */
/* #include <malloc.h>    */         /* malloc() stuff */
/* #include <assert.h> */            /* assert() debugging macro */ 

#include <sys/types.h>          /* needed for _res */ 
#include <netinet/in.h>         /* needed for _res */ 
#include <pwd.h>                /* password structures for getpwuid() */
#ifndef WIN32
#include <arpa/nameser.h>       /* needed for _res */ 
#include <resolv.h>             /* Internet structures for _res */
#endif

/* I'm only keeping these netCDF include files around because i'm worried that the
   function prototypes in nc.h are needed here. Eventually the prototypes for these
   routines should be broken into separate files, like csz.h... */ 
#include <netcdf.h>             /* netCDF def'ns */
#include "nc.h"                 /* netCDF operator universal def'ns */

#ifndef bool
#define bool int
#endif
#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

void 
Exit_gracefully(void)
{
  char *time_buf_finish;
  time_t clock;
  
  /* end the clock */  
  
  clock=time((time_t *)NULL);
  time_buf_finish=ctime(&clock);
/*  (void)fprintf(stderr,"\tfinish = %s\n",time_buf_finish);*/

  (void)fclose(stderr);
  (void)fclose(stdin);
  (void)fclose(stdout);

  exit(EXIT_SUCCESS);
} /* end Exit_gracefully() */ 

char *
cmd_ln_sng(int argc,char **argv)
/* 
   int argc: input argument count
   char **argv: input argument list
   char *cmd_ln_sng(): output command line
*/ 
{
  char *cmd_ln;
  
  int cmd_ln_sz=0;
  int idx;

  for(idx=0;idx<argc;idx++){
    cmd_ln_sz+=(int)strlen(argv[idx])+1;
  } /* end loop over args */ 
  cmd_ln=(char *)malloc(cmd_ln_sz*sizeof(char));
  if(argc <= 0){
    cmd_ln=(char *)malloc(sizeof(char));
    cmd_ln[0]='\0';
  }else{
    (void)strcpy(cmd_ln,argv[0]);
    for(idx=1;idx<argc;idx++){
      (void)strcat(cmd_ln," ");
      (void)strcat(cmd_ln,argv[idx]);
    } /* end loop over args */ 
  } /* end else */ 

  return cmd_ln;
} /* end cmd_ln_sng() */ 

lim_sct *
lim_prs(int nbr_lim,char **lim_arg)
/* 
   int nbr_lim: input number of dimensions with limits
   char **lim_arg: input list of user-specified dimension limits
   lim_sct *lim_prs(): output structure holding user-specified strings for min and max limits
 */ 
{
  /* Routine to set name, min_sng, max_sng elements of 
     a comma separated list of names and ranges. This routine
     merely evaluates the syntax of the input expressions and
     does not attempt to validate the dimensions or their ranges
     against those present in the input netCDF file. */

  /* Valid syntax adheres to nm,[min_sng][,[max_sng]][,srd_sng] */

  void usg_prn(void);

  char **arg_lst;

  char *dlm=",";

  lim_sct *lim;

  int idx;
  int arg_nbr;

  lim=(lim_sct *)malloc(nbr_lim*sizeof(lim_sct));

  for(idx=0;idx<nbr_lim;idx++){

    /* Hyperslab specifications are processed as a normal text list. */ 
    arg_lst=lst_prs(lim_arg[idx],dlm,&arg_nbr);

    /* Check syntax */ 
    if(
       arg_nbr < 2 || /* Need more than just dimension name */ 
       arg_nbr > 4 || /* Too much info */ 
       arg_lst[0] == NULL || /* Dimension name not specified */ 
       (arg_nbr == 3 && arg_lst[1] == NULL && arg_lst[2] == NULL) || /* No min or max when stride not specified */ 
       (arg_nbr == 4 && arg_lst[3] == NULL) || /* Stride should be specified */ 
       False){
      (void)fprintf(stdout,"%s: ERROR in hyperslab specification %s\n",prg_nm_get(),lim_arg[idx]);
      exit(EXIT_FAILURE);
    } /* end if */ 

    /* Initialize structure */ 
    /* lim strings which are not explicitly set by the user will remain as NULLs,
       i.e., specifying the default setting will appear as if nothing at all was set.
       Hopefully, in the routines that follow, the branch followed by a dimension for which
       all the default settings were specified (e.g.,"-d foo,,,,") will yield the same answer
       as the branch for which no hyperslab along that dimension was set.
     */ 
    lim[idx].nm=NULL;
    lim[idx].min_sng=NULL;
    lim[idx].max_sng=NULL;
    lim[idx].srd_sng=NULL;

    /* Fill in structure */ 
    lim[idx].nm=arg_lst[0];
    lim[idx].min_sng=lim[idx].max_sng=arg_lst[1];
    if(arg_nbr > 2) lim[idx].max_sng=arg_lst[2];
    if(arg_nbr > 3) lim[idx].srd_sng=arg_lst[3];

  } /* End loop over lim */

  return lim;

} /* end lim_prs() */ 

char **
lst_prs(char *sng_in,const char *dlm,int *nbr_lst)
/* 
   char *sng_in: input/output delimited argument list (delimiters are changed to NULL on output)
   const char *dlm: input delimiter
   int *nbr_lst: output number of elements in list
   char **lst_prs: output array of list elements
 */ 
{
  /* Routine creates a list of strings from a given string and an arbitrary delimiter */ 

  /* The number of members of the list is always one more than the number of delimiters, e.g.,
     foo,,3, has 4 arguments: "foo", "", "3" and "".
     A delimiter without an argument is valid syntax, meaning choose the default argument.
     Therefore a storage convention is necessary to indicate the default argument was selected.
     Either NULL or '\0' can be used without required the use of an additional flag. 
     NULL can not be printed, but is useful as a logical flag since it's value is False. 
     On the other hand, '\0', the empty string, can be printed but is not as useful as a flag. 
     Currently, NCO implements the former convention, where default selections are set to NULL.
   */
    
  char **lst;
  char **lst_ptr;
  char *tok_ptr;
  char *sng_in_ptr;

  int dlm_len;
  int idx;

  dlm_len=strlen(dlm);

  /* Do not increment actual sng_in pointer while searching for delimiters---increment a dummy pointer instead. */ 
  sng_in_ptr=sng_in; 

  /* First element does not require a delimiter in front of it */ 
  *nbr_lst=1;

  /* Count list members */ 
  while(sng_in_ptr=strstr(sng_in_ptr,dlm)){
    sng_in_ptr+=dlm_len;
    (*nbr_lst)++;
  } /* end while */ 

  lst=(char **)malloc(*nbr_lst*sizeof(char *));

  sng_in_ptr=sng_in; 
  lst[0]=sng_in;
  idx=0;
  while(sng_in_ptr=strstr(sng_in_ptr,dlm)){
    /* NULL terminate previous arg */ 
    *sng_in_ptr='\0';
    sng_in_ptr+=dlm_len;
    lst[++idx]=sng_in_ptr;
  } /* end while */ 

  /* A default list member is assumed whenever two delimiters are adjacent to eachother, such that
     the length of the string between them is 0. If the list ends with a delimiter, then the last
     element of the list is also assumed to be a default list member. */ 
  /* This loop sets default list members to NULL */
  for(idx=0;idx<*nbr_lst;idx++)
    if(strlen(lst[idx]) == 0) lst[idx]=NULL;

  if(dbg_lvl_get() == 5){
    (void)fprintf(stderr,"%d elements in delimited list\n",*nbr_lst);
    for(idx=0;idx<*nbr_lst;idx++) 
      (void)fprintf(stderr,"lst[%d] = %s\n",idx,(lst[idx] == NULL) ? "NULL" : lst[idx]);
    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
  } /* end debug */

  return lst;
} /* end lst_prs() */ 

char *
fl_nm_prs(char *fl_nm,int fl_nbr,int *nbr_fl,char **fl_lst_in,int nbr_abb_arg,char **fl_lst_abb,char *fl_pth)
/* 
   char *fl_nm: input current filename, if any
   int fl_nbr: input ordinal index of file in input file list
   int *nbr_fl: input/output number of files to be processed
   char **fl_lst_in: input user-specified filenames
   char **fl_lst_abb: input NINTAP-style arguments, if any
   char *fl_pth: input path prefix for files in fl_lst_in
   char *fl_nm_prs: output name of file to retrieve
 */ 
{
  /* Routine to construct a file name from various input arguments and switches.
     This routine implements the NINTAP-style specification by using static
     memory to avoid repetition in the construction of the filename */

  static short FIRST_INVOCATION=1;

  static char *fl_nm_1st_dgt;
  static char *fl_nm_nbr_sng;
  static char fl_nm_nbr_sng_fmt[10];

  static int fl_nm_nbr_crr;
  static int fl_nm_nbr_dgt;
  static int fl_nm_nbr_ncr;
  static int fl_nm_nbr_max;
  static int fl_nm_nbr_min;

  /* Free any old filename space */ 
  if(fl_nm != NULL) (void)free(fl_nm);

  /* Construct the filename from NINTAP-style arguments and the input name  */ 
  if(fl_lst_abb != NULL){
    if(FIRST_INVOCATION){
      char *fl_nm_nc_psn=NULL;
      int fl_nm_sfx_len=0;
      
      /* Parse the abbreviation list analogously to CCM Processor ICP "NINTAP" */ 
      if(nbr_fl != NULL) *nbr_fl=atoi(fl_lst_abb[0]);
      
      if(nbr_abb_arg > 1){
	fl_nm_nbr_dgt=atoi(fl_lst_abb[1]);
      }else{
	fl_nm_nbr_dgt=3;
      }/* end if */
      
      if(nbr_abb_arg > 2){
	fl_nm_nbr_ncr=atoi(fl_lst_abb[2]);
      }else{
	fl_nm_nbr_ncr=1;
      } /* end if */
      
      if(nbr_abb_arg > 3){
	fl_nm_nbr_max=atoi(fl_lst_abb[3]);
      }else{
	fl_nm_nbr_max=0;
      } /* end if */
      
      if(nbr_abb_arg > 4){
	fl_nm_nbr_min=atoi(fl_lst_abb[4]);
      }else{
	fl_nm_nbr_min=1;
      } /* end if */
      
      /* Is there a .nc suffix? */ 
      if(strncmp(fl_lst_in[0]+strlen(fl_lst_in[0])-3,".nc",3) == 0){
	fl_nm_sfx_len=3;
      } /* end if */
      
      /* Is there a .cdf suffix? */ 
      if(strncmp(fl_lst_in[0]+strlen(fl_lst_in[0])-4,".cdf",4) == 0){
	fl_nm_sfx_len=4;
      } /* end if */
      
      /* Initialize static information useful for future invocations  */ 
      fl_nm_1st_dgt=fl_lst_in[0]+strlen(fl_lst_in[0])-fl_nm_nbr_dgt-fl_nm_sfx_len;
      fl_nm_nbr_sng=(char *)malloc((fl_nm_nbr_dgt+1)*sizeof(char));
      fl_nm_nbr_sng=strncpy(fl_nm_nbr_sng,fl_nm_1st_dgt,fl_nm_nbr_dgt);
      fl_nm_nbr_sng[fl_nm_nbr_dgt]='\0';
      fl_nm_nbr_crr=atoi(fl_nm_nbr_sng);
      (void)sprintf(fl_nm_nbr_sng_fmt,"%%0%dd",fl_nm_nbr_dgt);

      /* The first filename is always specified on the command line anyway... */ 
      fl_nm=(char *)strdup(fl_lst_in[0]);

      /* Set flag that this routine has already been invoked at least once */ 
      FIRST_INVOCATION=False;

    }else{ /* end if FIRST_INVOCATION */
      /* Create the current filename from the previous filename */ 
      fl_nm_nbr_crr+=fl_nm_nbr_ncr;
      if(fl_nm_nbr_max) 
	if(fl_nm_nbr_crr > fl_nm_nbr_max) 
	  fl_nm_nbr_crr=fl_nm_nbr_min; 
      (void)sprintf(fl_nm_nbr_sng,fl_nm_nbr_sng_fmt,fl_nm_nbr_crr);
      fl_nm=(char *)strdup(fl_lst_in[0]);
      (void)strncpy(fl_nm+(fl_nm_1st_dgt-fl_lst_in[0]),fl_nm_nbr_sng,fl_nm_nbr_dgt);
    } /* end if not FIRST_INVOCATION */
  }else{ /* end if abbreviation list */
    fl_nm=(char *)strdup(fl_lst_in[fl_nbr]);
  } /* end if no abbreviation list */
  
  /* Prepend the path prefix */ 
  if(fl_pth != NULL){
    char *fl_nm_stub;

    fl_nm_stub=fl_nm;

    /* Allocate enough room for the joining slash '/' and the terminating NULL */ 
    fl_nm=(char *)malloc((strlen(fl_nm_stub)+strlen(fl_pth)+2)*sizeof(char));
    (void)strcpy(fl_nm,fl_pth);
    (void)strcat(fl_nm,"/");
    (void)strcat(fl_nm,fl_nm_stub);

    /* Free filestub space */ 
    (void)free(fl_nm_stub);
  } /* end if */

  /* Return the new filename */ 
  return(fl_nm);
} /* end fl_nm_prs() */ 

char *
fl_mk_lcl(char *fl_nm,char *fl_pth_lcl,int *FILE_RETRIEVED_FROM_REMOTE_LOCATION)
/* 
   char *fl_nm: input/output current filename, if any (destroyed)
   char *fl_pth_lcl: input local storage area for files retrieved from remote locations, if any
   int *FILE_RETRIEVED_FROM_REMOTE_LOCATION: output flag set if file had to be retrieved from remote system
   char *fl_mk_lcl(): output filename locally available file
*/ 
{
  /* Routine to locate the input file, retrieve it from a remote storage system if necessary, 
     create the local storage directory if neccessary, check the file for read-access,
     return the filename of the file on the local system */ 

  FILE *fp_in;
  char *fl_nm_lcl;
  char *fl_nm_stub;
  int rcd;
  struct stat stat_sct;
  
  /* Assume the local filename is the input filename */ 
  fl_nm_lcl=(char *)strdup(fl_nm);

  /* Remove any URL and machine-name components from the local filename */
  if(strstr(fl_nm_lcl,"ftp://") == fl_nm_lcl){
    char *fl_nm_lcl_tmp;
    char *fl_pth_lcl_tmp;

    /* Rearrange the fl_nm_lcl to get rid of the ftp://hostname part */ 
    fl_pth_lcl_tmp=strchr(fl_nm_lcl+6,'/');
    fl_nm_lcl_tmp=fl_nm_lcl;
    fl_nm_lcl=(char *)malloc(strlen(fl_pth_lcl_tmp)+1);
    (void)strcpy(fl_nm_lcl,fl_pth_lcl_tmp);
    (void)free(fl_nm_lcl_tmp);
  }else if(strchr(fl_nm_lcl,':')){
    char *fl_nm_lcl_tmp;
    char *fl_pth_lcl_tmp;

    /* Rearrange the fl_nm_lcl to get rid of the hostname: part */ 
    fl_pth_lcl_tmp=strchr(fl_nm_lcl+6,'/');
    fl_nm_lcl_tmp=fl_nm_lcl;
    fl_nm_lcl=(char *)malloc(strlen(fl_pth_lcl_tmp)+1);
    (void)strcpy(fl_nm_lcl,fl_pth_lcl_tmp);
    (void)free(fl_nm_lcl_tmp);
  } /* end if */ 
  
  /* Does the file exist on the local system? */ 
  rcd=stat(fl_nm_lcl,&stat_sct);
  
  /* If not, then check if the file exists on the local system under 
     the same path interpreted relative to the current working directory */ 
  if(rcd == -1){
    if(fl_nm_lcl[0] == '/'){
      rcd=stat(fl_nm_lcl+1,&stat_sct);
    } /* end if */ 
    if(rcd == 0){
      char *fl_nm_lcl_tmp;
      
      /* NB: simply adding one to the filename pointer is like deleting
	 the initial slash on the filename. Without copying the new name
	 into its own memory space, free(fl_nm_lcl) would not be able to free 
	 the initial byte. */ 
      fl_nm_lcl_tmp=(char *)strdup(fl_nm_lcl+1);
      (void)free(fl_nm_lcl);
      fl_nm_lcl=fl_nm_lcl_tmp;
      (void)fprintf(stderr,"%s: WARNING not searching for %s on remote filesystem, using local file %s instead\n",prg_nm_get(),fl_nm,fl_nm_lcl+1);
    } /* end if */
  } /* end if */ 
  
  /* Finally, check to see if the file exists on the local system in the
     directory specified for the storage of remotely retrieved files.
     This would be the case if some files had already been retrieved in
     a previous invocation of the program */
  if(rcd == -1){
    /* Where does the filename stub begin? NB: We are assuming that the local filename
       has a slash in it from now on (because the remote file system always has a slash) */ 
    fl_nm_stub=strrchr(fl_nm_lcl,'/')+1;

    /* Construct the local filename from the user-supplied local file path 
       along with the existing file stub.*/
    if(fl_pth_lcl != NULL){
      char *fl_nm_lcl_tmp;
      
      fl_nm_lcl_tmp=fl_nm_lcl;
      /* Allocate enough room for the joining slash '/' and the terminating NULL */ 
      fl_nm_lcl=(char *)malloc((strlen(fl_pth_lcl)+strlen(fl_nm_stub)+2)*sizeof(char));
      (void)strcpy(fl_nm_lcl,fl_pth_lcl);
      (void)strcat(fl_nm_lcl,"/");
      (void)strcat(fl_nm_lcl,fl_nm_stub);
      /* Free the old filename space */ 
      (void)free(fl_nm_lcl_tmp);
    } /* end if */ 
    
    /* At last, check for the file in the local storage directory */ 
    rcd=stat(fl_nm_lcl,&stat_sct);
    if (rcd != -1) (void)fprintf(stderr,"%s: WARNING not searching for %s on remote filesystem, using local file %s instead\n",prg_nm_get(),fl_nm,fl_nm_lcl);
  } /* end if */ 

  /* The file was not found locally, try to fetch it from the remote file system */
  if(rcd == -1){

    typedef struct{
      char *fmt;
      int nbr_fmt_char;
      int transfer_mode;
      int file_order;
    } rmt_fetch_cmd_sct;

    char *cmd_sys;
    char *fl_nm_rmt;
    char *fl_pth_lcl_tmp=NULL;
    
#if ( ! defined SUN4 )
    char cmd_mkdir[]="mkdir -m 777 -p";
#else
    char cmd_mkdir[]="mkdir -p";
#endif

    enum {
      synchronous, /* 0 */ 
      asynchronous}; /* 1 */ 

    enum {
      local_remote, /* 0 */ 
      remote_local}; /* 1 */ 

    int fl_pth_lcl_len;
    
    rmt_fetch_cmd_sct *rmt_cmd=NULL;
    rmt_fetch_cmd_sct msread={"msread -R %s %s",4,synchronous,local_remote};
    rmt_fetch_cmd_sct nrnet={"nrnet msget %s r flnm=%s l mail=FAIL",4,asynchronous,local_remote};
    rmt_fetch_cmd_sct rcp={"rcp -p %s %s",4,synchronous,remote_local};
    rmt_fetch_cmd_sct ftp={"",4,synchronous,remote_local};

    /* Why did the stat() command fail? */ 
/*    (void)perror(prg_nm_get());*/
    
    /* The remote filename is the input filename by definition */ 
    fl_nm_rmt=fl_nm;
    
    /* A URL specifier in the filename unambiguously signals to use anonymous ftp */ 
    if(rmt_cmd == NULL){
      if(strstr(fl_nm_rmt,"ftp://") == fl_nm_rmt){
#ifdef WIN32
      /* I have no idea how networking calls work in NT, so just exit */
      (void)fprintf(stdout,"%s: ERROR Networking required to obtain %s is not supported for Windows NT\n",prg_nm_get(),fl_nm_rmt);
      exit(EXIT_FAILURE);
#else
	char *fmt;
	char *usr_nm;
	char *host_nm_lcl;
	char *host_nm_rmt;
	char *usr_email;
	char fmt_template[]="ftp -n << END\nopen %s\nuser anonymous %s\nbin\nget %s %s\nquit\nEND";

	struct passwd *usr_pwd;

	uid_t usr_uid;

	rmt_cmd=&ftp;

	usr_uid=getuid();
	usr_pwd=getpwuid(usr_uid);
	usr_nm=usr_pwd->pw_name;
	/* DEBUG: 256 should be replaced by MAXHOSTNAMELEN from <sys/param.h>, but
	   MAXHOSTNAMELEN isn't in there on Solaris */ 
	host_nm_lcl=(char *)malloc((256+1)*sizeof(char));
	(void)gethostname(host_nm_lcl,256+1);
	if(strchr(host_nm_lcl,'.') == NULL){
	  /* The returned hostname did not include the full Internet domain name */ 
	  (void)res_init();
	  (void)strcat(host_nm_lcl,".");
	  (void)strcat(host_nm_lcl,_res.defdname);
	} /* end if */ 

	/* Add one for the joining "@" and one for the NULL byte */ 
	usr_email=(char *)malloc((strlen(usr_nm)+1+strlen(host_nm_lcl)+1)*sizeof(char));
	(void)sprintf(usr_email,"%s@%s",usr_nm,host_nm_lcl);
	/* Free the hostname space */ 
	(void)free(host_nm_lcl);

	/* The remote hostname begins directly after "ftp://" */ 
	host_nm_rmt=fl_nm_rmt+6;
	/* The filename begins right after the slash */ 
	fl_nm_rmt=strstr(fl_nm_rmt+6,"/")+1;
	/* NULL-terminate the hostname */
	*(fl_nm_rmt-1)='\0';
	
	/* Subtract the four characters replaced by new strings, and add one for the NULL byte */ 
	fmt=(char *)malloc((strlen(fmt_template)+strlen(host_nm_rmt)+strlen(usr_email)-4+1)*sizeof(char));
	(void)sprintf(fmt,fmt_template,host_nm_rmt,usr_email,"%s","%s");
	rmt_cmd->fmt=fmt;
	/* Free the space holding the user's E-mail address */ 
	(void)free(usr_email);
#endif /* !WIN32 */
      } /* end if */
    } /* end if */

    /* Otherwise, a single colon in the filename unambiguously signals to use rcp */ 
    if(rmt_cmd == NULL){
      if(strchr(fl_nm_rmt,':'))	rmt_cmd=&rcp;
    } /* end if */

    if(rmt_cmd == NULL){
      /* Does the msread command exist on the local system? */ 
      rcd=stat("/usr/local/bin/msread",&stat_sct);
      if(rcd == 0) rmt_cmd=&msread;
    } /* end if */
	
    if(rmt_cmd == NULL){
      /* Does the nrnet command exist on the local system? */ 
      rcd=stat("/usr/local/bin/nrnet",&stat_sct);
      if(rcd == 0) rmt_cmd=&nrnet;
    } /* end if */

    /* Before we look on the remote system for the filename, make sure 
       the filename has the correct syntax to exist on the remote system */
    if(rmt_cmd == &msread || rmt_cmd == &nrnet){
      if (fl_nm_rmt[0] != '/' || fl_nm_rmt[1] < 'A' || fl_nm_rmt[1] > 'Z'){
	(void)fprintf(stderr,"%s: ERROR %s is not on local filesystem and is not a syntactically valid filename on remote file system\n",prg_nm_get(),fl_nm_rmt);
	exit(EXIT_FAILURE);
      } /* end if */
    } /* end if */
    
    if(rmt_cmd == NULL){
      (void)fprintf(stderr,"%s: ERROR unable to determine method for remote retrieval of %s\n",prg_nm_get(),fl_nm_rmt);
      exit(EXIT_FAILURE);
    } /* end if */

    /* Find the path for storing the local file */
    fl_nm_stub=strrchr(fl_nm_lcl,'/')+1;
    /* Construct the local storage filepath name */ 
    fl_pth_lcl_len=strlen(fl_nm_lcl)-strlen(fl_nm_stub)-1;
    /* Allocate enough room for the terminating NULL */ 
    fl_pth_lcl_tmp=(char *)malloc((fl_pth_lcl_len+1)*sizeof(char));
    (void)strncpy(fl_pth_lcl_tmp,fl_nm_lcl,fl_pth_lcl_len);
    fl_pth_lcl_tmp[fl_pth_lcl_len]='\0';
    
    /* Warn the user when the local filepath was machine-derived from the remote name */ 
    if(fl_pth_lcl == NULL) (void)fprintf(stderr,"%s: WARNING deriving local filepath from remote filename, using %s\n",prg_nm_get(),fl_pth_lcl_tmp);

    /* Does the local filepath already exist on the local system? */
    rcd=stat(fl_pth_lcl_tmp,&stat_sct);
    /* If not, then create the local filepath */ 
    if(rcd != 0){
      /* Allocate enough room for the joining space ' ' and the terminating NULL */ 
      cmd_sys=(char *)malloc((strlen(cmd_mkdir)+fl_pth_lcl_len+2)*sizeof(char));
      (void)strcpy(cmd_sys,cmd_mkdir);
      (void)strcat(cmd_sys," ");
      (void)strcat(cmd_sys,fl_pth_lcl_tmp);
      if(dbg_lvl_get() > 0) (void)fprintf(stderr,"%s: Creating local directory %s with %s\n",prg_nm_get(),fl_pth_lcl_tmp,cmd_sys);
      rcd=system(cmd_sys); 
      if(rcd != 0){
	(void)fprintf(stderr,"%s: ERROR Unable to create local directory %s\n",prg_nm_get(),fl_pth_lcl_tmp);
	if(fl_pth_lcl == NULL) (void)fprintf(stderr,"%s: HINT Use -l option\n",prg_nm_get());
	exit(EXIT_FAILURE);
      } /* end if */
      /* Free local command space */ 
      (void)free(cmd_sys);
    } /* end if */ 

    /* Free local path space, if any */ 
    if(fl_pth_lcl_tmp != NULL) (void)free(fl_pth_lcl_tmp);

    /* Allocate enough room for the joining space ' ' and the terminating NULL */ 
    cmd_sys=(char *)malloc((strlen(rmt_cmd->fmt)-rmt_cmd->nbr_fmt_char+strlen(fl_nm_lcl)+strlen(fl_nm_rmt)+2)*sizeof(char));
    if(rmt_cmd->file_order == local_remote){
      (void)sprintf(cmd_sys,rmt_cmd->fmt,fl_nm_lcl,fl_nm_rmt);
    }else{
      (void)sprintf(cmd_sys,rmt_cmd->fmt,fl_nm_rmt,fl_nm_lcl);
    } /* end else */
    if(dbg_lvl_get() > 0) (void)fprintf(stderr,"%s: Retrieving file from remote location:\n%s",prg_nm_get(),cmd_sys);
    (void)fflush(stderr);
    /* Fetch the file from the remote file system */ 
    rcd=system(cmd_sys);
    /* Free local command space */ 
    (void)free(cmd_sys);

    /* Free the ftp script, which is the only dynamically allocated command */ 
    if(rmt_cmd == &ftp) (void)free(rmt_cmd->fmt);
   
    if(rmt_cmd->transfer_mode == synchronous){
      if(dbg_lvl_get() > 0) (void)fprintf(stderr,"\n");
      if(rcd != 0){
	(void)fprintf(stderr,"%s: ERROR Synchronous fetch command failed\n",prg_nm_get());
	exit(EXIT_FAILURE);
      } /* end if */
    }else{
      /* This is the appropriate place to insert a shell script invocation 
	 of a command to retrieve the file asynchronously and return the 
	 status to the NCO synchronously. */ 

      int fl_sz_crr=-2;
      int fl_sz_ntl=-1;
      int nbr_tm=100; /* maximum number of sleep periods before error exit */ 
      int tm_idx;
      int tm_sleep=10; /* seconds per stat() check for successful return */ 

      /* Asynchronous retrieval uses a sleep and poll technique */ 
      for(tm_idx=0;tm_idx<nbr_tm;tm_idx++){
	rcd=stat(fl_nm_lcl,&stat_sct);
	if(rcd == 0){
	  /* What is the current size of the file? */ 
	  fl_sz_ntl=fl_sz_crr;
	  fl_sz_crr=stat_sct.st_size;
	  /* If the file size has not changed for an entire sleep period, assume 
	     the file is completely retrieved. */ 
	  if(fl_sz_ntl == fl_sz_crr){
	    break;
	  } /* end if */
	} /* end if */
	/* Sleep for the specified time */ 
	(void)sleep((unsigned)tm_sleep);
	if(dbg_lvl_get() > 0) (void)fprintf(stderr,".");
	(void)fflush(stderr);
      } /* end for */ 
      if(tm_idx == nbr_tm){
	(void)fprintf(stderr,"%s: ERROR Maximum time (%d seconds = %.1f minutes) for asynchronous file retrieval exceeded.\n",prg_nm_get(),nbr_tm*tm_sleep,nbr_tm*tm_sleep/60.);
	exit(EXIT_FAILURE);
      } /* end if */
      if(dbg_lvl_get() > 0) (void)fprintf(stderr,"\n%s Retrieval successful after %d sleeps of %d seconds each = %.1f minutes\n",prg_nm_get(),tm_idx,tm_sleep,tm_idx*tm_sleep/60.);
    } /* end else transfer mode is asynchronous */
    *FILE_RETRIEVED_FROM_REMOTE_LOCATION=True;
  }else{ /* end if input file did not exist locally */
    *FILE_RETRIEVED_FROM_REMOTE_LOCATION=False;
  } /* end if file was already on the local system */ 

  /* Make sure we have read permission on the local file */
  if((fp_in=fopen(fl_nm_lcl,"r")) == NULL){
    (void)fprintf(stderr,"%s: ERROR User does not have read permission for %s\n",prg_nm_get(),fl_nm_lcl);
    exit(EXIT_FAILURE);
  }else{
    (void)fclose(fp_in);
  } /* end else */ 
  
  /* Free input filename space */ 
  (void)free(fl_nm);

  /* Return the local filename */ 
  return(fl_nm_lcl);

} /* end fl_mk_lcl() */ 

/* indexx() is from Numerical Recipes. It computes an index table which places
   sorts the input array in ascending order. I have made the arrin argument 
   and the local variable q integers for netCDF purposes. */ 
/* NB: Many Numerical Recipes routines, including this one, employ "one-based" arrays */ 
void indexx(int n,int *arrin,int *indx)
/*     int n,indx[];*/
/*     float arrin[];*/
/*     int arrin[];*/
{
  int l,j,ir,indxt,i;
/*  float q;*/
  int q;
  
  for (j=1;j<=n;j++) indx[j]=j;
  l=(n >> 1) + 1;
  ir=n;
  for (;;) {
    if (l > 1)
      q=arrin[(indxt=indx[--l])];
    else {
      q=arrin[(indxt=indx[ir])];
      indx[ir]=indx[1];
      if (--ir == 1) {
	indx[1]=indxt;
	return;
      }
    }
    i=l;
    j=l << 1;
    while (j <= ir) {
      if (j < ir && arrin[indx[j]] < arrin[indx[j+1]]) j++;
      if (q < arrin[indx[j]]) {
	indx[i]=indx[j];
	j += (i=j);
      }
      else j=ir+1;
    }
    indx[i]=indxt;
  }
} /* end indexx() */ 

char *
cvs_vrs_prs()
{
  /* Purpose: Return CVS version string */ 
  bool dly_snp;

  char *cvs_mjr_vrs_sng=NULL;
  char *cvs_mnr_vrs_sng=NULL;
  char *cvs_nm_ptr=NULL;
  char *cvs_nm_sng=NULL;
  char *cvs_pch_vrs_sng=NULL;
  char *cvs_vrs_sng=NULL;
  char *dlr_ptr=NULL;
  char *nco_sng_ptr=NULL;
  char *usc_1_ptr=NULL;
  char *usc_2_ptr=NULL;
  char cvs_Name[]="$Name: not supported by cvs2svn $"; 
  char nco_sng[]="nco"; 

  int cvs_nm_sng_len;
  int cvs_vrs_sng_len;
  int cvs_mjr_vrs_len;
  int cvs_mnr_vrs_len;
  int cvs_pch_vrs_len;
  int nco_sng_len;
  
  long cvs_mjr_vrs=-1L;
  long cvs_mnr_vrs=-1L;
  long cvs_pch_vrs=-1L;

  /* Is cvs_Name keyword expanded? */ 
  dlr_ptr=strstr(cvs_Name," $");
  if(dlr_ptr == NULL)(void)fprintf(stderr,"%s: WARNING cvs_vrs_prs() reports dlr_ptr == NULL\n%s: HINT Make sure CVS export uses -kkv\n",prg_nm_get(),prg_nm_get());
  cvs_nm_ptr=strstr(cvs_Name,"$Name: ");
  if(cvs_nm_ptr == NULL)(void)fprintf(stderr,"%s: WARNING cvs_vrs_prs() reports cvs_nm_ptr == NULL\n%s: HINT Make sure CVS export uses -kkv\n",prg_nm_get(),prg_nm_get());
  cvs_nm_sng_len=(int)(dlr_ptr-cvs_nm_ptr-7); /* 7 is strlen("$Name: ") */ 
  if(cvs_nm_sng_len > 0) dly_snp=False; else dly_snp=True;

  /* If not, this is a daily snapshot so use YYYYMMDD date for version string */ 
  if(dly_snp){
    int mth;
    int day;
    int yr;
    struct tm *gmt_tm;
    time_t clock;

    clock=time((time_t *)NULL);
    gmt_tm=gmtime(&clock); 
    /* localtime() gives YYYYMMDD in MDT, but this conflicts with RCS, which uses GMT */ 
    /*    gmt_tm=localtime(&clock); */

    mth=gmt_tm->tm_mon+1;
    day=gmt_tm->tm_mday;
    yr=gmt_tm->tm_year+1900;

    cvs_vrs_sng_len=4+2+2;
    cvs_vrs_sng=(char *)malloc(cvs_vrs_sng_len+1);
    (void)sprintf(cvs_vrs_sng,"%04i%02i%02i",yr,mth,day);
    return cvs_vrs_sng;
  } /* endif dly_snp */ 

  /* cvs_nm_sng is, e.g., "nco1_1" */ 
  cvs_nm_sng=(char *)malloc(cvs_nm_sng_len+1);
  strncpy(cvs_nm_sng,cvs_Name+7,cvs_nm_sng_len); /* 7 is strlen("$Name: ") */
  cvs_nm_sng[cvs_nm_sng_len]='\0';

  /* cvs_vrs_sng is, e.g., "1.1" */ 
  nco_sng_len=strlen(nco_sng);
  nco_sng_ptr=strstr(cvs_nm_sng,nco_sng);
  if(nco_sng_ptr == NULL)(void)fprintf(stderr,"%s: WARNING cvs_vrs_prs() reports nco_sng_ptr == NULL\n",prg_nm_get());
  usc_1_ptr=strstr(cvs_nm_sng,"_");
  if(usc_1_ptr == NULL)(void)fprintf(stderr,"%s: WARNING cvs_vrs_prs() reports usc_1_ptr == NULL\n",prg_nm_get());
  cvs_mjr_vrs_len=(int)(usc_1_ptr-cvs_nm_sng)-nco_sng_len; /* NB: cast pointer to int before subtracting */ 
  usc_2_ptr=strstr(usc_1_ptr+1,"_");
  cvs_mjr_vrs_sng=(char *)malloc(cvs_mjr_vrs_len+1);
  cvs_mjr_vrs_sng=strncpy(cvs_mjr_vrs_sng,cvs_nm_sng+nco_sng_len,cvs_mjr_vrs_len);
  cvs_mjr_vrs_sng[cvs_mjr_vrs_len]='\0';
  cvs_mjr_vrs=strtol(cvs_mjr_vrs_sng,(char **)NULL,10);
  if(usc_2_ptr == NULL){
    cvs_mnr_vrs_len=cvs_nm_sng_len-cvs_mjr_vrs_len-1;
    cvs_pch_vrs_len=0;
    cvs_vrs_sng_len=cvs_mjr_vrs_len+1+cvs_mnr_vrs_len;
  }else{
    cvs_mnr_vrs_len=usc_2_ptr-usc_1_ptr-1;
    cvs_pch_vrs_len=cvs_nm_sng_len-cvs_mjr_vrs_len-1-cvs_mnr_vrs_len-1;
    cvs_vrs_sng_len=cvs_mjr_vrs_len+1+cvs_mnr_vrs_len+1+cvs_pch_vrs_len;
  } /* end else */ 
  cvs_mnr_vrs_sng=(char *)malloc(cvs_mnr_vrs_len+1);
  cvs_mnr_vrs_sng=strncpy(cvs_mnr_vrs_sng,usc_1_ptr+1,cvs_mnr_vrs_len);
  cvs_mnr_vrs_sng[cvs_mnr_vrs_len]='\0';
  cvs_mnr_vrs=strtol(cvs_mnr_vrs_sng,(char **)NULL,10);

  cvs_pch_vrs_sng=(char *)malloc(cvs_pch_vrs_len+1);
  cvs_pch_vrs_sng[cvs_pch_vrs_len]='\0';
  cvs_vrs_sng=(char *)malloc(cvs_vrs_sng_len+1);
  if(usc_2_ptr != NULL){
    cvs_pch_vrs_sng=strncpy(cvs_pch_vrs_sng,usc_2_ptr+1,cvs_pch_vrs_len);
    cvs_pch_vrs=strtol(cvs_pch_vrs_sng,(char **)NULL,10);
    (void)sprintf(cvs_vrs_sng,"%li.%li.%li",cvs_mjr_vrs,cvs_mnr_vrs,cvs_pch_vrs);
  }else{
    (void)sprintf(cvs_vrs_sng,"%li.%li",cvs_mjr_vrs,cvs_mnr_vrs);
  }/* end else */ 

  if(dbg_lvl_get() == 4){
    (void)fprintf(stderr,"NCO version %s\n",cvs_vrs_sng);
    (void)fprintf(stderr,"cvs_nm_sng %s\n",cvs_nm_sng);
    (void)fprintf(stderr,"cvs_mjr_vrs_sng %s\n",cvs_mjr_vrs_sng);
    (void)fprintf(stderr,"cvs_mnr_vrs_sng %s\n",cvs_mnr_vrs_sng);
    (void)fprintf(stderr,"cvs_pch_vrs_sng %s\n",cvs_pch_vrs_sng);
    (void)fprintf(stderr,"cvs_mjr_vrs %li\n",cvs_mjr_vrs);
    (void)fprintf(stderr,"cvs_mnr_vrs %li\n",cvs_mnr_vrs);
    (void)fprintf(stderr,"cvs_pch_vrs %li\n",cvs_pch_vrs);
  } /* endif dbg */ 

  (void)free(cvs_mjr_vrs_sng);
  (void)free(cvs_mnr_vrs_sng);
  (void)free(cvs_pch_vrs_sng);
  (void)free(cvs_nm_sng);

  return cvs_vrs_sng;
} /* end cvs_vrs_prs() */ 

void
nc_lib_vrs_prn()
{
  /* Purpose: Print netCDF library version */ 

  char *lib_sng;
  char *nst_sng;
  char *vrs_sng;
  char *of_ptr;
  char *dlr_ptr;

  int vrs_sng_len;
  int nst_sng_len;

  lib_sng=(char *)strdup(nc_inq_libvers());

  /* As of netCDF 3.4, nc_inq_libvers() returned strings such as "3.4 of May 16 1998 14:06:16 $" */   
  of_ptr=strstr(lib_sng," of ");
  if(of_ptr == NULL)(void)fprintf(stderr,"%s: WARNING nc_lib_vrs_prn() reports of_ptr == NULL\n",prg_nm_get());
  vrs_sng_len=(int)(of_ptr-lib_sng);
  vrs_sng=(char *)malloc(vrs_sng_len+1);
  strncpy(vrs_sng,lib_sng,vrs_sng_len);
  vrs_sng[vrs_sng_len]='\0';

  dlr_ptr=strstr(lib_sng," $");
  if(dlr_ptr == NULL)(void)fprintf(stderr,"%s: WARNING nc_lib_vrs_prn() reports dlr_ptr == NULL\n",prg_nm_get());
  nst_sng_len=(int)(dlr_ptr-of_ptr-4); /* 4 is the length of " of " */ 
  nst_sng=(char *)malloc(nst_sng_len+1);
  strncpy(nst_sng,of_ptr+4,nst_sng_len); /* 4 is the length of " of " */ 
  nst_sng[nst_sng_len]='\0';

  (void)fprintf(stderr,"Linked to netCDF library version %s, compiled %s\n",vrs_sng,nst_sng);
  (void)fprintf(stdout,"NCO homepage URL is http://www.cgd.ucar.edu/cms/nco\n");

  (void)free(vrs_sng);
  (void)free(lib_sng);
  (void)free(nst_sng);
} /* end nc_lib_vrs_prn() */

void
copyright_prn(char *rcs_Id,char *rcs_Revision)
/* 
   char *rcs_Id: input RCS identification string
   char *rcs_Revision: input RCS revision string
 */ 
{
  char *date_sng;
  char *ver_sng;
  char *cvs_vrs_sng;

  int date_sng_len;
  int ver_sng_len;
  
  date_sng_len=10;
  date_sng=(char *)malloc((date_sng_len+1)*sizeof(char));
  (void)strncpy(date_sng,strchr(rcs_Id,'/')-4,date_sng_len);
  date_sng[date_sng_len]='\0';

  ver_sng_len=strrchr(rcs_Revision,'$')-strchr(rcs_Revision,':')-3;
  ver_sng=(char *)malloc((ver_sng_len+1)*sizeof(char));
  (void)strncpy(ver_sng,strchr(rcs_Revision,':')+2,ver_sng_len);
  ver_sng[ver_sng_len]='\0';

  cvs_vrs_sng=cvs_vrs_prs();

  (void)fprintf(stderr,"NCO version %s %s version %s (%s)\nCopyright 1995--1999 University Corporation for Atmospheric Research\n",cvs_vrs_sng,prg_nm_get(),ver_sng,date_sng);
  (void)free(ver_sng);
  (void)free(cvs_vrs_sng);
} /* end copyright_prn() */

void
fl_cp(char *fl_src,char *fl_dst)
/* 
   char *fl_src: input name of the source file to copy
   char *fl_dst: input name of the destination file
 */ 
{
  /* Routine to copy the first file to the second */ 

  char *cp_cmd;
  char cp_cmd_fmt[]="cp %s %s";

  int rcd;
  int nbr_fmt_char=4;
  
  /* Construct and execute the copy command */ 
  cp_cmd=(char *)malloc((strlen(cp_cmd_fmt)+strlen(fl_src)+strlen(fl_dst)-nbr_fmt_char+1)*sizeof(char));
  if(dbg_lvl_get() > 0) (void)fprintf(stderr,"Copying %s to %s...",fl_src,fl_dst);
  (void)sprintf(cp_cmd,cp_cmd_fmt,fl_src,fl_dst);
  rcd=system(cp_cmd);
  (void)free(cp_cmd);
  if(dbg_lvl_get() > 0) (void)fprintf(stderr,"done\n");
  
} /* end fl_cp() */ 

void
fl_mv(char *fl_src,char *fl_dst)
/* 
   char *fl_src: input name of the file to move
   char *fl_dst: input name of the destination file
 */ 
{
  /* Routine to move the first file to the second */ 

  char *mv_cmd;
  char mv_cmd_fmt[]="mv -f %s %s";

  int rcd;
  int nbr_fmt_char=4;
  
  /* Construct and execute the copy command */ 
  mv_cmd=(char *)malloc((strlen(mv_cmd_fmt)+strlen(fl_src)+strlen(fl_dst)-nbr_fmt_char+1)*sizeof(char));
  if(dbg_lvl_get() > 0) (void)fprintf(stderr,"Moving %s to %s...",fl_src,fl_dst);
  (void)sprintf(mv_cmd,mv_cmd_fmt,fl_src,fl_dst);
  rcd=system(mv_cmd);
  if(rcd == -1){
    (void)fprintf(stdout,"%s: ERROR fl_mv() is unable to execute mv command \"%s\"\n",prg_nm_get(),mv_cmd);
    exit(EXIT_FAILURE); 
  } /* end if */ 
  (void)free(mv_cmd);
  if(dbg_lvl_get() > 0) (void)fprintf(stderr,"done\n");
  
} /* end fl_mv() */ 

void 
fl_rm(char *fl_nm)
/* 
   char *fl_nm: input file to be removed
 */ 
{
  /* Routine to remove the specified file from the local system */ 

  int rcd;
  char rm_cmd_sys_dep[]="rm -f";
  char *rm_cmd;
  
  /* Remember to add one for the space and one for the terminating NULL character */ 
  rm_cmd=(char *)malloc((strlen(rm_cmd_sys_dep)+1+strlen(fl_nm)+1)*sizeof(char));
  (void)sprintf(rm_cmd,"%s %s",rm_cmd_sys_dep,fl_nm);

  if(dbg_lvl_get() > 0) (void)fprintf(stderr,"%s: Removing %s with %s\n",prg_nm_get(),fl_nm,rm_cmd);
  rcd=system(rm_cmd);
  if(rcd == -1) (void)fprintf(stderr,"%s: WARNING unable to remove %s, continuing anyway...\n",prg_nm_get(),fl_nm);

  (void)free(rm_cmd);

} /* end fl_rm() */ 

