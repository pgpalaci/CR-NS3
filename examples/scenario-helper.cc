/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2015 University of Washington
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
 * Authors: Nicola Baldo <nbaldo@cttc.es> and Tom Henderson <tomh@tomh.org>
 */

#include "scenario-helper.h"
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/lte-module.h>
#include <ns3/wifi-module.h>
#include <ns3/spectrum-module.h>
#include <ns3/applications-module.h>
#include <ns3/internet-module.h>
#include <ns3/propagation-module.h>
#include <ns3/config-store-module.h>
#include <ns3/flow-monitor-module.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScenarioHelper");


// Global Values are used in place of command line arguments so that these
// values may be managed in the ns-3 ConfigStore system.

static ns3::GlobalValue g_serverStartTimeSeconds ("serverStartTimeSeconds",
                                                  "Server start time (seconds)",
                                                  ns3::DoubleValue (3),
                                                  ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_clientStartTimeSeconds ("clientStartTimeSeconds",
                                                  "Client start time (seconds)",
                                                  ns3::DoubleValue (3),
                                                  ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_serverLingerTimeSeconds ("serverLingerTimeSeconds",
                                                   "Server linger time (seconds)",
                                                   ns3::DoubleValue (5),
                                                   ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_simulationLingerTimeSeconds ("simulationLingerTimeSeconds",
                                                       "Simulation linger time (seconds)",
                                                       ns3::DoubleValue (5),
                                                       ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_remDir ("remDir",
                                  "directory where to save REM-related output files",
                                  ns3::StringValue ("./"),
                                  ns3::MakeStringChecker ());

// Parse context strings of the form "/NodeList/3/DeviceList/1/Mac/Assoc"
// to extract the NodeId
uint32_t
ContextToNodeId (std::string context)
{
  std::string sub = context.substr (10);  // skip "/NodeList/"
  uint32_t pos = sub.find ("/Device");
  NS_LOG_DEBUG ("Found NodeId " << atoi (sub.substr (0, pos).c_str ()));
  return atoi (sub.substr (0,pos).c_str ());
}

// Parse context strings of the form "/NodeList/3/DeviceList/1/Mac/Assoc"
// to extract the DeviceId
uint32_t
ContextToDeviceId (std::string context)
{
  uint32_t pos = context.find ("/Device");
  std::string sub = context.substr (pos + 12); // skip "/DeviceList/"
  pos = sub.find ("/Mac");
  NS_LOG_DEBUG ("Found DeviceId " << atoi (sub.substr (0, pos).c_str ()));
  return atoi (sub.substr (0,pos).c_str ());
}

std::string
CellConfigToString (enum Config_e config)
{
  if (config == WIFI)
    {
      return "Wi-Fi";
    }
  else
    {
      return "LTE";
    }
}

Ptr<Node>
MacAddressToNode (Mac48Address address)
{
  Ptr<Node> n;
  Ptr<NetDevice> nd;
  for (uint32_t i = 0; i < NodeContainer::GetGlobal ().GetN(); i++)
    {
      n = NodeContainer::GetGlobal ().Get (i);
      for (uint32_t j = 0; j < n->GetNDevices (); j++)
        {
          nd = n->GetDevice (j);
          Address a = nd->GetAddress (); 
          Mac48Address mac = Mac48Address::ConvertFrom (a);
          if (address == mac)
           {
             NS_LOG_DEBUG ("Found node " << n->GetId () << " for address " << address);
             return n;
           }
        }
    }
  return 0;
}

Ptr<WifiNetDevice>
FindFirstWifiNetDevice (Ptr<Node> ap)
{
  Ptr<WifiNetDevice> wifi;
  Ptr<NetDevice> nd;
  for (uint32_t i = 0; i < ap->GetNDevices (); i++)
    {
      nd = ap->GetDevice (i);
      wifi = DynamicCast<WifiNetDevice> (nd);
      if (wifi)
        {
          NS_LOG_DEBUG ("Found wifi device on interface " << i);
          return wifi;
        }
    }
  return 0;
}

Ptr<PointToPointNetDevice>
FindFirstPointToPointNetDevice (Ptr<Node> ap)
{
  Ptr<PointToPointNetDevice> p2p;
  Ptr<NetDevice> nd;
  for (uint32_t i = 0; i < ap->GetNDevices (); i++)
    {
      nd = ap->GetDevice (i);
      p2p = DynamicCast<PointToPointNetDevice> (nd);
      if (p2p)
        {
          NS_LOG_DEBUG ("Found p2p device on interface " << i);
          return p2p;
        }
    }
  return 0;
}

Ptr<PointToPointNetDevice>
GetRemoteDevice (Ptr<Node> ap, Ptr<PointToPointNetDevice> p2p)
{
  for (uint32_t i = 0; i < p2p->GetChannel ()->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = p2p->GetChannel ()->GetDevice (i);
      Ptr<PointToPointNetDevice> tmp2 = DynamicCast<PointToPointNetDevice> (tmp);
      if (tmp2 != p2p)
        {
          return tmp2;
        }
    }
  NS_ASSERT (false);
  return 0;
}

void
ConfigureRouteForStation (std::string context, Mac48Address address)
{
  // We receive the context string of the STA that has just associated
  // and the BSSID of the AP in the 'address' parameter.
  // We need to install the IP address of the AP as this STA's default
  // route.  We need to install the IP address of the AP's point-to-point
  // interface with the client node, as a next hop host route to this STA.

  // Step 1: Obtain STA IP address
  NS_LOG_DEBUG ("ConfigureRouteForStation: " << context << " " << address);
  uint32_t myNodeId = ContextToNodeId (context);
  Ptr<Node> myNode = NodeContainer::GetGlobal ().Get (myNodeId);
  uint32_t myDeviceId = ContextToDeviceId (context);
  Ptr<Ipv4> myIp = myNode->GetObject<Ipv4> ();
  Ptr<NetDevice> myNd = myNode->GetDevice (myDeviceId);
  int32_t myIface = myIp->GetInterfaceForDevice (myNd);
  Ipv4InterfaceAddress myAddr = myIp->GetAddress (myIface, 0);
  NS_LOG_DEBUG ("STA IP address is: " << myAddr.GetLocal ());

  // Step 2: Install default route to AP on STA
  Ptr<Node> ap = MacAddressToNode (address);
  Ptr<WifiNetDevice> wifi = FindFirstWifiNetDevice (ap);
  Ptr<Ipv4> ip = ap->GetObject<Ipv4> ();
  int32_t iface = ip->GetInterfaceForDevice (wifi);
  Ipv4InterfaceAddress apAddr = ip->GetAddress (iface, 0);
  NS_LOG_DEBUG ("ConfigureRouteForStation ap addr: " << apAddr.GetLocal ());
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> myStaticRouting = ipv4RoutingHelper.GetStaticRouting (myNode->GetObject<Ipv4> ());
  myStaticRouting->SetDefaultRoute (apAddr.GetLocal (), myIface);
  NS_LOG_DEBUG ("Setting STA default to: " << apAddr.GetLocal () << " " << myIface);

  // Step 3: Install host route on client node 
  Ptr<PointToPointNetDevice> p2p = FindFirstPointToPointNetDevice (ap);
  iface = ip->GetInterfaceForDevice (p2p);
  apAddr = ip->GetAddress (iface, 0);
  Ptr<PointToPointNetDevice> remote = GetRemoteDevice (ap, p2p);
  ip = remote->GetNode ()->GetObject<Ipv4> ();
  iface = ip->GetInterfaceForDevice (remote);
  Ptr<Ipv4StaticRouting> clientStaticRouting = ipv4RoutingHelper.GetStaticRouting (remote->GetNode ()->GetObject<Ipv4> ());
  clientStaticRouting->AddHostRouteTo (myAddr.GetLocal (), apAddr.GetLocal (), iface);
  NS_LOG_DEBUG ("Setting client host route to " << myAddr.GetLocal () << " to nextHop " << apAddr << " " << iface);
}

void
PrintFlowMonitorStats (Ptr<FlowMonitor> monitor, FlowMonitorHelper& flowmonHelper, double duration)
{
  // Print per-flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
      std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / duration/ 1000 / 1000  << " Mbps\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      // Measure the duration of the flow from receiver's perspective
      double rxDuration = i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000  << " Mbps\n";
      if (i->second.rxPackets > 0)
        {
          std::cout << "  Mean delay:  " << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << " ms\n";
          std::cout << "  Mean jitter:  " << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets  << " ms\n";
        }
      else
        {
          std::cout << "  Mean delay:  0 ms\n";
          std::cout << "  Mean jitter: 0 ms\n";
        }
      std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
    }
}

bool
MatchingFlow (Ipv4FlowClassifier::FiveTuple first, Ipv4FlowClassifier::FiveTuple second)
{
  if ((first.sourceAddress == second.destinationAddress) &&
      (first.destinationAddress == second.sourceAddress) &&
      (first.protocol == second.protocol) &&
      (first.sourcePort == second.destinationPort) &&
      (first.destinationPort == second.sourcePort))
    {
      return true;
    }
  return false;
}

struct FlowRecord
{
  FlowId flowId;
  Ipv4FlowClassifier::FiveTuple fiveTuple;
  FlowMonitor::FlowStats flowStats; 
};

void
SaveTcpFlowMonitorStats (std::string filename, std::string simulationParams, Ptr<FlowMonitor> monitor, FlowMonitorHelper& flowmonHelper, double duration)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ofstream::out | std::ofstream::app);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  
  std::map<FlowId, FlowRecord> tupleMap;
  typedef std::map<FlowId, FlowRecord>::iterator tupleMapI;
  typedef std::pair<FlowRecord, FlowRecord> FlowPair;
  std::vector<FlowPair> downlinkFlowList;

  // Print per-flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      bool found = false;
      for (tupleMapI j = tupleMap.begin (); j != tupleMap.end (); ++j)
        {
          if (MatchingFlow (t, j->second.fiveTuple))
            {
              // We have found a pair of flow records; insert the pair
              // into the flowList.  i->second and j->second contain the
              // flow stats.  Erase the j record from the tupleMap
              FlowRecord firstFlow;
              firstFlow.flowId = i->first;
              firstFlow.fiveTuple = t;
              firstFlow.flowStats = i->second;
              FlowRecord secondFlow = j->second;
              // For downlink, the smaller IP address is the source
              // of the traffic in these scenarios
              FlowPair flowPair;
              if (t.sourceAddress < j->second.fiveTuple.sourceAddress)
                {
                  flowPair = std::make_pair (firstFlow, secondFlow);
                }
              else
                {
                  flowPair = std::make_pair (secondFlow, firstFlow);
                }
              downlinkFlowList.push_back (flowPair);
              tupleMap.erase (j);
              found = true;
              break;
            }
        }
      if (found == false)
        {
          FlowRecord newFlow;
          newFlow.flowId = i->first;
          newFlow.fiveTuple = t;
          newFlow.flowStats = i->second;
          tupleMap.insert (std::pair<FlowId, FlowRecord> (i->first, newFlow));
        }
    }
  for (std::vector<FlowPair>::size_type idx = 0; idx != downlinkFlowList.size (); idx++) 
    {
      // statistics for the downlink (data stream)
      FlowRecord record = downlinkFlowList[idx].first;
      outFile << record.flowId << " " << record.fiveTuple.sourceAddress << " " << record.fiveTuple.destinationAddress 
              << " " << record.flowStats.txPackets 
              << " " << record.flowStats.txBytes 
              << " " << record.flowStats.txBytes * 8.0 / duration/ 1000 / 1000 
              << " " << record.flowStats.rxBytes;
      // Measure the duration of the flow from receiver's perspective
      double rxDuration = record.flowStats.timeLastRxPacket.GetSeconds () - record.flowStats.timeFirstTxPacket.GetSeconds ();
      outFile << " " << record.flowStats.rxBytes * 8.0 / rxDuration / 1000 / 1000;
      if (record.flowStats.rxPackets > 0)
        {
          outFile << " " << 1000 * record.flowStats.delaySum.GetSeconds () / record.flowStats.rxPackets;
          outFile << " " << 1000 * record.flowStats.jitterSum.GetSeconds () / record.flowStats.rxPackets;
        }
      else
        {
          outFile << "  0";
          outFile << "  0";
        }
      outFile << " " << record.flowStats.rxPackets;

      outFile << " "; // add space separator 
      // statistics for the uplink (ACK stream)
      record = downlinkFlowList[idx].second;
      outFile << record.flowId << " " << record.fiveTuple.sourceAddress << " " << record.fiveTuple.destinationAddress 
              << " " << record.flowStats.txPackets 
              << " " << record.flowStats.txBytes 
              << " " << record.flowStats.txBytes * 8.0 / duration/ 1000 / 1000 
              << " " << record.flowStats.rxBytes;
      // Measure the duration of the flow from receiver's perspective
      rxDuration = record.flowStats.timeLastRxPacket.GetSeconds () - record.flowStats.timeFirstTxPacket.GetSeconds ();
      outFile << " " << record.flowStats.rxBytes * 8.0 / rxDuration / 1000 / 1000;
      if (record.flowStats.rxPackets > 0)
        {
          outFile << " " << 1000 * record.flowStats.delaySum.GetSeconds () / record.flowStats.rxPackets;
          outFile << " " << 1000 * record.flowStats.jitterSum.GetSeconds () / record.flowStats.rxPackets;
        }
      else
        {
          outFile << "  0";
          outFile << "  0";
        }
      outFile << " " << record.flowStats.rxPackets;
      outFile << std::endl;
    }

  outFile.close ();
}


