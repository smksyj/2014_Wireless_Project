/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "carafwifimanager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)
#define FRAG_MIN 500
#define FRAG_MAX 1000
#define RTS_ON 0
#define RTS_OFF 8000
#define FREQ_SAMPLE 10
#define CONTROL_RATE 9

#define DELAY_FRAG_INC 1
#define DELAY_FRAG_DEC 2
#define DELAY_RTS_ON 3
#define DELAY_RTS_OFF 4

NS_LOG_COMPONENT_DEFINE("ns3::CarafWifiManager");

namespace ns3 {
   NS_OBJECT_ENSURE_REGISTERED (CarafWifiManager);

  TypeId
  CarafWifiManager::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::CarafWifiManager")
      .SetParent<WifiRemoteStationManager> ()
      .AddConstructor<CarafWifiManager> ()
      /*.AddAttribute ("TimerThreshold", "The 'timer' threshold in the CARAF algorithm.",
                     UintegerValue (15),
                     MakeUintegerAccessor (&CarafWifiManager::m_timerThreshold),
                     MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("SuccessThreshold",
                     "The minimum number of sucessfull transmissions to try a new rate.",
                     UintegerValue (10),
                     MakeUintegerAccessor (&CarafWifiManager::m_successThreshold),
                     MakeUintegerChecker<uint32_t> ())*/
      ;
    return tid;
  }

  CarafWifiManager::CarafWifiManager ()
  {
    NS_LOG_FUNCTION (this);
  }
  CarafWifiManager::~CarafWifiManager ()
  {
    NS_LOG_FUNCTION (this);
  }

  WifiRemoteStation *
  CarafWifiManager::DoCreateStation (void) const
  {
    NS_LOG_FUNCTION (this);
    CarafWifiRemoteStation *station = new CarafWifiRemoteStation();

    station->m_rate = 0;
    station->fragmentationThreshold = MAX_FRAGMENTATION_THRESHOLD;
    station->init = false;
    station->rtsCtsOn = false;

    station->returnTrial = false;
    station->returnTrialLimit = 1;
    station->rtsCount = 0;

    return station;
  }

  void
  CarafWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
  {
    NS_LOG_FUNCTION (this << station);
  }

  void
  CarafWifiManager::DoReportDataFailed (WifiRemoteStation *st)
  {
    CarafWifiRemoteStation *station = (CarafWifiRemoteStation *)st;
    station->m_failed++;
    station->m_retry++;

    (station->sampleCount)++;

    if (station->sampleCount == FREQ_SAMPLE) { // Enough samples collected
      station->sampleCount = 0;

      NS_LOG_UNCOND (station << " : FAILED");

      // Log result
      station->m_succeedArray[station->m_inputIndex++] = false;

      // Set inputIndex to 0 if it's 10
      if (station->m_inputIndex == 10) {
       station->m_inputIndex = 0;
      }

      // Count succeesrate
      int tempCount = 0;
      for (int i=0; i<10; i++) {
       if (station->m_succeedArray[i])
        tempCount++;
      }

      // Control fragThreshold
      if (tempCount < CONTROL_RATE && (GetFragmentationThreshold() > FRAG_MIN)) { // Decrease m_fragThreshold
       station->m_delayedSet = DELAY_FRAG_DEC;

      } else if (tempCount < CONTROL_RATE && (GetFragmentationThreshold() <= FRAG_MIN)) { // RTS ON
       station->m_delayedSet = DELAY_RTS_ON;

      } else if (tempCount >= CONTROL_RATE && GetRtsCtsThreshold() == RTS_ON) { // RTS OFF
       station->m_delayedSet = DELAY_RTS_OFF;

      } else if (tempCount >= CONTROL_RATE && GetRtsCtsThreshold() == RTS_OFF && GetFragmentationThreshold() < FRAG_MAX) { // Increase m_fragThreshold
       station->m_delayedSet = DELAY_FRAG_INC;

      } else if (tempCount >= CONTROL_RATE && GetRtsCtsThreshold() == RTS_OFF && GetFragmentationThreshold() >= FRAG_MAX) { // Keep m_fragThreshold

      } else { // Unexpected situation, ERROR
       NS_LOG_UNCOND("Unexpected case : Thresholds control");
       NS_LOG_UNCOND("RTS : " << GetRtsCtsThreshold() << ", FRAG : " << GetFragmentationThreshold());
       exit(0);
      }

      //NS_LOG_UNCOND(station->m_delayedSet);

      // Reset m_succeedArray
      if (station->m_delayedSet != 0) {
       for (int i=0; i<10; i++)
        station->m_succeedArray[i] = true;
      }
    }
  }

  void
  CarafWifiManager::DoReportRxOk (WifiRemoteStation *station,
                                  double rxSnr, WifiMode txMode)
  {
    NS_LOG_FUNCTION (this << station << rxSnr << txMode);
  }

  void CarafWifiManager::DoReportRtsOk (WifiRemoteStation *station,
                                        double ctsSnr, WifiMode ctsMode, double rtsSnr)
  {
    NS_LOG_FUNCTION (this << station << ctsSnr << ctsMode << rtsSnr);
    NS_LOG_DEBUG ("station=" << station << " rts ok");
  }

  void CarafWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                         double ackSnr, WifiMode ackMode, double dataSnr)
  {
  
    //NS_LOG_UNCOND (this << " : DATAOK");
    ArfWifiRemoteStation *station = (ArfWifiRemoteStation *) st;
    station->m_failed = 0;
    station->m_retry = 0;

    // Log Succeed
    (station->sampleCount)++;
    if (station->sampleCount == FREQ_SAMPLE) { // Enough samples collected
      station->sampleCount = 0;
      NS_LOG_UNCOND (this << " : OK");

      // Log result
      station->m_succeedArray[station->m_inputIndex++] = true;

      // Set inputIndex to 0 if it's 10
      if (station->m_inputIndex == 10) {
       station->m_inputIndex = 0;
      }

      // Count succeesrate
      int tempCount = 0;
      for (int i=0; i<10; i++) {
       if (station->m_succeedArray[i])
        tempCount++;
      }

      // Control fragThreshold
      if (tempCount < CONTROL_RATE && (GetFragmentationThreshold() > FRAG_MIN)) { // Decrease m_fragThreshold
       station->m_delayedSet = DELAY_FRAG_DEC;

      } else if (tempCount < CONTROL_RATE && (GetFragmentationThreshold() <= FRAG_MIN)) { // RTS ON
       station->m_delayedSet = DELAY_RTS_ON;

      } else if (tempCount >= CONTROL_RATE && GetRtsCtsThreshold() == RTS_ON) { // RTS OFF
       station->m_delayedSet = DELAY_RTS_OFF;

      } else if (tempCount >= CONTROL_RATE && GetRtsCtsThreshold() == RTS_OFF && GetFragmentationThreshold() < FRAG_MAX) { // Increase m_fragThreshold
       station->m_delayedSet = DELAY_FRAG_INC;

      } else if (tempCount >= CONTROL_RATE && GetRtsCtsThreshold() == RTS_OFF && GetFragmentationThreshold() >= FRAG_MAX) { // Keep m_fragThreshold

      } else { // Unexpected situation, ERROR
       NS_LOG_UNCOND("Unexpected case : Thresholds control");
       NS_LOG_UNCOND("RTS : " << GetRtsCtsThreshold() << ", FRAG : " << GetFragmentationThreshold());
       exit(0);
      }

      //NS_LOG_UNCOND(station->m_delayedSet);

      // Reset m_succeedArray
      if (station->m_delayedSet != 0) {
       for (int i=0; i<10; i++)
        station->m_succeedArray[i] = true;
      }
    }
  }

  void
  CarafWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
  {
    NS_LOG_FUNCTION (this << station);
  }

  void
  CarafWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
  {
    NS_LOG_FUNCTION (this << station);
  }

  WifiTxVector
  CarafWifiManager::DoGetDataTxVector (WifiRemoteStation *st, uint32_t size)
  {
    NS_LOG_FUNCTION (this << st << size);
    CarafWifiRemoteStation *station = (CarafWifiRemoteStation *) st;
   
    //NS_LOG_UNCOND("Enter DoGetDataTxVector");
   // NS_LOG_UNCOND(this << " Supported : " << GetSupported(station, station->m_rate));

    return WifiTxVector (GetSupported (station, station->m_rate),
                         GetDefaultTxPowerLevel (),
                         GetLongRetryCount (station),
                         GetShortGuardInterval (station),
                         Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()),
                         GetNumberOfTransmitAntennas (station),
                         GetStbc (station));
  }

  WifiTxVector
  CarafWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
  {
    NS_LOG_FUNCTION (this << st);
    CarafWifiRemoteStation *station = (CarafWifiRemoteStation *) st;
    //NS_LOG_UNCOND(this << " DoGetRtsTxVector");
    return WifiTxVector (GetSupported (station, 0),
                         GetDefaultTxPowerLevel (),
                         GetLongRetryCount (station),
                         GetShortGuardInterval (station),
                         Min (GetNumberOfReceiveAntennas (station),GetNumberOfTransmitAntennas()),
                         GetNumberOfTransmitAntennas (station),
                         GetStbc (station));
  }

  bool
  CarafWifiManager::IsLowLatency (void) const
  { 
    NS_LOG_FUNCTION (this);
    return true;
  }

  bool 
  CarafWifiManager::IsLastFragment (Mac48Address address, const WifiMacHeader *header,
                       Ptr<const Packet> packet, uint32_t fragmentNumber)
  {
    NS_LOG_UNCOND("FragmmentNumber: " << fragmentNumber);

    //WifiRemoteStation *station = WifiRemoteStationManager::PublicLookup(address, header);

   // NS_LOG_UNCOND(station);

    return WifiRemoteStationManager::IsLastFragment(address, header, packet, fragmentNumber);
  }

    bool
  CarafWifiManager::IsLowLatency (void) const
  {
    NS_LOG_FUNCTION (this);
    return true;
  }

  uint32_t getSucceedCount(bool array[]) {
    uint32_t temp = 0;

    for (int i=0; i<10; i++)
     if (array[i])
      temp++;

    return temp;
  }
}
}
