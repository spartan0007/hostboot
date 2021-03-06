/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwpf/hwp/dram_training/mem_startclocks/cen_mem_startclocks.C $ */
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
// $Id: cen_mem_startclocks.C,v 1.13 2014/04/07 19:01:06 gollub Exp $
// $Source: /afs/awd/projects/eclipz/KnowledgeBase/.cvsroot/eclipz/chips/centaur/working/procedures/ipl/fapi/cen_mem_startclocks.C,v $
//------------------------------------------------------------------------------
// *! (C) Copyright International Business Machines Corp. 2012
// *! All Rights Reserved -- Property of IBM
// *! ***  ***
//------------------------------------------------------------------------------
// *! TITLE       : cen_mem_startclocks
// *! DESCRIPTION : see additional comments below
// *! OWNER NAME  : Mark Fredrickson  Email: mfred@us.ibm.com
// *! BACKUP NAME : Mark Bellows      Email: bellows@us.ibm.com
// *! SCREEN      : pervasive_screen
// #! ADDITIONAL COMMENTS : See below
//
// The purpose of this procedure is to drop the fences and release the tholds associated with the Centaur chip MEM PLL.
// to allow propagation of MEM Centaur clocks to internal logic, arrays, and PHYs.
// See sepecific instructions below.
//
// Note: This procedure only starts the clocks in the MEM chiplet.
//       Use the cen_sbe_tp_chiplet_init1.S procedure to start the clocks in the PERV chiplet.
//       Use the cen_sbe_startclocks.S procedure to start the clocks in the NEST chiplet.
//
// The following details are from the Common POR spreadsheet sections 20.1 and 21.1 and from the Centaur Chip Spec.
//
// Common clock start actions:
//   Write SCOM      address 0x6B0F0013  bit(18)=0b0                   multicast, drop fence in GP3
//   Write SCOM      address 0x6B0F0014  bit(28)=0b1                   multicast, enable EDRAM, just chiplets with EDRAM logic
//   ---not centaur---Write SCOM      address 0x6B0F0102  bit(40)=0b1                   enable EDRAM GP0
//   Write SCOM      address 0x6B000004  bit(63)=0b0                   multicast, drop pervasive fence in GP0
//   Write SCOM      address 0x6B000004  bit(0)=0b0, bit(1)=0b0        multicast, clear mux selects in GP0
//   Write SCOM      address 0x6B000005  bit(11)=0b1                   abist_mode_dc for core chiplets (core recovery)
//   Write SCOM      address 0x6B030007  0x0000000000000000            Write CC Scan Region Reg, set all bits='0' prior clk start
//
// Centaur-specific clock start actions:
//   Write SCOM      address 0x03030006  data=0x4FE0 0600 0000 0000    unicast, write CC clock region reg in MEM chiplet. start clocks
//   Write SCOM      address 0x03030006  data=0x4FE0 0E00 0000 0000    unicast, write CC clock region reg in MEM chiplet. start clocks
//   Read SCOM       address 0x03030008  expect=0x0000 001F FFFF FFFF  unicast, read clock status reg in MEM chiplet
//   Write SCOM      address 0x02030006  data=0x4FE0 0600 0000 0000    unicast, write CC clock region reg in NEST chiplet. start clocks
//   Write SCOM      address 0x02030006  data=0x4FE0 0E00 0000 0000    unicast, write CC clock region reg in NEST chiplet. start clocks
//   Read SCOM       address 0x02030008  expect=0x0000 001F FFFF FFFF  unicast, read clock status reg in NEST chiplet
//   Write CFAM      address 0x13        bit(02)=0b1                   Set MemReset Stability Control
//   Write CFAM      address 0x13        bit(04)=0b1                   Release D3PHY PLL Reset Control
//
// More common clock start actions:
//   Write SCOM      address 0x6B000004  bit(3)=0b0                    multicast, clear force_align in all Chiplets in GP0
//   Write SCOM      address 0x6B000004  bit(2)=0b0                    multicast, clear flushmode_inhibit in Chiplet in GP0
//
// Enable Drivers and Receivers
//   Write CFAM      address 0x13        bit(22:23)=0b11,bit(28:30)=0b111   Enable drivers and receivers
//
//------------------------------------------------------------------------------
// Don't forget to create CVS comments when you check in your changes!
//------------------------------------------------------------------------------

#include <fapi.H>
#include <cen_scom_addresses.H>
#include <cen_mem_startclocks.H>
#include <mss_unmask_errors.H>

//  Constants
// CFAM FSI GP4 register bit/field definitions
const uint8_t FSI_GP4_MEMRESET_STABILITY_BIT  = 2;
const uint8_t FSI_GP4_DPHY_PLLRESET_BIT       = 4;