void
SaveUdpFlowMonitorStats (std::string filename, std::string simulationParams, Ptr<FlowMonitor> monitor, FlowMonitorHelper& flowmonHelper, double duration)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ofstream::out | std::ofstream::app);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }

  // Print per-flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      NS_ASSERT_MSG (t.sourceAddress < t.destinationAddress ,
                 "Flow " << t.sourceAddress << " --> " << t.destinationAddress
                 << " is probably not downlink");  
      outFile << i->first << " " << t.sourceAddress << " " << t.destinationAddress 
              << " " << i->second.txPackets 
              << " " << i->second.txBytes 
              << " " << i->second.txBytes * 8.0 / duration/ 1000 / 1000 
              << " " << i->second.rxBytes;
      // Measure the duration of the flow from receiver's perspective
      double rxDuration = i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();
      outFile << " " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
      if (i->second.rxPackets > 0)
        {
          outFile << " " << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;
          outFile << " " << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets;
        }
      else
        {
          outFile << "  0";
          outFile << "  0";
        }
      outFile << " " << i->second.rxPackets << std::endl;
    }
  outFile.close ();
}

void
ConfigureLte (Ptr<LteHelper> lteHelper, Ptr<PointToPointEpcHelper> epcHelper, Ipv4AddressHelper& internetIpv4Helper, NodeContainer bsNodes, NodeContainer ueNodes, NodeContainer clientNodes, NetDeviceContainer& bsDevices, NetDeviceContainer& ueDevices, struct PhyParams phyParams, std::vector<LteSpectrumValueCatcher>& lteDlSinrCatcherVector, std::bitset<40> absPattern, Transport_e transport)
{


  // For LTE, the client node needs to be connected only to the PGW/SGW node
  // The EpcHelper will then take care of connecting the PGW/SGW node to the eNBs
  Ptr<Node> clientNode = clientNodes.Get (0);
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.0)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, clientNode);

  Ipv4InterfaceContainer internetIpIfaces = internetIpv4Helper.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address clientNodeAddr = internetIpIfaces.GetAddress (1);
  
  // make LTE and network reachable from the client node
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> clientNodeStaticRouting = ipv4RoutingHelper.GetStaticRouting (clientNode->GetObject<Ipv4> ());
  clientNodeStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  

  
  // LTE configuration parametes
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));
 // LTE-U DL transmission @5180 MHz
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (255444));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  // needed for initial cell search
  lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (255444));
  // LTE calibration
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute ("Gain",   DoubleValue (phyParams.m_bsTxGain));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (phyParams.m_bsTxPower));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (phyParams.m_ueTxPower));

  switch (transport)
    {
    case TCP:
      Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (LteEnbRrc::RLC_AM_ALWAYS));
      break;
      
    case UDP:
    default:
      Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (LteEnbRrc::RLC_UM_ALWAYS));
      break;
    }

  // Create Devices and install them in the Nodes (eNBs and UEs)
  bsDevices = lteHelper->InstallEnbDevice (bsNodes);
  ueDevices = lteHelper->InstallUeDevice (ueNodes);

  // additional eNB-specific configuration
  for (uint32_t n = 0; n < bsDevices.GetN (); ++n)
    {
      Ptr<NetDevice> enbDevice = bsDevices.Get (n);
      Ptr<LteEnbNetDevice> enbLteDevice = enbDevice->GetObject<LteEnbNetDevice> ();
      enbLteDevice->GetRrc ()->SetAbsPattern (absPattern);
    }  

  NetDeviceContainer ueLteDevs (ueDevices);


  // additional UE-specific configuration
  Ipv4InterfaceContainer clientIpIfaces;
  NS_ASSERT_MSG (lteDlSinrCatcherVector.empty (), "Must provide an empty lteDlSinCatcherVector");
  // side effect: will create LteSpectrumValueCatchers
  // note that nobody else should resize this vector otherwise callbacks will be using invalid pointers
  lteDlSinrCatcherVector.resize (ueNodes.GetN ()); 
  
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      Ptr<NetDevice> ueDevice = ueLteDevs.Get (u);
      Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();

      // assign IP address to UEs
      Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevice));
      clientIpIfaces.Add (ueIpIface);
      // set the default gateway for the UE
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      // set up SINR monitoring      
      Ptr<LtePhy> uePhy = ueLteDevice->GetPhy ()->GetObject<LtePhy> ();
      Ptr<LteAverageChunkProcessor> monitorLteChunkProcessor  = Create<LteAverageChunkProcessor> ();
      monitorLteChunkProcessor->AddCallback (MakeCallback (&LteSpectrumValueCatcher::ReportValue, &lteDlSinrCatcherVector.at(u)));
      uePhy->GetDownlinkSpectrumPhy ()->AddDataSinrChunkProcessor (monitorLteChunkProcessor);      
   }

  // instruct all devices to attach using the LTE initial cell selection procedure
  lteHelper->Attach (ueDevices);
}

