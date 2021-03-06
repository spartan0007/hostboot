/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/usr/isteps/istep09list.H $                        */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2012,2014                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */

#ifndef __ISTEPS_ISTEP09LIST_H
#define __ISTEPS_ISTEP09LIST_H

/**
 * @file    istep09list.H
 *
 *  IStep 09    Activate PowerBus
 *  IPL FLow Doc        v1.28 (12/03/12)
 *
 *    09.1    proc_build_smp
 *          : Integrate PgP Islands into SMP
 *    09.2    host_slave_sbe_update
 *          : placeholder for secure boot
 *
 *  *****************************************************************
 *  THIS FILE WAS GENERATED ON 2012-04-11:1607
 *  *****************************************************************
 *
 *  Please see the note in initsvcstructs.H for description of
 *      the ISTEPNAME macro.
 *
 */

#include    <initservice/initsvcstructs.H>
#include    <initservice/initsvcreasoncodes.H>

//  include prototypes file
#include    "../../../usr/hwpf/hwp/activate_powerbus/activate_powerbus.H"
#include <config.h>

namespace   INITSERVICE
{
    const   TaskInfo    g_istep09[]  =   {
        {
                "",         // dummy, index 0
                NULL,
                { NONE, EXT_IMAGE, IPL_NOOP, false }
        },
// Reduce ext img for VPO
#ifndef CONFIG_VPO_COMPILE
        {
                ISTEPNAME(09,01,"proc_build_smp"),
                ACTIVATE_POWERBUS::call_proc_build_smp,
                { START_FN, EXT_IMAGE, NORMAL_IPL_OP, true }
        },
        {
                ISTEPNAME(09,02,"host_slave_sbe_update"),
                ACTIVATE_POWERBUS::call_host_slave_sbe_update,
                { START_FN, EXT_IMAGE, NORMAL_IPL_OP, true }
        },
#endif
};

const DepModInfo  g_istep09Dependancies = {
    {
#ifndef CONFIG_VPO_COMPILE
        DEP_LIB(libactivate_powerbus.so),
        DEP_LIB(libdram_initialization.so),
        DEP_LIB(libsbe.so),
        DEP_LIB(libbuild_winkle_images.so),
#endif
        NULL
    }
};

const ExtTaskInfo g_istep09TaskList = {
    &(g_istep09[0]),
    ( sizeof(g_istep09)/sizeof(TaskInfo) ),
    &g_istep09Dependancies
};

};  // end namespace

#endif
