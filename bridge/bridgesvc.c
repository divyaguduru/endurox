/* 
** Bridge server
** This is special kind of EnduroX Server.
** It does configures the library to work in bridged mode.
** It parses command line by looking of following config:
** In client mode (-tA = type [A]ctive, connect to server)
** -n <Node id of server> -i <IP of the server> -p <Port of the server> -tA
** In server mode (-tP = type [P]asive, wait for call)
** -n <node id of server> -i <Bind address usually 0.0.0.0> -p <Port to bind on> -tP
** Notes for multi thread:
** - Outgoing message fully received by xatmi main thread, 
** and is submitted to thread pool. The treads perform async send to network.
** - Incoming message from network also are fully received from main thread.
** Once read fully, submitted to thread pool for further processing.
** In case if thread count is set to 0, then do not use threading model, just
** just do direct calls if send & receive.
**
** @file bridgesvc.c
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
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "bridge.h"
#include "../libatmisrv/srv_int.h"
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public bridge_cfg_t G_bridge_cfg;
public __thread int G_thread_init = FALSE; /* Was thread init done?     */
/*---------------------------Statics------------------------------------*/
private int M_init_ok = FALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Send status to NDRXD
 * @param is_connected TRUE connected, FALSE not connected
 * @return SUCCEED/FAIL 
 */
private int br_send_status(int is_connected)
{
    int ret = SUCCEED;
    
    bridge_info_t gencall;

    /* Reset gencall */
    memset(&gencall, 0, sizeof(gencall));
    gencall.nodeid = G_bridge_cfg.nodeid;
    
    NDRX_LOG(log_debug, "Reporting to ndrxd (is_connected=%d)", is_connected);
    
    /* Send command call to ndrxd */
    ret=cmd_generic_call((is_connected?NDRXD_COM_BRCON_RQ:NDRXD_COM_BRDISCON_RQ),
                    NDRXD_SRC_BRIDGE,
                    NDRXD_CALL_TYPE_BRIDGEINFO,
                    (command_call_t *)&gencall, sizeof(bridge_info_t),
                    ndrx_get_G_atmi_conf()->reply_q_str,
                    ndrx_get_G_atmi_conf()->reply_q,
                    (mqd_t)FAIL,   /* do not keep open ndrxd q open */
                    ndrx_get_G_atmi_conf()->ndrxd_q_str,
                    0, NULL,
                    NULL,
                    NULL,
                    NULL,
                    FALSE);
    
    return ret;
}
/**
 * Connection have been established
 */
public int br_connected(exnetcon_t *net)
{
    int ret=SUCCEED;
    
    G_bridge_cfg.con = net;
    NDRX_LOG(log_debug, "Net=%p", G_bridge_cfg.net);
    
    /* Send our clock to other node. */
    if (SUCCEED==br_send_clock())
    {
        ret=br_send_status(TRUE);
    }
    return ret;
}

/**
 * Bridge is disconnected.
 */
public int br_disconnected(exnetcon_t *net)
{
    int ret=SUCCEED;
    
    G_bridge_cfg.con = NULL;
    
    ret=br_send_status(FALSE);
    
    return ret;  
}

/**
 * Report status to ndrxd by callback, so that when ndrxd is being restarted
 * we get back correct bridge state.
 * @return SUCCEED/FAIL
 */
public int br_report_to_ndrxd_cb(void)
{
    int ret = SUCCEED;
    
    NDRX_LOG(log_warn, "br_report_to_ndrxd_cb: Reporting to ndrxd bridge status: %s", 
            (G_bridge_cfg.con?"Connected":"Disconnected"));
    if (G_bridge_cfg.con)
    {
        ret=br_send_status(TRUE);
    }
    else
    {
        ret=br_send_status(FALSE);
    }
    
    return ret;
}

/**
 * Periodic poll callback.
 * @return 
 */
public int poll_timer(void)
{
    return exnet_periodic();
}

/**
 * Invode before poll.
 * @return 
 */
int b4poll(void)
{
    return exnet_b4_poll_cb();
}

/**
 * Bridge service entry
 * @param p_svc - data & len used only...!
 */
