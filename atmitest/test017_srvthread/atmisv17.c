/* 
**
** @file atmisv17.c
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

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */


#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <thpool.h>

struct thread_server
{
    char *context_data; /* malloced by enduro/x */
    char *buffer; /* buffer data, managed by enduro/x */
};
/* note we must malloc this struct too. */
typedef struct thread_server thread_server_t;


threadpool M_thpool; /* threads for service */

/* threaded function... */
void _TH_TESTSVFN (void *ptr, int *p_finish_off)
{
    int ret=SUCCEED;
    double d;
    int i;
    thread_server_t *thread_data = (thread_server_t *)ptr;
    UBFH *p_ub = (UBFH *)thread_data->buffer;
    
    if (SUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to init worker client");
        exit(1);
    }
    
    /* restore context. */
    if (SUCCEED!=tpsrvsetctxdata(thread_data->context_data, SYS_SRV_THREAD))
    {
        NDRX_LOG(log_error, "Failed to set context");
        exit(1);
    }
    
    /* free up the transport data.*/
    free(thread_data->context_data);
    free(thread_data);
    
    /* !!!************* OK. we are ready to go! **********************!!!*/
    
    NDRX_LOG(log_debug, "TESTSVFN got call");

    /* Just print the buffer 
    Bprint(p_ub);*/
    if (NULL==(p_ub = (UBFH *)tprealloc((char *)p_ub, 4096))) /* allocate some stuff for more data to put in  */
    {
        ret=FAIL;
        goto out;
    }
    
    if (FAIL==Bget(p_ub, T_DOUBLE_FLD, Boccur(p_ub, T_DOUBLE_FLD)-1, (char *)&d, 0))
    {
        ret=FAIL;
        goto out;
    }

    d+=1;

    if (FAIL==Badd(p_ub, T_DOUBLE_FLD, (char *)&d, 0))
    {
        ret=FAIL;
        goto out;
    }
    
    if (FAIL==ret)
        NDRX_LOG(log_debug, "ALARM!!! WE GOT FAIL TESTERROR!!!!!");

    
out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);

}


/* main server thread... 
 * NOTE: p_svc - this is local variable of enduro's main thread (on stack).
 * but p_svc->data - is auto buffer, will be freed when main thread returns.
 *                      Thus we need a copy of buffer for thread.
 */
void TESTSVFN (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data; /* this is auto-buffer */
    pthread_t thread;
    pthread_attr_t attr; 
    long size;
    char btype[16];
    char stype[16];
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (0==(size = tptypes (p_svc->data, btype, stype)))
    {
        NDRX_LOG(log_error, "Zero buffer received!");
        exit(1);
    }
    
    thread_server_t *thread_data = malloc(sizeof(thread_server_t));
    
    /* not using sub-type - on tpreturn/forward for thread it will be auto-free */
    thread_data->buffer =  tpalloc(btype, NULL, size);
    
    
    if (NULL==thread_data->buffer)
    {
        NDRX_LOG(log_error, "tpalloc failed of type %s size %ld", btype, size);
        exit(1);
    }
    
    /* copy off the data */
    memcpy(thread_data->buffer, p_svc->data, size);
    
    thread_data->context_data = tpsrvgetctxdata();
    
    /*
    if (SUCCEED!=pthread_create (&thread, &attr, (void *) &_TH_TESTSVFN, thread_data))
    {
        ret=FAIL;
        goto out;
    }
     */
    thpool_add_work(M_thpool, (void*)_TH_TESTSVFN, (void *)thread_data);
    
    
out:
    if (SUCCEED==ret)
    {
        /* serve next.. */
        tpcontinue();
    }
    else
    {
        /* return error back */
        tpreturn(  TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
    }
}
/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

        /* service request handlers */
    if (NULL==(M_thpool = thpool_init(10)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: 10)!");
        FAIL_OUT(ret);
    }

    
    if (SUCCEED!=tpadvertise("TESTSV", TESTSVFN))
    {
        NDRX_LOG(log_error, "Failed to initialize TESTSV (first)!");
        ret=FAIL;
    }
    
out:    
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
