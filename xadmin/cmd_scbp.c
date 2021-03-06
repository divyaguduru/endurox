/* 
** `sc' (stop client) and `bp' (boot client) command implementation
**
** @file cmd_pc.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>
#include <Exfields.h>

#include "xa_cmn.h"
#include <ndrx.h>
#include <cpm.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Call the client process monitor with command
 * @return
 */
private int call_cpm(char *svcnm, char *cmd, char *tag, char *subsect)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, CPM_DEF_BUFFER_SZ);
    int ret=SUCCEED;
    long rsplen;
    char output[CPM_OUTPUT_SIZE];
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_CPMTAG, 0, tag, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CPMCOMMAND to %s!", tag);        
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CPMSUBSECT to %s!", subsect);        
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_CPMCOMMAND, 0, cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CPMCOMMAND to %s!", cmd);
        FAIL_OUT(ret);
    }
    
    /* Call the client admin */
    if (FAIL==(ret=tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen, 0L)))
    {
        fprintf(stderr, "%s\n", tpstrerror(tperrno));
    }
    
    /* print the stuff we got from CPM. */
    if (SUCCEED==Bget(p_ub, EX_CPMOUTPUT, 0, (char *)output, 0L))
    {
        fprintf(stdout, "%s\n", output);
    }

out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Stop client
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_sc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    char tag[CPM_TAG_LEN];
    char subsect[CPM_SUBSECT_LEN] = {"-"};
    
    ncloptmap_t clopt[] =
    {
        {'t', BFLD_STRING, (void *)tag, sizeof(tag), 
                                NCLOPT_MAND | NCLOPT_HAVE_VALUE, "Tag"},
        {'s', BFLD_STRING, (void *)subsect, sizeof(subsect), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Subsection"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        FAIL_OUT(ret);
    }
    
    ret = call_cpm(NDRX_SVC_CPM, CPM_CMD_SC, tag, subsect);
    
out:
    return ret;
}

/**
 * Boot client
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_bc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    char tag[CPM_TAG_LEN];
    char subsect[CPM_SUBSECT_LEN] = {"-"};
    
    ncloptmap_t clopt[] =
    {
        {'t', BFLD_STRING, (void *)tag, sizeof(tag), 
                                NCLOPT_MAND | NCLOPT_HAVE_VALUE, "Tag"},
        {'s', BFLD_STRING, (void *)subsect, sizeof(subsect), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Subsection"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        FAIL_OUT(ret);
    }
    
    ret = call_cpm(NDRX_SVC_CPM, CPM_CMD_BC, tag, subsect);
    
out:
    return ret;
}

