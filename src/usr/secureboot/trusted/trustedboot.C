/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/secureboot/trusted/trustedboot.C $                    */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2016                        */
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
/**
 * @file trustedboot.C
 *
 * @brief Trusted boot interfaces
 */

// ----------------------------------------------
// Includes
// ----------------------------------------------
#include <string.h>
#include <sys/time.h>
#include <trace/interface.H>
#include <errl/errlentry.H>
#include <errl/errlmanager.H>
#include <errl/errludtarget.H>
#include <errl/errludstring.H>
#include <targeting/common/targetservice.H>
#include <secureboot/trustedbootif.H>
#include <secureboot/trustedboot_reasoncodes.H>
#include <sys/mmio.h>
#include "trustedboot.H"
#include "trustedTypes.H"
#include "trustedbootCmds.H"
#include "trustedbootUtils.H"
#include "base/tpmLogMgr.H"
#include "base/trustedboot_base.H"
#include "../settings.H"

namespace TRUSTEDBOOT
{

extern SystemTpms systemTpms;

void getTPMs( std::list<TpmTarget>& o_info )
{
    TRACUCOMP(g_trac_trustedboot,ENTER_MRK"getTPMs()");

    for (size_t idx = 0; idx < MAX_SYSTEM_TPMS; idx ++)
    {
        if (systemTpms.tpm[idx].available && !systemTpms.tpm[idx].failed)
        {

            o_info.push_back(systemTpms.tpm[idx]);
        }
    }

    TRACUCOMP(g_trac_trustedboot,EXIT_MRK"getTPMs() : Size:%d", o_info.size());

}

errlHndl_t getTpmLogDevtreeInfo(TpmTarget & i_target,
                                uint64_t & io_logAddr,
                                size_t & o_allocationSize,
                                uint64_t & o_xscomAddr,
                                uint32_t & o_i2cMasterOffset)
{
    errlHndl_t err = NULL;
    TRACUCOMP( g_trac_trustedboot,
               ENTER_MRK"getTpmLogDevtreeInfo() Chip:%d Addr:%lX %lX",
               i_target.chip, io_logAddr
               ,(uint64_t)(i_target.logMgr));

    o_allocationSize = 0;

    if (NULL != i_target.logMgr &&
        i_target.available)
    {
        err = TpmLogMgr_getDevtreeInfo(i_target.logMgr,
                                       io_logAddr,
                                       o_allocationSize,
                                       o_xscomAddr,
                                       o_i2cMasterOffset);
    }
    TRACUCOMP( g_trac_trustedboot,
               EXIT_MRK"getTpmLogDevtreeInfo() Addr:%lX",io_logAddr);
    return err;
}

void setTpmDevtreeInfo(TpmTarget & i_target,
                       uint64_t i_xscomAddr,
                       uint32_t i_i2cMasterOffset)
{
    TRACUCOMP( g_trac_trustedboot,
               ENTER_MRK"setTpmLogDevtreeOffset() Chip:%d "
               "Xscom:%lX Master:%X",
               i_target.chip, i_xscomAddr, i_i2cMasterOffset);

    if (NULL != i_target.logMgr)
    {
        TpmLogMgr_setTpmDevtreeInfo(i_target.logMgr,
                                    i_xscomAddr, i_i2cMasterOffset);
    }
}

bool enabled()
{
    bool ret = false;
#ifdef CONFIG_TPMDD
    bool foundFunctional = false;

    for (size_t idx = 0; idx < MAX_SYSTEM_TPMS; idx ++)
    {
        if ((!systemTpms.tpm[idx].failed &&
             systemTpms.tpm[idx].available) ||
            !systemTpms.tpm[idx].initAttempted)
        {
            foundFunctional = true;
            break;
        }
    }
    // If we have a functional TPM we are enabled
    ret = foundFunctional;
#endif
    return ret;
}

void* host_update_master_tpm( void *io_pArgs )
{
    errlHndl_t err = NULL;
    bool unlock = false;

    TRACDCOMP( g_trac_trustedboot,
               ENTER_MRK"host_update_master_tpm()" );
    TRACUCOMP( g_trac_trustedboot,
               ENTER_MRK"host_update_master_tpm()");

    do
    {

        // Get a node Target
        TARGETING::TargetService& tS = TARGETING::targetService();
        TARGETING::Target* nodeTarget = NULL;
        tS.getMasterNodeTarget( nodeTarget );

        if (nodeTarget == NULL)
            break;

        // Skip this target if target is non-functional
        if(!nodeTarget->getAttr<TARGETING::ATTR_HWAS_STATE>().  \
           functional)
        {
            continue;
        }

        mutex_lock( &(systemTpms.tpm[TPM_MASTER_INDEX].tpmMutex) );
        unlock = true;

        if (!systemTpms.tpm[TPM_MASTER_INDEX].failed &&
            TPMDD::tpmPresence(nodeTarget, TPMDD::TPM_PRIMARY))
        {
            // Initialize the TPM, this will mark it as non-functional on fail
            tpmInitialize(systemTpms.tpm[TPM_MASTER_INDEX],
                          nodeTarget,
                          TPMDD::TPM_PRIMARY);

        }
        else
        {
            systemTpms.tpm[TPM_MASTER_INDEX].available = false;
        }

        // Allocate the TPM log if it hasn't been already
        if (!systemTpms.tpm[TPM_MASTER_INDEX].failed &&
            systemTpms.tpm[TPM_MASTER_INDEX].available &&
            NULL == systemTpms.tpm[TPM_MASTER_INDEX].logMgr)
        {
            systemTpms.tpm[TPM_MASTER_INDEX].logMgr = new TpmLogMgr;
            err = TpmLogMgr_initialize(
                        systemTpms.tpm[TPM_MASTER_INDEX].logMgr);
            if (NULL != err)
            {
                break;
            }
        }

        // Now we need to replay any existing entries in the log into the TPM
        if (!systemTpms.tpm[TPM_MASTER_INDEX].failed &&
            systemTpms.tpm[TPM_MASTER_INDEX].available)
        {
            tpmReplayLog(systemTpms.tpm[TPM_MASTER_INDEX]);
        }

        if (systemTpms.tpm[TPM_MASTER_INDEX].failed ||
            !systemTpms.tpm[TPM_MASTER_INDEX].available)
        {

            /// @todo RTC:134913 Switch to backup chip if backup TPM avail

            // Master TPM not available
            TRACFCOMP( g_trac_trustedboot,
                       "Master TPM Existence Fail");

            /*@
             * @errortype
             * @reasoncode     RC_TPM_EXISTENCE_FAIL
             * @severity       ERRL_SEV_UNRECOVERABLE
             * @moduleid       MOD_HOST_UPDATE_MASTER_TPM
             * @userdata1      node
             * @userdata2      0
             * @devdesc        No TPMs found in system.
             */
            err = new ERRORLOG::ErrlEntry( ERRORLOG::ERRL_SEV_UNRECOVERABLE,
                                           MOD_HOST_UPDATE_MASTER_TPM,
                                           RC_TPM_EXISTENCE_FAIL,
                                           TARGETING::get_huid(nodeTarget),
                                           0,
                                           true /*Add HB SW Callout*/ );

            err->collectTrace( SECURE_COMP_NAME );
            break;
        }

        // Lastly we will check on the backup TPM and see if it is enabled
        //  in the attributes at least
        TPMDD::tpm_info_t tpmInfo;
        tpmInfo.chip = TPMDD::TPM_BACKUP;
        err = TPMDD::tpmReadAttributes(nodeTarget, tpmInfo);
        if (NULL != err)
        {
            // We don't want to log this error we will just assume
            //   the backup doesn't exist
            delete err;
            err = NULL;
            TRACUCOMP( g_trac_trustedboot,
                       "host_update_master_tpm() tgt=0x%X "
                       "Marking backup TPM unavailable due to attribute fail",
                       TARGETING::get_huid(nodeTarget));
            systemTpms.tpm[TPM_BACKUP_INDEX].available = false;
            break;
        }
        else if (!tpmInfo.tpmEnabled)
        {
            TRACUCOMP( g_trac_trustedboot,
                       "host_update_master_tpm() tgt=0x%X "
                       "Marking backup TPM unavailable",
                       TARGETING::get_huid(nodeTarget));
            systemTpms.tpm[TPM_BACKUP_INDEX].available = false;
        }

    } while ( 0 );

    if( unlock )
    {
        mutex_unlock(&(systemTpms.tpm[TPM_MASTER_INDEX].tpmMutex));
    }

    if (NULL == err)
    {
        // Log config entries to TPM - needs to be after mutex_unlock
        err = tpmLogConfigEntries(systemTpms.tpm[TPM_MASTER_INDEX]);
    }

    TRACDCOMP( g_trac_trustedboot,
               EXIT_MRK"host_update_master_tpm() - %s",
               ((NULL == err) ? "No Error" : "With Error") );
    return err;
}


void tpmInitialize(TRUSTEDBOOT::TpmTarget & io_target,
                   TARGETING::Target* i_nodeTarget,
                   TPMDD::tpm_chip_types_t i_chip)
{
    errlHndl_t err = NULL;

    TRACDCOMP( g_trac_trustedboot,
               ENTER_MRK"tpmInitialize()" );
    TRACUCOMP( g_trac_trustedboot,
               ENTER_MRK"tpmInitialize() tgt=0x%X chip=%d",
               TARGETING::get_huid(io_target.nodeTarget),
               io_target.chip);

    do
    {

        // TPM Initialization sequence

        io_target.nodeTarget = i_nodeTarget;
        io_target.chip = i_chip;
        io_target.initAttempted = true;
        io_target.available = true;
        io_target.failed = false;

        // TPM_STARTUP
        err = tpmCmdStartup(&io_target);
        if (NULL != err)
        {
            break;
        }

        // TPM_GETCAPABILITY to read FW Version
        err = tpmCmdGetCapFwVersion(&io_target);
        if (NULL != err)
        {
            break;
        }


    } while ( 0 );


    // If the TPM failed we will mark it not functional
    if (NULL != err)
    {
        tpmMarkFailed(&io_target);
        // Log this failure
        errlCommit(err, SECURE_COMP_ID);
    }

    TRACDCOMP( g_trac_trustedboot,
               EXIT_MRK"tpmInitialize()");
}

void tpmReplayLog(TRUSTEDBOOT::TpmTarget & io_target)
{
    TRACUCOMP(g_trac_trustedboot, ENTER_MRK"tpmReplayLog()");
    errlHndl_t err = NULL;
    bool unMarshalError = false;


    // Create EVENT2 structure to be populated by getNextEvent()
    TCG_PCR_EVENT2 l_eventLog;
    // Move past header event to get a pointer to the first event
    // If there are no events besides the header, l_eventHndl = NULL
    const uint8_t* l_eventHndl = TpmLogMgr_getFirstEvent(io_target.logMgr);
    while ( l_eventHndl != NULL )
    {
        // Get next event
        l_eventHndl = TpmLogMgr_getNextEvent(io_target.logMgr,
                                             l_eventHndl, &l_eventLog,
                                             &unMarshalError);
        if (unMarshalError)
        {
            /*@
             * @errortype
             * @reasoncode     RC_TPM_UNMARSHALING_FAIL
             * @severity       ERRL_SEV_UNRECOVERABLE
             * @moduleid       MOD_TPM_REPLAY_LOG
             * @userdata1      Starting address of event that caused error
             * @userdata2      0
             * @devdesc        Unmarshal error while replaying tpm log.
             */
            err = new ERRORLOG::ErrlEntry( ERRORLOG::ERRL_SEV_UNRECOVERABLE,
                                        MOD_TPM_REPLAY_LOG,
                                        RC_TPM_UNMARSHALING_FAIL,
                                        reinterpret_cast<uint64_t>(l_eventHndl),
                                        0,
                                        true /*Add HB SW Callout*/ );

            err->collectTrace( SECURE_COMP_NAME );
            break;
        }

        // Extend to tpm
        if (EV_ACTION == l_eventLog.eventType)
        {
            TRACUBIN(g_trac_trustedboot, "tpmReplayLog: Extending event:",
                     &l_eventLog, sizeof(TCG_PCR_EVENT2));
            for (size_t i = 0; i < l_eventLog.digests.count; i++)
            {

                TPM_Alg_Id l_algId = (TPM_Alg_Id)l_eventLog.digests.digests[i]
                                                                .algorithmId;
                err = tpmCmdPcrExtend(&io_target,
                            (TPM_Pcr)l_eventLog.pcrIndex,
                            l_algId,
                            l_eventLog.digests.digests[i].digest.bytes,
                            getDigestSize(l_algId));
                if (err)
                {
                    break;
                }
            }
            if (err)
            {
                break;
            }
        }
    }
    // If the TPM failed we will mark it not functional and commit errl
    if (err)
    {
        tpmMarkFailed(&io_target);
        errlCommit(err, SECURE_COMP_ID);
        delete err;
        err = NULL;
    }
}

errlHndl_t tpmLogConfigEntries(TRUSTEDBOOT::TpmTarget & io_target)
{
    TRACFCOMP(g_trac_trustedboot, ENTER_MRK"tpmLogConfigEntries()");

    errlHndl_t l_err = NULL;

    do
    {
        // Create digest buffer and set to largest config entry size.
        uint8_t l_digest[sizeof(uint64_t)];
        memset(l_digest, 0, sizeof(uint64_t));

        // Security switches
        uint64_t l_securitySwitchValue = Singleton<SECUREBOOT::Settings>::
                                                instance().getSecuritySwitch();
        TRACFCOMP(g_trac_trustedboot, "security switch value = 0x%X",
                                l_securitySwitchValue);
        // Extend to TPM - PCR_1
        memcpy(l_digest, &l_securitySwitchValue, sizeof(l_securitySwitchValue));
        l_err = pcrExtend(PCR_1, l_digest, sizeof(l_securitySwitchValue),
                          "Security Switches");
        if (l_err)
        {
            break;
        }
        memset(l_digest, 0, sizeof(uint64_t));

        // Chip type and EC
        // Fill in the actual PVR of chip
        // Layout of the PVR is (32-bit): (see cpuid.C for latest format)
        //     2 nibbles reserved.
        //     2 nibbles chip type.
        //     1 nibble technology.
        //     1 nibble major DD.
        //     1 nibble reserved.
        //     1 nibble minor D
        uint32_t l_pvr = mmio_pvr_read() & 0xFFFFFFFF;
        TRACDCOMP(g_trac_trustedboot, "PVR of chip = 0x%X", l_pvr);
        // Extend to TPM - PCR_1
        memcpy(l_digest, &l_pvr, sizeof(l_pvr));
        l_err = pcrExtend(PCR_1, l_digest, sizeof(l_pvr),"PVR of Chip");
        if (l_err)
        {
            break;
        }
        memset(l_digest, 0, sizeof(uint64_t));

        // Figure out which node we are running on
        TARGETING::Target* l_masterProc = NULL;
        TARGETING::targetService().masterProcChipTargetHandle(l_masterProc);
        TARGETING::EntityPath l_entityPath =
                        l_masterProc->getAttr<TARGETING::ATTR_PHYS_PATH>();
        const TARGETING::EntityPath::PathElement l_pathElement =
                        l_entityPath.pathElementOfType(TARGETING::TYPE_NODE);
        uint64_t l_nodeid = l_pathElement.instance;
        // Extend to TPM - PCR_1,4,5,6
        memcpy(l_digest, &l_nodeid, sizeof(l_nodeid));
        const TPM_Pcr l_pcrs[] = {PCR_1,PCR_4,PCR_5,PCR_6};
        for (size_t i = 0; i < (sizeof(l_pcrs)/sizeof(TPM_Pcr)) ; ++i)
        {
            l_err = pcrExtend(l_pcrs[i], l_digest, sizeof(l_nodeid),"Node id");
            if (l_err)
            {
                break;
            }
        }
        if (l_err)
        {
            break;
        }

    } while(0);

    return l_err;
}

} // end TRUSTEDBOOT
