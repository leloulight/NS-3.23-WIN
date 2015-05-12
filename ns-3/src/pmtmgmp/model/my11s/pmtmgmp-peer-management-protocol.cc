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

#include "ns3/pmtmgmp-peer-management-protocol.h"
#include "pmtmgmp-peer-management-protocol-mac.h"
#include "ie-my11s-configuration.h"
#include "ie-my11s-id.h"
#include "ns3/wmn-point-device.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wmn-wifi-interface-mac.h"
#include "ns3/wmn-wifi-interface-mac-plugin.h"
#include "ns3/wifi-net-device.h"
#include "ns3/trace-source-accessor.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PmtmgmpPeerManagementProtocol");
  
namespace my11s {
  
/***************************************************
 * PeerManager
 ***************************************************/
NS_OBJECT_ENSURE_REGISTERED (PmtmgmpPeerManagementProtocol);

TypeId
PmtmgmpPeerManagementProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::my11s::PmtmgmpPeerManagementProtocol")
    .SetParent<Object> ()
    .SetGroupName ("Wmn")
    .AddConstructor<PmtmgmpPeerManagementProtocol> ()
    // maximum number of peer links. Now we calculate the total
    // number of peer links on all interfaces
    .AddAttribute ( "MaxNumberOfPmtmgmpPeerLinks",
                    "Maximum number of peer links",
                    UintegerValue (32),
                    MakeUintegerAccessor (
                      &PmtmgmpPeerManagementProtocol::m_maxNumberOfPmtmgmpPeerLinks),
                    MakeUintegerChecker<uint8_t> ()
                    )
    .AddAttribute ( "MaxBeaconShiftValue",
                    "Maximum number of TUs for beacon shifting",
                    UintegerValue (15),
                    MakeUintegerAccessor (
                      &PmtmgmpPeerManagementProtocol::m_maxBeaconShift),
                    MakeUintegerChecker<uint16_t> ()
                    )
    .AddAttribute ( "EnableBeaconCollisionAvoidance",
                    "Enable/Disable Beacon collision avoidance.",
                    BooleanValue (true),
                    MakeBooleanAccessor (
                      &PmtmgmpPeerManagementProtocol::SetBeaconCollisionAvoidance, &PmtmgmpPeerManagementProtocol::GetBeaconCollisionAvoidance),
                    MakeBooleanChecker ()
                    )
    .AddTraceSource ("LinkOpen",
                     "New peer link opened",
                     MakeTraceSourceAccessor (&PmtmgmpPeerManagementProtocol::m_linkOpenTraceSrc),
                     "ns3::PmtmgmpPeerManagementProtocol::LinkOpenCloseTracedCallback"
                     )
    .AddTraceSource ("LinkClose",
                     "New peer link closed",
                     MakeTraceSourceAccessor (&PmtmgmpPeerManagementProtocol::m_linkCloseTraceSrc),
                     "ns3::PmtmgmpPeerManagementProtocol::LinkOpenCloseTracedCallback"
                     )

  ;
  return tid;
}
PmtmgmpPeerManagementProtocol::PmtmgmpPeerManagementProtocol () :
  m_lastAssocId (0), m_lastLocalLinkId (1), m_enableBca (true), m_maxBeaconShift (15)
{
  m_beaconShift = CreateObject<UniformRandomVariable> ();
}
PmtmgmpPeerManagementProtocol::~PmtmgmpPeerManagementProtocol ()
{
  m_wmnId = 0;
}
void
PmtmgmpPeerManagementProtocol::DoDispose ()
{
  //cancel cleanup event and go through the map of peer links,
  //deleting each
  for (PmtmgmpPeerLinksMap::iterator j = m_PmtmgmpPeerLinks.begin (); j != m_PmtmgmpPeerLinks.end (); j++)
    {
      for (PmtmgmpPeerLinksOnInterface::iterator i = j->second.begin (); i != j->second.end (); i++)
        {
          (*i) = 0;
        }
      j->second.clear ();
    }
  m_PmtmgmpPeerLinks.clear ();
  m_plugins.clear ();
}

