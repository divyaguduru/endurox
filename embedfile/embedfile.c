/* 
** Generate source code for resource file
**
** @file embedfile.c
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

#include <atmi.h>
#include <atmi_int.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <ubf.h>
#include <ubf_int.h>
#include <fdatatype.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/ 
#define INF     1
#define OUTF    2
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Main entry point for `embedfile' utility
 */
int main(int argc, char** argv)
{
    int ret = SUCCEED;
    FILE *in=NULL, *out=NULL;
    int c;
    int len = 0;
    char *inf, *outfpfx;
    char outf[PATH_MAX+1] = {EOS};
    int counter = 0;

    if (argc<3)
    {
        fprintf(stderr, "syntax: %s <input file> <output file>\n", argv[0]);
        FAIL_OUT(ret);
    }
    
    inf = argv[INF];
    outfpfx = argv[OUTF];
    
    sprintf(outf, "%s.c", outfpfx);
    
    if (NULL==(in=fopen(inf, "rb")))
    {
        fprintf(stderr, "Failed to open [%s]: \n", inf);
        FAIL_OUT(ret);
    }
    
    if (NULL==(out=fopen(outf, "wb")))
    {
        fprintf(stderr, "Failed to open [%s]: \n", outf);
        FAIL_OUT(ret);
    }
    
    /* write header */
    
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "const char G_resource_%s[] = {\n", outfpfx);
    
    /* read  & write */
    while (EOF!=(c=(fgetc(in))))
    {
        counter++;
        
        fprintf(out, "0x%02x,", c);
        
        if (0 == counter % 10)
        {
            fprintf(out, "\n");
        }
        else
        {
            fprintf(out, " ");
        }
    }
    
    /* close resource */
    fprintf(out, "0x00\n};\n");
    
    /* write off length (with out EOS..) */
    fprintf(out, "const size_t G_resource_%s_len = %d;\n", outfpfx, counter);
    
    /* put the EOL... (additionally so that we can easy operate resource as string) */
    
out:
    
    if (NULL!=in)
    {
        fclose(in);
    }
    
    if (NULL!=out)
    {
        fclose(out);
    }
    
    if (SUCCEED!=ret) 
    {
        unlink(argv[OUTF]);
    }
    return ret;
}

