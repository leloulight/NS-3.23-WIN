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
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#include "ie-hu11s-configuration.h"
#include "ie-hu11s-huper-management.h"
#include "hu11s-mac-header.h"
#include "huper-management-protocol-mac.h"
#include "huper-management-protocol.h"
#include "huper-link-frame.h"
#include "ns3/wmn-wifi-interface-mac.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wmn-information-element-vector.h"
#include "ns3/log.h"
namespace ns3 {
namespace hu11s {
HuperManagementProtocolMac::HuperManagementProtocolMac (uint32_t interface,
                                                      Ptr<HuperManagementProtocol> protocol)
{
  m_ifIndex = interface;
  m_protocol = protocol;
}

HuperManagementProtocolMac::~HuperManagementProtocolMac ()
{
}

void
HuperManagementProtocolMac::SetParent (Ptr<WmnWifiInterfaceMac> parent)
{
  m_parent = parent;
  m_parent->TraceConnectWithoutContext ("TxErrHeader", MakeCallback (&HuperManagementProtocolMac::TxError, this));
  m_parent->TraceConnectWithoutContext ("TxOkHeader",  MakeCallback (&HuperManagementProtocolMac::TxOk,    this));
}
void
HuperManagementProtocolMac::TxError (WifiMacHeader const &hdr)
{
  m_protocol->TransmissionFailure (m_ifIndex, hdr.GetAddr1 ());
}
void
HuperManagementProtocolMac::TxOk (WifiMacHeader const &hdr)
{
  m_protocol->TransmissionSuccess (m_ifIndex, hdr.GetAddr1 ());
}
bool
HuperManagementProtocolMac::Receive (Ptr<Packet> const_packet, const WifiMacHeader & header)
{
  // First of all we copy a packet, because we need to remove some
  //headers
  Ptr<Packet> packet = const_packet->Copy ();
  if (header.IsBeacon ())
    {
      MgtBeaconHeader beacon_hdr;
      packet->RemoveHeader (beacon_hdr);
      WmnInformationElementVector elements;
      packet->RemoveHeader (elements);
      Ptr<IeBeaconTiming> beaconTiming = DynamicCast<IeBeaconTiming> (elements.FindFirst (IE11S_BEACON_TIMING));
      Ptr<IeWmnId> wmnId = DynamicCast<IeWmnId> (elements.FindFirst (IE11S_WMN_ID));

      if ((wmnId != 0) && (m_protocol->GetWmnId ()->IsEqual (*wmnId)))
        {
          m_protocol->ReceiveBeacon (m_ifIndex, header.GetAddr2 (), MicroSeconds (
                                       beacon_hdr.GetBeaconIntervalUs ()), beaconTiming);
        }
      // Beacon shall not be dropped. May be needed to another plugins
      return true;
    }
  if (header.IsAction ())
    {
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);
      WifiActionHeader::ActionValue actionValue = actionHdr.GetAction ();
      // If can not handle - just return;
      if (actionHdr.GetCategory () != WifiActionHeader::SELF_PROTECTED)
        {
          return m_protocol->IsActiveLink (m_ifIndex, header.GetAddr2 ());
        }
      m_stats.rxMgt++;
      m_stats.rxMgtBytes += packet->GetSize ();
      Mac48Address huperAddress = header.GetAddr2 ();
      Mac48Address huperMpAddress = header.GetAddr3 ();
      HuperLinkFrameStart::PlinkFrameStartFields fields;
      {
        HuperLinkFrameStart huperFrame;
        huperFrame.SetPlinkFrameSubtype ((uint8_t) actionValue.selfProtectedAction);
        packet->RemoveHeader (huperFrame);
        fields = huperFrame.GetFields ();
        NS_ASSERT (fields.subtype == actionValue.selfProtectedAction); 
      }
      if ((actionValue.selfProtectedAction != WifiActionHeader::PEER_LINK_CLOSE) && !(m_parent->CheckSupportedRates (fields.rates))) 
        {
          m_protocol->ConfigurationMismatch (m_ifIndex, huperAddress);
          // Broken huper link frame - drop it
          m_stats.brokenMgt++;
          return false;
        }
     if ((actionValue.selfProtectedAction != WifiActionHeader::PEER_LINK_CONFIRM) && !fields.wmnId.IsEqual (
            *(m_protocol->GetWmnId ())))
        {
          m_protocol->ConfigurationMismatch (m_ifIndex, huperAddress);
          // Broken huper link frame - drop it
          m_stats.brokenMgt++;
          return false;
        }
      Ptr<IeHuperManagement> huperElement;
      //Huper Management element is the last element in this frame - so, we can use WmnInformationElementVector
      WmnInformationElementVector elements;
      packet->RemoveHeader (elements);
      huperElement = DynamicCast<IeHuperManagement>(elements.FindFirst (IE11S_HUPERING_MANAGEMENT));

      NS_ASSERT (huperElement != 0);
      //Check taht frame subtype corresponds huper link subtype
      if (huperElement->SubtypeIsOpen ())
        {
          m_stats.rxOpen++;
         NS_ASSERT (actionValue.selfProtectedAction == WifiActionHeader::PEER_LINK_OPEN);
        }
      if (huperElement->SubtypeIsConfirm ())
        {
          m_stats.rxConfirm++;
       NS_ASSERT (actionValue.selfProtectedAction == WifiActionHeader::PEER_LINK_CONFIRM); 
        }
      if (huperElement->SubtypeIsClose ())
        {
          m_stats.rxClose++;
           NS_ASSERT (actionValue.selfProtectedAction == WifiActionHeader::PEER_LINK_CLOSE); 
        }
      //Deliver Huper link management frame to protocol:
      m_protocol->ReceiveHuperLinkFrame (m_ifIndex, huperAddress, huperMpAddress, fields.aid, *huperElement,
                                        fields.config);
      // if we can handle a frame - drop it
      return false;
    }
  return m_protocol->IsActiveLink (m_ifIndex, header.GetAddr2 ());
}
bool
HuperManagementProtocolMac::UpdateOutcomingFrame (Ptr<Packet> packet, WifiMacHeader & header,
                                                 Mac48Address from, Mac48Address to)
{
  if (header.IsAction ())
    {
      WifiActionHeader actionHdr;
      packet->PeekHeader (actionHdr);
      if (actionHdr.GetCategory () == WifiActionHeader::SELF_PROTECTED) 
        {
          return true;
        }
    }
  if (header.GetAddr1 ().IsGroup ())
    {
      return true;
    }
  else
    {
      if (m_protocol->IsActiveLink (m_ifIndex, header.GetAddr1 ()))
        {
          return true;
        }
      else
        {
          m_stats.dropped++;
          return false;
        }
    }
}
void
HuperManagementProtocolMac::UpdateBeacon (WmnWifiBeacon & beacon) const
{
  if (m_protocol->GetBeaconCollisionAvoidance ())
    {
      Ptr<IeBeaconTiming> beaconTiming = m_protocol->GetBeaconTimingElement (m_ifIndex);
      beacon.AddInformationElement (beaconTiming);
    }
  beacon.AddInformationElement (m_protocol->GetWmnId ());
  m_protocol->NotifyBeaconSent (m_ifIndex, beacon.GetBeaconInterval ());
}

void
HuperManagementProtocolMac::SendHuperLinkManagementFrame (Mac48Address huperAddress, Mac48Address huperMpAddress,
                                                        uint16_t aid, IeHuperManagement huperElement, IeConfiguration wmnConfig)
{
  //Create a packet:
  wmnConfig.SetNeighborCount (m_protocol->GetNumberOfLinks ());
  Ptr<Packet> packet = Create<Packet> ();
  WmnInformationElementVector elements;
  elements.AddInformationElement (Ptr<IeHuperManagement> (&huperElement));
  packet->AddHeader (elements);
  HuperLinkFrameStart::PlinkFrameStartFields fields;
  fields.rates = m_parent->GetSupportedRates ();
  fields.capability = 0;
  fields.wmnId = *(m_protocol->GetWmnId ());
  fields.config = wmnConfig;
  HuperLinkFrameStart plinkFrame;
  //Create an 802.11 frame header:
  //Send management frame to MAC:
  WifiActionHeader actionHdr;
  if (huperElement.SubtypeIsOpen ())
    {
      m_stats.txOpen++;
      WifiActionHeader::ActionValue action;
      action.selfProtectedAction = WifiActionHeader::PEER_LINK_OPEN;
      fields.subtype = WifiActionHeader::PEER_LINK_OPEN;
      actionHdr.SetAction (WifiActionHeader::SELF_PROTECTED, action); 
    }
  if (huperElement.SubtypeIsConfirm ())
    {
      m_stats.txConfirm++;
      WifiActionHeader::ActionValue action;
       action.selfProtectedAction = WifiActionHeader::PEER_LINK_CONFIRM; 
      fields.aid = aid;
      fields.subtype = WifiActionHeader::PEER_LINK_CONFIRM;
      actionHdr.SetAction (WifiActionHeader::SELF_PROTECTED, action); 
    }
  if (huperElement.SubtypeIsClose ())
    {
      m_stats.txClose++;
      WifiActionHeader::ActionValue action;
      action.selfProtectedAction = WifiActionHeader::PEER_LINK_CLOSE; 
      fields.subtype = WifiActionHeader::PEER_LINK_CLOSE;
      fields.reasonCode = huperElement.GetReasonCode ();
      actionHdr.SetAction (WifiActionHeader::SELF_PROTECTED, action); 
    }
  plinkFrame.SetPlinkFrameStart (fields);
  packet->AddHeader (plinkFrame);
  packet->AddHeader (actionHdr);
  m_stats.txMgt++;
  m_stats.txMgtBytes += packet->GetSize ();
  // Wifi Mac header:
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (huperAddress);
  hdr.SetAddr2 (m_parent->GetAddress ());
  //Addr is not used here, we use it as our MP address
  hdr.SetAddr3 (m_protocol->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  m_parent->SendManagementFrame (packet, hdr);
}

Mac48Address
HuperManagementProtocolMac::GetAddress () const
{
  if (m_parent != 0)
    {
      return m_parent->GetAddress ();
    }
  else
    {
      return Mac48Address ();
    }
}
void
HuperManagementProtocolMac::SetBeaconShift (Time shift)
{
  if (shift != Seconds (0))
    {
      m_stats.beaconShift++;
    }
  m_parent->ShiftTbtt (shift);
}
HuperManagementProtocolMac::Statistics::Statistics () :
  txOpen (0), txConfirm (0), txClose (0), rxOpen (0), rxConfirm (0), rxClose (0), dropped (0), brokenMgt (0),
  txMgt (0), txMgtBytes (0), rxMgt (0), rxMgtBytes (0), beaconShift (0)
{
}
void
HuperManagementProtocolMac::Statistics::Print (std::ostream & os) const
{
  os << "<Statistics "
  "txOpen=\"" << txOpen << "\"" << std::endl <<
  "txConfirm=\"" << txConfirm << "\"" << std::endl <<
  "txClose=\"" << txClose << "\"" << std::endl <<
  "rxOpen=\"" << rxOpen << "\"" << std::endl <<
  "rxConfirm=\"" << rxConfirm << "\"" << std::endl <<
  "rxClose=\"" << rxClose << "\"" << std::endl <<
  "dropped=\"" << dropped << "\"" << std::endl <<
  "brokenMgt=\"" << brokenMgt << "\"" << std::endl <<
  "txMgt=\"" << txMgt << "\"" << std::endl <<
  "txMgtBytes=\"" << txMgtBytes << "\"" << std::endl <<
  "rxMgt=\"" << rxMgt << "\"" << std::endl <<
  "rxMgtBytes=\"" << rxMgtBytes << "\"" << std::endl <<
  "beaconShift=\"" << beaconShift << "\"/>" << std::endl;
}
void
HuperManagementProtocolMac::Report (std::ostream & os) const
{
  os << "<HuperManagementProtocolMac "
  "address=\"" << m_parent->GetAddress () << "\">" << std::endl;
  m_stats.Print (os);
  os << "</HuperManagementProtocolMac>" << std::endl;
}
void
HuperManagementProtocolMac::ResetStats ()
{
  m_stats = Statistics ();
}
uint32_t
HuperManagementProtocolMac::GetLinkMetric (Mac48Address huperAddress)
{
  return m_parent->GetLinkMetric (huperAddress);
}
int64_t
HuperManagementProtocolMac::AssignStreams (int64_t stream)
{ 
  return m_protocol->AssignStreams (stream);
}

} // namespace hu11s
} // namespace ns3

