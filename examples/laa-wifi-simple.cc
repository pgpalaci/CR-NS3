/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

//
//  This example program can be used to experiment with wireless coexistence
//  in a simple scenario.  The scenario consists of two cells whose radio
//  coverage overlaps but representing, notionally, two operators in the
//  same region whose transmissions may impact mechanisms such as clear
//  channel assessment and adaptive modulation and coding.
//  The technologies used are LTE Licensed Assisted Access (LAA)
//  operating on EARFCN 255444 (5.180 GHz), and Wi-Fi 802.11n
//  operating on channel 36 (5.180 GHz).    
//
//  The notional scenario consists of two operators A and B, each
//  operating a single cell, and each cell consisting of a base
//  station (BS) and a user equipment (UE), as represented in the following 
//  diagram:
//
// 
//    BS A  . .  d2 . .  UE B 
//     |                   |
//  d1 |                d1 |
//     |                   |
//    UE A  . .  d2 . .  BS B
//  
//  cell A              cell B
//
//  where 'd1' and 'd2' distances (in meters) separate the nodes.
//
//  When using LTE, the BS is modeled as an eNB and the UE as a UE.
//  When using Wi-Fi, the BS is modeled as an AP and the UE as a STA.
//
//  In addition, both BS are connected to a "backhaul" client node that
//  originates data transfer in the downlink direction from client to UE(s).
//  The links from client to BS are modeled as well provisioned; low latency
//  links.
//
//  The figure may be redrawn as follows:
//
//     +---------------------------- client
//     |                               |
//    BS A  . .  d2 . .  UE B          |
//     |                   |           |
//  d1 |                d1 |           |
//     |                   |           |
//    UE A  . .  d2 . .  BS B----------+
//  
//
//  In general, the program can be configured at run-time by passing
//  command-line arguments.  The command
//  ./waf --run "laa-wifi-coexistence-simple --help"
//  will display all of the available run-time help options, and in
//  particular, the command
//  ./waf --run "laa-wifi-coexistence-simple --PrintGlobals" should
//  display the following:
//
// Global values:
//     --ChecksumEnabled=[false]
//         A global switch to enable all checksums for all protocols
//     --RngRun=[1]
//         The run number used to modify the global seed
//     --RngSeed=[1]
//         The global seed of all rng streams
//     --SchedulerType=[ns3::MapScheduler]
//         The object class to use as the scheduler implementation
//     --SimulatorImplementationType=[ns3::DefaultSimulatorImpl]
//         The object class to use as the simulator implementation
//     --cellConfigA=[Lte]
//         Lte or Wifi
//     --cellConfigB=[Wifi]
//         Lte or Wifi
//     --d1=[10]
//         intra-cell separation (e.g. AP to STA)
//     --d2=[50]
//         inter-cell separation
//     --duration=[1]
//         Data transfer duration (seconds)
//     --pcapEnabled=[false]
//         Whether to enable pcap traces for Wi-Fi
//     --transport=[Udp]
//         Whether to use Tcp or Udp
//
//  The bottom seven are specific to this example.  In particular,
//  passing the argument '--cellConfigA=Wifi' will cause both
//  cells to use Wifi but on a different SSID, while the (default)
//  will cell A to use LTE and cellB to use Wi-Fi.  Passing an argument 
//  of '--cellConfigB=Lte" will cause both cells to use LTE.
//
//  In addition, some other variables may be modified at compile-time
//  such as simulation run-time.  For simplification, an IEEE 802.11n
//  configuration is used with an 'Ideal' rate control.
//
//  We refer to the left-most cell as 'cell A' and the right-most
//  cell as 'cell B'.  Both cells are configured to run UDP transfers from
//  client to BS.  The application data rate is 20 Mb/s, which saturates
//  the Wi-Fi link but can be handled by the LTE link.
//
//  The following sample output is provided.
//
//  When run with no arguments:
//
//  ./waf --run "laa-wifi-coexistence-simple"
//  d1: 10 meters; d2: 50 meters; data xfer duration: 1 sec; IP packet 1500 bytes
//  Cell Type	Throughput [Mbit/s]	Sent/Received [packets]		SINR [dB]
//  0	LTE	19.6306			1667/1667			55.7339
//  1	Wi-Fi	0			1667/0			        52.2582
//
//  In this case, LTE (non-duty-cycled) prevents Wi-Fi from using the channel.
//
//  When run with an argument to configure Wi-Fi only:
//
// ./waf --run "laa-wifi-coexistence-simple --cellConfigA=Wifi"
// d1: 	10 meters; d2: 50 meters; data xfer duration: 1 sec; IP packet 1500 bytes
// Cell	Type	Throughput [Mbit/s]	Sent/Received [packets]		SINR [dB]
// 0	Wi-Fi	12.7181			1667/1080			42.9208
// 1	Wi-Fi	13.1538			1667/1117			43.1333
// 
// in this case, the channel usage is split between the two Wi-Fi
// networks.
//
//  When run with an argument to configure LTE only:
//
// ./waf --run "laa-wifi-coexistence-simple --cellConfigB=Lte"
// d1: 	10 meters; d2: 50 meters; data transfer duration: 1 sec; IP packet 1500 bytes
// Cell	Type	Throughput [Mbit/s]	Sent/Received [packets]		SINR [dB]
// 0	LTE	19.5482			1667/1660			13.9791
// 1	LTE	19.5482			1667/1660			13.9791
// 
// in this case, the two LTE cells are interfering with each other.
//
// Isolating the two networks can be done by setting 'd2' to a very large
// value such as 10000 meters:
//
// ./waf --run "laa-wifi-coexistence-simple --d2=10000"
// d1: 	10 meters; d2: 10000 meters; data transfer duration: 1 sec; IP packet 1500 bytes
// Cell	Type	Throughput [Mbit/s]	Sent/Received [packets]		SINR [dB]
// 0	LTE	19.6306			1667/1667			55.6756
// 1	Wi-Fi	19.6306			1667/1667			50.3538
//
// and for two Wi-Fi networks:
//
// ./waf --run "laa-wifi-coexistence-simple --d2=10000 --cellConfigA=Wifi"
//
// Another option is to switch to an FTP-like application; this can be done
// by passing the "--transport=Tcp" argument.  This will cause a 1 MB file
// to be sent, and the output displayed will be in bytes rather than packets
//  
// ./waf --run "laa-wifi-coexistence-simple --transport=Tcp"
// d1: 	10 meters; d2: 50 meters; data transfer duration: 1 sec; IP packet 1500 bytes
// Cell	Type	Throughput [Mbit/s]	Sent/Received [bytes]		SINR [dB]
// 0	LTE	8.192			1024000/1024000			55.7339
// 1	Wi-Fi	0			0/0			        52.2582
//
// Finally, the "--pcapEnabled=1" argument can be used to generate PCAP trace
// files that can be read by Wireshark.  When using this option, it is advised
// to also set "--checksumEnabled=1" to compute TCP and IP checksums
//
// Beyond the above summary statistics, the program outputs the statistics
// counted by the FlowMonitor framework.  This is another way to measure 
// throughput but has the added statistics for transfer delay and jitter.
//
// Statistics will look something like this:
//
// Flow 1 (192.168.0.2 -> 192.168.0.1)
//  Tx Packets: 1914
//  Tx Bytes:   1123532
//  TxOffered:  8.98826 Mbps
//  Rx Bytes:   1123532
//  Throughput: 4.87982 Mbps
//  Mean delay:  105.592 ms
//  Mean jitter:  1.14371 ms
//  Rx Packets: 1914
//
// In addition, the program outputs various statistics files from the
// available LTE and Wi-Fi traces, including, for Wi-Fi, some statistics
// patterned after the 'athstats' tool, and for LTE, the stats
// described in the LTE Module documentation, which can be found at
// https://www.nsnam.org/docs/models/html/lte-user.html#simulation-output
//
// These files are named:
//  athstats-ap_002_000   
//  athstats-sta_003_000  
//  DlMacStats.txt
//  DlPdcpStats.txt
//  DlRlcStats.txt 
//  DlRsrpSinrStats.txt   
//  DlRxPhyStats.txt      
//  DlTxPhyStats.txt      
//  UlInterferenceStats.txt         
//  UlRxPhyStats.txt
//  UlSinrStats.txt
//  UlTxPhyStats.txt
//  etc.

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/lte-module.h>
#include <ns3/wifi-module.h>
#include <ns3/config-store-module.h>
#include <ns3/spectrum-module.h>
#include <ns3/applications-module.h>
#include <ns3/flow-monitor-module.h>
#include <ns3/propagation-module.h>
#include "scenario-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LaaWifiCoexistenceSimple");

