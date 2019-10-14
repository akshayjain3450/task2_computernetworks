/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

 /*
  * This is the architecture
  * n0-----n1-----n2
  *   			|
						|
						|
						n3
  * L1 -> 5Mbps, 2ms
  * L2 -> 10Mbps, 5ms
  * n0 - client, n2 - server
  *
  */

#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

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
	Time::SetResolution (Time::NS);
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

	NodeContainer nodes;
	nodes.Create (4);

	InternetStackHelper stack;
	stack.Install (nodes);

	PointToPointHelper p2p1;
	p2p1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
	p2p1.SetChannelAttribute ("Delay", StringValue ("10ms"));

	PointToPointHelper p2p2;
	p2p2.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
	p2p2.SetChannelAttribute ("Delay", StringValue ("10ms"));

	PointToPointHelper p2p3;
	p2p3.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
	p2p3.SetChannelAttribute ("Delay", StringValue ("10ms"));

	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	NetDeviceContainer devices;
	devices = p2p1.Install (nodes.Get (0), nodes.Get (1));
	Ipv4InterfaceContainer interfaces = address.Assign (devices);

	devices = p2p2.Install (nodes.Get (1), nodes.Get (2));
	address.SetBase ("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces2 = address.Assign (devices);

	devices = p2p3.Install (nodes.Get (2), nodes.Get (3));
	address.SetBase ("10.1.3.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces3 = address.Assign (devices);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	UdpEchoServerHelper echoServer (9);

	ApplicationContainer serverApps = echoServer.Install (nodes.Get (2));
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Minutes (999999999));

	UdpEchoClientHelper echoClient (interfaces2.GetAddress (1), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.01)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
	clientApps.Start (Seconds (1.0));
	clientApps.Stop (Minutes(999999999));

  /*UdpEchoServerHelper echoServer1 (10);

	ApplicationContainer serverApps1 = echoServer1.Install (nodes.Get (3));
	serverApps1.Start (Seconds (1.0));
	serverApps1.Stop (Seconds (20000.0));

	UdpEchoClientHelper echoClient1 (interfaces3.GetAddress (1), 10);
	echoClient1.SetAttribute ("MaxPackets", UintegerValue (200000));
	echoClient1.SetAttribute ("Interval", TimeValue (Seconds (0.01)));
	echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps1 = echoClient1.Install (nodes.Get (2));
	clientApps1.Start (Seconds (1.0));
	clientApps1.Stop (Seconds ());*/

	AnimationInterface anim ("l1q1.xml");
	anim.SetConstantPosition(nodes.Get(0),0.0,0.0);
	anim.SetConstantPosition(nodes.Get(1),100.0,0.0);
	anim.SetConstantPosition(nodes.Get(2),200.0,0.0);
	anim.SetConstantPosition(nodes.Get(3),100.0,100.0);

	Ptr<FlowMonitor> flowmon;
	  FlowMonitorHelper flowmonHelper;
	  flowmon = flowmonHelper.InstallAll ();

	  Simulator::Stop (Seconds(10.1));
	  Simulator::Run ();

	  printStats (flowmonHelper, true);
	  Simulator::Destroy ();

	return 0;
}
