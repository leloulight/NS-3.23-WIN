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

#include "ns3/assert.h"
#include "ns3/address-utils.h"
#include "my11s-mac-header.h"
#include "ns3/packet.h"

namespace ns3 {
	namespace my11s {
		/***********************************************************
		 *  Here Wmn Mac Header functionality is defined.
		 ***********************************************************/
		TypeId
			WmnHeader::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::My11sMacHeader")
				.SetParent<Header>()
				.SetGroupName("Wmn")
				.AddConstructor<WmnHeader>();
			return tid;
		}
		WmnHeader::WmnHeader() :
			m_wmnFlags(0), m_wmnTtl(0), m_wmnSeqno(0), m_addr4(Mac48Address()), m_addr5(Mac48Address()),
			m_addr6(Mac48Address())
		{
		}
		WmnHeader::~WmnHeader()
		{
		}
		TypeId
			WmnHeader::GetInstanceTypeId() const
		{
			return GetTypeId();
		}
		void
			WmnHeader::SetAddr4(Mac48Address address)
		{
			m_addr4 = address;
		}
		void
			WmnHeader::SetAddr5(Mac48Address address)
		{
			m_addr5 = address;
		}
		void
			WmnHeader::SetAddr6(Mac48Address address)
		{
			m_addr6 = address;
		}
		Mac48Address
			WmnHeader::GetAddr4() const
		{
			return m_addr4;
		}
		Mac48Address
			WmnHeader::GetAddr5() const
		{
			return m_addr5;
		}
		Mac48Address
			WmnHeader::GetAddr6() const
		{
			return m_addr6;
		}
		void
			WmnHeader::SetWmnSeqno(uint32_t seqno)
		{
			m_wmnSeqno = seqno;
		}
		uint32_t
			WmnHeader::GetWmnSeqno() const
		{
			return m_wmnSeqno;
		}
		void
			WmnHeader::SetWmnTtl(uint8_t TTL)
		{
			m_wmnTtl = TTL;
		}
		uint8_t
			WmnHeader::GetWmnTtl() const
		{
			return m_wmnTtl;
		}
		void
			WmnHeader::SetAddressExt(uint8_t num_of_addresses)
		{
			NS_ASSERT(num_of_addresses <= 3);
			m_wmnFlags |= 0x03 & num_of_addresses;
		}
		uint8_t
			WmnHeader::GetAddressExt() const
		{
			return (0x03 & m_wmnFlags);
		}
		uint32_t
			WmnHeader::GetSerializedSize() const
		{
#ifndef PMTMGMP_UNUSED_MY_CODE
#ifndef PMTMGMP_TAG_INFOR_ATTACH
			return 8 + GetAddressExt() * 6;
#else
			return 12 + GetAddressExt() * 6;
#endif
#else
			return 6 + GetAddressExt() * 6;
#endif
		}
		void
			WmnHeader::Serialize(Buffer::Iterator start) const
		{
			Buffer::Iterator i = start;
			i.WriteU8(m_wmnFlags);
			i.WriteU8(m_wmnTtl);
			i.WriteHtolsbU32(m_wmnSeqno);
			uint8_t addresses_to_add = GetAddressExt();
			//Writing Address extensions:
			if ((addresses_to_add == 1) || (addresses_to_add == 3))
			{
				WriteTo(i, m_addr4);
			}
			if (addresses_to_add > 1)
			{
				WriteTo(i, m_addr5);
			}
			if (addresses_to_add > 1)
			{
				WriteTo(i, m_addr6);
			}
		}
		uint32_t
			WmnHeader::Deserialize(Buffer::Iterator start)
		{
			Buffer::Iterator i = start;
			uint8_t addresses_to_read = 0;
			m_wmnFlags = i.ReadU8();
			m_wmnTtl = i.ReadU8();
			m_wmnSeqno = i.ReadLsbtohU32();
			addresses_to_read = m_wmnFlags & 0x03;
			if ((addresses_to_read == 1) || (addresses_to_read == 3))
			{
				ReadFrom(i, m_addr4);
			}
			if (addresses_to_read > 1)
			{
				ReadFrom(i, m_addr5);
			}
			if (addresses_to_read > 1)
			{
				ReadFrom(i, m_addr6);
			}
			return i.GetDistanceFrom(start);
		}
		void
			WmnHeader::Print(std::ostream &os) const
		{
			os << "flags = " << (uint16_t)m_wmnFlags << std::endl << "ttl = " << (uint16_t)m_wmnTtl
				<< std::endl << "seqno = " << m_wmnSeqno << std::endl << "addr4 = " << m_addr4 << std::endl
				<< "addr5 = " << m_addr5 << std::endl << "addr6 = " << m_addr6 << std::endl;
		}
		bool
			operator== (const WmnHeader & a, const WmnHeader & b)
		{
			return ((a.m_wmnFlags == b.m_wmnFlags) && (a.m_wmnTtl == b.m_wmnTtl)
				&& (a.m_wmnSeqno == b.m_wmnSeqno) && (a.m_addr4 == b.m_addr4) && (a.m_addr5 == b.m_addr5)
				&& (a.m_addr6 == b.m_addr6));
		}
#ifndef PMTMGMP_UNUSED_MY_CODE
		void WmnHeader::SetMSECPindex(uint8_t index)
		{
			m_MSECPindex = index;
		}
		void WmnHeader::SetChangePath(uint8_t change)
		{
			m_ChangePath = change;
		}
		uint8_t WmnHeader::GetMSECPindex()
		{
			return m_MSECPindex;
		}
		uint8_t WmnHeader::GetChangePath()
		{
			return m_ChangePath;
		}
#ifdef PMTMGMP_TAG_INFOR_ATTACH
		void WmnHeader::SetSendIndex(uint32_t index)
		{
			m_SendIndex = index;
		}
		uint32_t WmnHeader::GetSendIndex()
		{
			return m_SendIndex;
		}
#endif
#endif
	} // namespace my11s
} // namespace ns3
