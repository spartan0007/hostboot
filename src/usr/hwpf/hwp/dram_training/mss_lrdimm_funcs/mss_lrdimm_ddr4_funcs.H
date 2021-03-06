/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwpf/hwp/dram_training/mss_lrdimm_funcs/mss_lrdimm_ddr4_funcs.H $ */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2016                        */
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
// $Id: mss_lrdimm_ddr4_funcs.H,v 1.3 2015/10/28 14:52:02 rwheeler Exp $
//------------------------------------------------------------------------------
// *! (C) Copyright International Business Machines Corp. 2013
// *! All Rights Reserved -- Property of IBM
// *! ***  ***
//------------------------------------------------------------------------------
// *! TITLE : mss_lrdimm_funcs.H
// *! DESCRIPTION : Tools for lrdimm centaur procedures
// *! OWNER NAME : KCOOK
// *! BACKUP NAME : MWUU
// #! ADDITIONAL COMMENTS :
//
// CCS related and general utility functions.
// Provides functions for mss_eff_conifg, mss_draminit, and mss_draminit_training
// for DDR4 LRDIMM.

//------------------------------------------------------------------------------
// Don't forget to create CVS comments when you check in your changes!
//------------------------------------------------------------------------------
// CHANGE HISTORY:
//------------------------------------------------------------------------------
// Version:|  Author: |  Date:  | Comment:
//---------|----------|---------|-----------------------------------------------
//  1.3    | 10/28/15 |rwheeler | added the ddrphy_pc_config1 reg definition.
//  1.1    | 03/14/14 | kcook   | First drop of Centaur

#ifndef _MSS_LRDIMM_DDR4_FUNCS_H
#define _MSS_LRDIMM_DDR4_FUNCS_H

//#define LRDIMM 1

//----------------------------------------------------------------------
//  Constants 
//----------------------------------------------------------------------
const uint64_t MAINT0_MBA_MAINT_BUFF0_DATA_ECC0_0x0301065d = 0x0301065d;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P0_0_0x800000010301183F = 0x800000010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P0_1_0x800004010301183F = 0x800004010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P0_2_0x800008010301183F = 0x800008010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P0_3_0x80000C010301183F = 0x80000C010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P0_4_0x800010010301183F = 0x800010010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P1_0_0x800100010301183F = 0x800100010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P1_1_0x800104010301183F = 0x800104010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P1_2_0x800108010301183F = 0x800108010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P1_3_0x80010C010301183F = 0x80010C010301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE1_P1_4_0x800110010301183F = 0x800110010301183Full;
//
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P0_0_0x800000000301183F = 0x800000000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P0_1_0x800004000301183F = 0x800004000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P0_2_0x800008000301183F = 0x800008000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P0_3_0x80000C000301183F = 0x80000C000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P0_4_0x800010000301183F = 0x800010000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P1_0_0x800100000301183F = 0x800100000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P1_1_0x800104000301183F = 0x800104000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P1_2_0x800108000301183F = 0x800108000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P1_3_0x80010C000301183F = 0x80010C000301183Full;
const uint64_t DPHY23_DDRPHY_DP18_DATA_BIT_ENABLE0_P1_4_0x800110000301183F = 0x800110000301183Full;
//                                                                            
const uint64_t DPHY01_DDRPHY_PC_CONFIG1_P0_0x8000C00D0301143F = 0x8000C00D0301143Full;
const uint64_t DPHY01_DDRPHY_PC_CONFIG1_P1_0x8001C00D0301143F = 0x8001C00D0301143Full;
const uint64_t DPHY23_DDRPHY_PC_CONFIG1_P0_0x8000C00D0301183F = 0x8000C00D0301183Full;
const uint64_t DPHY23_DDRPHY_PC_CONFIG1_P1_0x8001C00D0301183F = 0x8001C00D0301183Full;

//----------------------------------------------------------------------
//  Enums 
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// LRDIMM FUNCS
//----------------------------------------------------------------------
//--------------------------------------------------------------
// mss_create_db_ddr4  
// Determines DB control words and stores in attribute
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_create_db_ddr4( const fapi::Target& i_target_mba);

//--------------------------------------------------------------
// mss_lrdimm_ddr4_term_atts  
// eff config termination rewrite odts 
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_lrdimm_ddr4_term_atts( const fapi::Target& i_target_mba);

