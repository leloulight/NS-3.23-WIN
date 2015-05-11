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

#include "pmtmgmp-tag.h"

namespace ns3 {
namespace my11s {

NS_OBJECT_ENSURE_REGISTERED (PmtmgmpTag);
  
//Class PmtmgmpTag:
PmtmgmpTag::PmtmgmpTag () :
  m_address (Mac48Address::GetBroadcast ()), m_ttl (0), m_metric (0), m_seqno (0)
{
}

PmtmgmpTag::~PmtmgmpTag ()
{
}

void
PmtmgmpTag::SetAddress (Mac48Address retransmitter)
{
  m_address = retransmitter;
}

Mac48Address
PmtmgmpTag::GetAddress ()
{
  return m_address;
}

void
PmtmgmpTag::SetTtl (uint8_t ttl)
{
  m_ttl = ttl;
}

uint8_t
PmtmgmpTag::GetTtl ()
{
  return m_ttl;
}

void
PmtmgmpTag::SetMetric (uint32_t metric)
{
  m_metric = metric;
}

uint32_t
PmtmgmpTag::GetMetric ()
{
  return m_metric;
}

void
PmtmgmpTag::SetSeqno (uint32_t seqno)
{
  m_seqno = seqno;
}

uint32_t
PmtmgmpTag::GetSeqno ()
{
  return m_seqno;
}

TypeId
PmtmgmpTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::my11s::PmtmgmpTag").SetParent<Tag> ().AddConstructor<PmtmgmpTag> ().SetGroupName ("Wmn");
  return tid;
}

TypeId
PmtmgmpTag::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
PmtmgmpTag::GetSerializedSize () const
{
  return 6 //address
         + 1 //ttl
         + 4 //metric
         + 4; //seqno
}

void
PmtmgmpTag::Serialize (TagBuffer i) const
{
  uint8_t address[6];
  int j;
  m_address.CopyTo (address);
  i.WriteU8 (m_ttl);
  i.WriteU32 (m_metric);
  i.WriteU32 (m_seqno);
  for (j = 0; j < 6; j++)
    {
      i.WriteU8 (address[j]);
    }
}

void
PmtmgmpTag::Deserialize (TagBuffer i)
{
  uint8_t address[6];
  int j;
  m_ttl = i.ReadU8 ();
  m_metric = i.ReadU32 ();
  m_seqno = i.ReadU32 ();
  for (j = 0; j < 6; j++)
    {
      address[j] = i.ReadU8 ();
    }
  m_address.CopyFrom (address);
}

void
PmtmgmpTag::Print (std::ostream &os) const
{
  os << "address=" << m_address;
  os << "ttl=" << m_ttl;
  os << "metrc=" << m_metric;
  os << "seqno=" << m_seqno;
}
void
PmtmgmpTag::DecrementTtl ()
{
  m_ttl--;
}
} // namespace my11s
} // namespace ns3