NetDeviceContainer 
ConfigureWifiAp (NodeContainer bsNodes, struct PhyParams phyParams, Ptr<SpectrumChannel> channel, Ssid ssid)
{
  NetDeviceContainer apDevices;
  SpectrumWifiPhyHelper spectrumPhy = SpectrumWifiPhyHelper::Default ();
  spectrumPhy.SetChannel (channel);

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  HtWifiMacHelper mac = HtWifiMacHelper::Default ();

  // enable Block Ack for AC_BE
  mac.SetBlockAckThresholdForAc (AC_BE, 2);
  // enable MPDU aggregation for AC_BE
  mac.SetMpduAggregatorForAc (AC_BE,"ns3::MpduStandardAggregator");

  spectrumPhy.Set ("ShortGuardEnabled", BooleanValue (false));
  spectrumPhy.Set ("ChannelBonding", BooleanValue (false));
  spectrumPhy.Set ("TxGain", DoubleValue (phyParams.m_bsTxGain));
  spectrumPhy.Set ("RxGain", DoubleValue (phyParams.m_bsRxGain));
  spectrumPhy.Set ("TxPowerStart", DoubleValue (phyParams.m_bsTxPower));
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (phyParams.m_bsTxPower));
  spectrumPhy.Set ("RxNoiseFigure", DoubleValue (phyParams.m_bsNoiseFigure));
  spectrumPhy.Set ("Receivers", UintegerValue (2));
  spectrumPhy.Set ("Transmitters", UintegerValue (2));

  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "EnableBeaconJitter", BooleanValue (true));

  for (uint32_t i = 0; i < bsNodes.GetN (); i++)
    {
      //uint32_t channelNumber = 36 + 4 * (i%4); 
      uint32_t channelNumber = 36;
      spectrumPhy.SetChannelNumber (channelNumber); 
      apDevices.Add (wifi.Install (spectrumPhy, mac, bsNodes.Get (i)));
    }
  BooleanValue booleanValue;
  GlobalValue::GetValueByName ("pcapEnabled", booleanValue);
  if (booleanValue.Get() == true)
    {
      spectrumPhy.EnablePcap ("laa-wifi-ap", apDevices);
    } 
  return apDevices;
}

