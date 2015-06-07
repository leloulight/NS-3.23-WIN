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
			m_partPath = CreateObject<PmtmgmpRPpath>();
		}
		////附带路径相关函数
		void IePgen::AddPartPathNode(Mac48Address address)
		{
			m_partPath->AddPartPathNode(address);
		}
		Ptr<PmtmgmpRPpath> IePgen::GetPartPathNodeList()
		{
			return m_partPath;
		}
		void IePgen::SetOriginatorAddress(Mac48Address originator_address)
		{
			m_partPath->SetMTERPaddress(originator_address);
		}
		void IePgen::SetMSECPaddress(Mac48Address mSECP_address)
		{
			m_partPath->SetMSECPaddress(mSECP_address);
		}
		void IePgen::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_partPath->SetPathGenerationSequenceNumber(seq_number);
		}
		void IePgen::SetPathUpdateSeqNumber(uint32_t seq_number)
		{
			m_partPath->SetPathUpdateSeqNumber(seq_number);
		}
		void IePgen::SetNodeType(PmtmgmpProtocol::NodeType nodeType)
		{
			m_partPath->SetNodeType(nodeType);
		}
		void IePgen::SetHopcount(uint8_t hopcount)
		{
			m_partPath->SetHopcount(hopcount);
		}
		void IePgen::SetTTL(uint8_t ttl)
		{
			m_ttl = ttl;
		}
		void IePgen::SetMetric(uint32_t metric)
		{
			m_partPath->SetMetric(metric);
		}
		Mac48Address IePgen::GetOriginatorAddress() const
		{
			return m_partPath->GetMTERPaddress();
		}
		Mac48Address IePgen::GetMSECPaddress() const
		{
			return m_partPath->GetMSECPaddress();
		}
		uint32_t IePgen::GetPathGenerationSequenceNumber() const
		{
			return m_partPath->GetPathGenerationSequenceNumber();
		}
		uint32_t IePgen::GetPathUpdateSeqNumber() const
		{
			return m_partPath->GetPathUpdateSeqNumber();
		}
		PmtmgmpProtocol::NodeType IePgen::GetNodeType() const
		{
			return m_partPath->GetNodeType();
		}
		uint8_t IePgen::GetHopCount() const
		{
			return m_partPath->GetHopCount();
		}
		uint8_t IePgen::GetTtl() const
		{
			return m_ttl;
		}
		uint32_t IePgen::GetMetric() const
		{
			return m_partPath->GetMetric();
		}
		Ptr<PmtmgmpRPpath> IePgen::GetPartPath() const
		{
			return m_partPath;
		}
		void IePgen::DecrementTtl()
		{
			m_ttl--;
			m_partPath->SetHopcount(m_partPath->GetHopCount() + 1);
		}
		void IePgen::IncrementMetric(uint32_t metric, double m, uint8_t q)
		{
			double lastAALM = m_partPath->GetMetric();
			double newALM = metric;
			////按公式积累计算度量，见AALM计算公式
			m_partPath->SetMetric(std::floor((2 * m * newALM * (q + 1) + (m_partPath->GetHopCount() - 1)) / (m_partPath->GetHopCount() + 1) + 0.5));
		}
		WifiInformationElementId IePgen::ElementId() const
		{
			return IE11S_PGEN;
		}
		void IePgen::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " originator address               = " << m_partPath->GetMSECPaddress() << std::endl;
			os << " MSECP address                    = " << m_partPath->GetMSECPaddress() << std::endl;
			os << " path generation sequence number  = " << m_partPath->GetPathGenerationSequenceNumber() << std::endl;
			os << " path update sequence number      = " << m_partPath->GetPathUpdateSeqNumber() << std::endl;
			os << " node type                        = " << m_partPath->GetNodeType() << std::endl;
			os << " TTL                              = " << (uint16_t)m_ttl << std::endl;
			os << " hop count                        = " << (uint16_t)m_partPath->GetHopCount() << std::endl;
			os << " metric                           = " << m_partPath->GetMetric() << std::endl;
			os << " Part Path Node List are:" << std::endl;
			for (std::vector<Mac48Address>::const_iterator iter = m_partPath->GetPartPath().begin(); iter
				!= m_partPath->GetPartPath().end(); iter++)
			{
				os << "    " << *iter << std::endl;
			}
			os << "</information_element>" << std::endl;
		}
		void IePgen::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_partPath->GetMSECPaddress());
			WriteTo(i, m_partPath->GetMSECPaddress());
			i.WriteHtolsbU32(m_partPath->GetPathGenerationSequenceNumber());
			i.WriteHtolsbU32(m_partPath->GetPathUpdateSeqNumber());
			i.WriteU8(m_partPath->GetNodeType());
			i.WriteU8(m_partPath->GetHopCount());
			i.WriteU8(m_ttl);
			i.WriteHtolsbU32(m_partPath->GetMetric());
			i.WriteU8(m_partPath->GetCurrentNodeListNum());

			////避免 vector循环报错
			std::vector<Mac48Address> nodelist = m_partPath->GetPartPath();

			for (std::vector<Mac48Address>::const_iterator iter = nodelist.begin(); iter != nodelist.end(); iter++)
			{
				WriteTo(i, *iter);
			}
		}
		uint8_t IePgen::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			Mac48Address address;
			ReadFrom(i, address);
			m_partPath->SetMTERPaddress(address);
			ReadFrom(i, address);
			m_partPath->SetMSECPaddress(address); 
			m_partPath->SetPathGenerationSequenceNumber(i.ReadLsbtohU32());
			m_partPath->SetPathUpdateSeqNumber(i.ReadLsbtohU32());
			m_partPath->SetNodeType((PmtmgmpProtocol::NodeType) i.ReadU8());
			m_partPath->SetHopcount(i.ReadU8());
			m_ttl = i.ReadU8();
			m_partPath->SetMetric(i.ReadLsbtohU32());
			uint8_t size = i.ReadU8();
			Mac48Address temp;
			for (int j = 0; j < size; j++)
			{
				ReadFrom(i, temp);
				m_partPath->AddPartPathNode(temp);
			}
			return i.GetDistanceFrom(start);
		}
		uint8_t IePgen::GetInformationFieldSize() const
		{
			uint8_t retval = 6							//Source address (originator)
				+ 6										//MSECP address
				+ 4										//Path Generation Sequence Number
				+ 4										//Path Update Sequence Number
				+ 1 									//NodeType
				+ 1										//Hopcount
				+ 1										//TTL
				+ 4										//metric
				+ 1										//Part Path Node List Size
				+ m_partPath->GetPartPath().size() * 6;	//Part Path Node List
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