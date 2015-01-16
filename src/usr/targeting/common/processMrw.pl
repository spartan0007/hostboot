#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/usr/targeting/common/processMrw.pl $
#
# OpenPOWER HostBoot Project
#
# Contributors Listed Below - COPYRIGHT 2015
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.
#
# IBM_PROLOG_END_TAG

use strict;
use XML::Simple;
use Data::Dumper;
use Targets;
use Math::BigInt;
use Getopt::Long;
use File::Basename;

my $VERSION = "1.0.0";

my $force          = 0;
my $serverwiz_file = "";
my $version        = 0;
my $debug          = 0;
my $report         = 0;
my $sdr_file       = "";

GetOptions(
    "f"   => \$force,             # numeric
    "x=s" => \$serverwiz_file,    # string
    "d"   => \$debug,
    "v"   => \$version,
    "r"   => \$report,
  )                               # flag
  or printUsage();

if ($version == 1)
{
    die "\nprocessMrw.pl\tversion $VERSION\n";
}

if ($serverwiz_file eq "")
{
    printUsage();
}

$XML::Simple::PREFERRED_PARSER = 'XML::Parser';

my $targetObj = Targets->new;
if ($force == 1)
{
    $targetObj->{force} = 1;
}
if ($debug == 1)
{
    $targetObj->{debug} = 1;
}

$targetObj->setVersion($VERSION);
my $xmldir = dirname($serverwiz_file);
$targetObj->loadXML($serverwiz_file);

#--------------------------------------------------
## loop through all targets and do stuff
foreach my $target (sort keys %{ $targetObj->getAllTargets() })
{
    my $type = $targetObj->getType($target);
    if ($type eq "SYS")
    {
        processSystem($targetObj, $target);
    }
    elsif ($type eq "PROC")
    {
        processProcessor($targetObj, $target);
    }
    elsif ($type eq "MEMBUF")
    {
        processMembuf($targetObj, $target);
    }
    elsif ($type eq "FSP")
    {
        processBmc($targetObj, $target);
    }
}


## check topology
foreach my $n (keys %{$targetObj->{TOPOLOGY}}) {
    foreach my $p (keys %{$targetObj->{TOPOLOGY}->{$n}}) {
        if ($targetObj->{TOPOLOGY}->{$n}->{$p} > 1) {
            print "ERROR: Fabric topology invalid.  2 targets have same ".
                  "FABRIC_NODE_ID,FABRIC_CHIP_ID ($n,$p)\n";
            $targetObj->myExit(3);
        }
    }
}
## check for errors
if ($targetObj->getMasterProc() eq "")
{
    print "ERROR: Master Processor not defined.  Please instaitant a BMC
 and connect LPC bus\n";
    $targetObj->myExit(3);
}
foreach my $target (keys %{ $targetObj->getAllTargets() })
{
    errorCheck($targetObj, $target);
}

#--------------------------------------------------
## write out final XML
my $xml_fh;
my $filename = $xmldir . "/" . $targetObj->getSystemName() . "_hb.mrw.xml";
print "Creating XML: $filename\n";
open($xml_fh, ">$filename") || die "Unable to create: $filename";
$targetObj->printXML($xml_fh, "top");
close $xml_fh;
if (!$targetObj->{errorsExist})
{
    print "MRW created successfully!\n";
}


## optionally print out report
if ($report)
{
    my $report_file = $xmldir . "/" . $targetObj->getSystemName() . ".rpt";
    open(SUM,">$report_file") || die "Unable to create: $report_file\n";
    my $ref = $targetObj->{targeting}->{SYS}[0]{NODES}[0]{PROCS};
    foreach my $proc (@{$ref})
    {
        foreach my $mcs (@{$proc->{MCSS}})
        {
            my $mcs_target = $mcs->{KEY};
            my $membuf=$mcs->{MEMBUFS}[0];
            my $membuf_target = $membuf->{KEY};

            my $sch = $targetObj->getAttribute($mcs_target,
                      "SCHEMATIC_INTERFACE");
            my $aff = $targetObj->getAttribute($mcs_target,"AFFINITY_PATH");
            my $huid = $targetObj->getAttribute($mcs_target,"HUID");
            print SUM "$sch | $huid | $mcs_target | $aff\n";
            if ($membuf_target ne "") {
                foreach my $mba (@{$membuf->{MBAS}}) {
                    my $mba_target = $mba->{KEY};

                    $huid = $targetObj->getAttribute($mba_target,"HUID");
                    $aff = $targetObj->getAttribute($mcs_target,
                                   "AFFINITY_PATH");
                    print SUM "\t | $huid | $mba_target | $aff\n";
                    foreach my $dimm (@{$mba->{DIMMS}}) {
                        my $dimm_target = $dimm->{KEY};
                        $aff = $targetObj->getAttribute($dimm_target,
                                   "AFFINITY_PATH");
                        $huid = $targetObj->getAttribute($dimm_target,"HUID");
                        my $p = $targetObj->getAttribute($dimm_target,
                                  "MBA_PORT");
                        my $d = $targetObj->getAttribute($dimm_target,
                                   "MBA_DIMM");
                        my $i2c = $targetObj->getAttributeField($dimm_target,
                                  "EEPROM_VPD_PRIMARY_INFO","devAddr");
                        my $sens = $targetObj->getAttribute($dimm_target,
                                  "IPMI_SENSORS");
                        my @s = split(/\,/,$sens);

                        print SUM "\t\t$huid | $dimm_target".
                                  " | $aff | $p | $d | $i2c | ".
                                  "$s[0],$s[1] | $s[2],$s[3]\n";
                    }
                }
            }
        }
    }
    close SUM;
}