NetDeviceContainer 
ConfigureWifiSta (NodeContainer ueNodes, struct PhyParams phyParams, Ptr<SpectrumChannel> channel, Ssid ssid)
{
  NetDeviceContainer staDevices;
  SpectrumWifiPhyHelper spectrumPhy = SpectrumWifiPhyHelper::Default ();
  spectrumPhy.SetChannel (channel);

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  HtWifiMacHelper mac = HtWifiMacHelper::Default ();

  spectrumPhy.Set ("ShortGuardEnabled", BooleanValue (false));
  spectrumPhy.Set ("ChannelBonding", BooleanValue (false));
  spectrumPhy.Set ("TxGain", DoubleValue (phyParams.m_ueTxGain));
  spectrumPhy.Set ("RxGain", DoubleValue (phyParams.m_ueRxGain));
  spectrumPhy.Set ("TxPowerStart", DoubleValue (phyParams.m_ueTxPower));
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (phyParams.m_ueTxPower));
  spectrumPhy.Set ("RxNoiseFigure", DoubleValue (phyParams.m_ueNoiseFigure));
  spectrumPhy.Set ("Receivers", UintegerValue (2));
  spectrumPhy.Set ("Transmitters", UintegerValue (2));

  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  for (uint32_t i = 0; i < ueNodes.GetN (); i++)
    {
      //uint32_t channelNumber = 36 + 4 * (i%4); 
      uint32_t channelNumber = 36;
      spectrumPhy.SetChannelNumber (channelNumber); 
      staDevices.Add (wifi.Install (spectrumPhy, mac, ueNodes.Get (i)));
    }
  BooleanValue booleanValue;
  GlobalValue::GetValueByName ("pcapEnabled", booleanValue);
  if (booleanValue.Get() == true)
    {
      spectrumPhy.EnablePcap ("laa-wifi-sta", staDevices);
    } 
  return staDevices;
}

ApplicationContainer
ConfigureUdpServers (NodeContainer servers, Time startTime, Time stopTime)
{
  ApplicationContainer serverApps;
  uint32_t port = 9; // discard port
  UdpServerHelper myServer (port);
  serverApps = myServer.Install (servers);
  serverApps.Start (startTime);
  serverApps.Stop (stopTime);
  return serverApps;
}

ApplicationContainer
ConfigureUdpClients (NodeContainer client, Ipv4InterfaceContainer servers, Time startTime, Time stopTime, Time interval)
{
  // Randomly distribute the start times across 100ms interval
  Ptr<UniformRandomVariable> randomVariable = CreateObject<UniformRandomVariable> ();
  randomVariable->SetAttribute ("Max", DoubleValue (0.100));
  uint32_t packetSize = 1000;  // 1000-byte IP datagrams
  uint32_t remotePort = 9;
  ApplicationContainer clientApps;
  UdpClientHelper clientHelper (Address (), 0);
  clientHelper.SetAttribute ("MaxPackets", UintegerValue (1e6));
  clientHelper.SetAttribute ("Interval", TimeValue (interval));
  clientHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
  clientHelper.SetAttribute ("RemotePort", UintegerValue (remotePort));

  ApplicationContainer pingApps;
  for (uint32_t i = 0; i < servers.GetN (); i++)
    {
      Ipv4Address ip = servers.GetAddress (i, 0);
      clientHelper.SetAttribute ("RemoteAddress", AddressValue (ip));
      clientApps.Add (clientHelper.Install (client));

      // Seed the ARP cache by pinging early in the simulation
      // This is a workaround until a static ARP capability is provided
      V4PingHelper ping (ip);
      pingApps.Add (ping.Install (client));
    }
  clientApps.Start (startTime + Seconds (randomVariable->GetValue ()));
  clientApps.Stop (stopTime);
  // Add one or two pings for ARP at the beginnning of the simulation
  pingApps.Start (Seconds (1) + Seconds (randomVariable->GetValue ()));
  pingApps.Stop (Seconds (3));
  return clientApps;
}

