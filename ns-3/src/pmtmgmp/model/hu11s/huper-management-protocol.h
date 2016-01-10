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

#ifndef HU11S_HUPER_MAN_H
#define HU11S_HUPER_MAN_H

#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"
#include "ie-hu11s-beacon-timing.h"
#include "ie-hu11s-huper-management.h"
#include "huper-link.h"

#include <map>
namespace ns3 {
class WmnPointDevice;
class UniformRandomVariable;
namespace hu11s {
class HuperManagementProtocolMac;
class HuperLink;
class IeWmnId;
class IeHuperManagement;
class IeConfiguration;
/**
 * \ingroup hu11s
 *
 * \brief 802.11s Huper Management Protocol model
 */
class HuperManagementProtocol : public Object
{
public:
  HuperManagementProtocol ();
  ~HuperManagementProtocol ();
  static TypeId GetTypeId ();
  void DoDispose ();
  /**
   * \brief Install PMP on given wmn point.
   *
   * Installing protocol cause installing its interface MAC plugins.
   *
   * Also MP aggregates all installed protocols, PMP protocol can be accessed
   * via WmnPointDevice::GetObject<HuperManagementProtocol>();
   */
  bool Install (Ptr<WmnPointDevice>);
  /**
   * \brief Methods that handle beacon sending/receiving procedure.
   *
   * \name This methods interact with MAC_layer plug-in
   * \{
   */
  /**
   * \brief When we are sending a beacon - we fill beacon timing
   * element
   * \return IeBeaconTiming is a beacon timing element that should be present in beacon
   * \param interface is a interface sending a beacon
   */
  Ptr<IeBeaconTiming> GetBeaconTimingElement (uint32_t interface);
  /**
   * \brief To initiate huper link we must notify about received beacon
   * \param interface the interface where a beacon was received from
   * \param huperAddress address of station, which sent a beacon
   * \param beaconInterval beacon interval (needed by beacon loss counter)
   * \param beaconTiming beacon timing element (needed by BCA)
   */
  void ReceiveBeacon (uint32_t interface, Mac48Address huperAddress, Time beaconInterval, Ptr<IeBeaconTiming> beaconTiming);
  // \}
  /**
   * \brief Methods that handle Huper link management frames
   * interaction:
   * \{
   */
  /**
   * Deliver Huper link management information to the protocol-part
   * \param interface is a interface ID of a given MAC (interfaceID rather
   * than MAC address, because many interfaces may have the same MAC)
   * \param huperAddress is address of huper
   * \param huperWmnPointAddress is address of huper wmn point device (equal
   * to huper address when only one interface)
   * \param aid is association ID, which huper has assigned to us
   * \param huperManagementElement is huper link management element
   * \param wmnConfig is wmn configuration element taken from the huper
   * management frame
   */
  void ReceiveHuperLinkFrame (
    uint32_t interface,
    Mac48Address huperAddress,
    Mac48Address huperWmnPointAddress,
    uint16_t aid,
    IeHuperManagement huperManagementElement,
    IeConfiguration wmnConfig
    );
  /**
   * \brief Cancels huper link due to broken configuration (Wmn ID or Supported
   * rates)
   */
  void ConfigurationMismatch (uint32_t interface, Mac48Address huperAddress);
  /**
   * \brief Cancels huper link due to successive transmission failures
   */
  void TransmissionFailure (uint32_t interface, const Mac48Address huperAddress);
  /**
   * \brief resets transmission failure statistics
   */
  void TransmissionSuccess (uint32_t interface, const Mac48Address huperAddress);
  /**
   * \brief Checks if there is established link
   */
  bool IsActiveLink (uint32_t interface, Mac48Address huperAddress);
  // \}
  ///\name Interface to other protocols (MLME)
  // \{
  /// Set huper link status change callback
  void SetHuperLinkStatusCallback (Callback<void, Mac48Address, Mac48Address, uint32_t, bool> cb);
  /// Find active huper link by my interface and huper interface MAC
  Ptr<HuperLink> FindHuperLink (uint32_t interface, Mac48Address huperAddress);
  /// Get list of all active huper links
  std::vector < Ptr<HuperLink> > GetHuperLinks () const;
  /// Get list of active hupers of my given interface
  std::vector<Mac48Address> GetHupers (uint32_t interface) const;
  /// Get wmn point address.
  /// \todo this used by plugins only. Now MAC plugins can ask MP addrress directly from main MAC
  Mac48Address GetAddress ();
  uint8_t GetNumberOfLinks ();
  void SetWmnId (std::string s);
  Ptr<IeWmnId> GetWmnId () const;
  /// Enable or disable beacon collision avoidance
  void SetBeaconCollisionAvoidance (bool enable);
  bool GetBeaconCollisionAvoidance () const;
  /// Notify about beacon send event, needed to schedule BCA
  void NotifyBeaconSent (uint32_t interface, Time beaconInterval);
  // \}
  
