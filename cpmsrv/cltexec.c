/* 
** Execute client processes (start, stop and signal handling...)
**
** @file cltexec.c
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
#include <libxml/xmlreader.h>
#include <errno.h>
#include <unistd.h>
#include <ndrstandard.h>
#include <userlog.h>
#include <atmi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/param.h>
#include <sys_mqueue.h>
#include <sys/resource.h>
#include <sys/wait.h>


#include "cpmsrv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
private pthread_t M_signal_thread; /* Signalled thread */
private int M_signal_thread_set = FALSE; /* Signal thread is set */

/*---------------------------Prototypes---------------------------------*/

#if EX_CPM_NO_THREADS
/**
 * Handle the child signal
 * @return
 */
public void sign_chld_handler(int sig)
{
    pid_t chldpid;
    int stat_loc;
    struct rusage rusage;

    memset(&rusage, 0, sizeof(rusage));

    while (0<(chldpid = wait3(&stat_loc, WNOHANG|WUNTRACED, &rusage)))
    {
        /* - no debug please... Can cause lockups...
        NDRX_LOG(log_warn, "sigchld: PID: %d exit status: %d",
                                           chldpid, stat_loc);
        */
        
        /* Search for the client & mark it as dead */
        
        cpm_process_t * c = cpm_get_client_by_pid(chldpid);
        
        if (NULL!=c)
        {
            c->dyn.cur_state = CLT_STATE_NOTRUN;
            c->dyn.exit_status = stat_loc;
            /* Set status change time */
            cpm_set_cur_time(c);
        }
        
    }

    /*signal(SIGCHLD, sign_chld_handler);*/
}

#else

/**
 * Checks for child exit.
 * We will let mainthread to do all internal struct related work!
 * @return Got child exit
 */
private void * check_child_exit(void *arg)
{
    pid_t chldpid;
    int stat_loc;
    sigset_t blockMask;
    int sig;
    
        
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    
    NDRX_LOG(log_debug, "check_child_exit - enter...");
    for (;;)
    {
        int got_something = 0;

/* seems not working on darwin ... thus just wait for pid.
 * if we do not have any childs, then sleep for 1 sec.
 */
#ifndef EX_OS_DARWIN
        NDRX_LOG(log_debug, "about to sigwait()");
        if (SUCCEED!=sigwait(&blockMask, &sig))         /* Wait for notification signal */
        {
            NDRX_LOG(log_warn, "sigwait failed:(%s)", strerror(errno));

        }        
#endif
        
        NDRX_LOG(log_debug, "about to wait()");
        while ((chldpid = wait(&stat_loc)) >= 0)
        {
            
            /* Bug #108 01/04/2015, mvitolin
             * If config file is changed by foreground thread in this time,
             * then we must synchronize with them.
             */
            cpm_lock_config();
            
            cpm_process_t * c = cpm_get_client_by_pid(chldpid);
            got_something++;
                   
            if (NULL!=c)
            {
                c->dyn.cur_state = CLT_STATE_NOTRUN;
                c->dyn.exit_status = stat_loc;
                /* Set status change time */
                cpm_set_cur_time(c);
            }
            
            cpm_unlock_config(); /* we are done... */
            
        }
#if EX_OS_DARWIN
        NDRX_LOG(6, "wait: %s", strerror(errno));
        if (!got_something)
        {
            sleep(1);
        }
#endif
    }
   
    NDRX_LOG(log_debug, "check_child_exit: %s", strerror(errno));
    return NULL;
}

/**
 * Initialize polling lib
 * not thread safe.
 * @return
 */
public void ndrxd_sigchld_init(void)
{
    sigset_t blockMask;
    pthread_attr_t pthread_custom_attr;
    pthread_attr_t pthread_custom_attr_dog;
    struct sigaction sa; /* Seem on AIX signal might slip.. */
    char *fn = "ndrxd_sigchld_init";

    NDRX_LOG(log_debug, "%s - enter", fn);
    
    /* our friend AIX, might just ignore the SIG_BLOCK and raise signal
     * Thus we will handle the stuff in as it was in Enduro/X 2.5
     */
    
    /* sa.sa_handler = sign_chld_handler; they are blocked... */
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* restart system calls please... */
    sigaction (SIGCHLD, &sa, 0);
    
    /* Block the notification signal (do not need it here...) */
    
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    
    if (pthread_sigmask(SIG_BLOCK, &blockMask, NULL) == -1)
    {
        NDRX_LOG(log_always, "%s: sigprocmask failed: %s", fn, strerror(errno));
    }
    
    pthread_attr_init(&pthread_custom_attr);
    pthread_attr_init(&pthread_custom_attr_dog);
    
    /* set some small stacks size, 1M should be fine! */
    pthread_attr_setstacksize(&pthread_custom_attr, 2048*1024);
    pthread_create(&M_signal_thread, &pthread_custom_attr, 
            check_child_exit, NULL);

    M_signal_thread_set = TRUE;
}
#endif