#--------------------------------------------------
#--------------------------------------------------
## Processing subroutines

#--------------------------------------------------
## SDRs
##

sub processSdr
{
    my $targetObj = shift;
    my $target = shift;
    my $sdrObj = shift;

    my @ipmi_sensors;
    my %id_lu;

    if ($targetObj->isBadAttribute($target,"ENTITY_ID_LOOKUP")==1)
    {
        return;
    }
    my $entity_id_lookup = $targetObj->getAttribute($target,"ENTITY_ID_LOOKUP");
    if ($entity_id_lookup ne "") {
        my $pos=0;
        if ($targetObj->isBadAttribute($target,"IPMI_INSTANCE")==1)
        {
            $pos = $targetObj->getAttribute($target,"ENTITY_INSTANCE");
        }
        else
        {
            $pos = $targetObj->getAttribute($target,"IPMI_INSTANCE");
        }
        print  "$entity_id_lookup,$pos\n";
        my @ids = split(/,/,$entity_id_lookup);
        foreach my $i (@ids)
        {
            my $i_int = oct($i);
            if ($i_int>0)
            {
                $id_lu{$i_int}=1;
            }
        }
        foreach my $s (@{$sdrObj->{data}})
        {
            my $sdr_entity_id = oct($s->{entity_id});
            if ($s->{name}=~/Core/) {
                $sdr_entity_id=208;
            }
            if ($id_lu{$sdr_entity_id}>0)
            {
                my $sensor_id=$s->{sensor_id};
                my $instance_id=oct($s->{entity_instance});
                my $sensor_type=oct($s->{sensor_type});
                if ($instance_id==$pos)
                {
                    print "IMPORTING: $target; pos=$pos; entity_id=".
                           "$sdr_entity_id; ".
                           "sensor_id=$sensor_id; sensor_type=$sensor_type\n";
                    my $eid=sprintf("0x%02x%02x",$sensor_type,$sdr_entity_id);
                    my $sid=sprintf("0x%02x",$s->{sensor_id});
                    push(@ipmi_sensors,$eid.",".$sid);
                    $id_lu{$sdr_entity_id}++;
                }
            }
        }
        foreach my $k (keys(%id_lu)) {
            if ($id_lu{$k}==1)
            {
                print "WARNING: $target; pos=$pos; entity_id=$k; ".
                      "not imported\n";
                #$targetObj->myExit(3);
            }
        }
        for (my $i=@ipmi_sensors;$i<16;$i++)
        {
            push(@ipmi_sensors,"0xFFFF,0xFF");
        }
        my $ipmi = join(',',@ipmi_sensors);
        $targetObj->setAttribute($target,"IPMI_SENSORS",$ipmi);
    }
}

#--------------------------------------------------
## System
##

sub processSystem
{
    my $targetObj = shift;
    my $target    = shift;

    $targetObj->setAttribute($target, "MAX_MCS_PER_SYSTEM",
        $targetObj->{NUM_PROCS_PER_NODE} * $targetObj->{MAX_MCS});
    $targetObj->setAttribute($target, "MAX_PROC_CHIPS_PER_NODE",
        $targetObj->{NUM_PROCS_PER_NODE});
    parseBitwise($targetObj,$target,"CDM_POLICIES");
}
sub processBmc
{
    my $targetObj = shift;
    my $target    = shift;
    my $i2cs=$targetObj->findConnections($target,"I2C","PROC");
    if ($i2cs ne "")
    {
       foreach my $i2c (@{$i2cs->{CONN}})
       {
           my $addr=$targetObj->getBusAttribute(
                $i2c->{SOURCE},$i2c->{BUS_NUM},"I2C_ADDRESS");
           $targetObj->setAttribute(
                $i2c->{DEST_PARENT},"I2C_SLAVE_ADDRESS",$addr);
       }
    }
    my $lpcs=$targetObj->findConnections($target,"LPC","PROC");
    if ($lpcs ne "")
    {
       my $lpc=$lpcs->{CONN}->[0];
       $targetObj->setMasterProc($lpc->{DEST_PARENT});
    }
}


sub parseBitwise
{
    my $targetObj = shift;
    my $target = shift;
    my $attribute = shift;

    my $mask = 0;
    foreach my $e (keys %{ $targetObj->getEnumHash($attribute) }) {
        my $field = $targetObj->getAttributeField(
                    $target,$attribute."_BITMASK",$e);
        my $val=hex($targetObj->getEnumValue($attribute,$e));
        if ($field eq "true")
        {
            $mask=$mask | $val;
        }
    }
    $targetObj->setAttribute($target,$attribute,$mask);
 }
#--------------------------------------------------
## Processor
##

