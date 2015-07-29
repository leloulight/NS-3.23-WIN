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
#include "ie-my11s-pgen.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include <cmath>

namespace ns3 {
	namespace my11s {
		////其他函数
		IePgen::~IePgen()
		{
		}
		IePgen::IePgen()
		{
			m_PathInfo = CreateObject<PmtmgmpRoutePath>();
		}
		Ptr<PmtmgmpRoutePath> IePgen::GetPathInfo()
		{
			return m_PathInfo;
		}
		void IePgen::SetMTERPaddress(Mac48Address originator_address)
		{
			m_PathInfo->SetMTERPaddress(originator_address);
		}
		void IePgen::SetMSECPaddress(Mac48Address mSECP_address)
		{
			m_PathInfo->SetMSECPaddress(mSECP_address);
		}
		void IePgen::SetMSECPindex(uint8_t index)
		{
			m_PathInfo->SetMSECPindex(index);
		}
		void IePgen::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathInfo->SetPathGenerationSequenceNumber(seq_number);
		}
		void IePgen::SetNodeType(PmtmgmpProtocol::NodeType nodeType)
		{
			m_PathInfo->SetNodeType(nodeType);
		}
		void IePgen::SetHopcount(uint8_t hopcount)
		{
			m_PathInfo->SetHopcount(hopcount);
		}
		void IePgen::SetTTL(uint8_t ttl)
		{
			m_ttl = ttl;
		}
		void IePgen::SetMetric(uint32_t metric)
		{
			m_PathInfo->SetMetric(metric);
		}
		Mac48Address IePgen::GetMTERPAddress() const
		{
			return m_PathInfo->GetMTERPaddress();
		}
		Mac48Address IePgen::GetMSECPaddress() const
		{
			return m_PathInfo->GetMSECPaddress();
		}
		uint8_t IePgen::GetMSECPindex() const
		{
			return m_PathInfo->GetMSECPindex();
		}
		uint32_t IePgen::GetPathGenerationSequenceNumber() const
		{
			return m_PathInfo->GetPathGenerationSequenceNumber();
		}
		PmtmgmpProtocol::NodeType IePgen::GetNodeType() const
		{
			return m_PathInfo->GetNodeType();
		}
		uint8_t IePgen::GetHopCount() const
		{
			return m_PathInfo->GetHopCount();
		}
		uint8_t IePgen::GetTtl() const
		{
			return m_ttl;
		}
		uint32_t IePgen::GetMetric() const
		{
			return m_PathInfo->GetMetric();
		}
		Ptr<PmtmgmpRoutePath> IePgen::GetPathInfo() const
		{
			return m_PathInfo;
		}
		void IePgen::DecrementTtl()
		{
			m_ttl--;
			m_PathInfo->SetHopcount(m_PathInfo->GetHopCount() + 1);
		}
		void IePgen::IncrementMetric(uint32_t metric, double k)
		{
			double lastAALM = m_PathInfo->GetMetric();
			double i = m_PathInfo->GetHopCount();
			////按公式积累计算度量，见AALM计算公式
			m_PathInfo->SetMetric((sqrt(i) * k * metric + (i - 1) * (double) lastAALM ) / sqrt(i * (i + 1)));
		}
		WifiInformationElementId IePgen::ElementId() const
		{
			return IE11S_PGEN;
		}
		void IePgen::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " MTERP address               = " << m_PathInfo->GetMTERPaddress() << std::endl;
			os << " MSECP address                    = " << m_PathInfo->GetMSECPaddress() << std::endl;
			os << " MSECP index                    = " << m_PathInfo->GetMSECPindex() << std::endl;
			os << " path generation sequence number  = " << m_PathInfo->GetPathGenerationSequenceNumber() << std::endl;
			os << " node type                        = " << m_PathInfo->GetNodeType() << std::endl;
			os << " TTL                              = " << (uint16_t)m_ttl << std::endl;
			os << " hop count                        = " << (uint16_t)m_PathInfo->GetHopCount() << std::endl;
			os << " metric                           = " << m_PathInfo->GetMetric() << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IePgen::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_PathInfo->GetMTERPaddress());
			WriteTo(i, m_PathInfo->GetMSECPaddress());
			i.WriteU8(m_PathInfo->GetMSECPindex());
			i.WriteHtolsbU32(m_PathInfo->GetPathGenerationSequenceNumber());
			i.WriteU8(m_PathInfo->GetNodeType());
			i.WriteU8(m_PathInfo->GetHopCount());
			i.WriteU8(m_ttl);
			i.WriteHtolsbU32(m_PathInfo->GetMetric());
		}
		uint8_t IePgen::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
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
			m_ttl = i.ReadU8();
			m_PathInfo->SetMetric(i.ReadLsbtohU32());
			return i.GetDistanceFrom(start);
		}
		uint8_t IePgen::GetInformationFieldSize() const
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
		bool operator== (const IePgen & a, const IePgen & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IePgen &pgen)
		{
			pgen.Print(os);
			return os;
		}
	}
}
#endif