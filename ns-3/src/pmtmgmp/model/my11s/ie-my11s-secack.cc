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
* Author: Whimsy Duke <whimsyduke@163.com>
*/

#ifndef PMTMGMP_UNUSED_MY_CODE
#include "ie-my11s-secack.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"


namespace ns3 {
	namespace my11s {
		IeSecack::IeSecack()
		{
			m_PathInfo = CreateObject<PmtmgmpRoutePath>();
		}
		IeSecack::IeSecack(Ptr<PmtmgmpRoutePath> path)
		{
			m_PathInfo = path;
		}
		IeSecack::~IeSecack()
		{
		}
		Ptr<PmtmgmpRoutePath> IeSecack::GetPathInfo()
		{
			return m_PathInfo;
		}
		void IeSecack::SetMTERPaddress(Mac48Address originator_address)
		{
			m_PathInfo->SetMTERPaddress(originator_address);
		}
		void IeSecack::SetMSECPaddress(Mac48Address mSECP_address)
		{
			m_PathInfo->SetMSECPaddress(mSECP_address);
		}
		void IeSecack::SetMSECPindex(uint8_t index)
		{
			m_PathInfo->SetMSECPindex(index);
		}
		void IeSecack::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathInfo->SetPathGenerationSequenceNumber(seq_number);
		}
		void IeSecack::SetNodeType(PmtmgmpProtocol::NodeType nodeType)
		{
			m_PathInfo->SetNodeType(nodeType);
		}
		void IeSecack::SetHopcount(uint8_t hopcount)
		{
			m_PathInfo->SetHopcount(hopcount);
		}
		void IeSecack::SetTTL(uint8_t ttl)
		{
			m_PathInfo->SetTTL(ttl);
		}
		void IeSecack::SetMetric(uint32_t metric)
		{
			m_PathInfo->SetMetric(metric);
		}
		Mac48Address IeSecack::GetMTERPaddress() const
		{
			return m_PathInfo->GetMTERPaddress();
		}
		Mac48Address IeSecack::GetMSECPaddress() const
		{
			return m_PathInfo->GetMSECPaddress();
		}
		uint8_t IeSecack::GetMSECPindex() const
		{
			return m_PathInfo->GetMSECPindex();
		}
		uint32_t IeSecack::GetPathGenerationSequenceNumber() const
		{
			return m_PathInfo->GetPathGenerationSequenceNumber();
		}
		PmtmgmpProtocol::NodeType IeSecack::GetNodeType() const
		{
			return m_PathInfo->GetNodeType();
		}
		uint8_t IeSecack::GetHopCount() const
		{
			return m_PathInfo->GetHopCount();
		}
		uint8_t IeSecack::GetTtl() const
		{
			return m_PathInfo->GetHopCount();
		}
		uint32_t IeSecack::GetMetric() const
		{
			return m_PathInfo->GetMetric();
		}
		Ptr<PmtmgmpRoutePath> IeSecack::GetPathInfo() const
		{
			return m_PathInfo;
		}
		WifiInformationElementId IeSecack::ElementId() const
		{
			return IE11S_SECACK;
		}
		void IeSecack::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " MTERP address               = " << m_PathInfo->GetMTERPaddress() << std::endl;
			os << " MSECP address                    = " << m_PathInfo->GetMSECPaddress() << std::endl;
			os << " MSECP index                    = " << m_PathInfo->GetMSECPindex() << std::endl;
			os << " path generation sequence number  = " << m_PathInfo->GetPathGenerationSequenceNumber() << std::endl;
			os << " node type                        = " << m_PathInfo->GetNodeType() << std::endl;
			os << " TTL                              = " << (uint16_t)m_PathInfo->GetTTL() << std::endl;
			os << " hop count                        = " << (uint16_t)m_PathInfo->GetHopCount() << std::endl;
			os << " metric                           = " << m_PathInfo->GetMetric() << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IeSecack::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_PathInfo->GetMTERPaddress());
			WriteTo(i, m_PathInfo->GetMSECPaddress());
			i.WriteU8(m_PathInfo->GetMSECPindex());
			i.WriteHtolsbU32(m_PathInfo->GetPathGenerationSequenceNumber());
			i.WriteU8(m_PathInfo->GetNodeType());
			i.WriteU8(m_PathInfo->GetHopCount());
			i.WriteU8(m_PathInfo->GetTTL());
			i.WriteHtolsbU32(m_PathInfo->GetMetric());
		}
		uint8_t IeSecack::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			Mac48Address address;
			ReadFrom(i, address);
			m_PathInfo->SetMTERPaddress(address);
			ReadFrom(i, address);
			m_PathInfo->SetMSECPaddress(address);
			m_PathInfo->SetMSECPindex(i.ReadU8());
			m_PathInfo->SetPathGenerationSequenceNumber(i.ReadLsbtohU32());
			m_PathInfo->SetNodeType((PmtmgmpProtocol::NodeType) i.ReadU8());
			m_PathInfo->SetHopcount(i.ReadU8());
			m_PathInfo->SetTTL(i.ReadU8());
			m_PathInfo->SetMetric(i.ReadLsbtohU32());
			return i.GetDistanceFrom(start);
		}
		uint8_t IeSecack::GetInformationFieldSize() const
		{
			uint8_t retval = 6							//MTERP address (originator)
				+ 6										//MSECP address
				+ 1										//MSECP Index
				+ 4										//Path Generation Sequence Number
				+ 1 									//NodeType
				+ 1										//Hopcount
				+ 1										//TTL
				+ 4										//metric
				;
			return retval;
		}
		bool operator== (const IeSecack & a, const IeSecack & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IeSecack &secack)
		{
			secack.Print(os);
			return os;
		}
	}
}
#endif