bool
PmtmgmpPeerManagementProtocol::Install (Ptr<WmnPointDevice> mp)
{
  std::vector<Ptr<NetDevice> > interfaces = mp->GetInterfaces ();
  for (std::vector<Ptr<NetDevice> >::iterator i = interfaces.begin (); i != interfaces.end (); i++)
    {
      Ptr<WifiNetDevice> wifiNetDev = (*i)->GetObject<WifiNetDevice> ();
      if (wifiNetDev == 0)
        {
          return false;
        }
      Ptr<WmnWifiInterfaceMac> mac = wifiNetDev->GetMac ()->GetObject<WmnWifiInterfaceMac> ();
      if (mac == 0)
        {
          return false;
        }
      Ptr<PmtmgmpPeerManagementProtocolMac> plugin = Create<PmtmgmpPeerManagementProtocolMac> ((*i)->GetIfIndex (), this);
      mac->InstallPlugin (plugin);
      m_plugins[(*i)->GetIfIndex ()] = plugin;
      PmtmgmpPeerLinksOnInterface newmap;
      m_PmtmgmpPeerLinks[(*i)->GetIfIndex ()] = newmap;
    }
  // Wmn point aggregates all installed protocols
  m_address = Mac48Address::ConvertFrom (mp->GetAddress ());
  mp->AggregateObject (this);
  return true;
}

Ptr<IeBeaconTiming>
PmtmgmpPeerManagementProtocol::GetBeaconTimingElement (uint32_t interface)
{
  if (!GetBeaconCollisionAvoidance ())
    {
      return 0;
    }
  Ptr<IeBeaconTiming> retval = Create<IeBeaconTiming> ();
  PmtmgmpPeerLinksMap::iterator iface = m_PmtmgmpPeerLinks.find (interface);
  NS_ASSERT (iface != m_PmtmgmpPeerLinks.end ());
  for (PmtmgmpPeerLinksOnInterface::iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      //If we do not know peer Assoc Id, we shall not add any info
      //to a beacon timing element
      if ((*i)->GetBeaconInterval () == Seconds (0))
        {
          //No beacon was received, do not include to the beacon timing element
          continue;
        }
      retval->AddNeighboursTimingElementUnit ((*i)->GetLocalAid (), (*i)->GetLastBeacon (),
                                              (*i)->GetBeaconInterval ());
    }
  return retval;
}
void
PmtmgmpPeerManagementProtocol::ReceiveBeacon (uint32_t interface, Mac48Address peerAddress, Time beaconInterval, Ptr<IeBeaconTiming> timingElement)
{
  //PM STATE Machine
  //Check that a given beacon is not from our interface
  for (PmtmgmpPeerManagementProtocolMacMap::const_iterator i = m_plugins.begin (); i != m_plugins.end (); i++)
    {
      if (i->second->GetAddress () == peerAddress)
        {
          return;
        }
    }
  Ptr<PmtmgmpPeerLink> PmtmgmpPeerLink = FindPmtmgmpPeerLink (interface, peerAddress);
  if (PmtmgmpPeerLink == 0)
    {
      if (ShouldSendOpen (interface, peerAddress))
        {
          PmtmgmpPeerLink = InitiateLink (interface, peerAddress, Mac48Address::GetBroadcast ());
          PmtmgmpPeerLink->MLMEActivePmtmgmpPeerLinkOpen ();
        }
      else
        {
          return;
        }
    }
  PmtmgmpPeerLink->SetBeaconInformation (Simulator::Now (), beaconInterval);
  if (GetBeaconCollisionAvoidance ())
    {
      PmtmgmpPeerLink->SetBeaconTimingElement (*PeekPointer (timingElement));
    }
}

