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

		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpTag);

		//Class PmtmgmpTag:
		PmtmgmpTag::PmtmgmpTag() :
			m_address(Mac48Address::GetBroadcast()), 
			m_ttl(0),
#ifdef PMTMGMP_UNUSED_MY_CODE
			//不需要使用度量
			m_metric(0),
			m_seqno(0)
#else
			m_MSECPindex(0),
#ifdef PMTMGMP_TAG_INFOR_ATTACH
			m_ChangePath(0),
			m_SendIndex(0)
#else
			m_ChangePath(0)
#endif
#endif
		{
		}

		PmtmgmpTag::~PmtmgmpTag()
		{
		}

		void
			PmtmgmpTag::SetAddress(Mac48Address retransmitter)
		{
			m_address = retransmitter;
		}

		Mac48Address
			PmtmgmpTag::GetAddress()
		{
			return m_address;
		}

		void
			PmtmgmpTag::SetTtl(uint8_t ttl)
		{
			m_ttl = ttl;
		}

		uint8_t
			PmtmgmpTag::GetTtl()
		{
			return m_ttl;
		}

#ifdef PMTMGMP_UNUSED_MY_CODE
		//不需要使用度量
		void
			PmtmgmpTag::SetMetric(uint32_t metric)
		{
			m_metric = metric;
		}

		uint32_t
			PmtmgmpTag::GetMetric()
		{
			return m_metric;
		}
		void
			PmtmgmpTag::SetSeqno(uint32_t seqno)
		{
			m_seqno = seqno;
		}

		uint32_t
			PmtmgmpTag::GetSeqno()
		{
			return m_seqno;
		}
#endif

		TypeId
			PmtmgmpTag::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpTag").SetParent<Tag>().AddConstructor<PmtmgmpTag>().SetGroupName("Wmn");
			return tid;
		}

		TypeId
			PmtmgmpTag::GetInstanceTypeId() const
		{
			return GetTypeId();
		}

		uint32_t
			PmtmgmpTag::GetSerializedSize() const
		{
			return 6 //address
				+ 1 //ttl
#ifndef PMTMGMP_UNUSED_MY_CODE
				+ 1  //MSECPindex
#ifdef PMTMGMP_TAG_INFOR_ATTACH
				+ 1  //ChangePath
				+ 4; //SendIndex
#else
				+ 1; //ChangePath
#endif
#else
				+ 4 //metric
				+ 4; //seqno
#endif
		}

		void
			PmtmgmpTag::Serialize(TagBuffer i) const
		{
			uint8_t address[6];
			int j;
			m_address.CopyTo(address);
			i.WriteU8(m_ttl);
#ifdef PMTMGMP_UNUSED_MY_CODE
			//不需要使用度量
			i.WriteU32(m_metric);
			i.WriteU32(m_seqno);
#endif
			for (j = 0; j < 6; j++)
			{
				i.WriteU8(address[j]);
			}
#ifndef PMTMGMP_UNUSED_MY_CODE
			i.WriteU8(m_MSECPindex);
			i.WriteU8(m_ChangePath);
#ifdef PMTMGMP_TAG_INFOR_ATTACH
			i.WriteU32(m_SendIndex);
#endif
#endif
		}

		void
			PmtmgmpTag::Deserialize(TagBuffer i)
		{
			uint8_t address[6];
			int j;
			m_ttl = i.ReadU8();
#ifdef PMTMGMP_UNUSED_MY_CODE
			//不需要使用度量
			m_metric = i.ReadU32();
			m_seqno = i.ReadU32();
#endif
			for (j = 0; j < 6; j++)
			{
				address[j] = i.ReadU8();
			}
			m_address.CopyFrom(address);
#ifndef PMTMGMP_UNUSED_MY_CODE
			m_MSECPindex = i.ReadU8();
			m_ChangePath = i.ReadU8();
#ifdef PMTMGMP_TAG_INFOR_ATTACH
			m_SendIndex = i.ReadU32();
#endif
#endif
		}

		void
			PmtmgmpTag::Print(std::ostream &os) const
		{
			os << "address=" << m_address;
			os << "ttl=" << m_ttl;
#ifdef PMTMGMP_UNUSED_MY_CODE
			//不需要使用度量
			os << "metrc=" << m_metric;
			os << "seqno=" << m_seqno;
#endif
#ifndef PMTMGMP_UNUSED_MY_CODE
			os << "MSECPindex=" << m_MSECPindex;
			os << "ChangePath=" << m_ChangePath;
#ifdef PMTMGMP_TAG_INFOR_ATTACH
			os << "SendIndex=" << m_SendIndex;
#endif
#endif
		}
		void
			PmtmgmpTag::DecrementTtl()
		{
			m_ttl--;
		}
		void PmtmgmpTag::IncreaseChange()
		{
			m_ChangePath++;
		}
#ifndef PMTMGMP_UNUSED_MY_CODE
		void PmtmgmpTag::SetMSECPindex(uint8_t index)
		{
			m_MSECPindex = index;
		}
		void PmtmgmpTag::SetChangePath(uint8_t change)
		{
			m_ChangePath = change;
		}
		uint8_t PmtmgmpTag::GetMSECPindex()
		{
			return m_MSECPindex;
		}
		uint8_t PmtmgmpTag::GetChangePath()
		{
			return m_ChangePath;
		}
#ifdef PMTMGMP_TAG_INFOR_ATTACH
		void PmtmgmpTag::SetSendIndex(uint32_t index)
		{
			m_SendIndex = index;
		}
		uint32_t PmtmgmpTag::GetSendIndex()
		{
			return m_SendIndex;
		}
#endif
#endif
	} // namespace my11s
} // namespace ns3
