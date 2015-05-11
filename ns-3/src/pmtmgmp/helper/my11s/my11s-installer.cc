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
 */
#include "ns3/my11s-installer.h"
#include "ns3/pmtmgmp-peer-management-protocol.h"
#include "ns3/pmtmgmp-protocol.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wmn-wifi-interface-mac.h"

namespace ns3 {
using namespace my11s;
NS_OBJECT_ENSURE_REGISTERED (My11sStack);
  
TypeId
My11sStack::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::My11sStack")
    .SetParent<Object> ()
    .AddConstructor<My11sStack> ()
    .AddAttribute ("Root", 
                   "The MAC address of root wmn point.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&My11sStack::m_root),
                   MakeMac48AddressChecker ());
  return tid;
}
My11sStack::My11sStack () :
  m_root (Mac48Address ("ff:ff:ff:ff:ff:ff"))
{
}
My11sStack::~My11sStack ()
{
}
void
My11sStack::DoDispose ()
{
}
bool
My11sStack::InstallStack (Ptr<WmnPointDevice> mp)
{
  //Install Peer management protocol:
  Ptr<PeerManagementProtocol> pmp = CreateObject<PeerManagementProtocol> ();
  pmp->SetWmnId ("wmn");
  bool install_ok = pmp->Install (mp);
  if (!install_ok)
    {
      return false;
    }
  //Install PMTMGMP:
  Ptr<PmtmgmpProtocol> pmtmgmp = CreateObject<PmtmgmpProtocol> ();
  install_ok = pmtmgmp->Install (mp);
  if (!install_ok)
    {
      return false;
    }
  if (mp->GetAddress () == m_root)
    {
      pmtmgmp->SetRoot ();
    }
  //Install interaction between PMTMGMP and Peer management protocol:
  //PeekPointer()'s to avoid circular Ptr references
  pmp->SetPeerLinkStatusCallback (MakeCallback (&PmtmgmpProtocol::PeerLinkStatus, PeekPointer (pmtmgmp)));
  pmtmgmp->SetNeighboursCallback (MakeCallback (&PeerManagementProtocol::GetPeers, PeekPointer (pmp)));
  return true;
}
void
My11sStack::Report (const Ptr<WmnPointDevice> mp, std::ostream& os)
{
  mp->Report (os);

  std::vector<Ptr<NetDevice> > ifaces = mp->GetInterfaces ();
  for (std::vector<Ptr<NetDevice> >::const_iterator i = ifaces.begin (); i != ifaces.end (); ++i)
    {
      Ptr<WifiNetDevice> device = (*i)->GetObject<WifiNetDevice> ();
      NS_ASSERT (device != 0);
      Ptr<WmnWifiInterfaceMac> mac = device->GetMac ()->GetObject<WmnWifiInterfaceMac> ();
      NS_ASSERT (mac != 0);
      mac->Report (os);
    }
  Ptr<PmtmgmpProtocol> pmtmgmp = mp->GetObject<PmtmgmpProtocol> ();
  NS_ASSERT (pmtmgmp != 0);
  pmtmgmp->Report (os);

  Ptr<PeerManagementProtocol> pmp = mp->GetObject<PeerManagementProtocol> ();
  NS_ASSERT (pmp != 0);
  pmp->Report (os);
}
void
My11sStack::ResetStats (const Ptr<WmnPointDevice> mp)
{
  mp->ResetStats ();

  std::vector<Ptr<NetDevice> > ifaces = mp->GetInterfaces ();
  for (std::vector<Ptr<NetDevice> >::const_iterator i = ifaces.begin (); i != ifaces.end (); ++i)
    {
      Ptr<WifiNetDevice> device = (*i)->GetObject<WifiNetDevice> ();
      NS_ASSERT (device != 0);
      Ptr<WmnWifiInterfaceMac> mac = device->GetMac ()->GetObject<WmnWifiInterfaceMac> ();
      NS_ASSERT (mac != 0);
      mac->ResetStats ();
    }
  Ptr<PmtmgmpProtocol> pmtmgmp = mp->GetObject<PmtmgmpProtocol> ();
  NS_ASSERT (pmtmgmp != 0);
  pmtmgmp->ResetStats ();

  Ptr<PeerManagementProtocol> pmp = mp->GetObject<PeerManagementProtocol> ();
  NS_ASSERT (pmp != 0);
  pmp->ResetStats ();
}
} // namespace ns3
