
// CSMA protocol and LAN with 50 Nodes
// UDP echo server on Node 50
// first 10 Nodes  UDP echo clients
// TCP server (use PacketSinkHelper) on Node 1
// last 10 Nodes (41 to 50)  TCP clients (use BulkSendHelper)


#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/packet-sink.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");
void
printStats (FlowMonitorHelper &flowmon_helper, bool perFlowInfo) {
  Ptr<Ipv4FlowClassifier> classifier =
DynamicCast<Ipv4FlowClassifier>(flowmon_helper.GetClassifier());
  std::string proto;
  Ptr<FlowMonitor> monitor = flowmon_helper.GetMonitor ();
  std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();
  double totalTimeReceiving;
  uint64_t totalPacketsReceived, totalPacketsDropped,
totalBytesReceived,totalPacketsTransmitted;

  totalBytesReceived = 0, totalPacketsDropped = 0,
totalPacketsReceived = 0, totalTimeReceiving =
0,totalPacketsTransmitted = 0;
  for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow =
stats.begin(); flow != stats.end(); flow++)
  {
    Ipv4FlowClassifier::FiveTuple  t = classifier->FindFlow(flow->first);
    switch(t.protocol)
     {
     case(6):
         proto = "TCP";
         break;
     case(17):
         proto = "UDP";
         break;
     default:
         exit(1);
     }
     totalBytesReceived += (double) flow->second.rxBytes * 8;
     totalTimeReceiving += flow->second.timeLastRxPacket.GetSeconds ();
     totalPacketsReceived += flow->second.rxPackets;
     totalPacketsDropped += flow->second.txPackets - flow->second.rxPackets;
     totalPacketsTransmitted += flow->second.txPackets;
     if (perFlowInfo)
     {
      std::cout << "FlowID: " << flow->first << " (" << proto << " "
                 << t.sourceAddress << " / " << t.sourcePort << " --> "
                 << t.destinationAddress << " / " << t.destinationPort << ")" << std::endl;
       std::cout << "  Tx Bytes: " << flow->second.txBytes << std::endl;
std::cout << "  Lost Packets: " << flow->second.lostPackets << std::endl;
       std::cout << "  Pkt Lost Ratio: " <<((double)flow->second.txPackets-(double)flow->second.rxPackets)/(double)flow->second.txPackets << std::endl;
       std::cout << "  Throughput: " << (((double)flow->second.rxBytes * 8) /(flow->second.timeLastRxPacket.GetSeconds ()) ) << "bits/s" <<std::endl;
       std::cout << "  Mean{Delay}: " <<(flow->second.delaySum.GetSeconds()/flow->second.rxPackets) <<std::endl;
       std::cout << "  Mean{Jitter}: " <<(flow->second.jitterSum.GetSeconds()/(flow->second.rxPackets)) <<std::endl;
     }
}

     std::cout<< "Final throughput with (packets received/total time): "<<(totalPacketsReceived * 1024 * 8)/totalTimeReceiving<<" bps "<<std::endl;
     std::cout<< "Final throughput with (packets received/total time): "<<(totalBytesReceived)/totalTimeReceiving<<" bps "<<std::endl;
     std::cout<<"Total packets transmitted:"<<totalPacketsTransmitted<<std::endl;
     std::cout<<"Total packets received: "<< totalPacketsReceived<<std::endl;
     std::cout<<"Total packets dropped: "<< totalPacketsDropped<<std::endl;
     std::cout << "Packet Lost Ratio: " << totalPacketsDropped /(double) (totalPacketsReceived + totalPacketsDropped) << std::endl;
}


int
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 49;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  for (uint32_t i = 0; i < 10; i++)
  {
    if(i <= 1)
    {
      UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (2));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

      ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (i));
      clientApps.Start (Seconds (2.0));
      clientApps.Stop (Seconds (10.0));
    }
    else
    {
      UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (2 ));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

      ApplicationContainer clientApps = echoClient.Install (csmaNodes.Get (i));
      clientApps.Start (Seconds (2.0));
      clientApps.Stop (Seconds (10.0));
    }

  }
  PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (),9));
  ApplicationContainer sinkApps = sink.Install (p2pNodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));
  for(uint32_t j = 40;j < 50;j++)
  {
    BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (csmaInterfaces.GetAddress (1),9));
   // Set the amount of data to send in bytes.  Zero is unlimited.
   source.SetAttribute ("MaxBytes", UintegerValue (2));
   ApplicationContainer sourceApps = source.Install (csmaNodes.Get (j));
   sourceApps.Start (Seconds (0.0));
   sourceApps.Stop (Seconds (10.0));
  }


  pointToPoint.EnablePcapAll ("second");
  csma.EnablePcap ("second", csmaDevices.Get (1), true);

  Ptr<FlowMonitor> flowmon;
	  FlowMonitorHelper flowmonHelper;
	  flowmon = flowmonHelper.InstallAll ();

	  Simulator::Stop (Seconds(10.1));
	  Simulator::Run ();

	  printStats (flowmonHelper, true);
	  Simulator::Destroy ();
  return 0;
}
