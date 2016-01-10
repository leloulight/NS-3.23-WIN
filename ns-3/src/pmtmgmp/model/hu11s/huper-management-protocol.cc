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

#include "ns3/huper-management-protocol.h"
#include "huper-management-protocol-mac.h"
#include "ie-hu11s-configuration.h"
#include "ie-hu11s-id.h"
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

NS_LOG_COMPONENT_DEFINE ("HuperManagementProtocol");
  
namespace hu11s {
  
/***************************************************
 * HuperManager
 ***************************************************/
NS_OBJECT_ENSURE_REGISTERED (HuperManagementProtocol);

TypeId
HuperManagementProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::hu11s::HuperManagementProtocol")
    .SetParent<Object> ()
    .SetGroupName ("Wmn")
    .AddConstructor<HuperManagementProtocol> ()
    // maximum number of huper links. Now we calculate the total
    // number of huper links on all interfaces
    .AddAttribute ( "MaxNumberOfHuperLinks",
                    "Maximum number of huper links",
                    UintegerValue (32),
                    MakeUintegerAccessor (
                      &HuperManagementProtocol::m_maxNumberOfHuperLinks),
                    MakeUintegerChecker<uint8_t> ()
                    )
    .AddAttribute ( "MaxBeaconShiftValue",
                    "Maximum number of TUs for beacon shifting",
                    UintegerValue (15),
                    MakeUintegerAccessor (
                      &HuperManagementProtocol::m_maxBeaconShift),
                    MakeUintegerChecker<uint16_t> ()
                    )
    .AddAttribute ( "EnableBeaconCollisionAvoidance",
                    "Enable/Disable Beacon collision avoidance.",
                    BooleanValue (true),
                    MakeBooleanAccessor (
                      &HuperManagementProtocol::SetBeaconCollisionAvoidance, &HuperManagementProtocol::GetBeaconCollisionAvoidance),
                    MakeBooleanChecker ()
                    )
    .AddTraceSource ("LinkOpen",
                     "New huper link opened",
                     MakeTraceSourceAccessor (&HuperManagementProtocol::m_linkOpenTraceSrc),
                     "ns3::HuperManagementProtocol::LinkOpenCloseTracedCallback"
                     )
    .AddTraceSource ("LinkClose",
                     "New huper link closed",
                     MakeTraceSourceAccessor (&HuperManagementProtocol::m_linkCloseTraceSrc),
                     "ns3::HuperManagementProtocol::LinkOpenCloseTracedCallback"
                     )

  ;
  return tid;
}
HuperManagementProtocol::HuperManagementProtocol () :
  m_lastAssocId (0), m_lastLocalLinkId (1), m_enableBca (true), m_maxBeaconShift (15)
{
  m_beaconShift = CreateObject<UniformRandomVariable> ();
}
HuperManagementProtocol::~HuperManagementProtocol ()
{
  m_wmnId = 0;
}
void
HuperManagementProtocol::DoDispose ()
{
  //cancel cleanup event and go through the map of huper links,
  //deleting each
  for (HuperLinksMap::iterator j = m_huperLinks.begin (); j != m_huperLinks.end (); j++)
    {
      for (HuperLinksOnInterface::iterator i = j->second.begin (); i != j->second.end (); i++)
        {
          (*i) = 0;
        }
      j->second.clear ();
    }
  m_huperLinks.clear ();
  m_plugins.clear ();
}

bool
HuperManagementProtocol::Install (Ptr<WmnPointDevice> mp)
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
      Ptr<HuperManagementProtocolMac> plugin = Create<HuperManagementProtocolMac> ((*i)->GetIfIndex (), this);
      mac->InstallPlugin (plugin);
      m_plugins[(*i)->GetIfIndex ()] = plugin;
      HuperLinksOnInterface newmap;
      m_huperLinks[(*i)->GetIfIndex ()] = newmap;
    }
  // Wmn point aggregates all installed protocols
  m_address = Mac48Address::ConvertFrom (mp->GetAddress ());
  mp->AggregateObject (this);
  return true;
}

