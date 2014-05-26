#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
  CommandLine cmd;
  Simulator::Run();
  Simulator::Destroy();

  return 0;
};
