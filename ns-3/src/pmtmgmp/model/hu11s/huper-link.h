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
 */

#ifndef HUPER_LINK_H
#define HUPER_LINK_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/mac48-address.h"
#include "ns3/event-id.h"
#include "ns3/ie-hu11s-beacon-timing.h"
#include "ns3/ie-hu11s-huper-management.h"
#include "ns3/ie-hu11s-configuration.h"

namespace ns3 {
namespace hu11s {

class HuperManagementProtocolMac;
/**
 * \ingroup hu11s
 *
 * \brief Huper link model for 802.11s Huper Management protocol
 */
class HuperLink : public Object
{
public:
  friend class HuperManagementProtocol;
  /// Support object system
  static TypeId GetTypeId ();
  /// C-tor create empty link
  HuperLink ();
  ~HuperLink ();
  void DoDispose ();
  /// Huper Link state:
  enum  HuperState {
    IDLE,
    OPN_SNT,
    CNF_RCVD,
    OPN_RCVD,
    ESTAB,
    HOLDING,
  };
  /// Process beacon received from huper
  void SetBeaconInformation (Time lastBeacon, Time BeaconInterval);
  /**
   * \brief Method used to detect huper link changes
   *
   * \param cb is a callback, which notifies, that on interface (uint32_t), huper link
   * with address (Mac48Address) was opened (bool is true) or closed (bool is false) 
   */
  void  SetLinkStatusCallback (Callback<void, uint32_t, Mac48Address, bool> cb);
  /**
   * \name Huper link getters/setters
   * \{
   */
  void SetHuperAddress (Mac48Address macaddr);
  void SetHuperWmnPointAddress (Mac48Address macaddr);
  void SetInterface (uint32_t interface);
  void SetLocalLinkId (uint16_t id);
  void SetLocalAid (uint16_t aid);
  uint16_t GetHuperAid () const;
  void SetBeaconTimingElement (IeBeaconTiming beaconTiming);
  Mac48Address GetHuperAddress () const;
  uint16_t GetLocalAid () const;
  Time GetLastBeacon () const;
  Time GetBeaconInterval () const;
  IeBeaconTiming GetBeaconTimingElement () const;
  //IeHuperManagement GetHuperLinkDescriptorElement ()const;
  //\}

  /**
   * \name MLME
   * \{
   */
  /// MLME-CancelHuperLink.request
  void MLMECancelHuperLink (PmpReasonCode reason);
  /// MLME-ActiveHuperLinkOpen.request
  void MLMEActiveHuperLinkOpen ();
  /// MLME-HuperingRequestReject
  void MLMEHuperingRequestReject ();
  /// Callback type for MLME-SignalHuperLinkStatus event
  typedef Callback<void, uint32_t, Mac48Address, Mac48Address, HuperLink::HuperState, HuperLink::HuperState> SignalStatusCallback;
  /// Set callback
  void MLMESetSignalStatusCallback (SignalStatusCallback);
  /// Reports about transmission success/failure
  void TransmissionSuccess ();
  void TransmissionFailure ();
  //\}
  ///\brief Statistics
  void Report (std::ostream & os) const;
private:
  /// Huper link events, see 802.11s draft 11B.3.3.2
  enum  HuperEvent
  {
    CNCL,       ///< Cancel huper link
    ACTOPN,     ///< Active huper link open
    CLS_ACPT,   ///< HuperLinkClose_Accept
    OPN_ACPT,   ///< HuperLinkOpen_Accept
    OPN_RJCT,   ///< HuperLinkOpen_Reject
    REQ_RJCT,   ///< HuperLinkOpenReject by internal reason
    CNF_ACPT,   ///< HuperLinkConfirm_Accept
    CNF_RJCT,   ///< HuperLinkConfirm_Reject
    TOR1,       ///< Timeout of retry timer
    TOR2,       ///< also timeout of retry timer
    TOC,        ///< Timeout of confirm timer
    TOH,        ///< Timeout of holding (graceful closing) timer
  };
  /// State transition
  void StateMachine (HuperEvent event, PmpReasonCode = REASON11S_RESERVED);
  /**
   * \name Link response to received management frames
   *
   * \attention In all this methods {local/huper}LinkID correspond to _huper_ station, as written in
   * received frame, e.g. I am huperLinkID and huper link is localLinkID .
   *
   * \{
   */
  /// Close link
  void Close (uint16_t localLinkID, uint16_t huperLinkID, PmpReasonCode reason);
  /// Accept open link
  void OpenAccept (uint16_t localLinkId, IeConfiguration conf, Mac48Address huperMp);
  /// Reject open link
  void OpenReject (uint16_t localLinkId, IeConfiguration conf, Mac48Address huperMp, PmpReasonCode reason);
  /// Confirm accept
  void ConfirmAccept (
    uint16_t localLinkId,
    uint16_t huperLinkId,
    uint16_t huperAid,
    IeConfiguration conf,
    Mac48Address huperMp
    );
  /// Confirm reject
  void  ConfirmReject (
    uint16_t localLinkId,
    uint16_t huperLinkId,
    IeConfiguration  conf,
    Mac48Address huperMp,
    PmpReasonCode reason
    );
  //\}
  /// True if link is established
  bool  LinkIsEstab () const;
  /// True if link is idle. Link can be deleted in this state
  bool  LinkIsIdle () const;
  /**
   * Set pointer to MAC-plugin, which is responsible for sending huper
   * link management frames
   */
  void SetMacPlugin (Ptr<HuperManagementProtocolMac> plugin);
  /**
   * \name Event handlers
   * \{
   */
  void ClearRetryTimer ();
  void ClearConfirmTimer ();
  void ClearHoldingTimer ();
  void SetHoldingTimer ();
  void SetRetryTimer ();
  void SetConfirmTimer ();
  //\}