ApplicationContainer
ConfigureTcpServers (NodeContainer servers, Time startTime)
{
  ApplicationContainer serverApps;
  uint16_t port = 50000;
  Address apLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", apLocalAddress);
  serverApps = packetSinkHelper.Install (servers);
  serverApps.Start (startTime);
  return serverApps;
}

ApplicationContainer
ConfigureTcpClients (NodeContainer client, Ipv4InterfaceContainer servers, Time startTime)
{
  // Randomly distribute the start times across 100ms interval
  Ptr<UniformRandomVariable> randomVariable = CreateObject<UniformRandomVariable> ();
  randomVariable->SetAttribute ("Max", DoubleValue (0.100));
  ApplicationContainer clientApps;
  uint16_t port = 50000;
  uint32_t ftpSegSize = 1448; // bytes
  uint32_t ftpFileSize = 512000; // bytes
  FileTransferHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("SendSize", UintegerValue (ftpSegSize));
  ftp.SetAttribute ("FileSize", UintegerValue (ftpFileSize));

  ApplicationContainer pingApps;
  for (uint32_t i = 0; i < servers.GetN (); i++)
    {
      Ipv4Address ip = servers.GetAddress (i, 0);
      AddressValue remoteAddress (InetSocketAddress (ip, port));
      ftp.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (ftp.Install (client));

      // Seed the ARP cache by pinging early in the simulation
      // This is a workaround until a static ARP capability is provided
      V4PingHelper ping (ip);
      pingApps.Add (ping.Install (client));
    }
  clientApps.Start (startTime + Seconds (randomVariable->GetValue ()));
  // Add one or two pings for ARP at the beginnning of the simulation
  pingApps.Start (Seconds (1) + Seconds (randomVariable->GetValue ()));
  pingApps.Stop (Seconds (3));
  return clientApps;
}

void
StartFileTransfer (Ptr<ExponentialRandomVariable> ftpArrivals, ApplicationContainer clients, uint32_t nextClient, Time stopTime)
{
  NS_LOG_DEBUG ("Starting file transfer on client " << nextClient << " at time " << Simulator::Now ().GetSeconds ());
  Ptr<Application> app = clients.Get (nextClient);
  NS_ASSERT (app);
  Ptr<FileTransferApplication> fileTransfer = DynamicCast <FileTransferApplication> (app);
  NS_ASSERT (fileTransfer);
  fileTransfer->SendFile ();

  // We want to alternate between operators.  If there are N clients in the
  // container, then clients 0 to (N/2-1) are for operator A, and clients
  // N/2 to (N-1) are for operator B.  We would want a selection pattern
  // such as "0, 10, 1, 11, 2, 12 ... 19, 0, 10, 1, 11..."  
  if (nextClient == (clients.GetN () - 1))
    {
      nextClient = 0;
    }
  else if (nextClient < (clients.GetN ()/2))
    {
      nextClient += (clients.GetN ()/2);
    }
  else
    {
      nextClient -= (clients.GetN ()/2 -1);
    }
  NS_ASSERT (nextClient >= 0 && nextClient < clients.GetN ());
  Time nextTime = Seconds (ftpArrivals->GetValue ());
  if (Simulator::Now () + nextTime < stopTime)
    {
      Simulator::Schedule (nextTime, &StartFileTransfer, ftpArrivals, clients, nextClient, stopTime);
    }
} 



void 
PrintGnuplottableNodeListToFile (std::string filename, NodeContainer nodes, bool printId, std::string label, std::string howToPlot)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeContainer::Iterator it = nodes.Begin (); it != nodes.End (); ++it)
    {
      Ptr<Node> node = *it;
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
      outFile << "set label \"" ;
      if (printId)
        {
          outFile << label << node->GetId (); 
        }
      outFile << "\" at "<< pos.x << "," << pos.y 
              << " " << howToPlot
              << std::endl;
    }
}



