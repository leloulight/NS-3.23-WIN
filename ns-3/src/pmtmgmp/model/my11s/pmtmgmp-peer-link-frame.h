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
 */

#ifndef PMTMGMP_PEER_LINK_FRAME_START_H
#define PMTMGMP_PEER_LINK_FRAME_START_H
#include "ns3/header.h"
#include "ns3/supported-rates.h"
#include "my11s-mac-header.h"
#include "ie-my11s-configuration.h"
#include "ie-my11s-peering-protocol.h"
#include "ie-my11s-id.h"
namespace ns3
{
class WmnWifiInterfaceMac;
namespace my11s
{
/**
 * \ingroup my11s
 *
 * \brief 802.11s Peer link management frame
 * 
 * Peer link management frame included the following (see chapters 7.4.12.1-7.4.12.3 of 802.11s):
 * - Subtype field
 * - Association ID field
 * - Supported rates
 * - Wmn ID of wmn
 */
class PmtmgmpPeerLinkFrameStart : public Header
{
public:
  PmtmgmpPeerLinkFrameStart ();
  ///\brief fields:
  struct PlinkFrameStartFields
  {
    uint8_t subtype;
    IePeeringProtocol protocol; //Peering protocol version - in all subtypes - 3 octets
    uint16_t capability;        //open and confirm
    uint16_t aid;               //confirm only
    SupportedRates rates;       //open and confirm
    IeWmnId wmnId;            //open and close
    IeConfiguration config;     //open and confirm
    uint16_t reasonCode;        //close only
  };
  ///\attention: must be set before deserialize, before only multihop
  //action header knows about subtype
  void SetPlinkFrameSubtype (uint8_t subtype);
  void SetPlinkFrameStart (PlinkFrameStartFields);
  PlinkFrameStartFields GetFields () const;

  // Inherited from header:
  static  TypeId   GetTypeId ();
  virtual TypeId   GetInstanceTypeId () const;
  virtual void     Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize () const;
  virtual void     Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint8_t m_subtype;
  IePeeringProtocol m_protocol;
  uint16_t m_capability;
  uint16_t m_aid;
  SupportedRates m_rates;
  IeWmnId m_wmnId;
  IeConfiguration m_config;
  uint16_t m_reasonCode;

  friend bool operator== (const PmtmgmpPeerLinkFrameStart & a, const PmtmgmpPeerLinkFrameStart & b);

  PmtmgmpPeerLinkFrameStart& operator= (const PmtmgmpPeerLinkFrameStart &);
  PmtmgmpPeerLinkFrameStart (const PmtmgmpPeerLinkFrameStart &);

};
bool operator== (const PmtmgmpPeerLinkFrameStart & a, const PmtmgmpPeerLinkFrameStart & b);
} // namespace my11s
} // namespace ns3
#endif
