/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwpf/hwp/core_activate/proc_stop_deadman_timer/proc_stop_deadman_timer.H $ */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2012,2015                        */
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
// $Id: proc_stop_deadman_timer.H,v 1.4 2015/07/27 00:44:20 jmcgill Exp $
// $Source: /afs/awd/projects/eclipz/KnowledgeBase/.cvsroot/eclipz/chips/p8/working/procedures/ipl/fapi/proc_stop_deadman_timer.H,v $
//------------------------------------------------------------------------------
// *|
// *! (C) Copyright International Business Machines Corp. 2012
// *! All Rights Reserved -- Property of IBM
// *! ***  ***
// *|
// *! TITLE       : proc_stop_deadman_timer.H
// *! DESCRIPTION : Stops deadman timer
// *!
// *! OWNER NAME  : Greg Still              Email: stillgs@us.ibm.com
// *!
//------------------------------------------------------------------------------

#ifndef _PROC_STOP_DEADMAN_TIMER_H_
#define _PROC_STOP_DEADMAN_TIMER_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <fapi.H>

//------------------------------------------------------------------------------
// Structure definitions
//------------------------------------------------------------------------------

// function pointer typedef definition for HWP call support
typedef fapi::ReturnCode (*proc_stop_deadman_timer_FP_t)(const fapi::Target &,
                                                         bool &);

//------------------------------------------------------------------------------
// Function prototypes
//------------------------------------------------------------------------------


extern "C"
{

/**
 * @brief Stop the deadman timer
 *
 * @param[in] i_target  Reference to chip target
 * @param[out] o_intr_service_running Indicates state of interrupt service
 *
 * @return ReturnCode
 */
fapi::ReturnCode proc_stop_deadman_timer(const fapi::Target & i_target,
                                         bool & o_intr_service_running);

} // extern "C"

#endif // _PROC_STOP_DEADMAN_TIMER_H_
