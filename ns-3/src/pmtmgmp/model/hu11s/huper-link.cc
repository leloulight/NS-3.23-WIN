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
 *          Aleksey Kovalenko <kovalenko@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "huper-management-protocol-mac.h"
#include "ns3/huper-link.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/traced-value.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Hu11sHuperManagementProtocol");
  
namespace hu11s {

NS_OBJECT_ENSURE_REGISTERED ( HuperLink);

TypeId
HuperLink::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::hu11s::HuperLink")
    .SetParent<Object> ()
    .SetGroupName ("Wmn")
    .AddConstructor<HuperLink> ()
    .AddAttribute ( "RetryTimeout",
                    "Retry timeout",
                    TimeValue (TimeValue (MicroSeconds (40 * 1024))),
                    MakeTimeAccessor (
                      &HuperLink::m_dot11WmnRetryTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "HoldingTimeout",
                    "Holding timeout",
                    TimeValue (TimeValue (MicroSeconds (40 * 1024))),
                    MakeTimeAccessor (
                      &HuperLink::m_dot11WmnHoldingTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "ConfirmTimeout",
                    "Confirm timeout",
                    TimeValue (TimeValue (MicroSeconds (40 * 1024))),
                    MakeTimeAccessor (
                      &HuperLink::m_dot11WmnConfirmTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "MaxRetries",
                    "Maximum number of retries",
                    UintegerValue (4),
                    MakeUintegerAccessor (
                      &HuperLink::m_dot11WmnMaxRetries),
                    MakeUintegerChecker<uint16_t> ()
                    )
    .AddAttribute ( "MaxBeaconLoss",
                    "Maximum number of lost beacons before link will be closed",
                    UintegerValue (2),
                    MakeUintegerAccessor (
                      &HuperLink::m_maxBeaconLoss),
                    MakeUintegerChecker<uint16_t> (1)
                    )
    .AddAttribute ( "MaxPacketFailure",
                    "Maximum number of failed packets before link will be closed",
                    UintegerValue (2),
                    MakeUintegerAccessor (
                      &HuperLink::m_maxPacketFail),
                    MakeUintegerChecker<uint16_t> (1)
                    )
  ;
  return tid;
}


//-----------------------------------------------------------------------------
// HuperLink public interface
//-----------------------------------------------------------------------------
HuperLink::HuperLink () :
  m_huperAddress (Mac48Address::GetBroadcast ()),
  m_huperWmnPointAddress (Mac48Address::GetBroadcast ()),
  m_localLinkId (0),
  m_huperLinkId (0),
  m_assocId (0),
  m_huperAssocId (0),
  m_lastBeacon (Seconds (0)),
  m_beaconInterval (Seconds (0)),
  m_packetFail (0),
  m_state (IDLE),
  m_retryCounter (0),
  m_maxPacketFail (3)
{
}
HuperLink::~HuperLink ()
{
}
void
HuperLink::DoDispose ()
{
  m_retryTimer.Cancel ();
  m_holdingTimer.Cancel ();
  m_confirmTimer.Cancel ();
  m_beaconLossTimer.Cancel ();
  m_beaconTiming.ClearTimingElement ();
}
void
HuperLink::SetHuperAddress (Mac48Address macaddr)
{
  m_huperAddress = macaddr;
}
void
HuperLink::SetHuperWmnPointAddress (Mac48Address macaddr)
{
  m_huperWmnPointAddress = macaddr;
}
void
HuperLink::SetInterface (uint32_t interface)
{
  m_interface = interface;
}
void
HuperLink::SetLocalLinkId (uint16_t id)
{
  m_localLinkId = id;
}
void
HuperLink::SetLocalAid (uint16_t aid)
{
  m_assocId = aid;
}
void
HuperLink::SetBeaconInformation (Time lastBeacon, Time beaconInterval)
{
  m_lastBeacon = lastBeacon;
  m_beaconInterval = beaconInterval;
  m_beaconLossTimer.Cancel ();
  Time delay = Seconds (beaconInterval.GetSeconds () * m_maxBeaconLoss);
  NS_ASSERT (delay.GetMicroSeconds () != 0);
  m_beaconLossTimer = Simulator::Schedule (delay, &HuperLink::BeaconLoss, this);
}
void
HuperLink::MLMESetSignalStatusCallback (HuperLink::SignalStatusCallback cb)
{
  m_linkStatusCallback = cb;
}
void
HuperLink::BeaconLoss ()
{
  StateMachine (CNCL);
}
void
HuperLink::TransmissionSuccess ()
{
  m_packetFail = 0;
}
void
HuperLink::TransmissionFailure ()
{
  m_packetFail++;
  if (m_packetFail == m_maxPacketFail)
    {
      StateMachine (CNCL);
      m_packetFail = 0;
    }
}

void
HuperLink::SetBeaconTimingElement (IeBeaconTiming beaconTiming)
{
  m_beaconTiming = beaconTiming;
}
Mac48Address
HuperLink::GetHuperAddress () const
{
  return m_huperAddress;
}
uint16_t
HuperLink::GetLocalAid () const
{
  return m_assocId;
}
uint16_t
HuperLink::GetHuperAid () const
{
  return m_huperAssocId;
}

Time
HuperLink::GetLastBeacon () const
{
  return m_lastBeacon;
}
Time
HuperLink::GetBeaconInterval () const
{
  return m_beaconInterval;
}
IeBeaconTiming
HuperLink::GetBeaconTimingElement () const
{
  return m_beaconTiming;
}
void
HuperLink::MLMECancelHuperLink (PmpReasonCode reason)
{
  StateMachine (CNCL, reason);
}
void
HuperLink::MLMEActiveHuperLinkOpen ()
{
  StateMachine (ACTOPN);
}
void
HuperLink::MLMEHuperingRequestReject ()
{
  StateMachine (REQ_RJCT, REASON11S_HUPERING_CANCELLED);
}
void
HuperLink::Close (uint16_t localLinkId, uint16_t huperLinkId, PmpReasonCode reason)
{
  if (huperLinkId != 0 && m_localLinkId != huperLinkId)
    {
      return;
    }
  if (m_huperLinkId == 0)
    {
      m_huperLinkId = localLinkId;
    }
  else
    {
      if (m_huperLinkId != localLinkId)
        {
          return;
        }
    }
  StateMachine (CLS_ACPT, reason);
}
void
HuperLink::OpenAccept (uint16_t localLinkId, IeConfiguration conf, Mac48Address huperMp)
{
  m_huperLinkId = localLinkId;
  m_configuration = conf;
  if (m_huperWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_huperWmnPointAddress == huperMp);
    }
  else
    {
      m_huperWmnPointAddress = huperMp;
    }
  StateMachine (OPN_ACPT);
}
void
HuperLink::OpenReject (uint16_t localLinkId, IeConfiguration conf, Mac48Address huperMp, PmpReasonCode reason)
{
  if (m_huperLinkId == 0)
    {
      m_huperLinkId = localLinkId;
    }
  m_configuration = conf;
  if (m_huperWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_huperWmnPointAddress == huperMp);
    }
  else
    {
      m_huperWmnPointAddress = huperMp;
    }
  StateMachine (OPN_RJCT, reason);
}
void
HuperLink::ConfirmAccept (uint16_t localLinkId, uint16_t huperLinkId, uint16_t huperAid, IeConfiguration conf,
                         Mac48Address huperMp)
{
  if (m_localLinkId != huperLinkId)
    {
      return;
    }
  if (m_huperLinkId == 0)
    {
      m_huperLinkId = localLinkId;
    }
  else
    {
      if (m_huperLinkId != localLinkId)
        {
          return;
        }
    }
  m_configuration = conf;
  m_huperAssocId = huperAid;
  if (m_huperWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_huperWmnPointAddress == huperMp);
    }
  else
    {
      m_huperWmnPointAddress = huperMp;
    }
  StateMachine (CNF_ACPT);
}
void
HuperLink::ConfirmReject (uint16_t localLinkId, uint16_t huperLinkId, IeConfiguration conf,
                         Mac48Address huperMp, PmpReasonCode reason)
{
  if (m_localLinkId != huperLinkId)
    {
      return;
    }
  if (m_huperLinkId == 0)
    {
      m_huperLinkId = localLinkId;
    }
  else
    {
      if (m_huperLinkId != localLinkId)
        {
          return;
        }
    }
  m_configuration = conf;
  if (m_huperWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_huperWmnPointAddress == huperMp);
    }
  m_huperWmnPointAddress = huperMp;
  StateMachine (CNF_RJCT, reason);
}
bool
HuperLink::LinkIsEstab () const
{
  return (m_state == ESTAB);
}
bool
HuperLink::LinkIsIdle () const
{
  return (m_state == IDLE);
}
void
HuperLink::SetMacPlugin (Ptr<HuperManagementProtocolMac> plugin)
{
  m_macPlugin = plugin;
}
//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------
void
HuperLink::StateMachine (HuperEvent event, PmpReasonCode reasoncode)
{
  switch (m_state)
    {
    case IDLE:
      switch (event)
        {
        case CNCL:
        case CLS_ACPT:
          m_state = IDLE;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, IDLE, IDLE);
          break;
        case REQ_RJCT:
          SendHuperLinkClose (reasoncode);
          break;
        case ACTOPN:
          m_state = OPN_SNT;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, IDLE, OPN_SNT);
          SendHuperLinkOpen ();
          SetRetryTimer ();
          break;
        case OPN_ACPT:
          m_state = OPN_RCVD;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, IDLE, OPN_RCVD);
          SendHuperLinkConfirm ();
          SendHuperLinkOpen ();
          SetRetryTimer ();
          break;
        default:
          //11B.5.3.4 of 802.11s Draft D3.0
          //All other events shall be ignored in this state
          break;
        }
      break;
    case OPN_SNT:
      switch (event)
        {
        case TOR1:
          SendHuperLinkOpen ();
          m_retryCounter++;
          SetRetryTimer ();
          break;
        case CNF_ACPT:
          m_state = CNF_RCVD;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_SNT, CNF_RCVD);
          ClearRetryTimer ();
          SetConfirmTimer ();
          break;
        case OPN_ACPT:
          m_state = OPN_RCVD;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_SNT, OPN_RCVD);
          SendHuperLinkConfirm ();
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case TOR2:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (REASON11S_WMN_MAX_RETRIES);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (REASON11S_HUPERING_CANCELLED);
          SetHoldingTimer ();
          break;
        default:
          //11B.5.3.5 of 802.11s Draft D3.0
          //All other events shall be ignored in this state
          break;
        }
      break;
    case CNF_RCVD:
      switch (event)
        {
        case CNF_ACPT:
          break;
        case OPN_ACPT:
          m_state = ESTAB;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, CNF_RCVD, ESTAB);
          ClearConfirmTimer ();
          SendHuperLinkConfirm ();
          NS_ASSERT (m_huperWmnPointAddress != Mac48Address::GetBroadcast ());
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, CNF_RCVD, HOLDING);
          ClearConfirmTimer ();
          SendHuperLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case CNF_RJCT:
        case OPN_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, CNF_RCVD, HOLDING);
          ClearConfirmTimer ();
          SendHuperLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, CNF_RCVD, HOLDING);
          ClearConfirmTimer ();
          SendHuperLinkClose (REASON11S_HUPERING_CANCELLED);
          SetHoldingTimer ();
          break;
        case TOC:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, CNF_RCVD, HOLDING);
          SendHuperLinkClose (REASON11S_WMN_CONFIRM_TIMEOUT);
          SetHoldingTimer ();
          break;
        default:
          //11B.5.3.6 of 802.11s Draft D3.0
          //All other events shall be ignored in this state
          break;
        }
      break;
    case OPN_RCVD:
      switch (event)
        {
        case TOR1:
          SendHuperLinkOpen ();
          m_retryCounter++;
          SetRetryTimer ();
          break;
        case CNF_ACPT:
          m_state = ESTAB;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_RCVD, ESTAB);
          ClearRetryTimer ();
          NS_ASSERT (m_huperWmnPointAddress != Mac48Address::GetBroadcast ());
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case TOR2:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (REASON11S_WMN_MAX_RETRIES);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (REASON11S_HUPERING_CANCELLED);
          SetHoldingTimer ();
          break;
        default:
          //11B.5.3.7 of 802.11s Draft D3.0
          //All other events shall be ignored in this state
          break;
        }
      break;
    case ESTAB:
      switch (event)
        {
        case OPN_ACPT:
          SendHuperLinkConfirm ();
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, ESTAB, HOLDING);
          SendHuperLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, ESTAB, HOLDING);
          ClearRetryTimer ();
          SendHuperLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, ESTAB, HOLDING);
          SendHuperLinkClose (REASON11S_HUPERING_CANCELLED);
          SetHoldingTimer ();
          break;
        default:
          //11B.5.3.8 of 802.11s Draft D3.0
          //All other events shall be ignored in this state
          break;
        }
      break;
    case HOLDING:
      switch (event)
        {
        case CLS_ACPT:
          ClearHoldingTimer ();
          // fall through:
        case TOH:
          m_state = IDLE;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, HOLDING, IDLE);
          break;
        case OPN_ACPT:
        case CNF_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, HOLDING, HOLDING);
          // reason not spec in D2.0
          SendHuperLinkClose (REASON11S_HUPERING_CANCELLED);
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_huperAddress, m_huperWmnPointAddress, HOLDING, HOLDING);
          SendHuperLinkClose (reasoncode);
          break;
        default:
          //11B.5.3.9 of 802.11s Draft D3.0
          //All other events shall be ignored in this state
          break;
        }
      break;
    }
}
void
HuperLink::ClearRetryTimer ()
{
  m_retryTimer.Cancel ();
}
void
HuperLink::ClearConfirmTimer ()
{
  m_confirmTimer.Cancel ();
}
void
HuperLink::ClearHoldingTimer ()
{
  m_holdingTimer.Cancel ();
}
void
HuperLink::SendHuperLinkClose (PmpReasonCode reasoncode)
{
  IeHuperManagement huperElement;
  huperElement.SetHuperClose (m_localLinkId, m_huperLinkId, reasoncode);
  m_macPlugin->SendHuperLinkManagementFrame (m_huperAddress, m_huperWmnPointAddress, m_assocId, huperElement,
                                            m_configuration);
}
void
HuperLink::SendHuperLinkOpen ()
{
  IeHuperManagement huperElement;
  huperElement.SetHuperOpen (m_localLinkId);
  NS_ASSERT (m_macPlugin != 0);
  m_macPlugin->SendHuperLinkManagementFrame (m_huperAddress, m_huperWmnPointAddress, m_assocId, huperElement,
                                            m_configuration);
}
void
HuperLink::SendHuperLinkConfirm ()
{
  IeHuperManagement huperElement;
  huperElement.SetHuperConfirm (m_localLinkId, m_huperLinkId);
  m_macPlugin->SendHuperLinkManagementFrame (m_huperAddress, m_huperWmnPointAddress, m_assocId, huperElement,
                                            m_configuration);
}
void
HuperLink::SetHoldingTimer ()
{
  NS_ASSERT (m_dot11WmnHoldingTimeout.GetMicroSeconds () != 0);
  m_holdingTimer = Simulator::Schedule (m_dot11WmnHoldingTimeout, &HuperLink::HoldingTimeout, this);
}
void
HuperLink::HoldingTimeout ()
{
  StateMachine (TOH);
}
void
HuperLink::SetRetryTimer ()
{
  NS_ASSERT (m_dot11WmnRetryTimeout.GetMicroSeconds () != 0);
  m_retryTimer = Simulator::Schedule (m_dot11WmnRetryTimeout, &HuperLink::RetryTimeout, this);
}
void
HuperLink::RetryTimeout ()
{
  if (m_retryCounter < m_dot11WmnMaxRetries)
    {
      StateMachine (TOR1);
    }
  else
    {
      StateMachine (TOR2);
    }
}
void
HuperLink::SetConfirmTimer ()
{
  NS_ASSERT (m_dot11WmnConfirmTimeout.GetMicroSeconds () != 0);
  m_confirmTimer = Simulator::Schedule (m_dot11WmnConfirmTimeout, &HuperLink::ConfirmTimeout, this);
}
void
HuperLink::ConfirmTimeout ()
{
  StateMachine (TOC);
}
void
HuperLink::Report (std::ostream & os) const
{
  if (m_state != ESTAB)
    {
      return;
    }
  os << "<HuperLink" << std::endl <<
  "localAddress=\"" << m_macPlugin->GetAddress () << "\"" << std::endl <<
  "huperInterfaceAddress=\"" << m_huperAddress << "\"" << std::endl <<
  "huperWmnPointAddress=\"" << m_huperWmnPointAddress << "\"" << std::endl <<
  "metric=\"" << m_macPlugin->GetLinkMetric (m_huperAddress) << "\"" << std::endl <<
  "lastBeacon=\"" << m_lastBeacon.GetSeconds () << "\"" << std::endl <<
  "localLinkId=\"" << m_localLinkId << "\"" << std::endl <<
  "huperLinkId=\"" << m_huperLinkId << "\"" << std::endl <<
  "assocId=\"" << m_assocId << "\"" << std::endl <<
  "/>" << std::endl;
}
} // namespace hu11s
} // namespace ns3

