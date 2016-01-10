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
#include "ns3/hu11s-installer.h"
#include "ns3/Huper-management-protocol.h"
#include "ns3/mgmp-protocol.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wmn-wifi-interface-mac.h"

namespace ns3 {
using namespace hu11s;
NS_OBJECT_ENSURE_REGISTERED (Hu11sStack);
  
TypeId
Hu11sStack::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Hu11sStack")
    .SetParent<Object> ()
    .AddConstructor<Hu11sStack> ()
    .AddAttribute ("Root", 
                   "The MAC address of root wmn point.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&Hu11sStack::m_root),
                   MakeMac48AddressChecker ());
  return tid;
}
Hu11sStack::Hu11sStack () :
  m_root (Mac48Address ("ff:ff:ff:ff:ff:ff"))
{
}
Hu11sStack::~Hu11sStack ()
{
}
void
Hu11sStack::DoDispose ()
{
}
bool
Hu11sStack::InstallStack (Ptr<WmnPointDevice> mp)
{
  //Install Huper management protocol:
  Ptr<HuperManagementProtocol> pmp = CreateObject<HuperManagementProtocol> ();
  pmp->SetWmnId ("wmn");
  bool install_ok = pmp->Install (mp);
  if (!install_ok)
    {
      return false;
    }
  //Install MGMP:
  Ptr<MgmpProtocol> mgmp = CreateObject<MgmpProtocol> ();
  install_ok = mgmp->Install (mp);
  if (!install_ok)
    {
      return false;
    }
  if (mp->GetAddress () == m_root)
    {
      mgmp->SetRoot ();
    }
  //Install interaction between MGMP and Huper management protocol:
  //PeekPointer()'s to avoid circular Ptr references
  pmp->SetHuperLinkStatusCallback (MakeCallback (&MgmpProtocol::HuperLinkStatus, PeekPointer (mgmp)));
  mgmp->SetNeighboursCallback (MakeCallback (&HuperManagementProtocol::GetHupers, PeekPointer (pmp)));
  return true;
}
void
Hu11sStack::Report (const Ptr<WmnPointDevice> mp, std::ostream& os)
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
  Ptr<MgmpProtocol> mgmp = mp->GetObject<MgmpProtocol> ();
  NS_ASSERT (mgmp != 0);
  mgmp->Report (os);

  Ptr<HuperManagementProtocol> pmp = mp->GetObject<HuperManagementProtocol> ();
  NS_ASSERT (pmp != 0);
  pmp->Report (os);
}
void
Hu11sStack::ResetStats (const Ptr<WmnPointDevice> mp)
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
  Ptr<MgmpProtocol> mgmp = mp->GetObject<MgmpProtocol> ();
  NS_ASSERT (mgmp != 0);
  mgmp->ResetStats ();

  Ptr<HuperManagementProtocol> pmp = mp->GetObject<HuperManagementProtocol> ();
  NS_ASSERT (pmp != 0);
  pmp->ResetStats ();
}
} // namespace ns3
