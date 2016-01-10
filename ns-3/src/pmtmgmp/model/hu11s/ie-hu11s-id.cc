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
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#include "ie-hu11s-id.h"
#include "ns3/assert.h"

namespace ns3 {
namespace hu11s {

IeWmnId::IeWmnId ()
{
  for (uint8_t i = 0; i < 32; i++)
    {
      m_wmnId[i] = 0;
    }
}
IeWmnId::IeWmnId (std::string s)
{
  NS_ASSERT (s.size () < 32);
  const char *wmnid = s.c_str ();
  uint8_t len = 0;
  while (*wmnid != 0 && len < 32)
    {
      m_wmnId[len] = *wmnid;
      wmnid++;
      len++;
    }
  NS_ASSERT (len <= 32);
  while (len < 33)
    {
      m_wmnId[len] = 0;
      len++;
    }
}
WifiInformationElementId
IeWmnId::ElementId () const
{
  return IE11S_WMN_ID;
}
bool
IeWmnId::IsEqual (IeWmnId const &o) const
{
  uint8_t i = 0;
  while (i < 32 && m_wmnId[i] == o.m_wmnId[i] && m_wmnId[i] != 0)
    {
      i++;
    }
  if (m_wmnId[i] != o.m_wmnId[i])
    {
      return false;
    }
  return true;
}
bool
IeWmnId::IsBroadcast (void) const
{
  if (m_wmnId[0] == 0)
    {
      return true;
    }
  return false;
}
char *
IeWmnId::PeekString (void) const
{
  return (char *) m_wmnId;
}
uint8_t
IeWmnId::GetInformationFieldSize (void) const
{
  uint8_t size = 0;
  while (m_wmnId[size] != 0 && size < 32)
    {
      size++;
    }
  NS_ASSERT (size <= 32);
  return size;
}
void
IeWmnId::SerializeInformationField (Buffer::Iterator i) const
{
  uint8_t size = 0;
  while (m_wmnId[size] != 0 && size < 32)
    {
      i.WriteU8 (m_wmnId[size]);
      size++;
    }
}
uint8_t
IeWmnId::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  NS_ASSERT (length <= 32);
  i.Read (m_wmnId, length);
  m_wmnId[length] = 0;
  return i.GetDistanceFrom (start);
}
void
IeWmnId::Print (std::ostream& os) const
{
  os << std::endl << "<information_element id=" << ElementId () << ">" << std::endl;
  os << "wmnId =  " << PeekString ();
  os << "</information_element>" << std::endl;
}
bool
operator== (const IeWmnId & a, const IeWmnId & b)
{
  bool result (true);
  uint8_t size = 0;

  while (size < 32)
    {
      result = result && (a.m_wmnId[size] == b.m_wmnId[size]);
      if (a.m_wmnId[size] == 0)
        {
          return result;
        }
      size++;
    }
  return result;
}
std::ostream &
operator << (std::ostream &os, const IeWmnId &a)
{
  a.Print (os);
  return os;
}

std::istream &operator >> (std::istream &is, IeWmnId &a)
{
  std::string str;
  is >> str;
  a = IeWmnId (str.c_str ());
  return is;
}

ATTRIBUTE_HELPER_CPP (IeWmnId);


} // namespace hu11s
} // namespace ns3
