/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008,2009 IITP RAS
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
 *          Aleksey Kovalenko <kovalenko@iitp.ru>
 */


#ifndef WMN_CONFIGURATION_H
#define WMN_CONFIGURATION_H

#include "ns3/wmn-information-element-vector.h"

namespace ns3 {
namespace hu11s {
 
//according to IEEE 802.11 - 2012

//in 7.3.2.98.2 Active Path Selection Protocol Identifier - 802.11s-2011 
enum hu11sPathSelectionProtocol
{
  PROTOCOL_MGMP = 0x01,
};
 
//in 7.3.2.98.3 Active Path Selection Metric Identifier - 802.11s-2011 
enum hu11sPathSelectionMetric
{
  METRIC_AIRTIME = 0x01,
};

// in 7.3.2.98.4 Congestion Control Mode Identifier - 802.11s-2011 
enum hu11sCongestionControlMode
{
  CONGESTION_SIGNALING = 0x01,
  CONGESTION_NULL      = 0x00,
};

// in 7.3.2.98.5 Synchronization Method Identifier - 802.11s-2011 
enum hu11sSynchronizationProtocolIdentifier
{
  SYNC_NEIGHBOUR_OFFSET = 0x01,  //Neighbor offset synchronization method
  SYNC_NULL             = 0x00,  //Reserved
};

// in 7.3.2.98.6 Authentication Protocol Identifier - 802.11s-2011 
enum hu11sAuthenticationProtocol
{
  AUTH_NULL = 0x00,  //No authentication method is required to establish wmn huperings within the MBSS
  AUTH_SAE  = 0x01,  //SAE defined in 8.2a
  AUTH_IEEE = 0x02,  //IEEE 802.1X authentication
};
/**
 * \ingroup hu11s
 * \brief See 7.3.2.86.7 in 802.11s draft 3.0
 */
class Hu11sWmnCapability
{
public:
  Hu11sWmnCapability ();
  uint8_t  GetSerializedSize () const;
  Buffer::Iterator Serialize (Buffer::Iterator i) const;
  Buffer::Iterator Deserialize (Buffer::Iterator i);
  uint8_t GetUint8 () const;  
  bool acceptHuperLinks;
  bool MCCASupported;
  bool MCCAEnabled;
  bool forwarding;
  bool beaconTimingReport;
  bool TBTTAdjustment;
  bool powerSaveLevel;
  bool Is (uint8_t cap,uint8_t n) const;
  friend bool operator== (const Hu11sWmnCapability & a, const Hu11sWmnCapability & b);
};

/**
 * \ingroup hu11s
 * \brief Describes Wmn Configuration Element 
 * see 7.3.2.86 of 802.11s draft 3.0
 */
class IeConfiguration : public WifiInformationElement
{
public:
  IeConfiguration ();
  void SetRouting (hu11sPathSelectionProtocol routingId);
  void SetMetric (hu11sPathSelectionMetric metricId);
  bool IsMGMP ();
  bool IsAirtime ();
  void SetNeighborCount (uint8_t neighbors);
  uint8_t GetNeighborCount ();
  Hu11sWmnCapability const& WmnCapability ();
  
  // Inherited from WifiInformationElement
  virtual WifiInformationElementId ElementId () const;
  virtual uint8_t GetInformationFieldSize () const;
  virtual void SerializeInformationField (Buffer::Iterator i) const;
  virtual uint8_t DeserializeInformationField (Buffer::Iterator i, uint8_t length);
  virtual void Print (std::ostream& os) const;

private:
  /** Active Path Selection Protocol ID */
  hu11sPathSelectionProtocol m_APSPId;
  /** Active Path Metric ID */
  hu11sPathSelectionMetric   m_APSMId;
  /** Congestion Control Mode ID */
  hu11sCongestionControlMode m_CCMId;
  /** Sync protocol ID */
  hu11sSynchronizationProtocolIdentifier m_SPId;
  /** Auth protocol ID */
  hu11sAuthenticationProtocol m_APId;
  Hu11sWmnCapability m_wmnCap;
  uint8_t m_neighbors;
  friend bool operator== (const IeConfiguration & a, const IeConfiguration & b);
};
bool operator== (const IeConfiguration & a, const IeConfiguration & b);
bool operator== (const Hu11sWmnCapability & a, const Hu11sWmnCapability & b);
std::ostream &operator << (std::ostream &os, const IeConfiguration &config);
} // namespace hu11s
} // namespace ns3
#endif
