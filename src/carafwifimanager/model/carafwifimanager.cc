/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "carafwifimanager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)

NS_LOG_COMPONENT_DEFINE("ns3::CarafWifiManager");

namespace ns3 {
  const uint32_t MIN_FRAGMENTATION_THRESHOLD = 256;
  const uint32_t MAX_FRAGMENTATION_THRESHOLD = 2048;

  struct CarafWifiRemoteStation : public WifiRemoteStation {
    bool rtsCtsOn;
    bool init;
    uint32_t fragmentationThreshold;

    uint32_t m_rate;

    bool returnTrial;
    uint32_t returnTrialLimit;
    uint32_t rtsCount;
  };

  NS_OBJECT_ENSURE_REGISTERED (CarafWifiManager);

  TypeId
  CarafWifiManager::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::CarafWifiManager")
      .SetParent<WifiRemoteStationManager> ()
      .AddConstructor<CarafWifiManager> ()
      .AddAttribute ("TimerThreshold", "The 'timer' threshold in the ARF algorithm.",
                     UintegerValue (15),
                     MakeUintegerAccessor (&CarafWifiManager::m_timerThreshold),
                     MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("SuccessThreshold",
                     "The minimum number of sucessfull transmissions to try a new rate.",
                     UintegerValue (10),
                     MakeUintegerAccessor (&CarafWifiManager::m_successThreshold),
                     MakeUintegerChecker<uint32_t> ())
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
    NS_LOG_FUNCTION (this << st);
    CarafWifiRemoteStation *station = (CarafWifiRemoteStation *)st;
    NS_LOG_UNCOND("Enter DoReportDataFailed");

    if ( station->returnTrial ) {
      station->returnTrialLimit *= 2;
      station->rtsCount = 0;
      SetRtsCtsThreshold(0); // rts/cts 다시 사용
    } else { // fragmentation에서 fail인 경우
      station->fragmentationThreshold = station->fragmentationThreshold / 2;

      if ( station->fragmentationThreshold < MIN_FRAGMENTATION_THRESHOLD ) {
        station->fragmentationThreshold = MIN_FRAGMENTATION_THRESHOLD;
      }

      if ( station->fragmentationThreshold == MIN_FRAGMENTATION_THRESHOLD ) {
        station->rtsCtsOn = true;
        NS_LOG_UNCOND(this << " rtsCtsThreshold" << MIN_FRAGMENTATION_THRESHOLD);
        SetRtsCtsThreshold(MIN_FRAGMENTATION_THRESHOLD);
      } else {
      NS_LOG_UNCOND(this << " fragmentationThreshold : " << station->fragmentationThreshold);
        SetFragmentationThreshold(station->fragmentationThreshold);
      }
    }

    NS_LOG_UNCOND("Leave DoReportDataFailed");
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
    NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr);
    CarafWifiRemoteStation *station = (CarafWifiRemoteStation *)st;
    NS_LOG_DEBUG ("station=" << station << " inc rate");
    NS_LOG_UNCOND("Enter DoReportDataOk");
    if ( !station->init ) {
      // station->m_rate = station->m_state->m_operationalRateSet.size() - 1;
      station->m_rate = GetNSupported(station) - 1;
      station->init = true;
    }

    if ( station->rtsCtsOn ) {
      station->rtsCount++;
      if ( station->rtsCount > station->returnTrialLimit ) {
        station->fragmentationThreshold = MIN_FRAGMENTATION_THRESHOLD;
        SetFragmentationThreshold(station->fragmentationThreshold);
        SetRtsCtsThreshold(MAX_FRAGMENTATION_THRESHOLD);
        station->returnTrial = true;
      }
    } else if ( station->returnTrial ) {
      station->rtsCount = 0;
      station->returnTrialLimit = 1;
      station->returnTrial = false;
      station->rtsCtsOn = false;
    } else {
      station->fragmentationThreshold *= 2;
      if ( station->fragmentationThreshold > MAX_FRAGMENTATION_THRESHOLD ) {
        station->fragmentationThreshold = MAX_FRAGMENTATION_THRESHOLD;
      }
      NS_LOG_UNCOND(this << " DoReportDataOk -> fragThreshold : " << station->fragmentationThreshold);
      SetFragmentationThreshold(station->fragmentationThreshold);
    }
    NS_LOG_UNCOND("Leave DoReportDataOk");
  }

  void CarafWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                         double ackSnr, WifiMode ackMode, double dataSnr)
  {
    for ( int i = 0; i < 50; i++ ) {
      NS_LOG_UNCOND(i << " DoReportDataOk");
      SetFragmentationThreshold(MAX_FRAGMENTATION_THRESHOLD);
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
   
    NS_LOG_UNCOND("Enter DoGetDataTxVector");
    NS_LOG_UNCOND(this << " Supported : " << GetSupported(station, station->m_rate));

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
    NS_LOG_UNCOND(this << " DoGetRtsTxVector");
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
}
