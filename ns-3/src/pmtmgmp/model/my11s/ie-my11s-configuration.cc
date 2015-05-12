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

#include "ie-my11s-configuration.h"
#include "ns3/packet.h"
namespace ns3 {
namespace my11s {

My11sWmnCapability::My11sWmnCapability () :
  acceptPmtmgmpPeerLinks (true), MCCASupported (false), MCCAEnabled (false), forwarding (true), beaconTimingReport (
    true), TBTTAdjustment (true), powerSaveLevel (false)
{
}
uint8_t
My11sWmnCapability::GetSerializedSize () const
{
  return 1; 
}
uint8_t  
My11sWmnCapability::GetUint8 () const  //IEEE 802.11-2012 8.4.2.100.8 Wmn Capability 
{
  uint8_t result = 0;  
  if (acceptPmtmgmpPeerLinks)
    {
      result |= 1 << 0; //The Accepting Additional Wmn Peerings subfield is set to 1 if the wmn STA is willing to establish additional wmn peerings   with other wmn STAs and set to 0 otherwise
    }
  if (MCCASupported) // The MCCA Supported subfield is set to 1 if the wmn STA implements MCCA and set to 0 otherwise
    {
      result |= 1 << 1;
    }
  if (MCCAEnabled)
    {
      result |= 1 << 2;
    }
  if (forwarding)
    {
      result |= 1 << 3;
    }
  if (beaconTimingReport)
    {
      result |= 1 << 4;
    }
  if (TBTTAdjustment)
    {
      result |= 1 << 5;
    }
  if (powerSaveLevel)
    {
      result |= 1 << 6;
    }
  return result;
}
Buffer::Iterator
My11sWmnCapability::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (GetUint8 ());
  return i;
}
Buffer::Iterator
My11sWmnCapability::Deserialize (Buffer::Iterator i)
{
  uint8_t cap = i.ReadU8 ();
  acceptPmtmgmpPeerLinks = Is (cap, 0);
  MCCASupported = Is (cap, 1);
  MCCAEnabled = Is (cap, 2);
  forwarding = Is (cap, 3);
  beaconTimingReport = Is (cap, 4);
  TBTTAdjustment = Is (cap, 5);
  powerSaveLevel = Is (cap, 6);
  return i;
}
bool
My11sWmnCapability::Is (uint8_t cap, uint8_t n) const
{
  uint16_t mask = 1 << n;
  return (cap & mask);
}
WifiInformationElementId
IeConfiguration::ElementId () const
{
  return IE11S_WMN_CONFIGURATION;
}

IeConfiguration::IeConfiguration () :
  m_APSPId (PROTOCOL_PMTMGMP), m_APSMId (METRIC_AIRTIME), m_CCMId (CONGESTION_NULL), m_SPId (
    SYNC_NEIGHBOUR_OFFSET), m_APId (AUTH_NULL), m_neighbors (0)
{
}
uint8_t
IeConfiguration::GetInformationFieldSize () const
{
   return 0   // Version
         + 1  // APSPId
         + 1 // APSMId
         + 1 // CCMId
         + 1 // SPId
         + 1 // APId
         + 1 // Wmn formation info (see 7.3.2.86.6 of 802.11s draft 3.0)
         + m_wmnCap.GetSerializedSize ();
}
void
IeConfiguration::SerializeInformationField (Buffer::Iterator i) const
{
  // Active Path Selection Protocol ID:
  i.WriteU8 (m_APSPId);
  // Active Path Metric ID:
  i.WriteU8 (m_APSMId);
  // Congestion Control Mode ID:
  i.WriteU8 (m_CCMId);
  // Sync:
  i.WriteU8 (m_SPId);
  // Auth:
  i.WriteU8 (m_APId);
  i.WriteU8 (m_neighbors << 1);
  m_wmnCap.Serialize (i);
}
uint8_t
IeConfiguration::DeserializeInformationField (Buffer::Iterator i, uint8_t length)
{
  Buffer::Iterator start = i;
  // Active Path Selection Protocol ID:
  m_APSPId = (my11sPathSelectionProtocol) i.ReadU8 ();
  // Active Path Metric ID:
  m_APSMId = (my11sPathSelectionMetric) i.ReadU8 ();
  // Congestion Control Mode ID:
  m_CCMId = (my11sCongestionControlMode) i.ReadU8 ();
  m_SPId = (my11sSynchronizationProtocolIdentifier) i.ReadU8 ();
  m_APId = (my11sAuthenticationProtocol) i.ReadU8 ();
  m_neighbors = (i.ReadU8 () >> 1) & 0xF;
  i = m_wmnCap.Deserialize (i);
  return i.GetDistanceFrom (start);
}
void
IeConfiguration::Print (std::ostream& os) const
{
  os << std::endl << "<information_element id=" << ElementId () << ">" << std::endl;
  os << "Number of neighbors:               = " << (uint16_t) m_neighbors
     << std::endl << "Active Path Selection Protocol ID: = " << (uint32_t) m_APSPId
     << std::endl << "Active Path Selection Metric ID:   = " << (uint32_t) m_APSMId
     << std::endl << "Congestion Control Mode ID:        = " << (uint32_t) m_CCMId
     << std::endl << "Synchronize protocol ID:           = " << (uint32_t) m_SPId
     << std::endl << "Authentication protocol ID:        = " << (uint32_t) m_APId
     << std::endl << "Capabilities:                      = " << m_wmnCap.GetUint8 () << std::endl;
  os << "</information_element>" << std::endl;
}
void
IeConfiguration::SetRouting (my11sPathSelectionProtocol routingId)
{
  m_APSPId = routingId;
}
void
IeConfiguration::SetMetric (my11sPathSelectionMetric metricId)
{
  m_APSMId = metricId;
}
bool
IeConfiguration::IsPMTMGMP ()
{
  return (m_APSPId == PROTOCOL_PMTMGMP);
}
bool
IeConfiguration::IsAirtime ()
{
  return (m_APSMId == METRIC_AIRTIME);
}
void
IeConfiguration::SetNeighborCount (uint8_t neighbors)
{
  m_neighbors = (neighbors > 31) ? 31 : neighbors;
}
uint8_t
IeConfiguration::GetNeighborCount ()
{
  return m_neighbors;
}
My11sWmnCapability const&
IeConfiguration::WmnCapability ()
{
  return m_wmnCap;
}
bool
operator== (const My11sWmnCapability & a, const My11sWmnCapability & b)
{
  return ((a.acceptPmtmgmpPeerLinks == b.acceptPmtmgmpPeerLinks) && (a.MCCASupported == b.MCCASupported) && (a.MCCAEnabled
                                                                                               == b.MCCAEnabled) && (a.forwarding == b.forwarding) && (a.beaconTimingReport == b.beaconTimingReport)
          && (a.TBTTAdjustment == b.TBTTAdjustment) && (a.powerSaveLevel == b.powerSaveLevel));
}
bool
operator== (const IeConfiguration & a, const IeConfiguration & b)
{
  return ((a.m_APSPId == b.m_APSPId) && (a.m_APSMId == b.m_APSMId) && (a.m_CCMId == b.m_CCMId) && (a.m_SPId
                                                                                                   == b.m_SPId) && (a.m_APId == b.m_APId) && (a.m_neighbors == b.m_neighbors) && (a.m_wmnCap
                                                                                                                                                                                  == b.m_wmnCap));
}
std::ostream &
operator << (std::ostream &os, const IeConfiguration &a)
{
  a.Print (os);
  return os;
}
} // namespace my11s
} // namespace ns3
