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

#include "pmtmgmp-peer-management-protocol-mac.h"
#include "ns3/pmtmgmp-peer-link.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/traced-value.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("My11sPmtmgmpPeerManagementProtocol");
  
namespace my11s {

NS_OBJECT_ENSURE_REGISTERED ( PmtmgmpPeerLink);

TypeId
PmtmgmpPeerLink::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::my11s::PmtmgmpPeerLink")
    .SetParent<Object> ()
    .SetGroupName ("Wmn")
    .AddConstructor<PmtmgmpPeerLink> ()
    .AddAttribute ( "RetryTimeout",
                    "Retry timeout",
                    TimeValue (TimeValue (MicroSeconds (40 * 1024))),
                    MakeTimeAccessor (
                      &PmtmgmpPeerLink::m_dot11WmnRetryTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "HoldingTimeout",
                    "Holding timeout",
                    TimeValue (TimeValue (MicroSeconds (40 * 1024))),
                    MakeTimeAccessor (
                      &PmtmgmpPeerLink::m_dot11WmnHoldingTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "ConfirmTimeout",
                    "Confirm timeout",
                    TimeValue (TimeValue (MicroSeconds (40 * 1024))),
                    MakeTimeAccessor (
                      &PmtmgmpPeerLink::m_dot11WmnConfirmTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "MaxRetries",
                    "Maximum number of retries",
                    UintegerValue (4),
                    MakeUintegerAccessor (
                      &PmtmgmpPeerLink::m_dot11WmnMaxRetries),
                    MakeUintegerChecker<uint16_t> ()
                    )
    .AddAttribute ( "MaxBeaconLoss",
                    "Maximum number of lost beacons before link will be closed",
                    UintegerValue (2),
                    MakeUintegerAccessor (
                      &PmtmgmpPeerLink::m_maxBeaconLoss),
                    MakeUintegerChecker<uint16_t> (1)
                    )
    .AddAttribute ( "MaxPacketFailure",
                    "Maximum number of failed packets before link will be closed",
                    UintegerValue (2),
                    MakeUintegerAccessor (
                      &PmtmgmpPeerLink::m_maxPacketFail),
                    MakeUintegerChecker<uint16_t> (1)
                    )
  ;
  return tid;
}


//-----------------------------------------------------------------------------
// PmtmgmpPeerLink public interface
//-----------------------------------------------------------------------------
PmtmgmpPeerLink::PmtmgmpPeerLink () :
  m_peerAddress (Mac48Address::GetBroadcast ()),
  m_peerWmnPointAddress (Mac48Address::GetBroadcast ()),
  m_localLinkId (0),
  m_PmtmgmpPeerLinkId (0),
  m_assocId (0),
  m_peerAssocId (0),
  m_lastBeacon (Seconds (0)),
  m_beaconInterval (Seconds (0)),
  m_packetFail (0),
  m_state (IDLE),
  m_retryCounter (0),
  m_maxPacketFail (3)
{
}
PmtmgmpPeerLink::~PmtmgmpPeerLink ()
{
}
void
PmtmgmpPeerLink::DoDispose ()
{
  m_retryTimer.Cancel ();
  m_holdingTimer.Cancel ();
  m_confirmTimer.Cancel ();
  m_beaconLossTimer.Cancel ();
  m_beaconTiming.ClearTimingElement ();
}
void
PmtmgmpPeerLink::SetPeerAddress (Mac48Address macaddr)
{
  m_peerAddress = macaddr;
}
void
PmtmgmpPeerLink::SetPeerWmnPointAddress (Mac48Address macaddr)
{
  m_peerWmnPointAddress = macaddr;
}
void
PmtmgmpPeerLink::SetInterface (uint32_t interface)
{
  m_interface = interface;
}
void
PmtmgmpPeerLink::SetLocalLinkId (uint16_t id)
{
  m_localLinkId = id;
}
void
PmtmgmpPeerLink::SetLocalAid (uint16_t aid)
{
  m_assocId = aid;
}
void
PmtmgmpPeerLink::SetBeaconInformation (Time lastBeacon, Time beaconInterval)
{
  m_lastBeacon = lastBeacon;
  m_beaconInterval = beaconInterval;
  m_beaconLossTimer.Cancel ();
  Time delay = Seconds (beaconInterval.GetSeconds () * m_maxBeaconLoss);
  NS_ASSERT (delay.GetMicroSeconds () != 0);
  m_beaconLossTimer = Simulator::Schedule (delay, &PmtmgmpPeerLink::BeaconLoss, this);
}
void
PmtmgmpPeerLink::MLMESetSignalStatusCallback (PmtmgmpPeerLink::SignalStatusCallback cb)
{
  m_linkStatusCallback = cb;
}
void
PmtmgmpPeerLink::BeaconLoss ()
{
  StateMachine (CNCL);
}
void
PmtmgmpPeerLink::TransmissionSuccess ()
{
  m_packetFail = 0;
}
void
PmtmgmpPeerLink::TransmissionFailure ()
{
  m_packetFail++;
  if (m_packetFail == m_maxPacketFail)
    {
      StateMachine (CNCL);
      m_packetFail = 0;
    }
}

void
PmtmgmpPeerLink::SetBeaconTimingElement (IeBeaconTiming beaconTiming)
{
  m_beaconTiming = beaconTiming;
}
Mac48Address
PmtmgmpPeerLink::GetPeerAddress () const
{
  return m_peerAddress;
}
uint16_t
PmtmgmpPeerLink::GetLocalAid () const
{
  return m_assocId;
}
uint16_t
PmtmgmpPeerLink::GetPeerAid () const
{
  return m_peerAssocId;
}

Time
PmtmgmpPeerLink::GetLastBeacon () const
{
  return m_lastBeacon;
}
Time
PmtmgmpPeerLink::GetBeaconInterval () const
{
  return m_beaconInterval;
}
IeBeaconTiming
PmtmgmpPeerLink::GetBeaconTimingElement () const
{
  return m_beaconTiming;
}
void
PmtmgmpPeerLink::MLMECancelPmtmgmpPeerLink (PmpReasonCode reason)
{
  StateMachine (CNCL, reason);
}
void
PmtmgmpPeerLink::MLMEActivePmtmgmpPeerLinkOpen ()
{
  StateMachine (ACTOPN);
}
void
PmtmgmpPeerLink::MLMEPeeringRequestReject ()
{
  StateMachine (REQ_RJCT, REASON11S_PMTMGMP_PEERING_CANCELLED);
}
void
PmtmgmpPeerLink::Close (uint16_t localLinkId, uint16_t PmtmgmpPeerLinkId, PmpReasonCode reason)
{
  if (PmtmgmpPeerLinkId != 0 && m_localLinkId != PmtmgmpPeerLinkId)
    {
      return;
    }
  if (m_PmtmgmpPeerLinkId == 0)
    {
      m_PmtmgmpPeerLinkId = localLinkId;
    }
  else
    {
      if (m_PmtmgmpPeerLinkId != localLinkId)
        {
          return;
        }
    }
  StateMachine (CLS_ACPT, reason);
}
void
PmtmgmpPeerLink::OpenAccept (uint16_t localLinkId, IeConfiguration conf, Mac48Address peerMp)
{
  m_PmtmgmpPeerLinkId = localLinkId;
  m_configuration = conf;
  if (m_peerWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_peerWmnPointAddress == peerMp);
    }
  else
    {
      m_peerWmnPointAddress = peerMp;
    }
  StateMachine (OPN_ACPT);
}
void
PmtmgmpPeerLink::OpenReject (uint16_t localLinkId, IeConfiguration conf, Mac48Address peerMp, PmpReasonCode reason)
{
  if (m_PmtmgmpPeerLinkId == 0)
    {
      m_PmtmgmpPeerLinkId = localLinkId;
    }
  m_configuration = conf;
  if (m_peerWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_peerWmnPointAddress == peerMp);
    }
  else
    {
      m_peerWmnPointAddress = peerMp;
    }
  StateMachine (OPN_RJCT, reason);
}
void
PmtmgmpPeerLink::ConfirmAccept (uint16_t localLinkId, uint16_t PmtmgmpPeerLinkId, uint16_t peerAid, IeConfiguration conf,
                         Mac48Address peerMp)
{
  if (m_localLinkId != PmtmgmpPeerLinkId)
    {
      return;
    }
  if (m_PmtmgmpPeerLinkId == 0)
    {
      m_PmtmgmpPeerLinkId = localLinkId;
    }
  else
    {
      if (m_PmtmgmpPeerLinkId != localLinkId)
        {
          return;
        }
    }
  m_configuration = conf;
  m_peerAssocId = peerAid;
  if (m_peerWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_peerWmnPointAddress == peerMp);
    }
  else
    {
      m_peerWmnPointAddress = peerMp;
    }
  StateMachine (CNF_ACPT);
}
void
PmtmgmpPeerLink::ConfirmReject (uint16_t localLinkId, uint16_t PmtmgmpPeerLinkId, IeConfiguration conf,
                         Mac48Address peerMp, PmpReasonCode reason)
{
  if (m_localLinkId != PmtmgmpPeerLinkId)
    {
      return;
    }
  if (m_PmtmgmpPeerLinkId == 0)
    {
      m_PmtmgmpPeerLinkId = localLinkId;
    }
  else
    {
      if (m_PmtmgmpPeerLinkId != localLinkId)
        {
          return;
        }
    }
  m_configuration = conf;
  if (m_peerWmnPointAddress != Mac48Address::GetBroadcast ())
    {
      NS_ASSERT (m_peerWmnPointAddress == peerMp);
    }
  m_peerWmnPointAddress = peerMp;
  StateMachine (CNF_RJCT, reason);
}
bool
PmtmgmpPeerLink::LinkIsEstab () const
{
  return (m_state == ESTAB);
}
bool
PmtmgmpPeerLink::LinkIsIdle () const
{
  return (m_state == IDLE);
}
void
PmtmgmpPeerLink::SetMacPlugin (Ptr<PmtmgmpPeerManagementProtocolMac> plugin)
{
  m_macPlugin = plugin;
}
//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------
void
PmtmgmpPeerLink::StateMachine (PeerEvent event, PmpReasonCode reasoncode)
{
  switch (m_state)
    {
    case IDLE:
      switch (event)
        {
        case CNCL:
        case CLS_ACPT:
          m_state = IDLE;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, IDLE, IDLE);
          break;
        case REQ_RJCT:
          SendPmtmgmpPeerLinkClose (reasoncode);
          break;
        case ACTOPN:
          m_state = OPN_SNT;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, IDLE, OPN_SNT);
          SendPmtmgmpPeerLinkOpen ();
          SetRetryTimer ();
          break;
        case OPN_ACPT:
          m_state = OPN_RCVD;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, IDLE, OPN_RCVD);
          SendPmtmgmpPeerLinkConfirm ();
          SendPmtmgmpPeerLinkOpen ();
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
          SendPmtmgmpPeerLinkOpen ();
          m_retryCounter++;
          SetRetryTimer ();
          break;
        case CNF_ACPT:
          m_state = CNF_RCVD;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_SNT, CNF_RCVD);
          ClearRetryTimer ();
          SetConfirmTimer ();
          break;
        case OPN_ACPT:
          m_state = OPN_RCVD;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_SNT, OPN_RCVD);
          SendPmtmgmpPeerLinkConfirm ();
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case TOR2:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_MAX_RETRIES);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_SNT, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_PMTMGMP_PEERING_CANCELLED);
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
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, CNF_RCVD, ESTAB);
          ClearConfirmTimer ();
          SendPmtmgmpPeerLinkConfirm ();
          NS_ASSERT (m_peerWmnPointAddress != Mac48Address::GetBroadcast ());
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, CNF_RCVD, HOLDING);
          ClearConfirmTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case CNF_RJCT:
        case OPN_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, CNF_RCVD, HOLDING);
          ClearConfirmTimer ();
          SendPmtmgmpPeerLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, CNF_RCVD, HOLDING);
          ClearConfirmTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_PMTMGMP_PEERING_CANCELLED);
          SetHoldingTimer ();
          break;
        case TOC:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, CNF_RCVD, HOLDING);
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_CONFIRM_TIMEOUT);
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
          SendPmtmgmpPeerLinkOpen ();
          m_retryCounter++;
          SetRetryTimer ();
          break;
        case CNF_ACPT:
          m_state = ESTAB;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_RCVD, ESTAB);
          ClearRetryTimer ();
          NS_ASSERT (m_peerWmnPointAddress != Mac48Address::GetBroadcast ());
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case TOR2:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_MAX_RETRIES);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, OPN_RCVD, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (REASON11S_PMTMGMP_PEERING_CANCELLED);
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
          SendPmtmgmpPeerLinkConfirm ();
          break;
        case CLS_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, ESTAB, HOLDING);
          SendPmtmgmpPeerLinkClose (REASON11S_WMN_CLOSE_RCVD);
          SetHoldingTimer ();
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, ESTAB, HOLDING);
          ClearRetryTimer ();
          SendPmtmgmpPeerLinkClose (reasoncode);
          SetHoldingTimer ();
          break;
        case CNCL:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, ESTAB, HOLDING);
          SendPmtmgmpPeerLinkClose (REASON11S_PMTMGMP_PEERING_CANCELLED);
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
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, HOLDING, IDLE);
          break;
        case OPN_ACPT:
        case CNF_ACPT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, HOLDING, HOLDING);
          // reason not spec in D2.0
          SendPmtmgmpPeerLinkClose (REASON11S_PMTMGMP_PEERING_CANCELLED);
          break;
        case OPN_RJCT:
        case CNF_RJCT:
          m_state = HOLDING;
          m_linkStatusCallback (m_interface, m_peerAddress, m_peerWmnPointAddress, HOLDING, HOLDING);
          SendPmtmgmpPeerLinkClose (reasoncode);
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
PmtmgmpPeerLink::ClearRetryTimer ()
{
  m_retryTimer.Cancel ();
}
void
PmtmgmpPeerLink::ClearConfirmTimer ()
{
  m_confirmTimer.Cancel ();
}
void
PmtmgmpPeerLink::ClearHoldingTimer ()
{
  m_holdingTimer.Cancel ();
}
void
PmtmgmpPeerLink::SendPmtmgmpPeerLinkClose (PmpReasonCode reasoncode)
{
  IePeerManagement peerElement;
  peerElement.SetPeerClose (m_localLinkId, m_PmtmgmpPeerLinkId, reasoncode);
  m_macPlugin->SendPmtmgmpPeerLinkManagementFrame (m_peerAddress, m_peerWmnPointAddress, m_assocId, peerElement,
                                            m_configuration);
}
void
PmtmgmpPeerLink::SendPmtmgmpPeerLinkOpen ()
{
  IePeerManagement peerElement;
  peerElement.SetPeerOpen (m_localLinkId);
  NS_ASSERT (m_macPlugin != 0);
  m_macPlugin->SendPmtmgmpPeerLinkManagementFrame (m_peerAddress, m_peerWmnPointAddress, m_assocId, peerElement,
                                            m_configuration);
}
void
PmtmgmpPeerLink::SendPmtmgmpPeerLinkConfirm ()
{
  IePeerManagement peerElement;
  peerElement.SetPeerConfirm (m_localLinkId, m_PmtmgmpPeerLinkId);
  m_macPlugin->SendPmtmgmpPeerLinkManagementFrame (m_peerAddress, m_peerWmnPointAddress, m_assocId, peerElement,
                                            m_configuration);
}
void
PmtmgmpPeerLink::SetHoldingTimer ()
{
  NS_ASSERT (m_dot11WmnHoldingTimeout.GetMicroSeconds () != 0);
  m_holdingTimer = Simulator::Schedule (m_dot11WmnHoldingTimeout, &PmtmgmpPeerLink::HoldingTimeout, this);
}
void
PmtmgmpPeerLink::HoldingTimeout ()
{
  StateMachine (TOH);
}
void
PmtmgmpPeerLink::SetRetryTimer ()
{
  NS_ASSERT (m_dot11WmnRetryTimeout.GetMicroSeconds () != 0);
  m_retryTimer = Simulator::Schedule (m_dot11WmnRetryTimeout, &PmtmgmpPeerLink::RetryTimeout, this);
}
void
PmtmgmpPeerLink::RetryTimeout ()
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
PmtmgmpPeerLink::SetConfirmTimer ()
{
  NS_ASSERT (m_dot11WmnConfirmTimeout.GetMicroSeconds () != 0);
  m_confirmTimer = Simulator::Schedule (m_dot11WmnConfirmTimeout, &PmtmgmpPeerLink::ConfirmTimeout, this);
}
void
PmtmgmpPeerLink::ConfirmTimeout ()
{
  StateMachine (TOC);
}
void
PmtmgmpPeerLink::Report (std::ostream & os) const
{
  if (m_state != ESTAB)
    {
      return;
    }
  os << "<PmtmgmpPeerLink" << std::endl <<
  "localAddress=\"" << m_macPlugin->GetAddress () << "\"" << std::endl <<
  "peerInterfaceAddress=\"" << m_peerAddress << "\"" << std::endl <<
  "peerWmnPointAddress=\"" << m_peerWmnPointAddress << "\"" << std::endl <<
  "metric=\"" << m_macPlugin->GetLinkMetric (m_peerAddress) << "\"" << std::endl <<
  "lastBeacon=\"" << m_lastBeacon.GetSeconds () << "\"" << std::endl <<
  "localLinkId=\"" << m_localLinkId << "\"" << std::endl <<
  "PmtmgmpPeerLinkId=\"" << m_PmtmgmpPeerLinkId << "\"" << std::endl <<
  "assocId=\"" << m_assocId << "\"" << std::endl <<
  "/>" << std::endl;
}
} // namespace my11s
} // namespace ns3