sub processProcessor
{
    my $targetObj = shift;
    my $target    = shift;

    #########################
    ## Copy PCIE attributes from socket
    ## In serverwiz, processor instances are not unique
    ## because plugged into socket
    ## so processor instance unique attributes are socket level.
    ## The grandparent is guaranteed to be socket.
    my $socket_target =
       $targetObj->getTargetParent($targetObj->getTargetParent($target));
    $targetObj->copyAttribute($socket_target,$target,"LOCATION_CODE");
    $targetObj->copyAttribute($socket_target,$target,"FRU_ID");


    foreach my $attr (sort (keys
           %{ $targetObj->getTarget($socket_target)->{TARGET}->{attribute} }))
    {
        if ($attr =~ /PROC\_PCIE/)
        {
            $targetObj->copyAttribute($socket_target,$target,$attr);
        }
    }
    $targetObj->log($target, "Processing PROC");
    foreach my $child (@{ $targetObj->getTargetChildren($target) })
    {
        $targetObj->log($target, "Processing PROC child: $child");
        my $child_type = $targetObj->getType($child);
        if ($child_type eq "NA" || $child_type eq "FSI")
        {
            $child_type = $targetObj->getMrwType($child);
        }
        if ($child_type eq "ABUS")
        {
            processAbus($targetObj, $child);
        }
        elsif ($child_type eq "XBUS")
        {
            processXbus($targetObj, $child);
        }
        elsif ($child_type eq "FSIM" || $child_type eq "FSICM")
        {
            processFsi($targetObj, $child, $target);
        }
        elsif ($child_type eq "PCI_CONFIGS")
        {
            foreach my $pci_child (@{ $targetObj->getTargetChildren($child) })
            {
               processPcie($targetObj, $pci_child, $target);
            }
        }
        elsif ($child_type eq "MCS")
        {
            processMcs($targetObj, $child, $target);
        }
        elsif ($child_type eq "OCC")
        {
            processOcc($targetObj, $child, $target);
        }
    }

    ## update path for mvpd's and sbe's
    my $path  = $targetObj->getAttribute($target, "PHYS_PATH");
    my $model = $targetObj->getAttribute($target, "MODEL");

    $targetObj->setAttributeField($target,
        "EEPROM_VPD_PRIMARY_INFO","i2cMasterPath",$path);
    $targetObj->setAttributeField($target,
        "EEPROM_VPD_BACKUP_INFO","i2cMasterPath",$path);
    $targetObj->setAttributeField($target,
        "EEPROM_SBE_PRIMARY_INFO","i2cMasterPath",$path);
    $targetObj->setAttributeField($target,
        "EEPROM_SBE_BACKUP_INFO","i2cMasterPath",$path);
    $targetObj->setAttributeField($target,
        "EEPROM_VPD_FRU_INFO","i2cMasterPath",$path);

    ## initialize master processor FSI's
    $targetObj->setAttributeField($target, "FSI_OPTION_FLAGS", "flipPort", "0");

    if ($target eq $targetObj->getMasterProc())
    {
        $targetObj->setAttributeField($target, "FSI_OPTION_FLAGS", "reserved",
            "0");
        $targetObj->setAttribute($target, "FSI_MASTER_CHIP",    "physical:sys");
        $targetObj->setAttribute($target, "FSI_MASTER_PORT",    "0xFF");
        $targetObj->setAttribute($target, "ALTFSI_MASTER_CHIP", "physical:sys");
        $targetObj->setAttribute($target, "ALTFSI_MASTER_PORT", "0xFF");
        $targetObj->setAttribute($target, "FSI_MASTER_TYPE",    "NO_MASTER");
        $targetObj->setAttribute($target, "FSI_SLAVE_CASCADE",  "0");
        $targetObj->setAttribute($target, "PROC_MASTER_TYPE", "ACTING_MASTER");
    }
    else
    {
        $targetObj->setAttribute($target, "PROC_MASTER_TYPE",
            "MASTER_CANDIDATE");
    }
    ## Update bus speeds
    processI2cSpeeds($targetObj,$target);

    ## these are hardcoded because code sets them properly
    $targetObj->setAttributeField($target, "SCOM_SWITCHES", "reserved",   "0");
    $targetObj->setAttributeField($target, "SCOM_SWITCHES", "useFsiScom", "1");
    $targetObj->setAttributeField($target, "SCOM_SWITCHES", "useInbandScom",
        "0");
    $targetObj->setAttributeField($target, "SCOM_SWITCHES", "useXscom", "0");

    processMembufVpdAssociation($targetObj,$target);
    setupBars($targetObj,$target);
}


sub processI2cSpeeds
{
    my $targetObj = shift;
    my $target    = shift;

    my @bus_speeds;
    my $bus_speed_attr=$targetObj->getAttribute($target,"I2C_BUS_SPEED_ARRAY");
    my @bus_speeds2 = split(/,/,$bus_speed_attr);
    $bus_speeds[0][0] = $bus_speeds2[0];
    $bus_speeds[0][1] = $bus_speeds2[1];
    $bus_speeds[0][2] = $bus_speeds2[2];
    $bus_speeds[1][0] = $bus_speeds2[3];
    $bus_speeds[1][1] = $bus_speeds2[4];
    $bus_speeds[1][2] = $bus_speeds2[5];

    my $i2cs=$targetObj->findConnections($target,"I2C","");
    if ($i2cs ne "") {
        foreach my $i2c (@{$i2cs->{CONN}}) {
            my $port=oct($targetObj->getAttribute($i2c->{SOURCE},"I2C_PORT"));
            my $engine=oct($targetObj->getAttribute(
                           $i2c->{SOURCE},"I2C_ENGINE"));
            my $bus_speed=$targetObj->getBusAttribute(
                  $i2c->{SOURCE},$i2c->{BUS_NUM},"I2C_SPEED");
            if ($bus_speed eq "" || $bus_speed==0) {
                print "ERROR: I2C bus speed not defined for $i2c->{SOURCE}\n";
                $targetObj->myExit(3);
            }
            ## choose lowest bus speed
            if ($bus_speeds[$engine][$port] eq "" ||
                  $bus_speeds[$engine][$port]==0  ||
                  $bus_speed < $bus_speeds[$engine][$port]) {
                $bus_speeds[$engine][$port] = $bus_speed;
            }
        }
    }
    $bus_speed_attr = $bus_speeds[0][0].",".
                      $bus_speeds[0][1].",".
                      $bus_speeds[0][2].",".
                      $bus_speeds[1][0].",".
                      $bus_speeds[1][1].",".
                      $bus_speeds[1][2];

    $targetObj->setAttribute($target,"I2C_BUS_SPEED_ARRAY",$bus_speed_attr);
}

