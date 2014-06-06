#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

#include "ns3/flow-monitor-module.h"
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
//		NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\ttotal: " << data << "\tpacket size: " << p->GetSize());
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
	ExponentialVariable expo(0.001);
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
		int rtsThre = 2000;
		int fragThre = 2000;
		CommandLine cmd;
		cmd.AddValue("nSTA", "number of stations", nSta);
		cmd.AddValue("RA", "rate adaptation algorithm index", ra_indx);
		cmd.AddValue("Seed", "seed", seed);
		cmd.AddValue("RtsThre", "RTS/CTS threshold", rtsThre);
		cmd.AddValue("FragThre", "Fragmentation threshold", fragThre);
		cmd.Parse (argc, argv);

		Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", UintegerValue(fragThre));
		  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",  UintegerValue(rtsThre));
		  
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
		wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","ReferenceLoss", DoubleValue(0.0));
		//wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
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
		wifi.SetRemoteStationManager (rateControl);

		/*wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
						"DataMode",StringValue ("DsssRate11Mbps"),
						"ControlMode",StringValue("DsssRate1Mbps"));
		*/			

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

		Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator>();
		positionAlloc->SetRho(70);
		positionAlloc->SetX(0);
		positionAlloc->SetY(0);
		mobility.SetPositionAllocator (positionAlloc);

		/*
		mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
										"X", StringValue("0.0"),
										"Y", StringValue("0.0"),
										"Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=50]"));
										*/

		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (StaNodes);
		
		Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator> ();
		apPositionAlloc->Add(Vector(0.0, 0.0, 0.0));
		mobility.SetPositionAllocator(apPositionAlloc);
		mobility.Install (ApNode);

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



		//Ptr<RandomGenerator> randomApp = CreateObject<RandomGenerator>();
		Address apAddress(InetSocketAddress(addresses.GetAddress(0), port));
		Ptr<Socket> socket[50];
		Ptr<RandomGenerator> randomApp[50] = CreateObject<RandomGenerator>();;
		for(int i=0; i<nSta; i++)
		{
				socket[i] = Socket::CreateSocket(StaNodes.Get(i), UdpSocketFactory::GetTypeId());
				randomApp[i]->SetRemote(socket[i], apAddress);
				StaNodes.Get(i)->AddApplication (randomApp[i]);
				randomApp[i]->SetStartTime(Seconds(1.0));
				randomApp[i]->SetStopTime(Seconds(10.0));
		}
		//	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
		//	wifiPhy.EnablePcap ("session6_ex1", devices);


		AnimationInterface anim("randomPacket.xml");

		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();

		Simulator::Stop (Seconds (10.0));
		Simulator::Run ();

		monitor->CheckForLostPackets();
		Ptr<Ipv4FlowClassifier> classifier =  DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

		ostringstream txrx_info;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
			std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
			std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
			std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
			std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
			std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
			txrx_info <<i->second.txPackets<<"\t"<<i->second.txBytes<<"\t"<<i->second.rxPackets<<"\t"<<i->second.rxBytes<<"\n";
	}


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
		ofstream fout;
		ostringstream out_filename;
		out_filename << "simulationResult_rts/nsta"<<nSta<<"_rts"<<rtsThre<<"_seed"<<seed<<".txt";
		//out_filename << "simulationResult_frag/nsta"<<nSta<<"_frag"<<fragThre<<"_seed"<<seed<<".txt";
		fout.open(out_filename.str().c_str(), ostream::out);
		if(!fout.good()){
				NS_LOG_UNCOND("File open failed");
		}

		/**txrx_info
		  * first col :  number of transmitted packets
		  * second col : total bytes of transmitted packets
		  * third col : number of received packets
		  * forth col : total bytes of received packets
		  */

		fout<<txrx_info.str();
		NS_LOG_UNCOND(  "Number of STAs= " << nSta<< " Aggregated Throughput: "<< (double)data*8/1000/1000/9 << " Mbps");
		return 0;
}