void
PmtmgmpPeerManagementProtocol::ReceivePmtmgmpPeerLinkFrame (uint32_t interface, Mac48Address peerAddress,
                                              Mac48Address peerWmnPointAddress, uint16_t aid, IePeerManagement peerManagementElement,
                                              IeConfiguration wmnConfig)
{
  Ptr<PmtmgmpPeerLink> PmtmgmpPeerLink = FindPmtmgmpPeerLink (interface, peerAddress);
  if (peerManagementElement.SubtypeIsOpen ())
    {
      PmpReasonCode reasonCode (REASON11S_RESERVED);
      bool reject = !(ShouldAcceptOpen (interface, peerAddress, reasonCode));
      if (PmtmgmpPeerLink == 0)
        {
          PmtmgmpPeerLink = InitiateLink (interface, peerAddress, peerWmnPointAddress);
        }
      if (!reject)
        {
          PmtmgmpPeerLink->OpenAccept (peerManagementElement.GetLocalLinkId (), wmnConfig, peerWmnPointAddress);
        }
      else
        {
          PmtmgmpPeerLink->OpenReject (peerManagementElement.GetLocalLinkId (), wmnConfig, peerWmnPointAddress,
                                reasonCode);
        }
    }
  if (PmtmgmpPeerLink == 0)
    {
      return;
    }
  if (peerManagementElement.SubtypeIsConfirm ())
    {
      PmtmgmpPeerLink->ConfirmAccept (peerManagementElement.GetLocalLinkId (),
                               peerManagementElement.GetPmtmgmpPeerLinkId (), aid, wmnConfig, peerWmnPointAddress);
    }
  if (peerManagementElement.SubtypeIsClose ())
    {
      PmtmgmpPeerLink->Close (peerManagementElement.GetLocalLinkId (), peerManagementElement.GetPmtmgmpPeerLinkId (),
                       peerManagementElement.GetReasonCode ());
    }
}
void
PmtmgmpPeerManagementProtocol::ConfigurationMismatch (uint32_t interface, Mac48Address peerAddress)
{
  Ptr<PmtmgmpPeerLink> PmtmgmpPeerLink = FindPmtmgmpPeerLink (interface, peerAddress);
  if (PmtmgmpPeerLink != 0)
    {
      PmtmgmpPeerLink->MLMECancelPmtmgmpPeerLink (REASON11S_WMN_CAPABILITY_POLICY_VIOLATION);
    }
}
void
PmtmgmpPeerManagementProtocol::TransmissionFailure (uint32_t interface, Mac48Address peerAddress)
{
  NS_LOG_DEBUG ("transmission failed between "<<GetAddress () << " and " << peerAddress << " failed, link will be closed");
  Ptr<PmtmgmpPeerLink> PmtmgmpPeerLink = FindPmtmgmpPeerLink (interface, peerAddress);
  if (PmtmgmpPeerLink != 0)
    {
      PmtmgmpPeerLink->TransmissionFailure ();
    }
}
void
PmtmgmpPeerManagementProtocol::TransmissionSuccess (uint32_t interface, Mac48Address peerAddress)
{
  NS_LOG_DEBUG ("transmission success "<< GetAddress () << " and " << peerAddress);
  Ptr<PmtmgmpPeerLink> PmtmgmpPeerLink = FindPmtmgmpPeerLink (interface, peerAddress);
  if (PmtmgmpPeerLink != 0)
    {
      PmtmgmpPeerLink->TransmissionSuccess ();
    }
}
Ptr<PmtmgmpPeerLink>
PmtmgmpPeerManagementProtocol::InitiateLink (uint32_t interface, Mac48Address peerAddress,
                                      Mac48Address peerWmnPointAddress)
{
  Ptr<PmtmgmpPeerLink> new_link = CreateObject<PmtmgmpPeerLink> ();
  //find a peer link  - it must not exist
  if (FindPmtmgmpPeerLink (interface, peerAddress) != 0)
    {
      NS_FATAL_ERROR ("Peer link must not exist.");
    }
  // Plugin must exist
  PmtmgmpPeerManagementProtocolMacMap::iterator plugin = m_plugins.find (interface);
  NS_ASSERT (plugin != m_plugins.end ());
  PmtmgmpPeerLinksMap::iterator iface = m_PmtmgmpPeerLinks.find (interface);
  NS_ASSERT (iface != m_PmtmgmpPeerLinks.end ());
  new_link->SetLocalAid (m_lastAssocId++);
  new_link->SetInterface (interface);
  new_link->SetLocalLinkId (m_lastLocalLinkId++);
  new_link->SetPeerAddress (peerAddress);
  new_link->SetPeerWmnPointAddress (peerWmnPointAddress);
  new_link->SetMacPlugin (plugin->second);
  new_link->MLMESetSignalStatusCallback (MakeCallback (&PmtmgmpPeerManagementProtocol::PmtmgmpPeerLinkStatus, this));
  iface->second.push_back (new_link);
  return new_link;
}