// GP3 register bit/field definitions
const uint8_t GP3_FENCE_EN_BIT     = 18;
const uint8_t GP3_EDRAM_ENABLE_BIT = 28;

// GP0 register bit/field definitions
const uint8_t GP0_ABSTCLK_MUXSEL_BIT    = 0;
const uint8_t GP0_SYNCCLK_MUXSEL_BIT    = 1;
const uint8_t GP0_FLUSHMODE_INHIBIT_BIT = 2;
const uint8_t GP0_FORCE_ALIGN_BIT       = 3;
const uint8_t GP0_ABIST_MODE_BIT        = 11;
const uint8_t GP0_PERV_FENCE_BIT        = 63;

// Clock Region Register clock start data patterns
const uint64_t CLK_REGION_REG_DATA_TO_START_NSL_ARY = 0x4FE0060000000000ull;
const uint64_t CLK_REGION_REG_DATA_TO_START_ALL     = 0x4FE00E0000000000ull;

// Clock Status Register expected pattern
const uint64_t MEM_CLK_STATUS_REG_EXP_DATA = 0x0000001FFFFFFFFFull;

// Chiplet FIR register expected pattern
const uint64_t MEM_XSTOP_FIR_REG_EXP_DATA = 0x0000000000000000ull;


extern "C" {


using namespace fapi;

// Procedures in this file
fapi::ReturnCode cen_mem_startclocks_cloned(const fapi::Target & i_target);

//------------------------------------------------------------------------------
fapi::ReturnCode cen_mem_startclocks(const fapi::Target & i_target)
{
    // Target is centaur
    
    fapi::ReturnCode l_rc;
    
    l_rc = cen_mem_startclocks_cloned(i_target);
    
	// If mss_unmask_pervasive_errors gets it's own bad rc,
	// it will commit the passed in rc (if non-zero), and return it's own bad rc.
	// Else if mss_unmask_pervasive_errors runs clean, 
	// it will just return the passed in rc.
	l_rc = mss_unmask_pervasive_errors(i_target, l_rc);

	return l_rc;
}


//------------------------------------------------------------------------------
fapi::ReturnCode cen_mem_startclocks_cloned(const fapi::Target & i_target)
{
    // Target is centaur

    fapi::ReturnCode rc;
    uint32_t rc_ecmd = 0;
    ecmdDataBufferBase scom_data(64);
    ecmdDataBufferBase cfam_data(32);



    FAPI_INF("********* cen_mem_startclocks start *********");
    do
    {
        //
        // The following details are from the Common POR spreadsheet sections 20.1 and 21.1 and from the Centaur Chip Spec.
        //
        // Common clock start actions:
        //


        //   Write SCOM      address 0x030F0013  bit(18)=0b0    , drop fence in GP3
        FAPI_DBG("Writing GP3 AND mask to clear chiplet fence (bit 18) ...");
        rc_ecmd |= scom_data.flushTo1();
        rc_ecmd |= scom_data.clearBit(GP3_FENCE_EN_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to clear chiplet fence.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_GP3_AND_0x030F0013, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing GP3 AND mask 0x030F0013 (bit 18) to clear chiplet fence.");
            break;
        }


        //   Write SCOM      address 0x030F0014  bit(28)=0b1    , enable EDRAM, just chiplets with EDRAM logic
        FAPI_DBG("Writing GP3 OR mask to enable EDRAM (bit 28) ...");
        rc_ecmd |= scom_data.flushTo0();
        rc_ecmd |= scom_data.setBit(GP3_EDRAM_ENABLE_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to enable EDRAM.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_GP3_OR_0x030F0014, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing GP3 OR mask 0x030F0014 (bit 28) to enable EDRAM.");
            break;
        }


        //   ---not needed for centaur---Write SCOM      address 0x6B0F0102  bit(40)=0b1                   enable EDRAM GP0


        //   Write SCOM      address 0x03000004  bit(63)=0b0               , drop pervasive fence in GP0
        //   Write SCOM      address 0x03000004  bit(0)=0b0, bit(1)=0b0    , clear mux selects in GP0
        FAPI_DBG("Writing GP0 AND mask to drop pervasive fence (bit 63) ...");
        FAPI_DBG("Writing GP0 AND mask to clear mux selects (bits 0-1) ...");
        rc_ecmd |= scom_data.flushTo1();
        rc_ecmd |= scom_data.clearBit(GP0_ABSTCLK_MUXSEL_BIT);
        rc_ecmd |= scom_data.clearBit(GP0_SYNCCLK_MUXSEL_BIT);
        rc_ecmd |= scom_data.clearBit(GP0_PERV_FENCE_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to drop pervasive fence and clear mux selects.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_GP0_AND_0x03000004, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing GP0 AND mask 0x03000004 (bits 0,1,63) to drop pervasive fence and clear mux selects.");
            break;
        }


        //   Write SCOM      address 0x03000005  bit(11)=0b1        abist_mode_dc for core chiplets (core recovery)
        FAPI_DBG("Writing GP0 OR mask to set abist_mode_dc (bit 11) ...");
        rc_ecmd |= scom_data.flushTo0();
        rc_ecmd |= scom_data.setBit(GP0_ABIST_MODE_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to set abist_mode_dc.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_GP0_OR_0x03000005, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing GP0 OR mask 0x03000005 (bit 11) to set abist_mode_dc.");
            break;
        }


        //   Write SCOM      address 0x03030007  0x0000000000000000     Write CC Scan Region Reg, set all bits='0' prior clk start
        FAPI_DBG("Writing CC Scan Region Register to all zeros prior to clock start ...");
        rc_ecmd |= scom_data.flushTo0();
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to flush Scan Region Register.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_CLK_SCANSEL_0x03030007, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing CC Scan Region Register 0x03030007 to all zeros prior to clock start.");
            break;
        }


        //
        // Centaur-specific clock start actions:


        //   Write SCOM      address 0x03030006  data=0x4FE0 0600 0000 0000    unicast, write CC clock region reg in MEM chiplet. start clocks
        FAPI_DBG("Writing CC Clock Region Register to 0x4FE0060000000000 to start array and nsl clocks ...");
        rc_ecmd |= scom_data.setDoubleWord(0, CLK_REGION_REG_DATA_TO_START_NSL_ARY);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to start array and nsl clocks.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_CLK_REGION_0x03030006, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing CC Clock Region Register 0x03030006 to 0x4FE0060000000000 to start array and nsl clocks.");
            break;
        }


        //   Write SCOM      address 0x03030006  data=0x4FE0 0E00 0000 0000    unicast, write CC clock region reg in MEM chiplet. start clocks
        FAPI_DBG("Writing CC Clock Region Register to 0x4FE00E0000000000 to start sl clocks ...");
        rc_ecmd |= scom_data.setDoubleWord(0, CLK_REGION_REG_DATA_TO_START_ALL);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to start sl clocks.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_CLK_REGION_0x03030006, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing CC Clock Region Register 0x03030006 to 0x4FE00E0000000000 to start sl clocks.");
            break;
        }


        //   Read SCOM       address 0x03030008  expect=0x0000 001F FFFF FFFF  unicast, read clock status reg in MEM chiplet
        FAPI_DBG("Reading CC Clock Status Register to see if clocks are running ...");
        rc = fapiGetScom( i_target, MEM_CLK_STATUS_0x03030008, scom_data);
        if ( rc )
        {
            FAPI_ERR("Error reading CC Clock Status Register 0x03030008.");
            break;
        }
        if ( scom_data.getDoubleWord(0) != MEM_CLK_STATUS_REG_EXP_DATA )
        {
            FAPI_ERR("Unexpected clock status! Clk Status Reg 0x03030008 = %16llX, but %16llX was expected.",scom_data.getDoubleWord(0),MEM_CLK_STATUS_REG_EXP_DATA);
            uint64_t MEM_CLK_STATUS_REG = scom_data.getDoubleWord(0);
            const fapi::Target & MEMBUF_CHIP_IN_ERROR = i_target;
            FAPI_SET_HWP_ERROR(rc, RC_CEN_MEM_STARTCLOCKS_UNEXPECTED_CLOCK_STATUS);
            break;
        }


        // The clocks for the NEST chiplet are started in the cen_sbe_startclocks.S procedure.    So don't do them here!
        //   Write SCOM      address 0x02030006  data=0x4FE0 0600 0000 0000    unicast, write CC clock region reg in NEST chiplet. start clocks
        //   Write SCOM      address 0x02030006  data=0x4FE0 0E00 0000 0000    unicast, write CC clock region reg in NEST chiplet. start clocks
        //   Read SCOM       address 0x02030008  expect=0x0000 001F FFFF FFFF  unicast, read clock status reg in NEST chiplet


        //   Write CFAM      address 0x13        bit(02)=0b1       Set MemReset Stability Control
        FAPI_DBG("Writing FSI GP4 register (bit2) to set MemReset Stability control ...");
        rc = fapiGetCfamRegister( i_target, CFAM_FSI_GP4_0x00001013, cfam_data);
        if (rc)
        {
            FAPI_ERR("Error reading FSI GP4 Regiter 0x00001013.");
            break;
        }
        rc_ecmd |= cfam_data.setBit(FSI_GP4_MEMRESET_STABILITY_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to set MemReset Stability control.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutCfamRegister( i_target, CFAM_FSI_GP4_0x00001013, cfam_data);
        if (rc)
        {
            FAPI_ERR("Error writing FSI GP4 0x00001013 (bit 2) to set MemReset Stability control.");
            break;
        }


        //   Write CFAM      address 0x13        bit(04)=0b1     Release D3PHY PLL Reset Control
        FAPI_DBG("Writing FSI GP4 register (bit4) to release D3PHY PLL Reset Control ...");
        rc = fapiGetCfamRegister( i_target, CFAM_FSI_GP4_0x00001013, cfam_data);
        if (rc)
        {
            FAPI_ERR("Error reading FSI GP4 Regiter 0x00001013.");
            break;
        }
        rc_ecmd |= cfam_data.setBit(FSI_GP4_DPHY_PLLRESET_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to release D3PHY PLL Reset Control.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutCfamRegister( i_target, CFAM_FSI_GP4_0x00001013, cfam_data);
        if (rc)
        {
            FAPI_ERR("Error writing FSI GP4 0x00001013 (bit 4) to release D3PHY PLL Reset Control.");
            break;
        }


        //
        // More common clock start actions:


        //   Write SCOM      address 0x03000004  bit(3)=0b0    clear force_align in all Chiplets in GP0
        FAPI_DBG("Writing GP0 AND mask to clear force_align (bit 3) ...");
        rc_ecmd |= scom_data.flushTo1();
        rc_ecmd |= scom_data.clearBit(GP0_FORCE_ALIGN_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to clear force_align.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_GP0_AND_0x03000004, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing GP0 AND mask 0x03000004 (bit 3) to clear force_align.");
            break;
        }


        //   Write SCOM      address 0x03000004  bit(2)=0b0    clear flushmode_inhibit in Chiplet in GP0
        FAPI_DBG("Writing GP0 AND mask to clear flushmode_inhibit (bit 2) ...");
        rc_ecmd |= scom_data.flushTo1();
        rc_ecmd |= scom_data.clearBit(GP0_FLUSHMODE_INHIBIT_BIT);
        if (rc_ecmd)
        {
            FAPI_ERR("Error 0x%x setting up ecmd data buffer to clear flushmode_inhibit.",  rc_ecmd);
            rc.setEcmdError(rc_ecmd);
            break;
        }
        rc = fapiPutScom( i_target, MEM_GP0_AND_0x03000004, scom_data);
        if (rc)
        {
            FAPI_ERR("Error writing GP0 AND mask 0x03000004 (bit 2) to clear flushmode_inhibit.");
            break;
        }


        // The enablement of RI and DI is done in cen_sbe_startclocks.   It does not need to be done here.


    } while(0);

    FAPI_INF("********* cen_mem_startclocks complete *********");
    return rc;
}

} //end extern C




