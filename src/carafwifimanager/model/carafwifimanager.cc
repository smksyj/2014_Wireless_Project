/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "carafwifimanager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#include "ns3/core-module.h"

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)

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

    station->m_successThreshold = m_successThreshold;
    station->m_rate = 0;
    station->m_failed = 0;
    station->m_retry = 0;

    station->m_fragThreshold = FRAG_MAX;
    station->m_rtsThreshold = RTS_OFF;

    for (int i=0; i<FREQ_SAMPLE; i++)
     station->m_succeedArray[i] = true;
  
    station->m_inputIndex = 0;
    station->m_succeedCount = FREQ_SAMPLE;
    station->sampleCount = 0;

    station->m_delayedSet = false;

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
      if (station->m_inputIndex == FREQ_SAMPLE) {
       station->m_inputIndex = 0;
      }

      // Count succeesrate
      int tempCount = 0;
      for (int i=0; i<FREQ_SAMPLE; i++) {
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
       for (int i=0; i<FREQ_SAMPLE; i++)
        station->m_succeedArray[i] = true;
      }
    }
  }

  void
  CarafWifiManager::DoReportRxOk (WifiRemoteStation *station,
                                  double rxSnr, WifiMode txMode)
  {
    
  }

  void CarafWifiManager::DoReportRtsOk (WifiRemoteStation *station,
                                        double ctsSnr, WifiMode ctsMode, double rtsSnr)
  {
    NS_LOG_UNCOND (this << " : RTSOK");
    NS_LOG_FUNCTION (this << station << ctsSnr << ctsMode << rtsSnr);
  }

  void CarafWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                         double ackSnr, WifiMode ackMode, double dataSnr)
  {
    CarafWifiRemoteStation *station = (CarafWifiRemoteStation *) st;
    station->m_failed = 0;
    station->m_retry = 0;

    // Log Succeed
    (station->sampleCount)++;
    if (station->sampleCount == FREQ_SAMPLE) { // Enough samples collected
      station->sampleCount = 0;
     
      // Log result
      station->m_succeedArray[station->m_inputIndex++] = true;

      // Set inputIndex to 0 if it's 10
      if (station->m_inputIndex == FREQ_SAMPLE) {
       station->m_inputIndex = 0;
      }

      // Count succeesrate
      int tempCount = 0;
       for (int i=0; i<FREQ_SAMPLE; i++) {
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

      // Reset m_succeedArray
      if (station->m_delayedSet != 0) {
       for (int i=0; i<FREQ_SAMPLE; i++)
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
      return WifiRemoteStationManager::IsLastFragment(address, header, packet, fragmentNumber);
  }

  uint32_t getSucceedCount(bool array[]) {
    uint32_t temp = 0;

    for (int i=0; i<FREQ_SAMPLE; i++)
     if (array[i])
      temp++;

    return temp;
  }
}