  /**
   * \name Work with management frames
   * \{
   */
  void SendHuperLinkClose (PmpReasonCode reasoncode);
  void SendHuperLinkOpen ();
  void SendHuperLinkConfirm ();
  //\}

  /**
   * \name Timeout handlers
   * \{
   */
  void HoldingTimeout ();
  void RetryTimeout ();
  void ConfirmTimeout ();
  //\}
  /// Several successive beacons were lost, close link
  void BeaconLoss ();
private:

  HuperLink& operator= (const HuperLink &);
  HuperLink (const HuperLink &);

  /// The number of interface I am associated with
  uint32_t m_interface;
  /// pointer to MAC plugin, which is responsible for huper management
  Ptr<HuperManagementProtocolMac> m_macPlugin;
  /// Huper address
  Mac48Address m_huperAddress;
  /// Wmn point address, equal to huper address in case of single
  /// interface wmn point
  Mac48Address m_huperWmnPointAddress;
  /// My ID of this link
  uint16_t m_localLinkId;
  /// Huper ID of this link
  uint16_t m_huperLinkId;
  /// My association ID
  uint16_t m_assocId;
  /// Assoc Id assigned to me by huper
  uint16_t m_huperAssocId;

  /// When last beacon was received
  Time  m_lastBeacon;
  /// Current beacon interval on corresponding interface
  Time  m_beaconInterval;
  /// How many successive packets were failed to transmit
  uint16_t m_packetFail;

  /// Current state
  HuperState m_state;
  /**
   * \brief Wmn interface configuration
   * \attention Is not used now, nothing to configure :)
   */
  IeConfiguration m_configuration;
  /// Beacon timing element received from the huper. Needed by BCA
  IeBeaconTiming m_beaconTiming;

  /**
   * \name Timers & counters used for internal state transitions
   * \{
   */
  uint16_t m_dot11WmnMaxRetries;
  Time     m_dot11WmnRetryTimeout;
  Time     m_dot11WmnHoldingTimeout;
  Time     m_dot11WmnConfirmTimeout;

  EventId  m_retryTimer;
  EventId  m_holdingTimer;
  EventId  m_confirmTimer;
  uint16_t m_retryCounter;
  EventId  m_beaconLossTimer;
  uint16_t m_maxBeaconLoss;
  uint16_t m_maxPacketFail;
  // \}
  /// How to report my status change
  SignalStatusCallback m_linkStatusCallback;
};

} // namespace hu11s
} // namespace ns3

#endif /* HUPER_LINK_H */