void 
ConfigureAndRunScenario (Config_e cellConfigA,
                         Config_e cellConfigB,
                         NodeContainer bsNodesA, 
                         NodeContainer bsNodesB, 
                         NodeContainer ueNodesA, 
                         NodeContainer ueNodesB,
                         struct PhyParams phyParams,
                         Time durationTime,
                         Transport_e transport,
                         std::string propagationLossModel,
                         bool disableApps,
                         double lteDutyCycle,
                         bool generateRem,
                         std::string outFileName,
                         std::string simulationParams)
{
  DoubleValue doubleValue;
  // Configure times that applications will run, based on 'duration' above
  // First, let the scenario initialize for some seconds
  GlobalValue::GetValueByName ("serverStartTimeSeconds", doubleValue);
  Time serverStartTime = Seconds (doubleValue.Get ());
  GlobalValue::GetValueByName ("clientStartTimeSeconds", doubleValue);
  Time clientStartTime = Seconds (doubleValue.Get ());
  // run servers a bit longer than clients 
  GlobalValue::GetValueByName ("serverLingerTimeSeconds", doubleValue);
  Time serverLingerTime = Seconds (doubleValue.Get ());
  // run simulation a bit longer than data transfers
  GlobalValue::GetValueByName ("simulationLingerTimeSeconds", doubleValue);
  Time simulationLingerTime = Seconds (doubleValue.Get ());

  // Now set start and stop times derived from the above
  Time serverStopTime = serverStartTime + durationTime + serverLingerTime;
  Time clientStopTime = clientStartTime + durationTime;
  // Overall simulation stop time is below
  Time stopTime = serverStopTime + simulationLingerTime; 


  // The max data rate of UDP applications is managed by the below
  // interval variable.  106 microseconds with 1000 byte packets
  // corresponds to roughly 75 Mb/s (max PHY rate of LTE SISO over
  // 20MHz bandwidth; the max PHY rate of Wi-Fi SISO is lower). Hence
  // this value will saturate the channel for a single UE scenario. 
  // To reduce the unnecessary computational load of simulation, we
  // divide this rate by the average number of UEs per cell; the
  // resulting rate is expected to saturate the channel in any case.
  // Note that even though the number of actual UEs in a cell
  // might be lower than the average, adaptive modulation and
  // coding/rate adaptation will typically cause many UEs to have a
  // lower rate than the maximum. Anyway, your mileage might vary.
  Time udpInterval = MicroSeconds (106 * ueNodesA.GetN () / bsNodesA.GetN ());

  std::cout << "Running simulation for " << durationTime.GetSeconds () << " sec of data transfer; " 
    << stopTime.GetSeconds () << " sec overall" << std::endl;
  
  std::cout << "Operator A: " << CellConfigToString (cellConfigA) 
    << "; number of cells " << bsNodesA.GetN () << "; number of UEs " 
    << ueNodesA.GetN () << std::endl;
  std::cout << "Operator B: " << CellConfigToString (cellConfigB) 
    << "; number of cells " << bsNodesB.GetN () << "; number of UEs " 
    << ueNodesB.GetN () << std::endl;

  // All application data will be sourced from the client node so that
  // it shows up on the downlink
  NodeContainer clientNodesA;  // for the backhaul application client
  clientNodesA.Create (1); // create one remote host for sourcing traffic
  NodeContainer clientNodesB;  // for the backhaul application client
  clientNodesB.Create (1); // create one remote host for sourcing traffic

  // For Wi-Fi, the client node needs to be connected to the bsNodes via a single
  // point-to-point link to each one.
  //
  //
  //                 client
  //               /  |    |
  //              /   |    |
  //           bs0   bs1... bsN
  //
  //  First we create a point to point helper that will instantiate the
  //  client <-> bsN link for each network

  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1000Gb/s")));
  p2pHelper.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));

  NetDeviceContainer p2pDevicesA;
  NetDeviceContainer p2pDevicesB;

  //
  // Wireless setup phase
  //

  // Device containers hold the WiFi or LTE NetDevice objects created
  NetDeviceContainer bsDevicesA;
  NetDeviceContainer ueDevicesA;
  NetDeviceContainer bsDevicesB;
  NetDeviceContainer ueDevicesB;

  // Start to create the wireless devices by first creating the shared channel
  // Note:  the design of LTE requires that we use an LteHelper to
  // create the channel, if we are potentially using LTE in one of the networks
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  lteHelper->SetAttribute ("PathlossModel", StringValue (propagationLossModel));
  // since LAA is using CA, RRC messages will be excanged in the
  // licensed bands, hence we model it using the ideal RRC 
  lteHelper->SetAttribute ("UseIdealRrc", BooleanValue (true));
  lteHelper->SetAttribute ("UsePdschForCqiGeneration", BooleanValue (true));

  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->Initialize ();
  std::vector<LteSpectrumValueCatcher> lteDlSinrCatcherVectorA;
  std::vector<LteSpectrumValueCatcher> lteDlSinrCatcherVectorB;

  // set channel MaxLossDb to discard all transmissions below -15dB SNR. 
  // The calculations assume -174 dBm/Hz noise PSD and 20 MHz bandwidth (73 dB)
  Ptr<SpectrumChannel> dlSpectrumChannel = lteHelper->GetDownlinkSpectrumChannel ();
  //           loss  = txpower -noisepower            -snr    ;   
  double dlMaxLossDb = (phyParams.m_bsTxPower + phyParams.m_bsTxGain) - 
                       (-174.0 + 73.0 + phyParams.m_ueNoiseFigure) -
                       (-15.0);
  dlSpectrumChannel->SetAttribute ("MaxLossDb", DoubleValue (dlMaxLossDb));
  Ptr<SpectrumChannel> ulSpectrumChannel = lteHelper->GetUplinkSpectrumChannel ();
  //           loss  = txpower -noisepower            -snr    ;   
  double ulMaxLossDb = (phyParams.m_ueTxPower + phyParams.m_ueTxGain)  -
                       (-174.0 + 73.0 + phyParams.m_bsNoiseFigure) -
                       (-15.0);
  ulSpectrumChannel->SetAttribute ("MaxLossDb", DoubleValue (ulMaxLossDb));

  // determine the LTE Almost Blank Subframe (ABS) pattern that will implement the desired duty cycle
  NS_ASSERT_MSG (lteDutyCycle >=0 && lteDutyCycle <= 1, "lteDutyCycle must be between 1 and 0");
  std::bitset<40> absPattern;
  // need at least two regular subframes for MIB and SIB1
  absPattern[0] = 0;
  absPattern[35] = 0;
  uint32_t regularSubframes = 2;
  int32_t subframe = 39;  
  while ((regularSubframes < 40) && (regularSubframes/40.0 < lteDutyCycle))
    {
      if (subframe != 0 && subframe != 35)
        {          
          absPattern[subframe] = 0;
          ++regularSubframes;
        }
      --subframe;        
    }
  while (subframe >= 0)
    {
      if (subframe != 0 && subframe != 35)
        {         
          absPattern[subframe] = 1; 
        }
      --subframe;
    }
  double actualLteDutyCycle = regularSubframes/40.0;
  if (cellConfigA == LTE || cellConfigB == LTE)
    {
      std::cout << "LTE duty cycle: requested " << lteDutyCycle << ", actual " << actualLteDutyCycle << ", ABS pattern " << absPattern << std::endl;
    }
  


  // LTE requires some Internet stack configuration prior to device installation
  // Wifi configures it after device installation
  InternetStackHelper internetStackHelper;
 // Install an internet stack to all the nodes with IP
  internetStackHelper.Install (clientNodesA);
  internetStackHelper.Install (clientNodesB);
  internetStackHelper.Install (ueNodesA);
  internetStackHelper.Install (ueNodesB);



  //
  // Configure the right technology for each operator A and B
  // 

  //
  // Configure operator A
  //

  if (cellConfigA == WIFI)
    {
      internetStackHelper.Install (bsNodesA);
      Ptr<SpectrumChannel> spectrumChannel = lteHelper->GetDownlinkSpectrumChannel ();
      bsDevicesA.Add (ConfigureWifiAp (bsNodesA, phyParams, spectrumChannel, Ssid ("ns380211n-A")));
      ueDevicesA.Add (ConfigureWifiSta (ueNodesA, phyParams, spectrumChannel, Ssid ("ns380211n-A")));
      Ipv4AddressHelper ipv4h;
      ipv4h.SetBase ("11.0.0.0", "255.255.0.0");
      // Add backhaul links to each BS
      for (uint32_t i = 0; i < bsNodesA.GetN (); i++)
        {
          p2pDevicesA.Add (p2pHelper.Install (clientNodesA.Get (0), bsNodesA.Get (i)));
          // Add IP addresses to backhaul links
          ipv4h.Assign (p2pDevicesA);
          ipv4h.NewNetwork ();
        }
    }
  else 
    { 
      NS_ASSERT (cellConfigA == LTE);      
      lteHelper->SetEnbDeviceAttribute ("CsgIndication", BooleanValue (true));
      lteHelper->SetEnbDeviceAttribute ("CsgId", UintegerValue (1));
      lteHelper->SetUeDeviceAttribute ("CsgId", UintegerValue (1));
      Ipv4AddressHelper internetIpv4Helper;
      internetIpv4Helper.SetBase ("1.0.0.0", "255.0.0.0");
      ConfigureLte (lteHelper, epcHelper, internetIpv4Helper, bsNodesA, ueNodesA, clientNodesA, bsDevicesA, ueDevicesA, phyParams, lteDlSinrCatcherVectorA, absPattern, transport);
    }

  //
  // Configure operator B
  //

  if (cellConfigB == WIFI)
    {
      internetStackHelper.Install (bsNodesB);
      Ptr<SpectrumChannel> spectrumChannel = lteHelper->GetDownlinkSpectrumChannel ();
      bsDevicesB.Add (ConfigureWifiAp (bsNodesB, phyParams, spectrumChannel, Ssid ("ns380211n-B")));
      ueDevicesB.Add (ConfigureWifiSta (ueNodesB, phyParams, spectrumChannel, Ssid ("ns380211n-B")));

      Ipv4AddressHelper ipv4h;
      ipv4h.SetBase ("12.0.0.0", "255.255.0.0");
      for (uint32_t i = 0; i < bsNodesB.GetN (); i++)
        {
          p2pDevicesB.Add (p2pHelper.Install (clientNodesB.Get (0), bsNodesB.Get (i)));
          // Add IP addresses to backhaul links
          ipv4h.Assign (p2pDevicesB);
          ipv4h.NewNetwork ();
        }
    }
  else 
    {

      NS_ASSERT (cellConfigB == LTE);      
      lteHelper->SetEnbDeviceAttribute ("CsgIndication", BooleanValue (true));
      lteHelper->SetEnbDeviceAttribute ("CsgId", UintegerValue (2));
      lteHelper->SetUeDeviceAttribute ("CsgId", UintegerValue (2));
      Ipv4AddressHelper internetIpv4Helper;
      internetIpv4Helper.SetBase ("2.0.0.0", "255.0.0.0");
      ConfigureLte (lteHelper, epcHelper,  internetIpv4Helper, bsNodesB, ueNodesB, clientNodesB, bsDevicesB, ueDevicesB, phyParams, lteDlSinrCatcherVectorB, absPattern, transport);
    }

  //
  // IP addressing setup phase
  //

  Ipv4InterfaceContainer ipBsA;
  Ipv4InterfaceContainer ipUeA;
  Ipv4InterfaceContainer ipBsB;
  Ipv4InterfaceContainer ipUeB;

  Ipv4AddressHelper ueAddress;
  ueAddress.SetBase ("17.0.0.0", "255.255.0.0");
  ipBsA = ueAddress.Assign (bsDevicesA);
  ipUeA = ueAddress.Assign (ueDevicesA);

  ueAddress.SetBase ("18.0.0.0", "255.255.0.0");
  ipBsB = ueAddress.Assign (bsDevicesB);
  ipUeB = ueAddress.Assign (ueDevicesB);

  // Routing
  // WiFi nodes will trigger an association callback, which can invoke
  // a method to configure the appropriate routes on client and STA
  Config::Connect("/NodeList/*/DeviceList/*/Mac/Assoc", MakeCallback(&ConfigureRouteForStation));

  //
  // Application setup phase
  //

  Ptr<ExponentialRandomVariable> ftpArrivals = CreateObject<ExponentialRandomVariable> ();
  double ftpLambda;
  bool found = GlobalValue::GetValueByNameFailSafe ("ftpLambda", doubleValue);
  if (found)
    {
      ftpLambda = doubleValue.Get ();
      ftpArrivals->SetAttribute ("Mean", DoubleValue (1/ftpLambda));
    }
  uint32_t nextClient = 0;

  ApplicationContainer serverApps, clientApps;
  if (disableApps == false)
    {
      if (transport == UDP)
        {
          serverApps.Add (ConfigureUdpServers (ueNodesA, serverStartTime, serverStopTime));
          clientApps.Add (ConfigureUdpClients (clientNodesA, ipUeA, clientStartTime, clientStopTime, udpInterval));
          serverApps.Add (ConfigureUdpServers (ueNodesB, serverStartTime, serverStopTime));
          clientApps.Add (ConfigureUdpClients (clientNodesB, ipUeB, clientStartTime, clientStopTime, udpInterval));
        }
      else
        {
          serverApps.Add (ConfigureTcpServers (ueNodesA, serverStartTime));
          clientApps.Add (ConfigureTcpClients (clientNodesA, ipUeA, clientStartTime));
          serverApps.Add (ConfigureTcpServers (ueNodesB, serverStartTime));
          clientApps.Add (ConfigureTcpClients (clientNodesB, ipUeB, clientStartTime));
          // Start file transfer arrival process
          double firstArrival = ftpArrivals->GetValue ();
          NS_LOG_DEBUG ("First FTP arrival at time " << clientStartTime.GetSeconds () + firstArrival);
          Simulator::Schedule (clientStartTime + Seconds (firstArrival), &StartFileTransfer, ftpArrivals, clientApps, nextClient, clientStopTime);
        }
    }

  // All of the client apps are on node clientNodes
  // Server apps are distributed among the ueNodesA and ueNodesB.  
  // Data primarily flows from client to server (i.e. on the downlink)

  //
  // Statistics setup phase
  //

  // Add flow monitors
  // *** NOTE ****
  // we'd like two FlowMonitors so that we can split operator A and B
  // stats. However if we just use two FlowMonitor instances but a
  // single FlowMonitorHelper it won't work - have a look at
  // FlowMonitorHelper::GetMonitor (). If you look at the docs, they
  // say that you should only use one FlowMonitorHelper instance
  // https://www.nsnam.org/docs/models/html/flow-monitor.html#helpers
  // but well I tried with two helpers and it seems to work :-)
  // this email might also be relevant
  // https://groups.google.com/d/msg/ns-3-users/rl77tJCcw8c/egMn__QyVNQJ
  
  FlowMonitorHelper flowmonHelperA;
  NodeContainer endpointNodesA;
  endpointNodesA.Add (clientNodesA);
  endpointNodesA.Add (ueNodesA);
  FlowMonitorHelper flowmonHelperB;
  NodeContainer endpointNodesB;
  endpointNodesB.Add (clientNodesB);
  endpointNodesB.Add (ueNodesB);
  

  Ptr<FlowMonitor> monitorA = flowmonHelperA.Install (endpointNodesA);
  monitorA->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitorA->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitorA->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));

  Ptr<FlowMonitor> monitorB = flowmonHelperB.Install (endpointNodesB);
  monitorB->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitorB->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitorB->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));


  // these slow down simulations, only enable them if you need them
  // lteHelper->EnablePhyTraces ();
  // lteHelper->EnableMacTraces ();
  // lteHelper->EnableRlcTraces ();
  // lteHelper->EnablePdcpTraces ();
  

  Ptr<RadioEnvironmentMapHelper> remHelper;
  if (generateRem)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("remDir", stringValue);
      std::string remDir = stringValue.Get ();

      PrintGnuplottableNodeListToFile (remDir + "/bs_A_labels.gnuplot", bsNodesA, true, "A_BS_", " center textcolor rgb \"cyan\" front point pt 5 lc rgb \"cyan\" offset 0,0.5");
      PrintGnuplottableNodeListToFile (remDir + "/ue_A_labels.gnuplot", ueNodesA, true, "A_UE_", " center textcolor rgb \"cyan\" front point pt 4 lc rgb \"cyan\" offset 0,0.5");
      PrintGnuplottableNodeListToFile (remDir + "/bs_B_labels.gnuplot", bsNodesB, true, "B_BS_", " center textcolor rgb \"chartreuse\" front point pt 7 lc rgb \"chartreuse\" offset 0,0.5");
      PrintGnuplottableNodeListToFile (remDir + "/ue_B_labels.gnuplot", ueNodesB, true, "B_UE_", " center textcolor rgb \"chartreuse\" front point pt 6 lc rgb \"chartreuse\" offset 0,0.5");

      PrintGnuplottableNodeListToFile (remDir + "/bs_A.gnuplot", bsNodesA, false, "", " point pt 5 lc rgb \"cyan\" front ");
      PrintGnuplottableNodeListToFile (remDir + "/ue_A.gnuplot", ueNodesA, false, "", " point pt 4 lc rgb \"cyan\" front ");
      PrintGnuplottableNodeListToFile (remDir + "/bs_B.gnuplot", bsNodesB, false, "", " point pt 7 lc rgb \"chartreuse\" front ");
      PrintGnuplottableNodeListToFile (remDir + "/ue_B.gnuplot", ueNodesB, false, "", " point pt 6 lc rgb \"chartreuse\" front ");

      remHelper = CreateObject<RadioEnvironmentMapHelper> ();
      remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
      remHelper->SetAttribute ("Earfcn", UintegerValue (255444));

      remHelper->Install ();
      // simulation will stop right after the REM has been generated
    }
  else
    {
      Simulator::Stop (stopTime);
    }

  //
  // Running the simulation
  //

  Simulator::Run ();

  //
  // Post-processing phase
  //

  std::cout << "--------monitorA----------" << std::endl;
  PrintFlowMonitorStats (monitorA, flowmonHelperA, durationTime.GetSeconds ());
  std::cout << "--------monitorB----------" << std::endl;
  PrintFlowMonitorStats (monitorB, flowmonHelperB, durationTime.GetSeconds ());

  if (transport == TCP)
    {
      SaveTcpFlowMonitorStats (outFileName + "_operatorA", simulationParams, monitorA, flowmonHelperA, durationTime.GetSeconds ());
      SaveTcpFlowMonitorStats (outFileName + "_operatorB", simulationParams, monitorB, flowmonHelperB, durationTime.GetSeconds ());  
    }
  else if (transport == UDP)
    {
      SaveUdpFlowMonitorStats (outFileName + "_operatorA", simulationParams, monitorA, flowmonHelperA, durationTime.GetSeconds ());
      SaveUdpFlowMonitorStats (outFileName + "_operatorB", simulationParams, monitorB, flowmonHelperB, durationTime.GetSeconds ());  
    }
  else
    {
      NS_FATAL_ERROR ("transport parameter invalid: " << transport);
    }

  Simulator::Destroy ();

}
