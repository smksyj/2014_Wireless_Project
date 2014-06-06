/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "carafwifimanager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)
#define FRAG_MAX 256
#define FRAG_MIN 2000

NS_LOG_COMPONENT_DEFINE("ns3::CarafWifiManager");

namespace ns3 {
  struct ArfWifiRemoteStation : public WifiRemoteStation
  {
    uint32_t m_timer;
    uint32_t m_success;
    uint32_t m_failed;
    bool m_recovery;
    uint32_t m_retry;

    uint32_t m_timerTimeout;
    uint32_t m_successThreshold;

    uint32_t m_fragThreshold;

    uint32_t m_rate;
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
    ArfWifiRemoteStation *station = new ArfWifiRemoteStation ();

    station->m_successThreshold = m_successThreshold;
    station->m_timerTimeout = m_timerThreshold;
    station->m_rate = 0;
    station->m_success = 0;
    station->m_failed = 0;
    station->m_recovery = false;
    station->m_retry = 0;
    station->m_timer = 0;
    station->m_fragThreshold = FRAG_MIN;

    return station;
  }

  void
  CarafWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
  {
    NS_LOG_FUNCTION (this << station);
  }
  /**
   * It is important to realize that "recovery" mode starts after failure of
   * the first transmission after a rate increase and ends at the first successful
   * transmission. Specifically, recovery mode transcends retransmissions boundaries.
   * Fundamentally, ARF handles each data transmission independently, whether it
   * is the initial transmission of a packet or the retransmission of a packet.
   * The fundamental reason for this is that there is a backoff between each data
   * transmission, be it an initial transmission or a retransmission.
   */
  void
  CarafWifiManager::DoReportDataFailed (WifiRemoteStation *st)
  {
    NS_LOG_FUNCTION (this << st);
    ArfWifiRemoteStation *station = (ArfWifiRemoteStation *)st;
    station->m_timer++;
    station->m_failed++;
    station->m_retry++;
    station->m_success = 0;

    // fragThreshold increase
    station->m_fragThreshold -= 2;
    // NS_LOG_UNCOND(this << " : Decreased " << station->m_fragThreshold);
    SetFragmentationThreshold (station->m_fragThreshold);

    if (station->m_recovery)
      {
        NS_ASSERT (station->m_retry >= 1);
        if (station->m_retry == 1)
          {
            // need recovery fallback
            if (station->m_rate != 0)
              {
               // station->m_rate--;
              }
          }
        station->m_timer = 0;
      }
    else
      {
        NS_ASSERT (station->m_retry >= 1);
        if (((station->m_retry - 1) % 2) == 1)
          {
            // need normal fallback
            if (station->m_rate != 0)
              {
               // station->m_rate--;
              }
          }
        if (station->m_retry >= 2)
          {
            station->m_timer = 0;
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
    NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr);
    ArfWifiRemoteStation *station = (ArfWifiRemoteStation *) st;
    station->m_timer++;
    station->m_success++;
    station->m_failed = 0;
    station->m_recovery = false;
    station->m_retry = 0;

    // fragThreshold decrease
    station->m_fragThreshold += 2;
    // NS_LOG_UNCOND(this << " : Increased " << station->m_fragThreshold);
    SetFragmentationThreshold (station->m_fragThreshold);

    NS_LOG_DEBUG ("station=" << station << " data ok success=" << station->m_success << ", timer=" << station->m_timer);
    if ((station->m_success == m_successThreshold
         || station->m_timer == m_timerThreshold)
        && (station->m_rate < (station->m_state->m_operationalRateSet.size () - 1)))
      {
        NS_LOG_DEBUG ("station=" << station << " inc rate");
        // station->m_rate++;
        NS_LOG_UNCOND (station->m_rate++);
        station->m_timer = 0;
        station->m_success = 0;
        station->m_recovery = true;
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

  // WifiMode
  // CarafWifiManager::DoGetDataMode (WifiRemoteStation *st, uint32_t size)
  // {
  //   NS_LOG_FUNCTION (this << st << size);
  //   ArfWifiRemoteStation *station = (ArfWifiRemoteStation *) st;
  //   return GetSupported (station, station->m_rate);
  // }
  // WifiMode
  // CarafWifiManager::DoGetRtsMode (WifiRemoteStation *st)
  // {
  //   NS_LOG_FUNCTION (this << st);
  //   // XXX: we could/should implement the Arf algorithm for
  //   // RTS only by picking a single rate within the BasicRateSet.
  //   ArfWifiRemoteStation *station = (ArfWifiRemoteStation *) st;
  //   return GetSupported (station, 0);
  // }
  WifiTxVector
  CarafWifiManager::DoGetDataTxVector (WifiRemoteStation *st, uint32_t size)
  {
    NS_LOG_FUNCTION (this << st << size);
    ArfWifiRemoteStation *station = (ArfWifiRemoteStation *) st;
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
    /// \todo we could/should implement the Arf algorithm for
    /// RTS only by picking a single rate within the BasicRateSet.
    ArfWifiRemoteStation *station = (ArfWifiRemoteStation *) st;
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