//--------------------------------------------------------------
// mss_lrdimm_ddr4_db_load  
// Writes initial  DB control words to DB from attributes
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_lrdimm_ddr4_db_load( fapi::Target& i_target, 
                                          uint32_t i_port_number, 
                                          uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_bcw_write 
// Writes single BCW to DB
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_bcw_write( fapi::Target& i_target_mba, uint32_t i_port_number,
                         uint8_t bcw_width, uint8_t bcw, uint8_t bcw_value,
                         uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_dram_write_leveling
// Executes DB-DRAM write leveing
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_dram_write_leveling( fapi::Target& i_target_mba, uint32_t i_port_number);

//--------------------------------------------------------------
// mss_store_db_delay
// Used at end of training steps to write found delay values to DB control word registers
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_store_db_delay(fapi::Target& i_target_mba, uint8_t i_mbaPosition, uint32_t i_port_number,
                                    uint32_t i_dimm_number, uint32_t i_rank_number,
                                    uint8_t i_cw_reg, uint8_t i_nibble_delay[],
                                    uint32_t& io_ccs_inst_cnt, uint8_t i_split_fine=0);
//--------------------------------------------------------------
// mss_step_delay_cw0
// Used in mxd training to step DB CW  delay register and query data bus
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_step_delay_cw0(fapi::Target& i_target_mba, uint8_t i_port_number, uint8_t i_dimm_number, uint8_t i_rank_number,
		uint8_t i_num_wr_rd, uint8_t o_nibble_delay[], uint8_t i_type, uint32_t& io_ccs_inst_cnt);
//--------------------------------------------------------------
// mss_step_delay_cw_tran
// Used in training to step DB CW  delay register and query data bus (works for both 0->1 transtion and 1->0 transition)
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_step_delay_cw_tran(fapi::Target& i_target_mba, uint32_t i_port_number, uint32_t i_dimm_number, uint32_t i_rank_number,
		                              uint8_t i_cw_reg, uint8_t i_num_wr_rd, uint8_t o_nibble_delay[], uint8_t i_type, uint32_t& io_ccs_inst_cnt);
//--------------------------------------------------------------
// mss_mr1_wr_lvl  
// Send MR1 command to set DRAM to write leveling mode or normal mode
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_mr1_wr_lvl(fapi::Target& i_target_mba, uint32_t i_port_number,
                                uint8_t wr_lvl, uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_mrep_training
// Conducts MDQ Receive Enable Phase Training between DB and DRAM
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_mrep_training( fapi::Target& i_target_mba, uint32_t i_port_number);
//--------------------------------------------------------------
// mss_db_mpr
// Write the db Multi purpose register (mpr)
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_db_mpr( fapi::Target& i_target_mba, uint8_t i_port_number, uint8_t i_pattern, uint32_t& io_ccs_inst_cnt);


//--------------------------------------------------------------
// mss_custom_DB_MPR_pattern
// custom db mpr pattern
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_custom_DB_MPR_pattern( fapi::Target& i_target_mba, uint8_t i_port_number);
//--------------------------------------------------------------
// mss_hostside_read_training_enable
// enable bata buffer to Host Interface read training
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_hostside_read_training_enable( fapi::Target& i_target_mba, uint8_t i_port_number);
//--------------------------------------------------------------
// mss_hostside_read_training_disable
// enable bata buffer to Host Interface read training
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_hostside_read_training_disable( fapi::Target& i_target_mba, uint8_t i_port_number);

//--------------------------------------------------------------
// disable Host Interface Write Leveling
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_hwl_training_disable( fapi::Target& i_target_mba, uint8_t i_port_number);
//--------------------------------------------------------------
// mss_hwl_training_enable
// enable Host Interface Write Leveling
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_hwl_training_enable( fapi::Target& i_target_mba, uint8_t i_port_number);
//--------------------------------------------------------------
// mss_mxd_training
// Conducts MRD or MWD coarse, normal, or find training. Still in development
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_mxd_training( fapi::Target& i_target_mba, uint8_t i_port_number, uint8_t i_type);
//--------------------------------------------------------------
// mss_add_rdmpr 
// Adds read command without activate to ccs
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_add_rdmpr( fapi::Target& i_target_mba, 
                               uint32_t i_port_number, uint32_t dimm_number, uint32_t rank_number,
                               uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_add_nop 
// Adds read command without activate to ccs
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_add_nop( fapi::Target& i_target_mba, 
                               uint32_t i_port_number, uint32_t dimm_number, uint32_t repeat_number,
                               uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_add_act_write 
// Adds write command with activate to ccs
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_add_act_write( fapi::Target& i_target_mba, 
                                    uint32_t i_port_number, uint32_t dimm_number, uint32_t rank_number, uint32_t i_address,
                                    uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_add_act_read 
// Adds write command with activate to ccs
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_add_act_read( fapi::Target& i_target_mba, 
                                    uint32_t i_port_number, uint32_t dimm_number, uint32_t rank_number, uint32_t i_address,
                                    uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_mpr_operation
// Sets MR3 command to MPR data flow or normal data flow
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_mpr_operation( fapi::Target& i_target_mba, uint32_t i_port_number,
                             uint8_t mpr_op,
                         uint32_t& io_ccs_inst_cnt);

//--------------------------------------------------------------
// mss_force_fifo_capture
// Sets force_fifo_capture bit in rd_dia_config5 registers to Force DQ capture or normal operation
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_force_fifo_capture( fapi::Target& i_target_centaur, uint8_t i_mbaPosition, 
                                        uint32_t i_port_number, uint32_t force_fifo);

//--------------------------------------------------------------
// mss_force_fifo_reliability
// Sets loopback_fix_en bit in rd_dia_config5(bit 60) registers to reset the fifo pointers 
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_force_fifo_reliability( fapi::Target& i_target_centaur, uint8_t i_mbaPosition,
                                            uint32_t i_port_number, uint32_t reliability);
//--------------------------------------------------------------
// mss_data_bit_set
// Sets single DQ byte or all DQ bytes to 0 or 1 through DATA_BIT_DIR registers. 
// Used with DFT_FORCE_OUTPUT during PBA mode
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_data_bit_set(fapi::Target& i_target_mba, uint8_t i_mbaPosition, 
                                        uint32_t i_port_number,
                            uint8_t byte, uint8_t dq_value);

//--------------------------------------------------------------
// mss_dft_force_outputs
// Sets DFT_FORCE_OUTPUTS bit to 0 or 1 to control DQ bus during PBA mode
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_dft_force_outputs(fapi::Target& i_target_centaur, uint8_t i_mbaPosition, 
                                        uint32_t i_port_number,
                            uint32_t force_outputs);

//--------------------------------------------------------------
// mss_dq_outputs
// Sets DQ OUTPUTS enable bit to 0 or 1 to control DQ bus during MWD mode
// Target = centaur.mba
//--------------------------------------------------------------
fapi::ReturnCode mss_dq_outputs(fapi::Target& i_target_centaur, uint8_t i_mbaPosition, 
                                        uint32_t i_port_number,
                            uint32_t force_outputs);

//--------------------------------------------------------------







#endif /* _MSS_LRDIMM_DDR4_FUNCS_H */

