#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace ns3;
using namespace std;

uint32_t data=0;

static void
RxCnt (Ptr<const Packet> p, const Address &a)
{
		data = data + p->GetSize();
		NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\ttotal: " << data << "\tpacket size: " << p->GetSize());
}

class RandomGenerator : public Application
{
	public:
		RandomGenerator ();
		virtual ~RandomGenerator();
		void SetDelay (RandomVariable delay);
		void SetSize (RandomVariable size);
		void SetRemote (std::string socketType, 
						Address remote);
		void SetRemote (Ptr<Socket> socket, Address remote);
	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);
		void DoGenerate (void);

		RandomVariable m_delay;
		RandomVariable m_size;
		Ptr<Socket> m_socket;
		EventId m_next;
};

RandomGenerator::RandomGenerator ()
		: m_socket(0), m_next()
{
	UniformVariable uniform(0.0,2000.0);
	ExponentialVariable expo(0.0001);
	SetDelay(expo);
	SetSize(uniform);
}

RandomGenerator::~RandomGenerator()
{
	m_socket = 0;
}

void
RandomGenerator::SetDelay (RandomVariable delay)
{
	m_delay = delay;
}

void
RandomGenerator::SetSize (RandomVariable size)
{
	m_size = size;
}

void 
RandomGenerator::SetRemote (std::string socketType, 
				Address remote)
{
	TypeId tid = TypeId::LookupByName (socketType);
	m_socket = Socket::CreateSocket (GetNode (), tid);
	m_socket->Bind ();
	m_socket->ShutdownRecv ();
	m_socket->Connect (remote);
}

void
RandomGenerator::SetRemote (Ptr<Socket> socket, Address remote)
{
	m_socket = socket;
	m_socket->Bind();
	m_socket->ShutdownRecv();
	m_socket->Connect(remote);
}

void
RandomGenerator::DoGenerate (void)
{
	m_next = Simulator::Schedule (Seconds (m_delay.GetValue ()), 
					&RandomGenerator::DoGenerate, this);
	Ptr<Packet> p = Create<Packet> ((m_size.GetInteger ()));
	m_socket->Send (p);
}

void
RandomGenerator::StartApplication (void)
{
	DoGenerate();
}

void
RandomGenerator::StopApplication (void)
{
	if(m_socket)
		m_socket->Close();
}