/*
*************** Do not edit this area ***************
This section is automatically updated by CVS when you check in this file.
Be sure to create CVS comments when you commit so that they can be included here.
$Log: cen_mem_startclocks.C,v $
Revision 1.13  2014/04/07 19:01:06  gollub

#|   1.55  | gollub   |07-APR-14| Added dependancy on mss_unmask_errors for cen_mem_startclocks.C

Revision 1.12  2013/11/15 16:30:02  mfred
Changes made by Mike Jones for gerrit review, mostly for improved error handling.

Revision 1.11  2013/07/08 13:38:27  mfred
Change one hwp_error usage from RC_MSS_UNEXPECTED_CLOCK_STATUS to RC_MSS_UNEXPECTED_MEM_CLK_STATUS.

Revision 1.10  2013/03/04 17:56:29  mfred
Add some header comments for BACKUP and SCREEN.

Revision 1.9  2012/06/07 13:52:27  jmcgill
use independent data buffers for cfam/scom accesses

Revision 1.8  2012/06/06 20:04:59  jmcgill
change FSI GP3/GP4/status register accesses from SCOM->CFAM

Revision 1.7  2012/05/31 18:29:20  mfred
Updates for RC checking and error messages, etc.

Revision 1.6  2012/05/09 21:26:40  mfred
Removed setting of RI, DI.  Added error checking to ecmdDataBuffer operations.  Removed unneeded statements.

Revision 1.5  2012/05/02 15:32:30  mfred
Take out some comments and unnecessary code

Revision 1.4  2012/04/26 15:29:55  mfred
fix some messages and comment out FIR error for now.

Revision 1.3  2012/04/26 14:35:34  mfred
Some fixes.

Revision 1.2  2012/03/26 13:30:24  mfred
Replace place_holder error msgs with real error msgs.

Revision 1.1  2012/03/23 20:34:32  mfred
Check in initial version of cen_mem_startclocks

*/