void TPBRIDGE (TPSVCINFO *p_svc)
{
    /* Dummy service, calls will never reach this point. */
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=SUCCEED;
    int c;
    int is_server = FAIL;
    char addr[EXNET_ADDR_LEN] = {EOS};
    int port=FAIL;
    int rcvtimeout = ndrx_get_G_atmi_env()->time_out;
    int backlog = 100;
    int flags = SRV_KEY_FLAGS_BRIDGE; /* This is bridge */
    int check=5;  /* Connection check interval, seconds */
    int periodic_zero = 0; /* send zero lenght messages periodically */
    NDRX_LOG(log_debug, "tpsvrinit called");
    
    G_bridge_cfg.nodeid = FAIL;
    G_bridge_cfg.timediff = 0;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "frn:i:p:t:T:z:c:g:s:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'r':
                NDRX_LOG(log_debug, "Will send refersh to node.");
                /* Will send refersh */
                flags|=SRV_KEY_FLAGS_SENDREFERSH;
                break;
            case 'n':
                G_bridge_cfg.nodeid=(short)atoi(optarg);
                NDRX_LOG(log_debug, "Node ID, -n = [%hd]", G_bridge_cfg.nodeid);
                break;
            case 'i':
                strcpy(addr, optarg);
                NDRX_LOG(log_debug, "IP server/binding addresss, -i = [%s]", addr);
                break;
            case 'p':
                port = atoi(optarg);
		/* port will be promoted to integer... */
                NDRX_LOG(log_debug, "Port no, -p = [%d]", port);
                break;
            case 'T':
                rcvtimeout = atoi(optarg);
                NDRX_LOG(log_debug, "Receive time-out (-T): %d", 
                        rcvtimeout);
                break;
            case 'b':
                backlog = atoi(optarg);
                NDRX_LOG(log_debug, "Backlog (-b): %d", 
                        backlog);
                break;
            case 'c':
                check = atoi(optarg);
                NDRX_LOG(log_debug, "check (-c): %d", 
                        check);
                break;
            case 'z':
                periodic_zero = atoi(optarg);
                NDRX_LOG(log_debug, "periodic_zero (-z): %d", 
                                periodic_zero);
                break;
            case 'f':
                G_bridge_cfg.common_format = TRUE;
                NDRX_LOG(log_debug, "Using common network protocol.");
                break;
            case 'g':
                strcpy(G_bridge_cfg.gpg_recipient, optarg);
                NDRX_LOG(log_debug, "Using GPG Encryption, recipient: [%s]", 
					G_bridge_cfg.gpg_recipient);
                break;
            case 's':
                strcpy(G_bridge_cfg.gpg_signer, optarg);
                NDRX_LOG(log_debug, "Using GPG Encryption, signer: [%s]", 
					G_bridge_cfg.gpg_signer);
                break;
            case 'P': 
                G_bridge_cfg.threadpoolsize = atol(optarg);
                break;
            case 't': 
                
                if ('P'==*optarg)
                {
                    NDRX_LOG(log_debug, "Server mode enabled - "
                            "will wait for call");
                    is_server = TRUE;
                }
                else
                {
                    NDRX_LOG(log_debug, "Client mode enabled - "
                            "will connect to server");
                    is_server = FALSE;
                }
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }
    
    if (G_bridge_cfg.threadpoolsize <= 0)
    {
        G_bridge_cfg.threadpoolsize = BR_DEFAULT_THPOOL_SIZE;
    }
    
    NDRX_LOG(log_info, "Threadpool size set to: %d", G_bridge_cfg.threadpoolsize);
    
    /* Check configuration */
    if (FAIL==G_bridge_cfg.nodeid)
    {
        NDRX_LOG(log_error, "Flag -n not set!");
        FAIL_OUT(ret);
    }
    
    if (EOS==addr[0])
    {
        NDRX_LOG(log_error, "Flag -i not set!");
        FAIL_OUT(ret);
    }
    
    if (FAIL==port)
    {
        NDRX_LOG(log_error, "Flag -p not set!");
        FAIL_OUT(ret);
    }
    
    if (FAIL==is_server)
    {
        NDRX_LOG(log_error, "Flag -T not set!");
        FAIL_OUT(ret);
    }
    
    /* Reset network structs */
    exnet_reset_struct(&G_bridge_cfg.net);
    
    /* Install call-backs */
    exnet_install_cb(&G_bridge_cfg.net, br_process_msg, br_connected, br_disconnected);
    
    ndrx_set_report_to_ndrxd_cb(br_report_to_ndrxd_cb);
    
    /* Then configure the lib - we will have only one client session! */
    if (SUCCEED!=exnet_configure(&G_bridge_cfg.net, rcvtimeout, addr, port, 
        4, is_server, backlog, 1, periodic_zero))
    {
        NDRX_LOG(log_error, "Failed to configure network lib!");
        FAIL_OUT(ret);
    }
    
    /* Register timer check.... */
    if (SUCCEED==ret &&
            SUCCEED!=tpext_addperiodcb(check, poll_timer))
    {
        ret=FAIL;
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
    }
    
    /* Register poller callback.... */
    if (SUCCEED==ret &&
            SUCCEED!=tpext_addb4pollcb(b4poll))
    {
        ret=FAIL;
        NDRX_LOG(log_error, "tpext_addb4pollcb failed: %s",
                        tpstrerror(tperrno));
    }
    
    /* Set server flags  */
    tpext_configbrige(G_bridge_cfg.nodeid, flags, br_got_message_from_q);
    
    snprintf(G_bridge_cfg.svc, sizeof(G_bridge_cfg.svc), NDRX_SVC_BRIDGE, G_bridge_cfg.nodeid);
    
    if (SUCCEED!=tpadvertise(G_bridge_cfg.svc, TPBRIDGE))
    {
        NDRX_LOG(log_error, "Failed to advertise %s service!", G_bridge_cfg.svc);
        ret=FAIL;
        goto out;
    }
    
    if (NULL==(G_bridge_cfg.thpool = thpool_init(G_bridge_cfg.threadpoolsize)))
    {
        NDRX_LOG(log_error, "Failed to initialize thread pool (cnt: %d)!", 
                G_bridge_cfg.threadpoolsize);
        FAIL_OUT(ret);
    }
    
    M_init_ok = TRUE;
    
out:
    return ret;
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
public void tp_thread_shutdown(void *ptr, int *p_finish_off)
{
    tpterm();
    
    *p_finish_off = TRUE;
    
}


/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    int i;
    NDRX_LOG(log_debug, "tpsvrdone called");
    
    if (NULL!=G_bridge_cfg.con)
        
    {
        exnet_close_shut(G_bridge_cfg.con);
    }
    
    /* If we were server, then close server socket too */
    if (G_bridge_cfg.net.is_server)
    {
        exnet_close_shut(&G_bridge_cfg.net);
    }
    
    if (M_init_ok)
    {
        /* Terminate the threads */
        for (i=0; i<G_bridge_cfg.threadpoolsize; i++)
        {
            thpool_add_work(G_bridge_cfg.thpool, (void *)tp_thread_shutdown, NULL);
        }
        
        /* Wait for threads to finish */
        thpool_wait(G_bridge_cfg.thpool);
        thpool_destroy(G_bridge_cfg.thpool);
    }
}