/**
 * Un-initialize sigchild monitor thread
 * @return
 */
public void ndrxd_sigchld_uninit(void)
{
    char *fn = "ndrxd_sigchld_uninit";

    NDRX_LOG(log_debug, "%s - enter", fn);
    
    if (!M_signal_thread_set)
    {
        NDRX_LOG(log_debug, "Signal thread was not initialized, nothing to do...");
        goto out;
    }

    NDRX_LOG(log_debug, "About to cancel signal thread");
    
    /* TODO: have a counter for number of sets, so that we can do 
     * un-init...
     */
    if (SUCCEED!=pthread_cancel(M_signal_thread))
    {
        NDRX_LOG(log_error, "Failed to kill poll signal thread: %s", strerror(errno));
    }
    else
    {
        void * res = SUCCEED;
        if (SUCCEED!=pthread_join(M_signal_thread, &res))
        {
            NDRX_LOG(log_error, "Failed to join pthread_join() signal thread: %s", 
                    strerror(errno));
        }

        if (res == PTHREAD_CANCELED)
        {
            NDRX_LOG(log_info, "Signal thread canceled ok!")
        }
        else
        {
            NDRX_LOG(log_info, "Signal thread failed to cancel "
                    "(should not happen!!)");
        }
    }
    
    M_signal_thread_set = FALSE;
    NDRX_LOG(log_debug, "finished ok");
out:
    return;
}

/**
 * Killall client running
 * @return SUCCEED
 */
public int cpm_killall(void)
{
    int ret = SUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    int is_any_running;
    ndrx_timer_t t;
    char *sig_str[3]={"SIGINT","SIGTERM", "SIGKILL"};
    int sig[3]={SIGINT,SIGTERM, SIGKILL};
    int i;
    int was_chld_kill;
    string_list_t* cltchildren = NULL;
    
    for (i=0; i<3; i++)
    {
        NDRX_LOG(log_warn, "Terminating all with %s", sig_str[i]);

        EXHASH_ITER(hh, G_clt_config, c, ct)
        {
            if (CLT_STATE_STARTED==c->dyn.cur_state)
            {
                NDRX_LOG(log_warn, "Killing: %s/%s/%d with %s",
                    c->tag, c->subsect, c->dyn.pid, sig_str[i]);
                
                
                /* if we kill with -9, then kill all childrent too
                 * this is lengthly operation, thus only for emergency kill only
                 */
                was_chld_kill = FALSE;
                if ((SIGKILL==sig[i] && (c->stat.flags & CPM_F_KILL_LEVEL_LOW)) ||
                        c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
                {
                    was_chld_kill = TRUE;
                    ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
                }
                
                kill(c->dyn.pid, sig[i]);
                
                if (was_chld_kill)
                {
                    ndrx_proc_kill_list(cltchildren);
                    ndrx_string_list_free(cltchildren);
                    cltchildren=NULL;
                }

            }
        }

        if (i<2) /*no wait for kill... */
        {
            is_any_running = FALSE;
            ndrx_timer_reset(&t);
            do
            {
                /* sign_chld_handler(0); */

                EXHASH_ITER(hh, G_clt_config, c, ct)
                {
                    if (CLT_STATE_STARTED==c->dyn.cur_state)
                    {
                        is_any_running = TRUE;
                        break;
                    }
                }

                if (is_any_running)
                {
                    usleep(CLT_STEP_INTERVAL_ALL);
                }
            }
            while (is_any_running && 
                    ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
        }
    }
    
    NDRX_LOG(log_debug, "cpm_killall done");
    return SUCCEED;
}

/**
 * Kill the process...
 * Firstly -2, then -15, then -9
 * We shall do kill in synchrounus mode.
 * 
 * @param c
 * @return 
 */
public int cpm_kill(cpm_process_t *c)
{
    int ret = SUCCEED;
    ndrx_timer_t t;
    string_list_t* cltchildren = NULL;
        
    NDRX_LOG(log_warn, "Stopping %s/%s - %s", c->tag, c->subsect, c->stat.command_line);
            
    /* INT interval */
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
    }
    
    kill(c->dyn.pid, SIGINT);
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);
        cltchildren=NULL;
    }
    
    ndrx_timer_reset(&t);
    do
    {
        /* sign_chld_handler(0); */
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
        }
    } while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGINT, continue with SIGTERM", 
            c->tag, c->subsect);
    
    /* TERM interval */
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
    }
    
    kill(c->dyn.pid, SIGTERM);
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_HIGH)
    {
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);
        cltchildren=NULL;
    }
    
    
    ndrx_timer_reset(&t);
    do
    {
        /* sign_chld_handler(0); */
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
        }
    } while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGTERM, kill with -9", 
                        c->tag, c->subsect);
    
    /* KILL interval */
    
    /* OK we are here to kill -9, then we shall killall children processes too */
    
    /* if we kill with -9, then kill all children too
     * this is lengthly operation, thus only for emergency kill only 
     */
    
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_LOW)
    {
        ndrx_proc_children_get_recursive(&cltchildren, c->dyn.pid);
    }
    
    kill(c->dyn.pid, SIGKILL);
    
    if (c->stat.flags & CPM_F_KILL_LEVEL_LOW)
    {
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);
        cltchildren=NULL;
    }

    ndrx_timer_reset(&t);
    do
    {
        /* sign_chld_handler(0); */
        if (CLT_STATE_STARTED==c->dyn.cur_state)
        {
            usleep(CLT_STEP_INTERVAL);
      
        }
    }
    while (CLT_STATE_STARTED==c->dyn.cur_state && 
            ndrx_timer_get_delta_sec(&t) < G_config.kill_interval);
    
    if (CLT_STATE_STARTED!=c->dyn.cur_state)
        goto out;
    
    NDRX_LOG(log_warn, "%s/%s Did not react on SIGKILL, giving up...", 
                        c->tag, c->subsect);
    