// Global Values are used in place of command line arguments so that these
// values may be managed in the ns-3 ConfigStore system.
static ns3::GlobalValue g_d1 ("d1",
                              "intra-cell separation (e.g. AP to STA)",
                              ns3::DoubleValue (10),
                              ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_d2 ("d2",
                              "inter-cell separation",
                              ns3::DoubleValue (50),
                              ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_duration ("duration",
                                    "Data transfer duration (seconds)",
                                    ns3::DoubleValue (1),
                                    ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_cellConfigA ("cellConfigA",
                                       "Lte or Wifi",
                                        ns3::EnumValue (LTE),
                                        ns3::MakeEnumChecker (WIFI, "Wifi",
                                                              LTE, "Lte"));
static ns3::GlobalValue g_cellConfigB ("cellConfigB",
                                       "Lte or Wifi",
                                        ns3::EnumValue (WIFI),
                                        ns3::MakeEnumChecker (WIFI, "Wifi",
                                                              LTE, "Lte"));

static ns3::GlobalValue g_pcap ("pcapEnabled",
                                "Whether to enable pcap trace files for Wi-Fi",
                                ns3::BooleanValue (false),
                                ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_transport ("transport",
                                     "whether to use Udp or Tcp",
                                     ns3::EnumValue (UDP),
                                     ns3::MakeEnumChecker (UDP, "Udp",
                                                           TCP, "Tcp"));

static ns3::GlobalValue g_lteDutyCycle ("lteDutyCycle",
                                    "Duty cycle value to be used for LTE",
                                    ns3::DoubleValue (1),
                                    ns3::MakeDoubleChecker<double> (0.0, 1.0));

static ns3::GlobalValue g_generateRem ("generateRem",
                                       "if true, will generate a REM and then abort the simulation;"
                                       "if false, will run the simulation normally (without generating any REM)",
                                       ns3::BooleanValue (false),
                                       ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_simTag ("simTag",
                                  "tag to be appended to output filenames to distinguish simulation campaigns",
                                  ns3::StringValue ("default"),
                                  ns3::MakeStringChecker ());

static ns3::GlobalValue g_outputDir ("outputDir",
                                     "directory where to store simulation results",
                                     ns3::StringValue ("./"),
                                     ns3::MakeStringChecker ());


// Global variables for use in callbacks.
double g_signalDbmAvg[2];
double g_noiseDbmAvg[2];
uint32_t g_samples[2];
uint16_t g_channelNumber[2];
uint32_t g_rate[2];

int
main (int argc, char *argv[])
{  
  CommandLine cmd;
  cmd.Parse (argc, argv);
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // parse again so you can override input file default values via command line
  cmd.Parse (argc, argv);

  DoubleValue doubleValue;
  EnumValue enumValue;
  BooleanValue booleanValue;
  StringValue stringValue;

  GlobalValue::GetValueByName ("d1", doubleValue);
  double d1 = doubleValue.Get ();
  GlobalValue::GetValueByName ("d2", doubleValue);
  double d2 = doubleValue.Get ();
  GlobalValue::GetValueByName ("cellConfigA", enumValue);
  enum Config_e cellConfigA = (Config_e) enumValue.Get ();
  GlobalValue::GetValueByName ("cellConfigB", enumValue);
  enum Config_e cellConfigB = (Config_e) enumValue.Get ();
  GlobalValue::GetValueByName ("duration", doubleValue);
  double duration = doubleValue.Get ();
  GlobalValue::GetValueByName ("transport", enumValue);
  enum Transport_e transport = (Transport_e) enumValue.Get ();
  GlobalValue::GetValueByName ("lteDutyCycle", doubleValue);
  double lteDutyCycle = doubleValue.Get ();
  GlobalValue::GetValueByName ("generateRem", booleanValue);
  bool generateRem = booleanValue.Get ();
  GlobalValue::GetValueByName ("simTag", stringValue);
  std::string simTag = stringValue.Get ();
  GlobalValue::GetValueByName ("outputDir", stringValue);
  std::string outputDir = stringValue.Get ();

  // Create nodes and containers
  NodeContainer bsNodesA, bsNodesB;  // for APs and eNBs
  NodeContainer ueNodesA, ueNodesB;  // for STAs and UEs
  NodeContainer allWirelessNodes;  // container to hold all wireless nodes 
  // Each network A and B gets one type of node each
  bsNodesA.Create (1);
  bsNodesB.Create (1);
  ueNodesA.Create (1);
  ueNodesB.Create (1);
  allWirelessNodes = NodeContainer (bsNodesA, bsNodesB, ueNodesA, ueNodesB);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));   // eNB1/AP in cell 0
  positionAlloc->Add (Vector (d2, d1, 0.0)); // AP in cell 1
  positionAlloc->Add (Vector (0.0, d1, 0.0));  // UE1/STA in cell 0
  positionAlloc->Add (Vector (d2, 0.0, 0.0));  // STA in cell 1
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (allWirelessNodes);

  bool disableApps = false;
  Time durationTime = Seconds (duration);


  // REM settings tuned to get a nice figure for this specific scenario
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::OutputFile", StringValue ("laa-wifi-simple.rem"));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMin", DoubleValue (-50));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMax", DoubleValue (250));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMin", DoubleValue (-50));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMax", DoubleValue (250));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XRes", UintegerValue (600));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YRes", UintegerValue (600));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::Z", DoubleValue (1.5));

  // we want deterministic behavior in this simple scenario, so we disable shadowing
  Config::SetDefault ("ns3::Ieee80211axIndoorPropagationLossModel::Sigma", DoubleValue (0));

  // Specify some physical layer parameters that will be used below and
  // in the scenario helper.
  PhyParams phyParams;
  phyParams.m_bsTxGain = 5; // dB antenna gain
  phyParams.m_bsRxGain = 5; // dB antenna gain
  phyParams.m_bsTxPower = 18; // dBm
  phyParams.m_bsNoiseFigure = 5; // dB
  phyParams.m_ueTxGain = 0; // dB antenna gain
  phyParams.m_ueRxGain = 0; // dB antenna gain
  phyParams.m_ueTxPower = 18; // dBm
  phyParams.m_ueNoiseFigure = 9; // dB
  
  // calculate rx power corresponding to d2 for logging purposes
  // note: a separate propagation loss model instance is used, make
  // sure the settings are consistent with the ones used for the
  // simulation 
  const uint32_t earfcn = 255444;
  double dlFreq = LteSpectrumValueHelper::GetCarrierFrequency (earfcn);
  Ptr<PropagationLossModel> plm = CreateObject<Ieee80211axIndoorPropagationLossModel> ();
  plm->SetAttribute ("Frequency", DoubleValue (dlFreq));
  double txPowerFactors = phyParams.m_bsTxGain + phyParams.m_ueRxGain + 
                          phyParams.m_bsTxPower;
  double rxPowerDbmD1 = plm->CalcRxPower (txPowerFactors, 
                                          bsNodesA.Get (0)->GetObject<MobilityModel> (),
                                          ueNodesA.Get (0)->GetObject<MobilityModel> ());
  double rxPowerDbmD2 = plm->CalcRxPower (txPowerFactors, 
                                          bsNodesA.Get (0)->GetObject<MobilityModel> (),
                                          ueNodesB.Get (0)->GetObject<MobilityModel> ());


  std::ostringstream simulationParams;
  simulationParams << d1 << " " << d2 << " " 
                   << rxPowerDbmD1 << " "
                   << rxPowerDbmD2 << " "
                   << lteDutyCycle << " ";
  
  ConfigureAndRunScenario (cellConfigA, cellConfigB, bsNodesA, bsNodesB, ueNodesA, ueNodesB, phyParams, durationTime, transport, "ns3::Ieee80211axIndoorPropagationLossModel", disableApps, lteDutyCycle, generateRem, outputDir + "/laa_wifi_simple_" + simTag, simulationParams.str ());
  
  return 0;
}
