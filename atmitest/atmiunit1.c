/* 
** Main ATMI unit test dispatcher.
**
** @file atmiunit1.c
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
#include <cgreen/cgreen.h>
#include "atmiuni1.h"

#include <ndrstandard.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

int system_dbg(char *cmd)
{
	int ret;
	fprintf(stderr, "************ RUNNING TEST: [%s] *********************\n", cmd);
	ret=system(cmd);
	fprintf(stderr, "************ FINISHED TEST: [%s] with %d ************\n", cmd, ret);
	return ret;
}

Ensure(test000_system)
{
    int ret;
    ret=system_dbg("test000_system/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test001_basiccall)
{
    int ret;
    ret=system_dbg("test001_basiccall/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test002_basicforward)
{
    int ret;
    ret=system_dbg("test002_basicforward/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test003_basicconvers)
{
    int ret;
    ret=system_dbg("test003_basicconvers/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test004_basicevent)
{
    int ret;
    ret=system_dbg("test004_basicevent/run.sh");
    assert_equal(ret, SUCCEED);

}

Ensure(test005_convconfload)
{
    int ret;
    ret=system_dbg("test005_convconfload/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test006_ulog)
{
    int ret;
    ret=system_dbg("test006_ulog/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test007_advertise)
{
    int ret;
    ret=system_dbg("test007_advertise/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test008_extensions)
{
    int ret;
    ret=system_dbg("test008_extensions/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test009_srvdie)
{
    int ret;
    ret=system_dbg("test009_srvdie/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test010_strtracecond)
{
    int ret;
    ret=system_dbg("test010_strtracecond/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test011_tout)
{
    int ret;
    ret=system_dbg("test011_tout/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test012_admqfull)
{
    int ret;
    ret=system_dbg("test012_admqfull/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test001_basiccall_dom)
{
    int ret;
    ret=system_dbg("test001_basiccall/run-dom.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test002_basicforward_dom)
{
    int ret;
    ret=system_dbg("test002_basicforward/run-dom.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test003_basicconvers_dom)
{
    int ret;
    ret=system_dbg("test003_basicconvers/run-dom.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test004_basicevent_dom)
{
    int ret;
    ret=system_dbg("test004_basicevent/run-dom.sh");
    assert_equal(ret, SUCCEED);

}

Ensure(test013_procnorsp)
{
    int ret;
    ret=system_dbg("test013_procnorsp/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test015_threads)
{
    int ret;
    ret=system_dbg("test015_threads/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test017_srvthread)
{
    int ret;
    ret=system_dbg("test017_srvthread/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test018_tpacalltout)
{
    int ret;
    ret=system_dbg("test018_tpacalltout/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test021_xafull)
{
    int ret;
    ret=system_dbg("test021_xafull/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test022_typedstring)
{
    int ret;
    ret=system_dbg("test022_typedstring/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test023_typedcarray)
{
    int ret;
    ret=system_dbg("test023_typedcarray/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test024_json)
{
    int ret;
    ret=system_dbg("test024_json/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test025_cpmsrv)
{
    int ret;
    ret=system_dbg("test025_cpmsrv/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test026_typedjson)
{
    int ret;
    ret=system_dbg("test026_typedjson/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test027_pscript)
{
    int ret;
    ret=system_dbg("test027_pscript/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test028_tmq)
{
    int ret;
    ret=system_dbg("test028_tmq/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test029_inicfg)
{
    int ret;
    ret=system_dbg("test029_inicfg/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test030_cconfsrv)
{
    int ret;
    ret=system_dbg("test030_cconfsrv/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test031_logging)
{
    int ret;
    ret=system_dbg("test031_logging/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test032_oapi)
{
    int ret;
    ret=system_dbg("test032_oapi/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test033_provision)
{
    int ret;
    ret=system_dbg("test033_provision/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test034_shmreuse)
{
    int ret;
    ret=system_dbg("test034_shmreuse/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test035_hkeep)
{
    int ret;
    ret=system_dbg("test035_hkeep/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test036_tprecover)
{
    int ret;
    ret=system_dbg("test036_tprecover/run.sh");
    assert_equal(ret, SUCCEED);
}

Ensure(test037_down)
{
    int ret;
    ret=system_dbg("test037_down/run.sh");
    assert_equal(ret, SUCCEED);
}

TestSuite *atmi_test_all(void)
{
    TestSuite *suite = create_test_suite();
    add_test(suite, test000_system);
    add_test(suite, test001_basiccall);
    add_test(suite, test002_basicforward);
    add_test(suite, test003_basicconvers);
    add_test(suite, test004_basicevent);
    add_test(suite, test005_convconfload);
    add_test(suite, test006_ulog);
    add_test(suite, test007_advertise);
    add_test(suite, test008_extensions);
    add_test(suite, test009_srvdie);
    add_test(suite, test010_strtracecond);
    add_test(suite, test011_tout);

#ifndef EX_OS_CYGWIN
#ifdef SYS64BIT
    /* We should skip this on non 64bit machines! */
    add_test(suite, test012_admqfull);
#endif
#endif

    add_test(suite, test001_basiccall_dom);
    add_test(suite, test002_basicforward_dom);
    add_test(suite, test003_basicconvers_dom);
    add_test(suite, test004_basicevent_dom);
    add_test(suite, test013_procnorsp);
    add_test(suite, test015_threads);
    add_test(suite, test017_srvthread);
    add_test(suite, test018_tpacalltout);
    add_test(suite, test021_xafull);
    add_test(suite, test022_typedstring);
    add_test(suite, test023_typedcarray);
    add_test(suite, test024_json);
    add_test(suite, test025_cpmsrv);
    add_test(suite, test026_typedjson);
#ifndef NDRX_DISABLEPSCRIPT
    add_test(suite, test027_pscript);
#endif
    add_test(suite, test028_tmq);
    add_test(suite, test029_inicfg);
    add_test(suite, test030_cconfsrv);
    add_test(suite, test031_logging);
    add_test(suite, test032_oapi);
    
#ifndef NDRX_DISABLEPSCRIPT
    add_test(suite, test033_provision);
#endif    
    
    /* Feature #139 mvitolin, 09/05/2017 */
    add_test(suite, test034_shmreuse);
    
    /* mvitolin Bug #112, 18/05/2017 */
    add_test(suite, test035_hkeep);
    /* mvitolin Bug #110, 22/05/2017 */
    add_test(suite, test036_tprecover);
    
    /* Bug #133 */
    add_test(suite, test037_down);
            
    return suite;
}

/*
 * Main test case entry
 */
int main(int argc, char** argv) {

    TestSuite *suite = create_test_suite();

    add_suite(suite, atmi_test_all());


    if (argc > 1) {
        return run_single_test(suite,argv[1],create_text_reporter());
    }

    return run_test_suite(suite, create_text_reporter());
}