  ///\brief: Report statistics
  void Report (std::ostream &) const;
  void ResetStats ();
  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  /**
   * TracedCallback signature for link open/close events.
   *
   * \param [in] myIface MAC address of my interface.
   * \param [in] huperIface MAC address of huper interface.
   */
  typedef void (* LinkOpenCloseTracedCallback)
    (const Mac48Address myIface, const Mac48Address huperIface);
   

private:
  virtual void DoInitialize ();
  
  // Private structures
  /// Keeps information about beacon of huper station: beacon interval, association ID, last time we have received a beacon
  struct BeaconInfo
  {
    uint16_t aid; //Assoc ID
    Time referenceTbtt; //When one of my station's beacons was put into a beacon queue;
    Time beaconInterval; //Beacon interval of my station;
  };
  /// We keep a vector of pointers to HuperLink class. This vector
  /// keeps all huper links at a given interface.
  typedef std::vector<Ptr<HuperLink> > HuperLinksOnInterface;
  /// This map keeps all huper links.
  typedef std::map<uint32_t, HuperLinksOnInterface>  HuperLinksMap;
  /// This map keeps relationship between huper address and its beacon information
  typedef std::map<Mac48Address, BeaconInfo>  BeaconsOnInterface;
  ///\brief This map keeps beacon information on all interfaces
  typedef std::map<uint32_t, BeaconsOnInterface> BeaconInfoMap;
  ///\brief this vector keeps pointers to MAC-plugins
  typedef std::map<uint32_t, Ptr<HuperManagementProtocolMac> > HuperManagementProtocolMacMap;

private:
  HuperManagementProtocol& operator= (const HuperManagementProtocol &);
  HuperManagementProtocol (const HuperManagementProtocol &);

  Ptr<HuperLink> InitiateLink (
    uint32_t interface,
    Mac48Address huperAddress,
    Mac48Address huperWmnPointAddress
    );
  /**
   * \name External huper-chooser
   * \{
   */
  bool ShouldSendOpen (uint32_t interface, Mac48Address huperAddress);
  bool ShouldAcceptOpen (uint32_t interface, Mac48Address huperAddress, PmpReasonCode & reasonCode);
  /**
   * \}
   * \brief Indicates changes in huper links
   */
  void HuperLinkStatus (uint32_t interface, Mac48Address huperAddress, Mac48Address huperWmnPointAddres, HuperLink::HuperState ostate, HuperLink::HuperState nstate);
  ///\brief BCA
  void CheckBeaconCollisions (uint32_t interface);
  void ShiftOwnBeacon (uint32_t interface);
  /**
   * \name Time<-->TU converters:
   * \{
   */
  Time TuToTime (int x);
  int TimeToTu (Time x);
  // \}

  /// Aux. method to register open links
  void NotifyLinkOpen (Mac48Address huperMp, Mac48Address huperIface, Mac48Address myIface, uint32_t interface);
  /// Aux. method to register closed links
  void NotifyLinkClose (Mac48Address huperMp, Mac48Address huperIface, Mac48Address myIface, uint32_t interface);
private:
  HuperManagementProtocolMacMap m_plugins;
  Mac48Address m_address;
  Ptr<IeWmnId> m_wmnId;

  uint16_t m_lastAssocId;
  uint16_t m_lastLocalLinkId;
  uint8_t m_maxNumberOfHuperLinks;
  /// Flag which enables BCA
  bool m_enableBca;
  /// Beacon can be shifted at [-m_maxBeaconShift; +m_maxBeaconShift] TUs
  uint16_t m_maxBeaconShift;
  /// Last beacon at each interface
  std::map<uint32_t, Time> m_lastBeacon;
  /// Beacon interval at each interface
  std::map<uint32_t, Time> m_beaconInterval;

  /**
   * \name Huper Links
   * \{
   */
  HuperLinksMap m_huperLinks;
  /**
   * \}
   */
  /**
   * \brief Callback to notify about huper link changes:
   * Mac48Address is huper address of wmn point,
   * Mac48Address is huper address of interface,
   * uint32_t - interface ID,
   * bool is status - true when new link has appeared, false - when link was closed,
   */
  Callback <void, Mac48Address, Mac48Address, uint32_t, bool> m_huperStatusCallback;

  /// Simple link open/close trace source type. Addresses are: src interface, dst interface
  typedef TracedCallback <Mac48Address, Mac48Address> LinkEventCallback;
  /// LinkOpen trace source
  LinkEventCallback m_linkOpenTraceSrc;
  /// LinkClose trace source
  LinkEventCallback m_linkCloseTraceSrc;

  ///\name Statistics:
  // \{
  struct Statistics {
    uint16_t linksTotal;
    uint16_t linksOpened;
    uint16_t linksClosed;

    Statistics (uint16_t t = 0);
    void Print (std::ostream & os) const;
  };
  struct Statistics m_stats;
  // \}
  /// Add randomness to beacon shift
  Ptr<UniformRandomVariable> m_beaconShift;
};

} // namespace hu11s
} // namespace ns3
#endif