out:
    return ret;
}

/**
 * Boot the client
 * @param c
 * @return 
 */
public int cpm_exec(cpm_process_t *c)
{
    pid_t pid;
    char cmd_str[PATH_MAX];
    char *cmd[PATH_MAX]; /* splitted pointers.. */
    char separators[]   = " ,\t\n";
    char *token;
    int numargs = 0;
    int fd_stdout;
    int fd_stderr;
    int ret = SUCCEED;

    NDRX_LOG(log_warn, "*********processing for startup %s *********", 
            c->stat.command_line);
    
    c->dyn.was_started = TRUE; /* We tried to start... */
    
    /* clone our self */
    pid = fork();

    if( pid == 0)
    {
        /* some small delay so that parent gets time for PIDhash setup! */
        usleep(9000);

        strcpy(cmd_str, c->stat.command_line);

        token = strtok(cmd_str, separators);
        while( token != NULL )
        {
            cmd[numargs] = token;
            token = strtok( NULL, separators );
            numargs++;
        }
        cmd[numargs] = NULL;
    
        /*  Override environment, if there is such thing */
        if (EOS!=c->stat.env[0])
        {
            if (SUCCEED!=ndrx_load_new_env(c->stat.env))
            {
                userlog("Failed to load custom env from: %s!", c->stat.env);
                exit(1);
            }
        }
        
        if (EOS!=c->stat.cctag[0])
        {
            if (SUCCEED!=setenv(NDRX_CCTAG, c->stat.cctag, TRUE))
            {
                userlog("Cannot set [%s] to [%s]: %s", 
                        NDRX_CCTAG, c->stat.cctag, strerror(errno));
                exit(1);
            }
        }
        
        /* Change working dir */
        if (EOS!=c->stat.wd[0])
        {
            if (SUCCEED!=chdir(c->stat.wd))
            {
                int err = errno;
                
                NDRX_LOG(log_error, "Failed to change working diretory: %s - %s!", 
                        c->stat.wd, strerror(err));
                userlog("Failed to change working diretory: %s - %s!", 
                        c->stat.wd, strerror(err));
                exit(1);
            }
        }
        
        /* make stdout go to file */
        if (EOS!=c->stat.log_stdout[0] &&
                FAIL!=(fd_stdout = open(c->stat.log_stdout, 
               O_WRONLY| O_CREAT  | O_APPEND, S_IRUSR | S_IWUSR)))
        {
            dup2(fd_stdout, 1); 
            close(fd_stdout);
        }
        
        if (EOS!=c->stat.log_stderr[0] &&
                FAIL!=(fd_stderr = open(c->stat.log_stderr, 
                O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)))
        {
            dup2(fd_stderr, 2);   /* make stderr go to file */
            close(fd_stderr);
        }
        
        /* reset signal handlers */
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        
        if (SUCCEED != execvp (cmd[0], cmd))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to start client, error: %d, %s", 
                    err, strerror(err));
            exit (err);
        }
    }
    else if (FAIL!=pid)
    {
        cpm_set_cur_time(c);
        c->dyn.pid = pid;
        c->dyn.cur_state = CLT_STATE_STARTED;
    }
    else
    {
        userlog("Failed to fork: %s", strerror(errno));
    }
    
out:
    return ret;
}