Ptr<IeBeaconTiming>
HuperManagementProtocol::GetBeaconTimingElement (uint32_t interface)
{
  if (!GetBeaconCollisionAvoidance ())
    {
      return 0;
    }
  Ptr<IeBeaconTiming> retval = Create<IeBeaconTiming> ();
  HuperLinksMap::iterator iface = m_huperLinks.find (interface);
  NS_ASSERT (iface != m_huperLinks.end ());
  for (HuperLinksOnInterface::iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      //If we do not know huper Assoc Id, we shall not add any info
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
HuperManagementProtocol::ReceiveBeacon (uint32_t interface, Mac48Address huperAddress, Time beaconInterval, Ptr<IeBeaconTiming> timingElement)
{
  //PM STATE Machine
  //Check that a given beacon is not from our interface
  for (HuperManagementProtocolMacMap::const_iterator i = m_plugins.begin (); i != m_plugins.end (); i++)
    {
      if (i->second->GetAddress () == huperAddress)
        {
          return;
        }
    }
  Ptr<HuperLink> huperLink = FindHuperLink (interface, huperAddress);
  if (huperLink == 0)
    {
      if (ShouldSendOpen (interface, huperAddress))
        {
          huperLink = InitiateLink (interface, huperAddress, Mac48Address::GetBroadcast ());
          huperLink->MLMEActiveHuperLinkOpen ();
        }
      else
        {
          return;
        }
    }
  huperLink->SetBeaconInformation (Simulator::Now (), beaconInterval);
  if (GetBeaconCollisionAvoidance ())
    {
      huperLink->SetBeaconTimingElement (*PeekPointer (timingElement));
    }
}

void
HuperManagementProtocol::ReceiveHuperLinkFrame (uint32_t interface, Mac48Address huperAddress,
                                              Mac48Address huperWmnPointAddress, uint16_t aid, IeHuperManagement huperManagementElement,
                                              IeConfiguration wmnConfig)
{
  Ptr<HuperLink> huperLink = FindHuperLink (interface, huperAddress);
  if (huperManagementElement.SubtypeIsOpen ())
    {
      PmpReasonCode reasonCode (REASON11S_RESERVED);
      bool reject = !(ShouldAcceptOpen (interface, huperAddress, reasonCode));
      if (huperLink == 0)
        {
          huperLink = InitiateLink (interface, huperAddress, huperWmnPointAddress);
        }
      if (!reject)
        {
          huperLink->OpenAccept (huperManagementElement.GetLocalLinkId (), wmnConfig, huperWmnPointAddress);
        }
      else
        {
          huperLink->OpenReject (huperManagementElement.GetLocalLinkId (), wmnConfig, huperWmnPointAddress,
                                reasonCode);
        }
    }
  if (huperLink == 0)
    {
      return;
    }
  if (huperManagementElement.SubtypeIsConfirm ())
    {
      huperLink->ConfirmAccept (huperManagementElement.GetLocalLinkId (),
                               huperManagementElement.GetHuperLinkId (), aid, wmnConfig, huperWmnPointAddress);
    }
  if (huperManagementElement.SubtypeIsClose ())
    {
      huperLink->Close (huperManagementElement.GetLocalLinkId (), huperManagementElement.GetHuperLinkId (),
                       huperManagementElement.GetReasonCode ());
    }
}
void
HuperManagementProtocol::ConfigurationMismatch (uint32_t interface, Mac48Address huperAddress)
{
  Ptr<HuperLink> huperLink = FindHuperLink (interface, huperAddress);
  if (huperLink != 0)
    {
      huperLink->MLMECancelHuperLink (REASON11S_WMN_CAPABILITY_POLICY_VIOLATION);
    }
}
void
HuperManagementProtocol::TransmissionFailure (uint32_t interface, Mac48Address huperAddress)
{
  NS_LOG_DEBUG ("transmission failed between "<<GetAddress () << " and " << huperAddress << " failed, link will be closed");
  Ptr<HuperLink> huperLink = FindHuperLink (interface, huperAddress);
  if (huperLink != 0)
    {
      huperLink->TransmissionFailure ();
    }
}
void
HuperManagementProtocol::TransmissionSuccess (uint32_t interface, Mac48Address huperAddress)
{
  NS_LOG_DEBUG ("transmission success "<< GetAddress () << " and " << huperAddress);
  Ptr<HuperLink> huperLink = FindHuperLink (interface, huperAddress);
  if (huperLink != 0)
    {
      huperLink->TransmissionSuccess ();
    }
}
Ptr<HuperLink>
HuperManagementProtocol::InitiateLink (uint32_t interface, Mac48Address huperAddress,
                                      Mac48Address huperWmnPointAddress)
{
  Ptr<HuperLink> new_link = CreateObject<HuperLink> ();
  //find a huper link  - it must not exist
  if (FindHuperLink (interface, huperAddress) != 0)
    {
      NS_FATAL_ERROR ("Huper link must not exist.");
    }
  // Plugin must exist
  HuperManagementProtocolMacMap::iterator plugin = m_plugins.find (interface);
  NS_ASSERT (plugin != m_plugins.end ());
  HuperLinksMap::iterator iface = m_huperLinks.find (interface);
  NS_ASSERT (iface != m_huperLinks.end ());
  new_link->SetLocalAid (m_lastAssocId++);
  new_link->SetInterface (interface);
  new_link->SetLocalLinkId (m_lastLocalLinkId++);
  new_link->SetHuperAddress (huperAddress);
  new_link->SetHuperWmnPointAddress (huperWmnPointAddress);
  new_link->SetMacPlugin (plugin->second);
  new_link->MLMESetSignalStatusCallback (MakeCallback (&HuperManagementProtocol::HuperLinkStatus, this));
  iface->second.push_back (new_link);
  return new_link;
}

Ptr<HuperLink>
HuperManagementProtocol::FindHuperLink (uint32_t interface, Mac48Address huperAddress)
{
  HuperLinksMap::iterator iface = m_huperLinks.find (interface);
  NS_ASSERT (iface != m_huperLinks.end ());
  for (HuperLinksOnInterface::iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      if ((*i)->GetHuperAddress () == huperAddress)
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
HuperManagementProtocol::SetHuperLinkStatusCallback (
  Callback<void, Mac48Address, Mac48Address, uint32_t, bool> cb)
{
  m_huperStatusCallback = cb;
}

std::vector<Mac48Address>
HuperManagementProtocol::GetHupers (uint32_t interface) const
{
  std::vector<Mac48Address> retval;
  HuperLinksMap::const_iterator iface = m_huperLinks.find (interface);
  NS_ASSERT (iface != m_huperLinks.end ());
  for (HuperLinksOnInterface::const_iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      if ((*i)->LinkIsEstab ())
        {
          retval.push_back ((*i)->GetHuperAddress ());
        }
    }
  return retval;
}

std::vector< Ptr<HuperLink> >
HuperManagementProtocol::GetHuperLinks () const
{
  std::vector< Ptr<HuperLink> > links;

  for (HuperLinksMap::const_iterator iface = m_huperLinks.begin (); iface != m_huperLinks.end (); ++iface)
    {
      for (HuperLinksOnInterface::const_iterator i = iface->second.begin ();
           i != iface->second.end (); i++)
        if ((*i)->LinkIsEstab ())
          links.push_back (*i);
    }
  return links;
}
bool
HuperManagementProtocol::IsActiveLink (uint32_t interface, Mac48Address huperAddress)
{
  Ptr<HuperLink> huperLink = FindHuperLink (interface, huperAddress);
  if (huperLink != 0)
    {
      return (huperLink->LinkIsEstab ());
    }
  return false;
}
bool
HuperManagementProtocol::ShouldSendOpen (uint32_t interface, Mac48Address huperAddress)
{
  return (m_stats.linksTotal <= m_maxNumberOfHuperLinks);
}

bool
HuperManagementProtocol::ShouldAcceptOpen (uint32_t interface, Mac48Address huperAddress,
                                          PmpReasonCode & reasonCode)
{
  if (m_stats.linksTotal > m_maxNumberOfHuperLinks)
    {
      reasonCode = REASON11S_WMN_MAX_HUPERS;
      return false;
    }
  return true;
}

void
HuperManagementProtocol::CheckBeaconCollisions (uint32_t interface)
{
  if (!GetBeaconCollisionAvoidance ())
    {
      return;
    }
  HuperLinksMap::iterator iface = m_huperLinks.find (interface);
  NS_ASSERT (iface != m_huperLinks.end ());
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
      //I have no hupers - may be our beacons are in collision
      ShiftOwnBeacon (interface);
      return;
    }
  //check whether all my hupers receive my beacon and I'am not in collision with other beacons

  for (HuperLinksOnInterface::iterator i = iface->second.begin (); i != iface->second.end (); i++)
    {
      bool myBeaconExists = false;
      IeBeaconTiming::NeighboursTimingUnitsList neighbors = (*i)->GetBeaconTimingElement ().GetNeighboursTimingElementsList ();
      for (IeBeaconTiming::NeighboursTimingUnitsList::const_iterator j = neighbors.begin (); j != neighbors.end (); j++)
        {
          if ((*i)->GetHuperAid () == (*j)->GetAid ())
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
HuperManagementProtocol::ShiftOwnBeacon (uint32_t interface)
{
  int shift = 0;
  do
    {
      shift = (int) m_beaconShift->GetValue ();
    }
  while (shift == 0);
  // Apply beacon shift parameters:
  HuperManagementProtocolMacMap::iterator plugin = m_plugins.find (interface);
  NS_ASSERT (plugin != m_plugins.end ());
  plugin->second->SetBeaconShift (TuToTime (shift));
}

Time
HuperManagementProtocol::TuToTime (int x)
{
  return MicroSeconds (x * 1024);
}
int
HuperManagementProtocol::TimeToTu (Time x)
{
  return (int)(x.GetMicroSeconds () / 1024);
}

void
HuperManagementProtocol::NotifyLinkOpen (Mac48Address huperMp, Mac48Address huperIface, Mac48Address myIface, uint32_t interface)
{
  NS_LOG_LOGIC ("link_open " << myIface << " " << huperIface);
  m_stats.linksOpened++;
  m_stats.linksTotal++;
  if (!m_huperStatusCallback.IsNull ())
    {
      m_huperStatusCallback (huperMp, huperIface, interface, true);
    }
  m_linkOpenTraceSrc (myIface, huperIface);
}

void
HuperManagementProtocol::NotifyLinkClose (Mac48Address huperMp, Mac48Address huperIface, Mac48Address myIface, uint32_t interface)
{
  NS_LOG_LOGIC ("link_close " << myIface << " " << huperIface);
  m_stats.linksClosed++;
  m_stats.linksTotal--;
  if (!m_huperStatusCallback.IsNull ())
    {
      m_huperStatusCallback (huperMp, huperIface, interface, false);
    }
  m_linkCloseTraceSrc (myIface, huperIface);
}

void
HuperManagementProtocol::HuperLinkStatus (uint32_t interface, Mac48Address huperAddress,
                                        Mac48Address huperWmnPointAddress, HuperLink::HuperState ostate, HuperLink::HuperState nstate)
{
  HuperManagementProtocolMacMap::iterator plugin = m_plugins.find (interface);
  NS_ASSERT (plugin != m_plugins.end ());
  NS_LOG_DEBUG ("Link between me:" << m_address << " my interface:" << plugin->second->GetAddress ()
                                   << " and huper wmn point:" << huperWmnPointAddress << " and its interface:" << huperAddress
                                   << ", at my interface ID:" << interface << ". State movement:" << ostate << " -> " << nstate);
  if ((nstate == HuperLink::ESTAB) && (ostate != HuperLink::ESTAB))
    {
      NotifyLinkOpen (huperWmnPointAddress, huperAddress, plugin->second->GetAddress (), interface);
    }
  if ((ostate == HuperLink::ESTAB) && (nstate != HuperLink::ESTAB))
    {
      NotifyLinkClose (huperWmnPointAddress, huperAddress, plugin->second->GetAddress (), interface);
    }
  if (nstate == HuperLink::IDLE)
    {
      Ptr<HuperLink> link = FindHuperLink (interface, huperAddress);
      NS_ASSERT (link == 0);
    }
}
uint8_t
HuperManagementProtocol::GetNumberOfLinks ()
{
  return m_stats.linksTotal;
}
Ptr<IeWmnId>
HuperManagementProtocol::GetWmnId () const
{
  NS_ASSERT (m_wmnId != 0);
  return m_wmnId;
}
void
HuperManagementProtocol::SetWmnId (std::string s)
{
  m_wmnId = Create<IeWmnId> (s);
}
Mac48Address
HuperManagementProtocol::GetAddress ()
{
  return m_address;
}
void
HuperManagementProtocol::NotifyBeaconSent (uint32_t interface, Time beaconInterval)
{
  m_lastBeacon[interface] = Simulator::Now ();
  Simulator::Schedule (beaconInterval - TuToTime (m_maxBeaconShift + 1), &HuperManagementProtocol::CheckBeaconCollisions,this, interface);
  m_beaconInterval[interface] = beaconInterval;
}
HuperManagementProtocol::Statistics::Statistics (uint16_t t) :
  linksTotal (t), linksOpened (0), linksClosed (0)
{
}
void
HuperManagementProtocol::Statistics::Print (std::ostream & os) const
{
  os << "<Statistics "
  "linksTotal=\"" << linksTotal << "\" "
  "linksOpened=\"" << linksOpened << "\" "
  "linksClosed=\"" << linksClosed << "\"/>" << std::endl;
}
void
HuperManagementProtocol::Report (std::ostream & os) const
{
  os << "<HuperManagementProtocol>" << std::endl;
  m_stats.Print (os);
  for (HuperManagementProtocolMacMap::const_iterator plugins = m_plugins.begin (); plugins != m_plugins.end (); plugins++)
    {
      //Take statistics from plugin:
      plugins->second->Report (os);
      //Print all active huper links:
      HuperLinksMap::const_iterator iface = m_huperLinks.find (plugins->second->m_ifIndex);
      NS_ASSERT (iface != m_huperLinks.end ());
      for (HuperLinksOnInterface::const_iterator i = iface->second.begin (); i != iface->second.end (); i++)
        {
          (*i)->Report (os);
        }
    }
  os << "</HuperManagementProtocol>" << std::endl;
}
void
HuperManagementProtocol::ResetStats ()
{
  m_stats = Statistics (m_stats.linksTotal); // don't reset number of links
  for (HuperManagementProtocolMacMap::const_iterator plugins = m_plugins.begin (); plugins != m_plugins.end (); plugins++)
    {
      plugins->second->ResetStats ();
    }
}

int64_t
HuperManagementProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_beaconShift->SetStream (stream);
  return 1;
}

void
HuperManagementProtocol::DoInitialize ()
{
  // If beacon interval is equal to the neighbor's one and one o more beacons received
  // by my neighbor coincide with my beacon - apply random uniformly distributed shift from
  // [-m_maxBeaconShift, m_maxBeaconShift] except 0.
  m_beaconShift->SetAttribute ("Min", DoubleValue (-m_maxBeaconShift));
  m_beaconShift->SetAttribute ("Max", DoubleValue (m_maxBeaconShift));
}

void
HuperManagementProtocol::SetBeaconCollisionAvoidance (bool enable)
{
  m_enableBca = enable;
}
bool
HuperManagementProtocol::GetBeaconCollisionAvoidance () const
{
  return m_enableBca;
}
} // namespace hu11s
} // namespace ns3