int main (int argc, char *argv[])
{

		int nSta = 1;
		int ra_indx = 0;
		int seed = 1;
		int thre = 2000;
		CommandLine cmd;
		cmd.AddValue("nSTA", "number of stations", nSta);
		cmd.AddValue("RA", "rate adaptation algorithm index", ra_indx);
		cmd.AddValue("Seed", "seed", seed);
		cmd.AddValue("thre", "threshold", thre);
		cmd.Parse (argc, argv);

		//Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
		  // turn off RTS/CTS for frames below 2200 bytes
		  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",  StringValue("500"));
		  
		NodeContainer ApNode;
		ApNode.Create (1);

		NodeContainer StaNodes;
		StaNodes.Create (nSta);
		NodeContainer c;
		c.Add (ApNode);
		c.Add (StaNodes);



		WifiHelper wifi;
		wifi.SetStandard (WIFI_PHY_STANDARD_80211b);


		YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
		//wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);


		YansWifiChannelHelper wifiChannel ;
		wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
		wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
		wifiPhy.SetChannel (wifiChannel.Create ());

		SeedManager::SetRun(seed);
		
		std::string rateControl("ns3::ArfWifiManager");
		
		switch(ra_indx){
				case 0:
						rateControl = "ns3::ArfWifiManager";
						break;
				case 1:
						rateControl = "ns3::AarfWifiManager";
						break;
				case 2:
						rateControl = "ns3::CaraWifiManager";
						break;
				case 3:
						rateControl = "ns3::IdealWifiManager";
						break;
				default:
						NS_LOG_UNCOND("index fail");
		}
		//


		NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
		//wifi.SetRemoteStationManager (rateControl);

		wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
						"DataMode",StringValue ("DsssRate11Mbps"),
						"ControlMode",StringValue("DsssRate1Mbps"));

		Ssid ssid = Ssid ("wifi-default");

		wifiMac.SetType ("ns3::StaWifiMac",
						"Ssid", SsidValue (ssid),
						"ActiveProbing", BooleanValue (false));

		NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, StaNodes);

		wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
		NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ApNode.Get(0));


		NetDeviceContainer devices;
		devices.Add (apDevice);
		devices.Add (staDevice);

		MobilityHelper mobility;

		Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
		//positionAlloc->Add (Vector (0.0, 0.0, 0.0));
		positionAlloc->Add (Vector (15.0, 15.0, 0.0));
		positionAlloc->Add (Vector (30.0, 0.0, 0.0));
		positionAlloc->Add (Vector (0.0, 30.0, 0.0));
		positionAlloc->Add (Vector (-30.0, 0.0, 0.0));
		positionAlloc->Add (Vector (0.0, -30.0, 0.0));
		positionAlloc->Add (Vector (0.0, 0.0, 30.0));

		mobility.SetPositionAllocator (positionAlloc);
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (c);


		InternetStackHelper internet;
		internet.Install (c);
		Ipv4AddressHelper ipv4;


		ipv4.SetBase ("10.1.1.0", "255.255.255.0");
		Ipv4InterfaceContainer addresses = ipv4.Assign (devices);


		uint16_t port = 9;

		PacketSinkHelper sink ("ns3::UdpSocketFactory",
						Address (InetSocketAddress (Ipv4Address::GetAny (), port)));

		ApplicationContainer app;

		app = sink.Install (ApNode.Get (0));
		app.Start (Seconds (0.0));
		app.Get(0)->TraceConnectWithoutContext ("Rx", MakeCallback(&RxCnt));


		/*
		   OnOffHelper onoff ("ns3::UdpSocketFactory",
		   Address (InetSocketAddress (Ipv4Address ("10.1.1.1"), port)));

		//onoff.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1)));
		//onoff.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
		onoff.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]")); 
		onoff.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
		onoff.SetAttribute ("DataRate", StringValue ("25Mbps"));
		onoff.SetAttribute ("PacketSize", UintegerValue (1500));
		 */
		/*
		   for(int i=0; i<nSta; i++)
		   {
		   app = onoff.Install (StaNodes.Get (i));
		   app.Start (Seconds (1.0));
		   app.Stop (Seconds (10.0));
		   }
		 */

		Ptr<RandomGenerator> randomApp = CreateObject<RandomGenerator>();
		Address apAddress(InetSocketAddress(addresses.GetAddress(0), port));
		for(int i=0; i<nSta; i++)
		{
				Ptr<Socket> socket = Socket::CreateSocket(StaNodes.Get(i), UdpSocketFactory::GetTypeId());
				randomApp->SetRemote(socket, apAddress);
				StaNodes.Get(i)->AddApplication (randomApp);
				randomApp->SetStartTime(Seconds(1.0));
				randomApp->SetStopTime(Seconds(10.0));
		}
		//	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
		//	wifiPhy.EnablePcap ("session6_ex1", devices);


		AnimationInterface anim("randomPacket.xml");

		Simulator::Stop (Seconds (10.0));
		Simulator::Run ();
		Simulator::Destroy ();
		//

		/*
		   ofstream fout;
		   ostringstream out_filename;
		   out_filename << "RaResult/thr_ra"<<ra_indx<<"_nsta"<<nSta<<"_seed"<<seed<<".txt";
		   fout.open(out_filename.str().c_str(), ostream::out);
		   if(!fout.good()){
		   NS_LOG_UNCOND("File open failed");
		   }

		   fout << (double)data*8/1000/1000/9;
		   fout.close();
		//
		 */
		NS_LOG_UNCOND(  rateControl << " >> Number of STAs= " << nSta<< " Aggregated Throughput: "<< (double)data*8/1000/1000/9 << " Mbps");
		return 0;
}

