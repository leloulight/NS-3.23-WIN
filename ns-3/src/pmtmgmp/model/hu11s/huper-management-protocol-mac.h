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

#ifndef HUPER_MANAGEMENT_PROTOCOL_MAC_H
#define HUPER_MANAGEMENT_PROTOCOL_MAC_H

#include "ns3/wmn-wifi-interface-mac-plugin.h"

namespace ns3 {
class WmnWifiInterfaceMac;
namespace hu11s {
class HuperManagementProtocol;
class IeConfiguration;
class IeHuperManagement;
class HuperManagementProtocol;
/**
 * \ingroup hu11s
 *
 * \brief This is plugin to Wmn WiFi MAC, which implements
 * interface to hu11s huper management protocol: it takes proper
 * frames from MAC-layer, extracts huper link management information
 * element and wmn configuration element and passes it to main part
 * of protocol
 */
class HuperManagementProtocolMac : public WmnWifiInterfaceMacPlugin
{
public:
  HuperManagementProtocolMac (uint32_t interface, Ptr<HuperManagementProtocol> protocol);
  ~HuperManagementProtocolMac ();
  
  // Inherited from plugin abstract class
  void SetParent (Ptr<WmnWifiInterfaceMac> parent);
  bool Receive (Ptr<Packet> packet, const WifiMacHeader & header);
  bool UpdateOutcomingFrame (Ptr<Packet> packet, WifiMacHeader & header, Mac48Address from, Mac48Address to);
  void UpdateBeacon (WmnWifiBeacon & beacon) const;
  int64_t AssignStreams (int64_t stream);
  ///\name Statistics
  // \{
  void Report (std::ostream &) const;
  void ResetStats ();
  uint32_t GetLinkMetric (Mac48Address huperAddress);
  // \}

private:
  HuperManagementProtocolMac& operator= (const HuperManagementProtocolMac &);
  HuperManagementProtocolMac (const HuperManagementProtocolMac &);

  friend class HuperManagementProtocol;
  friend class HuperLink;
  ///\name Create huper link management frames
  // \{
  /**
   * \brief This structure keeps all fields in huper link management frame,
   * which are not subclasses of WifiInformationElement
   */
  struct PlinkFrameStart
  {
    uint8_t subtype;
    uint16_t aid;
    SupportedRates rates;
    uint16_t qos;
  };
  Ptr<Packet> CreateHuperLinkOpenFrame ();
  Ptr<Packet> CreateHuperLinkConfirmFrame ();
  Ptr<Packet> CreateHuperLinkCloseFrame ();
  /// Parses the start of the frame, where no WifiInformationElements exist
  PlinkFrameStart ParsePlinkFrame (Ptr<const Packet> packet);
  // \}
  ///  Closes link when a proper number of successive transmissions have failed
  void TxError (WifiMacHeader const &hdr);
  void TxOk (WifiMacHeader const &hdr);
  /// BCA functionality
  void SetBeaconShift (Time shift);
  void SetHuperManagerProtcol (Ptr<HuperManagementProtocol> protocol);
  void SendHuperLinkManagementFrame (
    Mac48Address huperAddress,
    Mac48Address huperMpAddress,
    uint16_t aid,
    IeHuperManagement huperElement,
    IeConfiguration wmnConfig
    );
  ///\brief debug only, used to print established links
  Mac48Address GetAddress () const;
  ///\name Statistics
  // \{
  struct Statistics
  {
    uint16_t txOpen;
    uint16_t txConfirm;
    uint16_t txClose;
    uint16_t rxOpen;
    uint16_t rxConfirm;
    uint16_t rxClose;
    uint16_t dropped;
    uint16_t brokenMgt;
    uint16_t txMgt;
    uint32_t txMgtBytes;
    uint16_t rxMgt;
    uint32_t rxMgtBytes;
    uint16_t beaconShift;

    Statistics ();
    void Print (std::ostream & os) const;
  };

private:
  struct Statistics m_stats;
  ///\}
  ///\name Information about MAC and protocol:
  // \{
  Ptr<WmnWifiInterfaceMac> m_parent;
  uint32_t m_ifIndex;
  Ptr<HuperManagementProtocol> m_protocol;
  // \}
};

} // namespace hu11s
} // namespace ns3

#endif /* HUPER_MANAGEMENT_PROTOCOL_MAC_H */

