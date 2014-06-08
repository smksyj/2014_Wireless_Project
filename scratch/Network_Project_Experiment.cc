#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

#include "ns3/carafwifimanager.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Network_Project");

class MyApp : public Application 
{
public:
  MyApp ();
  virtual ~MyApp();
  void Setup (Ptr<Socket> socket, Address address, uint32_t 
              packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void ScheduleTx (void);
  void SendPacket (void);
  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent; };

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void 
MyApp::StopApplication (void)
{
  m_running = false;
  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }
  if (m_socket)
    {
      m_socket->Close ();
    }
}
void 
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}
void 
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);    
    }
}

uint32_t data = 0;

static void
RxCnt (Ptr<const Packet> p, const Address &a)
{
  data = data + p->GetSize();
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << data);
}

#define WIFI_NUMBER 3

int main(int argc, char *argv[]) {
  int16_t pktSize = 1000;
  // int nWifi = 5;
  int nWifi = WIFI_NUMBER;
  uint16_t rtsCtsThreshold = 2000;

  CommandLine cmd;
  cmd.AddValue("nWifi", "Number of Wifi STA Devices", nWifi);
  cmd.AddValue("rtsThre", "RTS/CTS threshold", rtsCtsThreshold);
  cmd.Parse(argc, argv);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(nWifi);

  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(channel.Create());

  WifiHelper wifi = WifiHelper::Default();
  wifi.SetRemoteStationManager("ns3::CarafWifiManager", "RtsCtsThreshold", UintegerValue(rtsCtsThreshold));

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default();

  Ssid ssid = Ssid("ns-3-ssid");
  mac.SetType("ns3::StaWifiMac",
              "Ssid", SsidValue(ssid),
              "ActiveProbing", BooleanValue(false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install(phy, mac, wifiStaNodes);
  mac.SetType("ns3::ApWifiMac",
              "Ssid", SsidValue(ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install(phy, mac, wifiApNode);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); 
  mobility.Install (wifiStaNodes);
  mobility.Install (wifiApNode);

  for ( int i = 0; i < nWifi; i++ ) {
    wifiStaNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(1, 1, 1));
  }

  InternetStackHelper stack;
  stack.Install(wifiApNode);
  stack.Install(wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiApInterface;
  Ipv4InterfaceContainer wifiStaInterfaces;
  wifiApInterface = address.Assign (apDevices);
  wifiStaInterfaces = address.Assign (staDevices);

  uint16_t port = 20803;
		
  PacketSinkHelper udpsink ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  ApplicationContainer udpapp = udpsink.Install (wifiApNode.Get (0));
  udpapp.Start (Seconds (0.0));
  udpapp.Stop (Seconds (7.0));
  Ptr<Socket> udpsocket[5];
  Ptr<MyApp> udpflow[5] = CreateObject<MyApp> ();

  for ( int j = 0; j < nWifi ; j++ ) {
    udpsocket[j] = Socket::CreateSocket (wifiStaNodes.Get (j), UdpSocketFactory::GetTypeId ());
    //NS_LOG_UNCOND("wifiStaNodes " << j << " : " << wifiStaNodes.Get(j));
    udpflow[j] ->Setup (udpsocket[j], Address (InetSocketAddress (wifiApInterface.GetAddress (0), port)), pktSize, 500000, DataRate ("50Mbps"));
    wifiStaNodes.Get (j)->AddApplication (udpflow[j]);    
    udpflow[j]->SetStartTime (Seconds (1.0));
    udpflow[j]->SetStopTime (Seconds (6.0));
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
		
  udpapp.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback(&RxCnt));
  Simulator::Stop(Seconds (7.0));
  phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.EnablePcap("Network_Project", apDevices.Get (0));
  Simulator::Run();
  Simulator::Destroy ();
		
  NS_LOG_UNCOND("Number of STAs= " << nWifi << ", PacketSize= "<< pktSize << ", RtsCtsThreshold= "<< rtsCtsThreshold << "   =>  Throughput= "<< (double)data*8/1000/1000/5 <<"Mbps");

  Simulator::Run();
  Simulator::Destroy();
};
