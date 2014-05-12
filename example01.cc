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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"  

using namespace ns3;

// 항상 필요
NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");
  
int main (int argc, char *argv[])
{
  // terminal에서 값들을 받아 올 수 있도록 함?
  uint32_t nPackets = 1;
  CommandLine cmd;
  cmd.AddValue("nPackets", "Number of packets to echo" , nPackets);
  cmd.Parse(argc, argv);

  // info_level에 설정한 곳까지 log가 남음?
  NS_LOG_INFO("Creating Topology");
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  
  // Container 만들고 노드 두 개 만듦
  NodeContainer nodes;
  nodes.Create (2);

  // 시뮬레이션 할 상황이 p2p라서 p2phelper로 만듦
  PointToPointHelper pointToPoint;
  //  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  //  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  // nodes에 netdevice를 설치?
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  // 주소 설정?
  address.SetBase ("10.1.1.0" /* address 시작? */, "255.255.255.0" /* subnet mask 설정 255.255.255.0 */);	

  // 하나씩 device들에 address를 넣어줌.
  // 0번 노드는 10.1.1.0, 1번 노드는 10.1.1.1, 2번 노드는 10.1.1.2 ...
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  UdpEchoServerHelper echoServer (9); // 9 : 포트 번호

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));  // 2번째 노드를 서버로 설정
  // 서버 app의 실행 시간 설정.
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // 클라이언트 설정
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1) /* 서버(1번)의 address 가져옴 */, 9);
  //  echoClient.SetAttribute ("MaxPackets", UintegerValue (1)); // 패킷 하나 보냄
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets)); // terminal에서 패킷 개수 받아서 변수로 씀
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0))); // 1초마다 보냄
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  // 클라이언트 만듦
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // added for ascii tracing option
  //AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll(ascii.CreateFileStream("myfirst.tr"));
  // added for pcap tracing option
  pointToPoint.EnablePcapAll("myfirst"); // 확장자 없이 할 것?

  // 설정 다 했으니 시뮬레이션 시작
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