Ptr<PmtmgmpPeerLink>
PmtmgmpPeerManagementProtocol::FindPmtmgmpPeerLink (uint32_t interface, Mac48Address peerAddress)
{
  PmtmgmpPeerLinksMap::iterator iface = m_PmtmgmpPeerLinks.find (interface);
  NS_ASSERT (iface != m_PmtmgmpPeerLinks.end ());
  for (PmtmgmpPeerLinksOnInterface::iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      if ((*i)->GetPeerAddress () == peerAddress)
        {
          if ((*i)->LinkIsIdle ())
            {
              (*i) = 0;
              (iface->second).erase (i);
              return 0;
            }
          else
            {
              return (*i);
            }
        }
    }
  return 0;
}
void
PmtmgmpPeerManagementProtocol::SetPmtmgmpPeerLinkStatusCallback (
  Callback<void, Mac48Address, Mac48Address, uint32_t, bool> cb)
{
  m_peerStatusCallback = cb;
}

std::vector<Mac48Address>
PmtmgmpPeerManagementProtocol::GetPeers (uint32_t interface) const
{
  std::vector<Mac48Address> retval;
  PmtmgmpPeerLinksMap::const_iterator iface = m_PmtmgmpPeerLinks.find (interface);
  NS_ASSERT (iface != m_PmtmgmpPeerLinks.end ());
  for (PmtmgmpPeerLinksOnInterface::const_iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      if ((*i)->LinkIsEstab ())
        {
          retval.push_back ((*i)->GetPeerAddress ());
        }
    }
  return retval;
}

std::vector< Ptr<PmtmgmpPeerLink> >
PmtmgmpPeerManagementProtocol::GetPmtmgmpPeerLinks () const
{
  std::vector< Ptr<PmtmgmpPeerLink> > links;

  for (PmtmgmpPeerLinksMap::const_iterator iface = m_PmtmgmpPeerLinks.begin (); iface != m_PmtmgmpPeerLinks.end (); ++iface)
    {
      for (PmtmgmpPeerLinksOnInterface::const_iterator i = iface->second.begin ();
           i != iface->second.end (); i++)
        if ((*i)->LinkIsEstab ())
          links.push_back (*i);
    }
  return links;
}
bool
PmtmgmpPeerManagementProtocol::IsActiveLink (uint32_t interface, Mac48Address peerAddress)
{
  Ptr<PmtmgmpPeerLink> PmtmgmpPeerLink = FindPmtmgmpPeerLink (interface, peerAddress);
  if (PmtmgmpPeerLink != 0)
    {
      return (PmtmgmpPeerLink->LinkIsEstab ());
    }
  return false;
}
bool
PmtmgmpPeerManagementProtocol::ShouldSendOpen (uint32_t interface, Mac48Address peerAddress)
{
  return (m_stats.linksTotal <= m_maxNumberOfPmtmgmpPeerLinks);
}

bool
PmtmgmpPeerManagementProtocol::ShouldAcceptOpen (uint32_t interface, Mac48Address peerAddress,
                                          PmpReasonCode & reasonCode)
{
  if (m_stats.linksTotal > m_maxNumberOfPmtmgmpPeerLinks)
    {
      reasonCode = REASON11S_WMN_MAX_PMTMGMP_PEERS;
      return false;
    }
  return true;
}

