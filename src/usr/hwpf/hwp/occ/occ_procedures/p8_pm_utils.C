/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwpf/hwp/occ/occ_procedures/p8_pm_utils.C $           */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2015                        */
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
// $Id: p8_pm_utils.C,v 1.5 2015/05/20 19:26:23 stillgs Exp $
// $Source: /afs/awd/projects/eclipz/KnowledgeBase/.cvsroot/eclipz/chips/p8/working/procedures/ipl/fapi/p8_pm_utils.C,v $
//------------------------------------------------------------------------------
// *! (C) Copyright International Business Machines Corp. 2011
// *! All Rights Reserved -- Property of IBM
// *! ***  ***
//------------------------------------------------------------------------------
// *! OWNER NAME: Greg Still         Email: stillgs@us.ibm.com
// *! BACKUP:     Mike Olsen         Email: cmolsen@us.ibm.com
// *!
// *!
/// \file p8_pm_utils.C
/// \brief Utility functions for PM FAPIs
///
//------------------------------------------------------------------------------

#ifndef _P8_PM_UTILS_C_
#define _P8_PM_UTILS_C_

// ----------------------------------------------------------------------
// Includes
// ----------------------------------------------------------------------
#include "p8_pm.H"

extern "C" {

using namespace fapi;

#include "p8_pm_utils.H"


//------------------------------------------------------------------------------
/**
 * Trace a set of FIRs (Globals and select Locals)
 *
 * @param[in] i_target Chip target
 * @param[in] i_msg    String to put out in the trace
 *
 * @retval ECMD_SUCCESS
 */
fapi::ReturnCode
p8_pm_glob_fir_trace(const fapi::Target& i_target,
                    const char * i_msg)
{
    fapi::ReturnCode    rc;
    ecmdDataBufferBase  data(64);
    uint64_t            address;
    uint8_t             trace_en_flag = false;

    CONST_UINT64_T( GLOB_XSTOP_FIR_0x01040000                                 , ULL(0x01040000) );
    CONST_UINT64_T( GLOB_RECOV_FIR_0x01040001                                 , ULL(0x01040001) );
    CONST_UINT64_T( TP_LFIR_0x0104000A                                        , ULL(0x0104000A) );

    do
    {

        //  Note: i_msg is put on on each record to allow for trace "greps"
        //  so as to see the "big picture" across when

        rc = FAPI_ATTR_GET(ATTR_PM_GLOBAL_FIR_TRACE_EN, NULL, trace_en_flag);
        if (rc)
        {
            FAPI_ERR("fapiGetAttribute of ATTR_PM_GLOBAL_FIR_TRACE_EN with rc = 0x%x", (uint32_t)rc);
            break;
        }

        // If trace is not enabled, leave.
        if (!trace_en_flag)
        {
            break;
        }

        //  ******************************************************************
        //  Check for xstops and recoverables and put in the trace
        //  ******************************************************************
        address =  READ_GLOBAL_XSTOP_FIR_0x570F001B;
        GETSCOM(rc, i_target, address, data);

        if (data.getNumBitsSet(0,64))
        {
            FAPI_INF("Xstop is **ACTIVE** %s", i_msg);
        }

        address =  READ_GLOBAL_RECOV_FIR_0x570F001C;
        GETSCOM(rc, i_target, address, data);
        if (data.getNumBitsSet(0,64))
        {
            FAPI_INF("Recoverable attention is **ACTIVE** %s", i_msg);
        }

        address =  READ_GLOBAL_RECOV_FIR_0x570F001C;
        GETSCOM(rc, i_target, address, data);
        if (data.getNumBitsSet(0,64))
        {
            FAPI_INF("Recoverable attention is **ACTIVE** %s", i_msg);
        }

        address =  GLOB_XSTOP_FIR_0x01040000;
        GETSCOM(rc, i_target, address, data);
        if (data.getNumBitsSet(0,64))
        {
            FAPI_INF("Glob Xstop FIR is **ACTIVE** %s", i_msg);
        }

        address =  GLOB_RECOV_FIR_0x01040001;
        GETSCOM(rc, i_target, address, data);
        if (data.getNumBitsSet(0,64))
        {
            FAPI_INF("Glob Recov FIR is **ACTIVE** %s", i_msg);
        }

        address =  TP_LFIR_0x0104000A;
        GETSCOM(rc, i_target, address, data);
        if (data.getNumBitsSet(0,64))
        {
            FAPI_INF("TP LFIR is **ACTIVE** %s", i_msg);
        }

    } while(0);
    return rc;
}

//------------------------------------------------------------------------------
/**
 * Trace PCBS FSMs
 *
 * @param[in] i_target Chip target
 * @param[in] i_msg    String to put out in the trace
 *
 * @retval ECMD_SUCCESS
 * @retval ERROR defined in xml
 */
fapi::ReturnCode
p8_pm_pcbs_fsm_trace_chip(const fapi::Target& i_target,
                            const char *      i_msg)
{
    fapi::ReturnCode                rc;
    ecmdDataBufferBase              data(64);
    uint8_t                         trace_en_flag = false;

    std::vector<fapi::Target>       l_exChiplets;
    uint8_t                         l_ex_number = 0;

    do
    {
        rc = FAPI_ATTR_GET(ATTR_PM_PCBS_FSM_TRACE_EN, NULL, trace_en_flag);
        if (rc)
        {
            FAPI_ERR("fapiGetAttribute of ATTR_PM_PCBS_FSM_TRACE_EN with rc = 0x%x", (uint32_t)rc);
            break;
        }

        // If trace is not enabled, leave.
        if (!trace_en_flag)
        {
            break;
        }

        rc = fapiGetChildChiplets(i_target,
                                  fapi::TARGET_TYPE_EX_CHIPLET,
                                  l_exChiplets,
                                  TARGET_STATE_FUNCTIONAL);
        if (rc)
        {
            FAPI_ERR("fapiGetChildChiplets with rc = 0x%x", (uint32_t)rc);
            break;
        }

        // For each chiplet
        for (uint8_t c=0; c< l_exChiplets.size(); c++)
        {

            rc = FAPI_ATTR_GET(ATTR_CHIP_UNIT_POS, &l_exChiplets[c], l_ex_number);
            if (rc)
            {
                FAPI_ERR("fapiGetAttribute of ATTR_CHIP_UNIT_POS with rc = 0x%x", (uint32_t)rc);
                break;
            }

            FAPI_DBG("\tCore number = %d", l_ex_number);

            rc = p8_pm_pcbs_fsm_trace (i_target, l_ex_number, i_msg);
            if (!rc.ok())
            {
                break;
            }
        }

    } while(0);
    return rc;
}

//------------------------------------------------------------------------------
/**
 * Trace PCBS FSMs for a given EX
 *
 * @param[in] i_target Chip target
 * @param[in] i_msg    String to put out in the trace
 *
 * @retval ECMD_SUCCESS
 */
fapi::ReturnCode
p8_pm_pcbs_fsm_trace  ( const fapi::Target& i_target,
                        uint32_t            i_ex_number,
                        const char *        i_msg)
{
    fapi::ReturnCode                rc;
    ecmdDataBufferBase              data(64);
    uint64_t                        address;
    uint8_t                         trace_en_flag = false;
    uint64_t                        ex_offset;

    do
    {
        rc = FAPI_ATTR_GET(ATTR_PM_PCBS_FSM_TRACE_EN, NULL, trace_en_flag);
        if (rc)
        {
            FAPI_ERR("fapiGetAttribute of ATTR_PM_PCBS_FSM_TRACE_EN with rc = 0x%x", (uint32_t)rc);
            break;
        }

        // If trace is not enabled, leave.
        if (!trace_en_flag)
        {
            break;
        }
    
        ex_offset = i_ex_number * 0x01000000;

        //  Note: i_msg is put on on each record to allow for trace "greps"
        //  so as to see the "big picture" across when

        //  ******************************************************************
        //  Read PCBS FSM Monitor0
        //  ******************************************************************
        address =  EX_PCBS_FSM_MONITOR1_REG_0x100F0170 + ex_offset;
        GETSCOM(rc, i_target, address, data);
        FAPI_INF("PCBS Monitor0 = 0x%016llX;  %s target:%s" ,
                                    data.getDoubleWord(0),
                                    i_msg,
                                    i_target.toEcmdString());

        //  ******************************************************************
        //  Read PCBS FSM Monitor1
        //  ******************************************************************
        address =  EX_PCBS_FSM_MONITOR2_REG_0x100F0171 + ex_offset;
        GETSCOM(rc, i_target, address, data);
        FAPI_INF("PCBS Monitor1 = 0x%016llX;  %s target:%s" ,
                                    data.getDoubleWord(0),
                                    i_msg,
                                    i_target.toEcmdString());

        //  ******************************************************************
        //  Read PCBS DPLL CPM PARM REG
        //  ******************************************************************
        address =  EX_DPLL_CPM_PARM_REG_0x100F0152 + ex_offset;
        GETSCOM(rc, i_target, address, data);
        FAPI_INF("DPLLC Monitor = 0x%016llX;  %s target:%s" ,
                                    data.getDoubleWord(0),
                                    i_msg,
                                    i_target.toEcmdString());

        //  ******************************************************************
        //  Read PCBS PMGP0
        //  ******************************************************************
        address =  EX_PMGP0_0x100F0100 + ex_offset;
        GETSCOM(rc, i_target, address, data);
        FAPI_INF("PMGP0 Monitor = 0x%016llX;  %s target:%s" ,
                                    data.getDoubleWord(0),
                                    i_msg,
                                    i_target.toEcmdString());

    } while(0);
    return rc;
}

} //end extern

#endif // _P8_PM_UTILS_H_