################################
## Setup address map

sub setupBars
{
    my $targetObj = shift;
    my $target = shift;
    #--------------------------------------------------
    ## Setup BARs

    my $node = $targetObj->getAttribute($target, "FABRIC_NODE_ID");
    my $proc   = $targetObj->getAttribute($target, "FABRIC_CHIP_ID");
    $targetObj->{TOPOLOGY}->{$node}->{$proc}++;

    my @bars=("FSP_BASE_ADDR","PSI_BRIDGE_BASE_ADDR",
              "INTP_BASE_ADDR","PHB_BASE_ADDRS","PCI_BASE_ADDRS_32",
              "PCI_BASE_ADDRS_64","RNG_BASE_ADDR","IBSCOM_PROC_BASE_ADDR");

    foreach my $bar (@bars)
    {
        my ($num,$base,$node_offset,$proc_offset,$offset) = split(/,/,
               $targetObj->getAttribute($target,$bar));
        my $i_base = Math::BigInt->new($base);
        my $i_node_offset = Math::BigInt->new($node_offset);
        my $i_proc_offset = Math::BigInt->new($proc_offset);
        my $i_offset = Math::BigInt->new($offset);

        my $value="";
        if ($num==0)
        {
            $value=$base;
        }
        else
        {
            for (my $i=0;$i<$num;$i++)
            {
                my $b=sprintf("0x%016X",
         $i_base+$i_node_offset*$node+$i_proc_offset*$proc+$i_offset*$i);
                my $sep=",";
                if ($i==$num-1)
                {
                    $sep="";
                }
                $value=$value.$b.$sep;
            }
        }
        $targetObj->setAttribute($target,$bar,$value);
    }
}

#--------------------------------------------------
## MCS
##
sub processMcs
{
    my $targetObj    = shift;
    my $target       = shift;
    my $parentTarget = shift;

    my $node = $targetObj->getAttribute($parentTarget, "FABRIC_NODE_ID");
    my $proc   = $targetObj->getAttribute($parentTarget, "FABRIC_CHIP_ID");

    my ($base,$node_offset,$proc_offset,$offset) = split(/,/,
               $targetObj->getAttribute($target,"IBSCOM_MCS_BASE_ADDR"));
    my $i_base = Math::BigInt->new($base);
    my $i_node_offset = Math::BigInt->new($node_offset);
    my $i_proc_offset = Math::BigInt->new($proc_offset);
    my $i_offset = Math::BigInt->new($offset);

    my $mcs = $targetObj->getAttribute($target, "MCS_NUM");
    my $mcsStr=sprintf("0x%016X",
         $i_base+$i_node_offset*$node+$i_proc_offset*$proc+$i_offset*$mcs);
    $targetObj->setAttribute($target, "IBSCOM_MCS_BASE_ADDR", $mcsStr);
}


#--------------------------------------------------
## XBUS
##
## Finds XBUS connections and creates PEER TARGET attributes

sub processXbus
{
    my $targetObj = shift;
    my $target    = shift;

    # $targetObj->setAttribute($target, "PEER_TARGET","");

}

#--------------------------------------------------
## ABUS
##
## Finds ABUS connections and creates PEER TARGET attributes

sub processAbus
{
    my $targetObj = shift;
    my $target    = shift;

    my $found_abus = 0;
   $targetObj->setAttribute($target, "PEER_PATH","physical:na");
   $targetObj->setAttribute($target, "EI_BUS_TX_LANE_INVERT","0");
   $targetObj->setAttribute($target, "EI_BUS_TX_MSBSWAP","0");
   # $targetObj->setAttribute($target, "PEER_TARGET","");

    my $abus_child_conn = $targetObj->getFirstConnectionDestination($target);
    if ($abus_child_conn ne "")
    {
        ## set attributes for both directions
        my $aff1 = $targetObj->getAttribute($target, "AFFINITY_PATH");
        my $aff2 = $targetObj->getAttribute($abus_child_conn, "AFFINITY_PATH");

        $targetObj->setAttribute($abus_child_conn, "PEER_TARGET",
            $targetObj->getAttribute($target, "PHYS_PATH"));
        $targetObj->setAttribute($target, "PEER_TARGET",
            $targetObj->getAttribute($abus_child_conn, "PHYS_PATH"));

        $targetObj->setAttribute($abus_child_conn, "PEER_PATH",
            $targetObj->getAttribute($target, "PHYS_PATH"));
        $targetObj->setAttribute($target, "PEER_PATH",
            $targetObj->getAttribute($abus_child_conn, "PHYS_PATH"));

        # copy Abus attributes to proc
        my $abus = $targetObj->getFirstConnectionBus($target);
        $targetObj->setAttribute($target, "EI_BUS_TX_LANE_INVERT",
            $abus->{bus_attribute}->{SOURCE_TX_LANE_INVERT}->{default});
        $targetObj->setAttribute($target, "EI_BUS_TX_MSBSWAP",
            $abus->{bus_attribute}->{SOURCE_TX_MSBSWAP}->{default});
        $targetObj->setAttribute($abus_child_conn, "EI_BUS_TX_LANE_INVERT",
            $abus->{bus_attribute}->{DEST_TX_LANE_INVERT}->{default});
        $targetObj->setAttribute($abus_child_conn, "EI_BUS_TX_MSBSWAP",
            $abus->{bus_attribute}->{DEST_TX_MSBSWAP}->{default});
        $found_abus = 1;
    }
}

