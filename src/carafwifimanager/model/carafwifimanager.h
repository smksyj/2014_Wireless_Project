/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef CARAFWIFIMANAGER_H
#define CARAFWIFIMANAGER_H

#include "ns3/wifi-remote-station-manager.h"

namespace ns3 {
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
    // virtual WifiMode DoGetDataMode (WifiRemoteStation *station, uint32_t size);
    // virtual WifiMode DoGetRtsMode (WifiRemoteStation *station);
    virtual WifiTxVector DoGetDataTxVector(WifiRemoteStation *station, uint32_t size);
    virtual WifiTxVector DoGetRtsTxVector(WifiRemoteStation *station);
    // virtual bool DoNeedRts(WifiRemoteStation *st, Ptr<const Packet> packet, bool normally);
    virtual bool IsLowLatency (void) const;
    virtual bool IsLastFragment(Mac48Address address, const WifiMacHeader *header,
                       Ptr<const Packet> packet, uint32_t fragmentNumber);

    uint32_t m_timerThreshold;
    uint32_t m_successThreshold;
  };
}

#endif /* CARAFWIFIMANAGER_H */

