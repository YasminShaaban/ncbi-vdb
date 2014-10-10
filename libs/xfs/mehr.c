/*===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 */

#include <klib/rc.h>
#include <klib/out.h>
#include <klib/namelist.h>
#include <klib/refcount.h>
#include <klib/printf.h>

#include <kfs/directory.h>
#include <kfs/file.h>

#include <kfg/config.h>

#include <kns/manager.h>
#include <kns/http.h>

#include "mehr.h"
#include "zehr.h"

#include <sysalloc.h>

#include <stdio.h>
#include <string.h>

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

/*)) Config and all config related
 ((*/

static const struct KConfig * _sConfig_MHR = NULL;
static const char * _sConfigPath_MHR = NULL;

LIB_EXPORT
rc_t CC
XFS_InitAll_MHR ( const char * ConfigFile )
{
    rc_t RCt;
    const struct KConfig * Config;

    RCt = 0;
    Config = NULL;

    if ( ConfigFile == NULL ) {
        return XFS_RC ( rcNull );
    }

printf ( "WARNING(MEHR): InitAll [%s]\n", ConfigFile );

        /* First we do inti config :lol: */
    RCt = XFS_LoadConfig_ZHR ( ConfigFile, & Config );
    if ( RCt == 0 ) {
        _sConfig_MHR = Config;
        _sConfigPath_MHR = string_dup_measure ( ConfigFile, NULL );
    }

    return RCt;
}   /* XFS_InitAll_MHR () */

LIB_EXPORT
rc_t CC
XFS_DisposeAll_MHR ()
{
    rc_t RCt;

    RCt = 0;

printf ( "WARNING(MEHR): DisposeAll [%s]\n", _sConfigPath_MHR );

    if ( _sConfig_MHR != NULL ) {
        KConfigRelease ( _sConfig_MHR );

        _sConfig_MHR = NULL;
    }

    if ( _sConfigPath_MHR != NULL ) {
        free ( ( char * ) _sConfigPath_MHR );

        _sConfigPath_MHR = NULL;
    }

    return RCt;
}   /* XFS_DisposeAll_MHR () */

LIB_EXPORT
const struct KConfig * CC
XFS_Config_MHR ()
{
    return _sConfig_MHR;
}   /* XFS_Config_MHR () */

LIB_EXPORT
const char * CC
XFS_ConfigPath_MHR ()
{
    return _sConfigPath_MHR;
}   /* XFS_ConfigPath_MHR () */

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

/*))
 //  That method will try open resource for reading local or remote
((*/
LIB_EXPORT
rc_t CC
XFS_OpenResourceRead_MHR (
                        const char * Resource,
                        const struct KFile ** RetFile
)
{
    rc_t RCt;
    struct KNSManager * knsManager;
    const struct KFile * File;
    struct KDirectory * Directory;

    RCt = 0;
    knsManager = NULL;
    File = NULL;

        /* Standard check */
    if ( Resource == NULL || RetFile == NULL ) {
        return XFS_RC ( rcNull );
    }

    * RetFile = NULL;

    if ( strstr ( Resource, "http" ) == Resource ) {
            /* That is stupid, but ... prolly this is URL */
        RCt = KNSManagerMake ( & knsManager );
        if ( RCt == 0 ) {
            RCt = KNSManagerMakeHttpFile (
                                knsManager,
                                & File,
                                NULL, /* doing that in absence conn */
                                0x01010000,
                                Resource
                                );
            if ( RCt != 0 ) {
                /* We failed to load it remotely, may be it is local */
                File = NULL;
                RCt = 0;
            }

            KNSManagerRelease ( knsManager );
        }
    }

        /* And here we are */
    if ( File == NULL ) {
        RCt = KDirectoryNativeDir ( & Directory );
        if ( RCt == 0 ) {
            RCt = KDirectoryOpenFileRead (
                                    Directory,
                                    & File,
                                    Resource
                                    );

            KDirectoryRelease ( Directory );
        }
    }

    if ( RCt == 0 ) {
        * RetFile = File;
    }

    return RCt;
}   /* XFS_OpenResourceRead_MHR () */

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/

/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/
/*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*/
