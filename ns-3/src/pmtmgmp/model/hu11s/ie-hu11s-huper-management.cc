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

#include "ie-hu11s-huper-management.h"
#include "ns3/assert.h"
#include "ns3/packet.h"

namespace ns3 {
namespace hu11s {

IeHuperManagement::IeHuperManagement () :
  m_length (3), m_subtype (HUPER_OPEN), m_localLinkId (0), m_huperLinkId (0), m_reasonCode (REASON11S_RESERVED)
{
}
WifiInformationElementId
IeHuperManagement::ElementId () const
{
  return IE11S_HUPERING_MANAGEMENT;
}
void
IeHuperManagement::SetHuperOpen (uint16_t localLinkId)
{
  m_length = 3;
  m_subtype = HUPER_OPEN;
  m_localLinkId = localLinkId;
}
void
IeHuperManagement::SetHuperClose (uint16_t localLinkId, uint16_t huperLinkId, PmpReasonCode reasonCode)
{
  m_length = 7;
  m_subtype = HUPER_CLOSE;
  m_localLinkId = localLinkId;
  m_huperLinkId = huperLinkId;
  m_reasonCode = reasonCode;
}

void
IeHuperManagement::SetHuperConfirm (uint16_t localLinkId, uint16_t huperLinkId)
{
  m_length = 5;
  m_subtype = HUPER_CONFIRM;
  m_localLinkId = localLinkId;
  m_huperLinkId = huperLinkId;
}

PmpReasonCode
IeHuperManagement::GetReasonCode () const
{
  return m_reasonCode;
}

uint16_t
IeHuperManagement::GetLocalLinkId () const
{
  return m_localLinkId;
}

uint16_t
IeHuperManagement::GetHuperLinkId () const
{
  return m_huperLinkId;
}

uint8_t
IeHuperManagement::GetInformationFieldSize (void) const
{
  return m_length;
}
uint8_t
IeHuperManagement::GetSubtype () const
{
  return m_subtype;
}
bool
IeHuperManagement::SubtypeIsOpen () const
{
  return (m_subtype == HUPER_OPEN);
}
bool
IeHuperManagement::SubtypeIsClose () const
{
  return (m_subtype == HUPER_CLOSE);
}
bool
IeHuperManagement::SubtypeIsConfirm () const
{
  return (m_subtype == HUPER_CONFIRM);
}

void
IeHuperManagement::SerializeInformationField (Buffer::Iterator i) const
{
  i.WriteU8 (m_subtype);
  i.WriteHtolsbU16 (m_localLinkId);
  if (m_length > 3)
    {
      i.WriteHtolsbU16 (m_huperLinkId);
    }
  if (m_length > 5)
    {
      i.WriteHtolsbU16 (m_reasonCode);
    }
}
uint8_t
IeHuperManagement::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_subtype = i.ReadU8 ();
  m_length = length;
  if (m_subtype == HUPER_OPEN)
    {
      NS_ASSERT (length == 3);
    }
  if (m_subtype == HUPER_CONFIRM)
    {
      NS_ASSERT (length == 5);
    }
  if (m_subtype == HUPER_CLOSE)
    {
      NS_ASSERT (length == 7);
    }
  m_localLinkId = i.ReadLsbtohU16 ();
  if (m_length > 3)
    {
      m_huperLinkId = i.ReadLsbtohU16 ();
    }
  if (m_length > 5)
    {
      m_reasonCode = (PmpReasonCode) i.ReadLsbtohU16 ();
    }
  return i.GetDistanceFrom (start);
}
void
IeHuperManagement::Print (std::ostream& os) const
{
  os << std::endl << "<information_element id=" << ElementId () << ">" << std::endl;
  os << " Subtype:      = " << (uint16_t) m_subtype << std::endl;
  os << " Length:       = " << (uint16_t) m_length << std::endl;
  os << " LocalLinkId:  = " << m_localLinkId << std::endl;
  os << " HuperLinkId:   = " << m_huperLinkId << std::endl;
  os << " ReasonCode:   = " << m_reasonCode << std::endl;
  os << "</information_element>" << std::endl;
}
bool
operator== (const IeHuperManagement & a, const IeHuperManagement & b)
{
  return ((a.m_length == b.m_length) && (a.m_subtype == b.m_subtype) && (a.m_localLinkId == b.m_localLinkId)
          && (a.m_huperLinkId == b.m_huperLinkId) && (a.m_reasonCode == b.m_reasonCode));
}
std::ostream &
operator << (std::ostream &os, const IeHuperManagement &a)
{
  a.Print (os);
  return os;
}
} // namespace hu11s
} // namespace ns3

