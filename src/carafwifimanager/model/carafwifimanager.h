/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef CARAFWIFIMANAGER_H
#define CARAFWIFIMANAGER_H

#define FRAG_MIN 256
#define FRAG_MAX 2048
#define RTS_ON 0
#define RTS_OFF 8000
#define FREQ_SAMPLE 1
#define CONTROL_RATE 1

#include "ns3/wifi-remote-station-manager.h"

namespace ns3 {

  struct CarafWifiRemoteStation : public WifiRemoteStation
  {
    uint32_t m_failed;
    uint32_t m_retry;

    uint32_t m_successThreshold;

    uint32_t m_fragThreshold;
    uint32_t m_rtsThreshold;

    uint32_t m_rate;

    bool m_succeedArray[FREQ_SAMPLE];
    uint32_t m_inputIndex;
    uint32_t m_succeedCount;

    uint32_t sampleCount;

    uint32_t m_delayedSet;
  };

  uint32_t getSucceedCount(bool array[]);

  void logIsSucceed(bool array[], uint32_t* inputIndex, bool isSucceed);

  void controlThreshold(bool array[], uint32_t* frag, uint32_t* rts);

  class CarafWifiManager : public WifiRemoteStationManager
  {
  public:
    static TypeId GetTypeId (void);
    CarafWifiManager ();
    virtual ~CarafWifiManager ();

  private:
    // overriden from base class
    virtual WifiRemoteStation * DoCreateStation (void) const;
    virtual void DoReportRxOk (WifiRemoteStation *station,
                               double rxSnr, WifiMode txMode);
    virtual void DoReportRtsFailed (WifiRemoteStation *station);
    virtual void DoReportDataFailed (WifiRemoteStation *station);
    virtual void DoReportRtsOk (WifiRemoteStation *station,
                                double ctsSnr, WifiMode ctsMode, double rtsSnr);
    virtual void DoReportDataOk (WifiRemoteStation *station,
                                 double ackSnr, WifiMode ackMode, double dataSnr);
    virtual void DoReportFinalRtsFailed (WifiRemoteStation *station);
    virtual void DoReportFinalDataFailed (WifiRemoteStation *station);
    virtual WifiTxVector DoGetDataTxVector(WifiRemoteStation *station, uint32_t size);
    virtual WifiTxVector DoGetRtsTxVector(WifiRemoteStation *station);
    virtual bool IsLowLatency (void) const;
    virtual bool IsLastFragment(Mac48Address address, const WifiMacHeader *header,
                       Ptr<const Packet> packet, uint32_t fragmentNumber);

    uint32_t m_timerThreshold;
    uint32_t m_successThreshold;
  };
}

#endif /* CARAFWIFIMANAGER_H */