#--------------------------------------------------
## FSI
##
## Finds FSI connections and creates FSI MASTER attributes at endpoint target

sub processFsi
{
    my $targetObj    = shift;
    my $target       = shift;
    my $parentTarget = shift;
    my $type         = $targetObj->getBusType($target);

    ## fsi can only have 1 connection
    my $fsi_child_conn = $targetObj->getFirstConnectionDestination($target);

    ## found something on other end
    if ($fsi_child_conn ne "")
    {
        my $fsi_link = $targetObj->getAttribute($target, "FSI_LINK");
        my $fsi_port = $targetObj->getAttribute($target, "FSI_PORT");
        my $cmfsi = $targetObj->getAttribute($target, "CMFSI");
        my $flip_port         = 0;
        my $proc_path = $targetObj->getAttribute($parentTarget,"PHYS_PATH");
        my $fsi_child_target = $targetObj->getTargetParent($fsi_child_conn);
        $targetObj->setFsiAttributes($fsi_child_target,
                    $type,$cmfsi,$proc_path,$fsi_port,$flip_port);
    }
}

#--------------------------------------------------
## PCIE
##
## Creates attributes from abstract PCI attributes on bus

sub processPcie
{
    my $targetObj    = shift;
    my $target       = shift;
    my $parentTarget = shift;


    ## process pcie config target
    ## this is a special target whose children are the different ways
    ## to configure iop/phb's

    # TODO RTC: TBD
    # add a 3rd IOP for Naples

    ## Get config children
    my @lane_swap;
    $lane_swap[0][0] = 0;
    $lane_swap[0][1] = 0;
    $lane_swap[1][0] = 0;
    $lane_swap[1][1] = 0;

    my @lane_mask;
    $lane_mask[0][0] = "0x0000";
    $lane_mask[0][1] = "0x0000";
    $lane_mask[1][0] = "0x0000";
    $lane_mask[1][1] = "0x0000";

    my @lane_rev;
    $lane_rev[0][0] = "";
    $lane_rev[0][1] = "";
    $lane_rev[1][0] = "";
    $lane_rev[1][1] = "";

    my @is_slot;
    $is_slot[0][0] = 0;
    $is_slot[0][1] = 0;
    $is_slot[1][0] = 0;
    $is_slot[1][1] = 0;

    my $phb_config = "00000000";

    my %cfg_check;
    my @equalization;

    my $wiring_table = $targetObj->getAttribute($target,"PCIE_LANE_SWAP_TABLE");
    $wiring_table=~s/\s+//g;
    $wiring_table=~s/\t+//g;
    $wiring_table=~s/\n+//g;

    my @t = split(/,/,$wiring_table);
    my %iop_swap;

    #iop_swap{iop}{clk swap}{clk group reversal}
    $iop_swap{0}{0}{'00'}=$t[0];
    $iop_swap{0}{0}{'01'}=$t[1];
    $iop_swap{0}{0}{'10'}=$t[2];
    $iop_swap{0}{0}{'11'}=$t[3];
    $iop_swap{0}{1}{'00'}=$t[4];
    $iop_swap{0}{1}{'01'}=$t[5];
    $iop_swap{0}{1}{'10'}=$t[6];
    $iop_swap{0}{1}{'11'}=$t[7];

    $iop_swap{1}{0}{'00'}=$t[8];
    $iop_swap{1}{0}{'01'}=$t[9];
    $iop_swap{1}{0}{'10'}=$t[10];
    $iop_swap{1}{0}{'11'}=$t[11];
    $iop_swap{1}{1}{'00'}=$t[12];
    $iop_swap{1}{1}{'01'}=$t[13];
    $iop_swap{1}{1}{'10'}=$t[14];
    $iop_swap{1}{1}{'11'}=$t[15];

    my @lane_eq;
    my $NUM_PHBS=4;
    for (my $p=0;$p<$NUM_PHBS;$p++)
    {
        for (my $lane=0;$lane<16;$lane++)
        {
           $equalization[$p][$lane] = "0x00,0x00";
        }
    }
    my $found=0;
    foreach my $child (@{ $targetObj->getTargetChildren($target) })
    {
        my $num_connections = $targetObj->getNumConnections($child);
        if ($num_connections > 0)
        {
            $found=1;
            my $pci_endpoint=$targetObj->getFirstConnectionDestination($child);

            my $bus = $targetObj->getConnectionBus($target, 0);
            my $iop_num = $targetObj->getAttribute($child, "IOP_NUM");
            my $swap_clks=$targetObj->getBusAttribute($child, 0,
                         "PCIE_SWAP_CLKS");

            my $lane_rev=$targetObj->getBusAttribute($child, 0,
                         "LANE_REVERSAL");

            my $phb_num = $targetObj->getAttribute($child, "PHB_NUM");
            my $lane_set = $targetObj->getAttribute($child, "PCIE_LANE_SET");
            my $capi = $targetObj->getAttribute($child, "ENABLE_CAPI");

            my $pci_endpoint_type =
              $targetObj->getAttribute(
                $targetObj->getTargetParent($pci_endpoint), "CLASS");

            if ($pci_endpoint_type eq "CARD")
            {
                $is_slot[$iop_num][$lane_set] = 1;
            }
            $lane_swap[$iop_num][$lane_set] =
              $targetObj->getBusAttribute($child, 0, "PCIE_SWAP_CLKS");
            $lane_mask[$iop_num][$lane_set] =
              $targetObj->getAttribute($child, "PCIE_LANE_MASK");
            $lane_rev[$iop_num][$lane_set] =
              $targetObj->getBusAttribute($child, 0, "LANE_REVERSAL");
            my $eq = $targetObj->getBusAttribute($child, 0,
              "PCIE_LANE_EQUALIZATION");
            my @eqs = split(/\,/,$eq);
            for (my $e=0;$e<@eqs;$e=$e+3)
            {
                if ($eqs[$e] eq "all")
                {
                    for (my $lane=0;$lane<16;$lane++)
                    {
                        $equalization[$phb_num][$lane]=$eqs[$e+1].",".
                              $eqs[$e+2];
                    }
                }
                else
                {
                    $equalization[$phb_num][$eqs[$e]] = $eqs[$e+1].",".
                              $eqs[$e+2];
                }
            }
            substr($phb_config, $phb_num, 1, "1");
        }
    }
    if ($found)
    {
    my $hex = sprintf('%X', oct("0b$phb_config"));

    $targetObj->setAttribute($parentTarget, "PROC_PCIE_PHB_ACTIVE","0x" . $hex);
    my $lane_mask_attr = sprintf("%s,%s,%s,%s",
        $lane_mask[0][0], $lane_mask[0][1],
        $lane_mask[1][0], $lane_mask[1][1]);
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_LANE_MASK",
        $lane_mask_attr);
    $targetObj->setAttribute($parentTarget,"PROC_PCIE_LANE_MASK_NON_BIFURCATED",
        $lane_mask_attr);
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_LANE_MASK_BIFURCATED",
        "0,0,0,0");

    my @iop_swap_lu;
    my @iop_lane_swap;
    for (my $iop=0;$iop<2;$iop++)
    {
        $iop_lane_swap[$iop] = $lane_swap[$iop][0] | $lane_swap[$iop][1];
        my $lane_rev = $lane_rev[$iop][0].$lane_rev[$iop][1];
        $iop_swap_lu[$iop]=
                 "0b".$iop_swap{$iop}{$iop_lane_swap[$iop]}{$lane_rev};
        if ($iop_swap_lu[$iop] eq "") {
          die "PCIE config for $iop,$iop_lane_swap[$iop],$lane_rev not found\n";
        }
    }
    my $lane_swap_attr0 = sprintf("%s,%s",$iop_lane_swap[0],
                          $iop_lane_swap[1]);
    my $lane_swap_attr1 = sprintf("%s,0,%s,0",$iop_lane_swap[0],
                          $iop_lane_swap[1]);

    $targetObj->setAttribute($parentTarget, "PROC_PCIE_IOP_SWAP",
        $lane_swap_attr0);
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_IOP_SWAP_NON_BIFURCATED",
        $lane_swap_attr1);
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_IOP_SWAP_BIFURCATED",
        "0,0,0,0");

    my $lane_rev_attr = sprintf("%s,0,%s,0",
                         oct($iop_swap_lu[0]),oct($iop_swap_lu[1]));

    $targetObj->setAttribute($parentTarget, "PROC_PCIE_IOP_REVERSAL",
        $lane_rev_attr);
    $targetObj->setAttribute($parentTarget,
        "PROC_PCIE_IOP_REVERSAL_NON_BIFURCATED",$lane_rev_attr);
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_IOP_REVERSAL_BIFURCATED",
        "0,0,0,0");

    my $is_slot_attr = sprintf("%s,%s,%s,%s",
        $is_slot[0][0], $is_slot[0][1], $is_slot[1][0], $is_slot[1][1]);
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_IS_SLOT", $is_slot_attr);

    ## don't support DSMP
    $targetObj->setAttribute($parentTarget, "PROC_PCIE_DSMP_CAPABLE","0,0,0,0");
    my $eq_str="";
    for (my $p=0;$p<$NUM_PHBS;$p++)
    {
        for (my $lane=0;$lane<16;$lane++)
        {
            $eq_str=$eq_str.$equalization[$p][$lane].",";
        }
    }
    $eq_str = substr($eq_str,0,length($eq_str)-1);
    $targetObj->setAttribute($parentTarget,"PROC_PCIE_LANE_EQUALIZATION",
         $eq_str);
    }
}
#--------------------------------------------------
## OCC
##
sub processOcc
{
    my $targetObj    = shift;
    my $target       = shift;
    my $parentTarget = shift;
    my $master_capable=0;
    if ($parentTarget eq $targetObj->getMasterProc())
    {
        $master_capable=1;
    }
    $targetObj->setAttribute($target,"OCC_MASTER_CAPABLE",$master_capable);
}

