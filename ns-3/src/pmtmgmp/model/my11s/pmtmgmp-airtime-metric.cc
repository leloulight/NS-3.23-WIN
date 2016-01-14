/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "pmtmgmp-airtime-metric.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-tx-vector.h"

namespace ns3 {
namespace my11s {
NS_OBJECT_ENSURE_REGISTERED (PmtmgmpAirtimeLinkMetricCalculator);
  
TypeId
PmtmgmpAirtimeLinkMetricCalculator::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::my11s::PmtmgmpAirtimeLinkMetricCalculator")
    .SetParent<Object> ()
    .SetGroupName ("Wmn")
    .AddConstructor<PmtmgmpAirtimeLinkMetricCalculator> ()
    .AddAttribute ( "TestLength",
                    "Rate should be estimated using test length.",
                    UintegerValue (1024),
                    MakeUintegerAccessor (
                      &PmtmgmpAirtimeLinkMetricCalculator::SetTestLength),
                    MakeUintegerChecker<uint16_t> (1)
                    )
    .AddAttribute ( "My11MetricTid",
                    "TID used to calculate metric (data rate)",
                    UintegerValue (0),
                    MakeUintegerAccessor (
                      &PmtmgmpAirtimeLinkMetricCalculator::SetHeaderTid),
                    MakeUintegerChecker<uint8_t> (0)
                    )
  ;
  return tid;
}
PmtmgmpAirtimeLinkMetricCalculator::PmtmgmpAirtimeLinkMetricCalculator ()
{
}
void
PmtmgmpAirtimeLinkMetricCalculator::SetHeaderTid (uint8_t tid)
{
  m_testHeader.SetDsFrom ();
  m_testHeader.SetDsTo ();
  m_testHeader.SetTypeData ();
  m_testHeader.SetQosTid (tid);
}
void
PmtmgmpAirtimeLinkMetricCalculator::SetTestLength (uint16_t testLength)
{
  m_testFrame = Create<Packet> (testLength + 6 /*Wmn header*/ + 36 /*802.11 header*/);
}
uint32_t
PmtmgmpAirtimeLinkMetricCalculator::CalculateMetric (Mac48Address peerAddress, Ptr<WmnWifiInterfaceMac> mac)
{
  /* Airtime link metric is defined in 11B.10 of 802.11s Draft D3.0 as:
   *
   * airtime = (O + Bt/r) /  (1 - frame error rate), where
   * o  -- the PHY dependent channel access which includes frame headers, training sequences,
   *       access protocol frames, etc.
   * bt -- the test packet length in bits (8192 by default),
   * r  -- the current bitrate of the packet,
   *
   * Final result is expressed in units of 0.01 Time Unit = 10.24 us (as required by 802.11s draft)
   */
  NS_ASSERT (!peerAddress.IsGroup ());
  //obtain current rate:
  WifiMode mode = mac->GetWifiRemoteStationManager ()->GetDataTxVector (peerAddress, &m_testHeader, m_testFrame, m_testFrame->GetSize ()).GetMode();
  //obtain frame error rate:
  double failAvg = mac->GetWifiRemoteStationManager ()->GetInfo (peerAddress).GetFrameErrorRate ();
  if (failAvg == 1)
    {
      // Retrun max metric value when frame error rate equals to 1
      return (uint32_t)0xffffffff;
    }
  NS_ASSERT (failAvg < 1.0);
  WifiTxVector txVector;
  txVector.SetMode (mode);
  //calculate metric
  uint32_t metric = (uint32_t)((double)( /*Overhead + payload*/
                                 mac->GetPifs () + mac->GetSlot () + mac->GetEifsNoDifs () + //DIFS + SIFS + AckTxTime = PIFS + SLOT + EifsNoDifs
                                 mac->GetWifiPhy ()->CalculateTxDuration (m_testFrame->GetSize (), txVector, WIFI_PREAMBLE_LONG, mac->GetWifiPhy ()->GetFrequency(), 0, 0)
                                 ).GetMicroSeconds () / (10.24 * (1.0 - failAvg)));
  return metric;
}
} // namespace my11s
} // namespace ns3