void
PmtmgmpPeerManagementProtocol::CheckBeaconCollisions (uint32_t interface)
{
  if (!GetBeaconCollisionAvoidance ())
    {
      return;
    }
  PmtmgmpPeerLinksMap::iterator iface = m_PmtmgmpPeerLinks.find (interface);
  NS_ASSERT (iface != m_PmtmgmpPeerLinks.end ());
  NS_ASSERT (m_plugins.find (interface) != m_plugins.end ());

  std::map<uint32_t, Time>::const_iterator lastBeacon = m_lastBeacon.find (interface);
  std::map<uint32_t, Time>::const_iterator beaconInterval = m_beaconInterval.find (interface);
  if ((lastBeacon == m_lastBeacon.end ()) || (beaconInterval == m_beaconInterval.end ()))
    {
      return;
    }
  //my last beacon in 256 us units
  uint16_t lastBeaconInTimeElement = (uint16_t) ((lastBeacon->second.GetMicroSeconds () >> 8) & 0xffff);

  NS_ASSERT_MSG (TuToTime (m_maxBeaconShift) <= m_beaconInterval[interface], "Wrong beacon shift parameters");

  if (iface->second.size () == 0)
    {
      //I have no peers - may be our beacons are in collision
      ShiftOwnBeacon (interface);
      return;
    }
  //check whether all my peers receive my beacon and I'am not in collision with other beacons

  for (PmtmgmpPeerLinksOnInterface::iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      bool myBeaconExists = false;
      IeBeaconTiming::NeighboursTimingUnitsList neighbors = (*i)->GetBeaconTimingElement ().GetNeighboursTimingElementsList ();
      for (IeBeaconTiming::NeighboursTimingUnitsList::const_iterator j = neighbors.begin (); j != neighbors.end (); j++)
        {
          if ((*i)->GetPeerAid () == (*j)->GetAid ())
            {
              // I am presented at neighbour's list of neighbors
              myBeaconExists = true;
              continue;
            }
          if (
            ((int16_t) ((*j)->GetLastBeacon () - lastBeaconInTimeElement) >= 0) &&
            (((*j)->GetLastBeacon () - lastBeaconInTimeElement) % (4 * TimeToTu (beaconInterval->second)) == 0)
            )
            {
              ShiftOwnBeacon (interface);
              return;
            }
        }
      if (!myBeaconExists)
        {
          // If I am not present in neighbor's beacon timing element, this may be caused by collisions with
          ShiftOwnBeacon (interface);
          return;
        }
    }
}

void
PmtmgmpPeerManagementProtocol::ShiftOwnBeacon (uint32_t interface)
{
  int shift = 0;
  do
    {
      shift = (int) m_beaconShift->GetValue ();
    }
  while (shift == 0);
  // Apply beacon shift parameters:
  PmtmgmpPeerManagementProtocolMacMap::iterator plugin = m_plugins.find (interface);
  NS_ASSERT (plugin != m_plugins.end ());
  plugin->second->SetBeaconShift (TuToTime (shift));
}

Time
PmtmgmpPeerManagementProtocol::TuToTime (int x)
{
  return MicroSeconds (x * 1024);
}
int
PmtmgmpPeerManagementProtocol::TimeToTu (Time x)
{
  return (int)(x.GetMicroSeconds () / 1024);
}

void
PmtmgmpPeerManagementProtocol::NotifyLinkOpen (Mac48Address peerMp, Mac48Address peerIface, Mac48Address myIface, uint32_t interface)
{
  NS_LOG_LOGIC ("link_open " << myIface << " " << peerIface);
  m_stats.linksOpened++;
  m_stats.linksTotal++;
  if (!m_peerStatusCallback.IsNull ())
    {
      m_peerStatusCallback (peerMp, peerIface, interface, true);
    }
  m_linkOpenTraceSrc (myIface, peerIface);
}

void
PmtmgmpPeerManagementProtocol::NotifyLinkClose (Mac48Address peerMp, Mac48Address peerIface, Mac48Address myIface, uint32_t interface)
{
  NS_LOG_LOGIC ("link_close " << myIface << " " << peerIface);
  m_stats.linksClosed++;
  m_stats.linksTotal--;
  if (!m_peerStatusCallback.IsNull ())
    {
      m_peerStatusCallback (peerMp, peerIface, interface, false);
    }
  m_linkCloseTraceSrc (myIface, peerIface);
}