sub processMembufVpdAssociation
{
    my $targetObj = shift;
    my $target    = shift;

    my $vpds=$targetObj->findConnections($target,"I2C","VPD");
    if ($vpds ne "" ) {
        my $vpd = $vpds->{CONN}->[0];

        my $membuf_assocs=$targetObj->findConnections($vpd->{DEST_PARENT},
                          "LOGICAL_ASSOCIATION","MEMBUF");
        if ($membuf_assocs ne "") {
            foreach my $membuf_assoc (@{$membuf_assocs->{CONN}}) {
                my $membuf_target = $membuf_assoc->{DEST_PARENT};
                setEepromAttributes($targetObj,
                       "EEPROM_VPD_PRIMARY_INFO",$membuf_target,$vpd);
                setEepromAttributes($targetObj,
                       "EEPROM_VPD_FRU_INFO",$membuf_target,$vpd,"0++");
                my $index = $targetObj->getBusAttribute($membuf_assoc->{SOURCE},
                                $membuf_assoc->{BUS_NUM}, "ISDIMM_MBVPD_INDEX");
                $targetObj->setAttribute(
                            $membuf_target,"ISDIMM_MBVPD_INDEX",$index);
            }
        }
    }
}

#--------------------------------------------------
## MEMBUF
##
## Finds I2C connections to DIMM and creates EEPROM attributes
## FYI:  I had to handle DMI busses in framework because they
## define affinity path

