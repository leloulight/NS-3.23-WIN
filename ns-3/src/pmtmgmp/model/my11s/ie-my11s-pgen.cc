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
			m_PathInfo = CreateObject<PmtmgmpRPpath>();
		}
#ifdef ROUTE_USE_PART_PATH ////不使用部分路径
		////附带路径相关函数
		void IePgen::AddPartPathNode(Mac48Address address)
		{
			m_PathInfo->AddPartPathNode(address);
		}
#endif
		Ptr<PmtmgmpRPpath> IePgen::GetPathInfo()
		{
			return m_PathInfo;
		}
		void IePgen::SetOriginatorAddress(Mac48Address originator_address)
		{
			m_PathInfo->SetMTERPaddress(originator_address);
		}
		void IePgen::SetMSECPaddress(Mac48Address mSECP_address)
		{
			m_PathInfo->SetMSECPaddress(mSECP_address);
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
		void IePgen::SetMetric(double metric)
		{
			m_PathInfo->SetMetric(metric);
		}
		Mac48Address IePgen::GetOriginatorAddress() const
		{
			return m_PathInfo->GetMTERPaddress();
		}
		Mac48Address IePgen::GetMSECPaddress() const
		{
			return m_PathInfo->GetMSECPaddress();
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
		double IePgen::GetMetric() const
		{
			return m_PathInfo->GetMetric();
		}
		Ptr<PmtmgmpRPpath> IePgen::GetPathInfo() const
		{
			return m_PathInfo;
		}
		void IePgen::DecrementTtl()
		{
			m_ttl--;
			m_PathInfo->SetHopcount(m_PathInfo->GetHopCount() + 1);
		}
		void IePgen::IncrementMetric(double metric, double k)
		{
			double lastAALM = m_PathInfo->GetMetric();
			double newALM = metric;
			////按公式积累计算度量，见AALM计算公式
			m_PathInfo->SetMetric((double)(k * newALM + (m_PathInfo->GetHopCount() - 1) * metric / sqrt(m_PathInfo->GetHopCount())) / sqrt(m_PathInfo->GetHopCount() + 1));
		}
		WifiInformationElementId IePgen::ElementId() const
		{
			return IE11S_PGEN;
		}
		void IePgen::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " originator address               = " << m_PathInfo->GetMSECPaddress() << std::endl;
			os << " MSECP address                    = " << m_PathInfo->GetMSECPaddress() << std::endl;
			os << " path generation sequence number  = " << m_PathInfo->GetPathGenerationSequenceNumber() << std::endl;
			os << " node type                        = " << m_PathInfo->GetNodeType() << std::endl;
			os << " TTL                              = " << (uint16_t)m_ttl << std::endl;
			os << " hop count                        = " << (uint16_t)m_PathInfo->GetHopCount() << std::endl;
			os << " metric                           = " << m_PathInfo->GetMetric() << std::endl;
#ifdef ROUTE_USE_PART_PATH ////不使用部分路径
			os << " Part Path Node List are:" << std::endl;
			for (std::vector<Mac48Address>::const_iterator iter = m_PathInfo->GetPathInfo().begin(); iter
				!= m_PathInfo->GetPathInfo().end(); iter++)
			{
				os << "    " << *iter << std::endl;
			}
#endif
			os << "</information_element>" << std::endl;
		}
		void IePgen::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_PathInfo->GetMSECPaddress());
			WriteTo(i, m_PathInfo->GetMSECPaddress());
			i.WriteHtolsbU32(m_PathInfo->GetPathGenerationSequenceNumber());
			i.WriteU8(m_PathInfo->GetNodeType());
			i.WriteU8(m_PathInfo->GetHopCount());
			i.WriteU8(m_ttl);
			i.WriteHtolsbU32(m_PathInfo->GetMetric());
#ifdef ROUTE_USE_PART_PATH ////不使用部分路径
			i.WriteU8(m_PathInfo->GetCurrentNodeListNum());

			////避免 vector循环报错
			std::vector<Mac48Address> nodelist = m_PathInfo->GetPathInfo();

			for (std::vector<Mac48Address>::const_iterator iter = nodelist.begin(); iter != nodelist.end(); iter++)
			{
				WriteTo(i, *iter);
			}
#endif
		}
		uint8_t IePgen::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			Mac48Address address;
			ReadFrom(i, address);
			m_PathInfo->SetMTERPaddress(address);
			ReadFrom(i, address);
			m_PathInfo->SetMSECPaddress(address); 
			m_PathInfo->SetPathGenerationSequenceNumber(i.ReadLsbtohU32());
			m_PathInfo->SetNodeType((PmtmgmpProtocol::NodeType) i.ReadU8());
			m_PathInfo->SetHopcount(i.ReadU8());
			m_ttl = i.ReadU8();
			m_PathInfo->SetMetric(i.ReadLsbtohU32());
#ifdef ROUTE_USE_PART_PATH ////不使用部分路径
			uint8_t size = i.ReadU8();
			Mac48Address temp;
			for (int j = 0; j < size; j++)
			{
				ReadFrom(i, temp);
				m_PathInfo->AddPartPathNode(temp);
			}
#endif
			return i.GetDistanceFrom(start);
		}
		uint8_t IePgen::GetInformationFieldSize() const
		{
			uint8_t retval = 6							//Source address (originator)
				+ 6										//MSECP address
				+ 4										//Path Generation Sequence Number
				+ 1 									//NodeType
				+ 1										//Hopcount
				+ 1										//TTL
				+ 8										//metric
#ifdef ROUTE_USE_PART_PATH ////不使用部分路径
				+ 1										//Part Path Node List Size
				+ m_PathInfo->GetPathInfo().size() * 6	//Part Path Node List
#endif
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