void
PmtmgmpPeerManagementProtocol::PmtmgmpPeerLinkStatus (uint32_t interface, Mac48Address peerAddress,
                                        Mac48Address peerWmnPointAddress, PmtmgmpPeerLink::PeerState ostate, PmtmgmpPeerLink::PeerState nstate)
{
  PmtmgmpPeerManagementProtocolMacMap::iterator plugin = m_plugins.find (interface);
  NS_ASSERT (plugin != m_plugins.end ());
  NS_LOG_DEBUG ("Link between me:" << m_address << " my interface:" << plugin->second->GetAddress ()
                                   << " and peer wmn point:" << peerWmnPointAddress << " and its interface:" << peerAddress
                                   << ", at my interface ID:" << interface << ". State movement:" << ostate << " -> " << nstate);
  if ((nstate == PmtmgmpPeerLink::ESTAB) && (ostate != PmtmgmpPeerLink::ESTAB))
    {
      NotifyLinkOpen (peerWmnPointAddress, peerAddress, plugin->second->GetAddress (), interface);
    }
  if ((ostate == PmtmgmpPeerLink::ESTAB) && (nstate != PmtmgmpPeerLink::ESTAB))
    {
      NotifyLinkClose (peerWmnPointAddress, peerAddress, plugin->second->GetAddress (), interface);
    }
  if (nstate == PmtmgmpPeerLink::IDLE)
    {
      Ptr<PmtmgmpPeerLink> link = FindPmtmgmpPeerLink (interface, peerAddress);
      NS_ASSERT (link == 0);
    }
}
uint8_t
PmtmgmpPeerManagementProtocol::GetNumberOfLinks ()
{
  return m_stats.linksTotal;
}
Ptr<IeWmnId>
PmtmgmpPeerManagementProtocol::GetWmnId () const
{
  NS_ASSERT (m_wmnId != 0);
  return m_wmnId;
}
void
PmtmgmpPeerManagementProtocol::SetWmnId (std::string s)
{
  m_wmnId = Create<IeWmnId> (s);
}
Mac48Address
PmtmgmpPeerManagementProtocol::GetAddress ()
{
  return m_address;
}
void
PmtmgmpPeerManagementProtocol::NotifyBeaconSent (uint32_t interface, Time beaconInterval)
{
  m_lastBeacon[interface] = Simulator::Now ();
  Simulator::Schedule (beaconInterval - TuToTime (m_maxBeaconShift + 1), &PmtmgmpPeerManagementProtocol::CheckBeaconCollisions,this, interface);
  m_beaconInterval[interface] = beaconInterval;
}
PmtmgmpPeerManagementProtocol::Statistics::Statistics (uint16_t t) :
  linksTotal (t), linksOpened (0), linksClosed (0)
{
}
void
PmtmgmpPeerManagementProtocol::Statistics::Print (std::ostream & os) const
{
  os << "<Statistics "
  "linksTotal=\"" << linksTotal << "\" "
  "linksOpened=\"" << linksOpened << "\" "
  "linksClosed=\"" << linksClosed << "\"/>" << std::endl;
}
void
PmtmgmpPeerManagementProtocol::Report (std::ostream & os) const
{
  os << "<PmtmgmpPeerManagementProtocol>" << std::endl;
  m_stats.Print (os);
  for (PmtmgmpPeerManagementProtocolMacMap::const_iterator plugins = m_plugins.begin (); plugins != m_plugins.end (); plugins++)
    {
      //Take statistics from plugin:
      plugins->second->Report (os);
      //Print all active peer links:
      PmtmgmpPeerLinksMap::const_iterator iface = m_PmtmgmpPeerLinks.find (plugins->second->m_ifIndex);
      NS_ASSERT (iface != m_PmtmgmpPeerLinks.end ());
      for (PmtmgmpPeerLinksOnInterface::const_iterator i = iface->second.begin (); i != iface->second.end (); i++)
        {
          (*i)->Report (os);
        }
    }
  os << "</PmtmgmpPeerManagementProtocol>" << std::endl;
}
void
PmtmgmpPeerManagementProtocol::ResetStats ()
{
  m_stats = Statistics (m_stats.linksTotal); // don't reset number of links
  for (PmtmgmpPeerManagementProtocolMacMap::const_iterator plugins = m_plugins.begin (); plugins != m_plugins.end (); plugins++)
    {
      plugins->second->ResetStats ();
    }
}

int64_t
PmtmgmpPeerManagementProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_beaconShift->SetStream (stream);
  return 1;
}

void
PmtmgmpPeerManagementProtocol::DoInitialize ()
{
  // If beacon interval is equal to the neighbor's one and one o more beacons received
  // by my neighbor coincide with my beacon - apply random uniformly distributed shift from
  // [-m_maxBeaconShift, m_maxBeaconShift] except 0.
  m_beaconShift->SetAttribute ("Min", DoubleValue (-m_maxBeaconShift));
  m_beaconShift->SetAttribute ("Max", DoubleValue (m_maxBeaconShift));
}

void
PmtmgmpPeerManagementProtocol::SetBeaconCollisionAvoidance (bool enable)
{
  m_enableBca = enable;
}
bool
PmtmgmpPeerManagementProtocol::GetBeaconCollisionAvoidance () const
{
  return m_enableBca;
}
} // namespace my11s
} // namespace ns3