sub processMembuf
{
    my $targetObj = shift;
    my $target    = shift;
    if ($targetObj->isBadAttribute($target, "PHYS_PATH", ""))
    {
        ##dmi is probably not connected.  will get caught in error checking
        return;
    }

    processMembufVpdAssociation($targetObj,$target);

    ## finds which gpio expander that controls vddr regs for membufs
    my $gpioexp=$targetObj->findConnections($target,"I2C","GPIO_EXPANDER");
    if ($gpioexp ne "" ) {
        my $vreg=$targetObj->findConnections(
            $gpioexp->{CONN}->[0]->{DEST_PARENT},"GPIO","VOLTAGE_REGULATOR");
        if ($vreg ne "") {
            my $vddPin = $targetObj->getAttribute(
                 $vreg->{CONN}->[0]->{SOURCE},"CHIP_UNIT");
            my $membufs=$targetObj->findConnections(
               $vreg->{CONN}->[0]->{DEST_PARENT},"POWER","MEMBUF");
            if ($membufs ne "") {
                foreach my $membuf (@{$membufs->{CONN}}) {
                    my $aff = $targetObj->getAttribute($membuf->{DEST_PARENT},
                        "PHYS_PATH");
                    setGpioAttributes($targetObj,$membuf->{DEST_PARENT},
                        $gpioexp->{CONN}->[0],$vddPin);

                }
            }
        }
    }
    ## Process MEMBUF to DIMM I2C connections
    my $dimms=$targetObj->findConnections($target,"I2C","SPD");
    if ($dimms ne "") {
        foreach my $dimm (@{$dimms->{CONN}}) {
            my $dimm_target = $targetObj->getTargetParent($dimm->{DEST_PARENT});
            setEepromAttributes($targetObj,
                       "EEPROM_VPD_PRIMARY_INFO",$dimm_target,
                       $dimm);
            setEepromAttributes($targetObj,
                       "EEPROM_VPD_FRU_INFO",$dimm_target,
                       $dimm,"0++");
        }
    }
    ## Update bus speeds
    processI2cSpeeds($targetObj,$target);

    ## Do MBA port mapping
    my %mba_port_map;
    my $ddrs=$targetObj->findConnections($target,"DDR3","DIMM");
    if ($ddrs ne "") {
        my %portmap;
        foreach my $ddr (@{$ddrs->{CONN}}) {
            my $mba=$ddr->{SOURCE};
            my $dimm=$ddr->{DEST_PARENT};
            my ($dimmnum,$port)=split(//,sprintf("%02b\n",$portmap{$mba}));
            $targetObj->setAttribute($dimm, "MBA_DIMM",$dimmnum);
            $targetObj->setAttribute($dimm, "MBA_PORT",$port);
            $portmap{$mba}++;

            ## Copy connector attributes
            my $dimmconn=$targetObj->getTargetParent($dimm);
        }
    }
}


sub setEepromAttributes
{
    my $targetObj = shift;
    my $name = shift;
    my $target = shift;
    my $conn_target = shift;
    my $fru = shift;

    my $port = $targetObj->getAttribute($conn_target->{SOURCE}, "I2C_PORT");
    my $engine = $targetObj->getAttribute($conn_target->{SOURCE}, "I2C_ENGINE");
    my $addr = $targetObj->getBusAttribute($conn_target->{SOURCE},
            $conn_target->{BUS_NUM}, "I2C_ADDRESS");

    my $path = $targetObj->getAttribute($conn_target->{SOURCE_PARENT},
               "PHYS_PATH");
    my $mem  = $targetObj->getAttribute($conn_target->{DEST_PARENT},
               "MEMORY_SIZE_IN_KB");
    my $cycle  = $targetObj->getAttribute($conn_target->{DEST_PARENT},
               "WRITE_CYCLE_TIME");
    my $page  = $targetObj->getAttribute($conn_target->{DEST_PARENT},
               "WRITE_PAGE_SIZE");
    my $offset  = $targetObj->getAttribute($conn_target->{DEST_PARENT},
               "BYTE_ADDRESS_OFFSET");

    $targetObj->setAttributeField($target, $name, "i2cMasterPath", $path);
    $targetObj->setAttributeField($target, $name, "port", $port);
    $targetObj->setAttributeField($target, $name, "devAddr", $addr);
    $targetObj->setAttributeField($target, $name, "engine", $engine);
    $targetObj->setAttributeField($target, $name, "byteAddrOffset", $offset);
    $targetObj->setAttributeField($target, $name, "maxMemorySizeKB", $mem);
    $targetObj->setAttributeField($target, $name, "writePageSize", $page);
    $targetObj->setAttributeField($target, $name, "writeCycleTime", $cycle);

    if ($fru ne "")
    {
        $targetObj->setAttributeField($target, $name, "fruId", $fru);
    }
}


sub setGpioAttributes
{
    my $targetObj = shift;
    my $target = shift;
    my $conn_target = shift;
    my $vddrPin = shift;

    my $port = $targetObj->getAttribute($conn_target->{SOURCE}, "I2C_PORT");
    my $engine = $targetObj->getAttribute($conn_target->{SOURCE}, "I2C_ENGINE");
    my $addr = $targetObj->getBusAttribute($conn_target->{SOURCE},
            $conn_target->{BUS_NUM}, "I2C_ADDRESS");
    my $path = $targetObj->getAttribute($conn_target->{SOURCE_PARENT},
               "PHYS_PATH");


    my $name="GPIO_INFO";
    $targetObj->setAttributeField($target, $name, "i2cMasterPath", $path);
    $targetObj->setAttributeField($target, $name, "port", $port);
    $targetObj->setAttributeField($target, $name, "devAddr", $addr);
    $targetObj->setAttributeField($target, $name, "engine", $engine);
    $targetObj->setAttributeField($target, $name, "vddrPin", $vddrPin);
}

#--------------------------------------------------
## ERROR checking
sub errorCheck
{
    my $targetObj = shift;
    my $target    = shift;
    my $type      = $targetObj->getType($target);

    ## error checking even for connections are done with attribute checks
    ##  since connections simply create attributes at source and/or destination
    ##
    ## also error checking after processing is complete vs during
    ## processing is easier
    my %attribute_checks = (
        SYS         => ['SYSTEM_NAME','OPAL_MODEL'],
        PROC_MASTER => ['I2C_SLAVE_ADDRESS'],
        PROC        => ['FSI_MASTER_CHIP','I2C_SLAVE_ADDRESS'],
        MEMBUF => [ 'PHYS_PATH', 'EI_BUS_TX_MSBSWAP', 'FSI_MASTER_PORT|0xFF' ],
        DIMM   => ['EEPROM_VPD_PRIMARY_INFO/devAddr'],
    );
    my %error_msg = (
        'EEPROM_VPD_PRIMARY_INFO/devAddr' =>
          'I2C connection to target is not defined',
        'FSI_MASTER_PORT' => 'This target is missing a required FSI connection',
        'FSI_MASTER_CHIP' => 'This target is missing a required FSI connection',
        'EI_BUS_TX_MSBSWAP' =>
          'DMI connection is missing to this membuf from processor',
        'PHYS_PATH' =>'DMI connection is missing to this membuf from processor',
        'I2C_SLAVE_ADDRESS' =>'I2C connection is missing from BMC to processor',
    );

    my @errors;
    if ($targetObj->getMasterProc() eq $target)
    {
        $type = "PROC_MASTER";
    }
    foreach my $attr (@{ $attribute_checks{$type} })
    {
        my ($a,         $v)     = split(/\|/, $attr);
        my ($a_complex, $field) = split(/\//, $a);
        if ($field ne "")
        {
            if ($targetObj->isBadComplexAttribute(
                    $target, $a_complex, $field, $v) )
            {
                push(@errors,sprintf(
                        "$a attribute is invalid (Target=%s)\n\t%s\n",
                        $target, $error_msg{$a}));
            }
        }
        else
        {
            if ($targetObj->isBadAttribute($target, $a, $v))
            {
                push(@errors,sprintf(
                        "$a attribute is invalid (Target=%s)\n\t%s\n",
                        $target, $error_msg{$a}));
            }
        }
    }
    if ($type eq "PROC")
    {
        ## note: DMI is checked on membuf side so don't need to check that here
        ## this checks if at least 1 abus is connected
        my $found_abus = 0;
        my $abus_error = "";
        foreach my $child (@{ $targetObj->getTargetChildren($target) })
        {
            my $child_type = $targetObj->getBusType($child);
            if ($child_type eq "ABUS" || $child_type eq "XBUS")
            {
                if ($targetObj->getMasterProc() ne $target)
                {
                    if (!$targetObj->isBadAttribute($child, "PEER_TARGET"))
                    {
                        $found_abus = 1;
                    }
                    else
                    {
                        $abus_error = sprintf(
"proc not connected to proc via Abus or Xbus (Target=%s)",$child);
                    }
                }
            }
        }
        if ($found_abus)
        {
            $abus_error = "";
        }
        else
        {
            push(@errors, $abus_error);
        }
    }
    if ($errors[0])
    {
        foreach my $err (@errors)
        {
            print "ERROR: $err\n";
        }
        $targetObj->myExit(3);
    }
}

sub printUsage
{
    print "
processMrwl.pl -x [XML filename] [OPTIONS]
Options:
        -f = force output file creation even when errors
        -d = debug mode
        -s [SDR XML file] = import SDRs
        -r = create report and save to [system_name].rpt
        -v = version
";
    exit(1);
}
