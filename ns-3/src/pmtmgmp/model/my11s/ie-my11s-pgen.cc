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
		////附带路径相关函数
		void IePgen::AddPartPathNode(Mac48Address address, uint8_t pathLength)
		{
			while (m_partPath.size() >= pathLength)
			{
				m_partPath.erase(m_partPath.begin());
			}
			m_partPath.push_back(address);
		}
		std::vector<Mac48Address> IePgen::GetPartPathNodeList()
		{
			return m_partPath;
		}
		////其他函数
		IePgen::~IePgen()
		{
		}
		IePgen::IePgen()
		{

		}
		void IePgen::SetOriginatorAddress(Mac48Address originator_address)
		{
			m_originatorAddress = originator_address;
		}
		void IePgen::SetMSECPaddress(Mac48Address mSECP_address)
		{
			m_MSECPaddress = mSECP_address;
		}
		void IePgen::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathGenerationSeqNumber = seq_number;
		}
		void IePgen::SetPathUpdateSeqNumber(uint32_t seq_number)
		{
			m_PathUpdateSeqNumber = seq_number;
		}
		void IePgen::SetNodeType(PmtmgmpProtocol::NodeType nodeType)
		{
			m_NodeType = nodeType;
		}
		void IePgen::SetHopcount(uint8_t hopcount)
		{
			m_hopCount = hopcount;
		}
		void IePgen::SetTTL(uint8_t ttl)
		{
			m_ttl = ttl;
		}
		void IePgen::SetMetric(uint32_t metric)
		{
			m_metric = metric;
		}
		Mac48Address IePgen::GetOriginatorAddress() const
		{
			return m_originatorAddress;
		}
		Mac48Address IePgen::GetMSECPaddress() const
		{
			return m_MSECPaddress;
		}
		uint32_t IePgen::GetPathGenerationSequenceNumber() const
		{
			return m_PathGenerationSeqNumber;
		}
		uint32_t IePgen::GetPathUpdateSeqNumber() const
		{
			return m_PathUpdateSeqNumber;
		}
		PmtmgmpProtocol::NodeType IePgen::GetNodeType() const
		{
			return m_NodeType;
		}
		uint8_t IePgen::GetHopCount() const
		{
			return m_hopCount;
		}
		uint8_t IePgen::GetTtl() const
		{
			return m_ttl;
		}
		uint32_t IePgen::GetMetric() const
		{
			return m_metric;
		}
		void IePgen::DecrementTtl()
		{
			m_ttl--;
			m_hopCount++;
		}
		void IePgen::IncrementMetric(uint32_t metric, double m, uint8_t q)
		{
			double lastAALM = m_metric;
			double newALM = metric;
			////按公式积累计算度量，见AALM计算公式
			m_metric = std::floor((2 * m * newALM * (q + 1) + (m_hopCount - 1)) / (m_hopCount + 1) + 0.5);
		}
		WifiInformationElementId IePgen::ElementId() const
		{
			return IE11S_PGEN;
		}
		void IePgen::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " originator address               = " << m_originatorAddress << std::endl;
			os << " MSECP address                    = " << m_MSECPaddress << std::endl;
			os << " path generation sequence number  = " << m_PathGenerationSeqNumber << std::endl;
			os << " path update sequence number      = " << m_PathUpdateSeqNumber << std::endl;
			os << " node type                        = " << m_NodeType << std::endl;
			os << " TTL                              = " << (uint16_t)m_ttl << std::endl;
			os << " hop count                        = " << (uint16_t)m_hopCount << std::endl;
			os << " metric                           = " << m_metric << std::endl;
			os << " Part Path Node List are:" << std::endl;
			for (std::vector<Mac48Address>::const_iterator iter = m_partPath.begin(); iter
				!= m_partPath.end(); iter++)
			{
				os << "    " << *iter << std::endl;
			}
			os << "</information_element>" << std::endl;
		}
		void IePgen::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_originatorAddress);
			WriteTo(i, m_MSECPaddress);
			i.WriteHtolsbU32(m_PathGenerationSeqNumber);
			i.WriteHtolsbU32(m_PathUpdateSeqNumber);
			i.WriteU8(m_NodeType);
			i.WriteU8(m_hopCount);
			i.WriteU8(m_ttl);
			i.WriteHtolsbU32(m_metric);
			i.WriteU8(m_partPath.size());
			for (std::vector<Mac48Address>::const_iterator iter = m_partPath.begin(); iter
				!= m_partPath.end(); iter++)
			{
				WriteTo(i, *iter);
			}
		}
		uint8_t IePgen::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			ReadFrom(i, m_originatorAddress);
			ReadFrom(i, m_MSECPaddress);
			m_PathGenerationSeqNumber = i.ReadLsbtohU32();
			m_PathUpdateSeqNumber = i.ReadLsbtohU32();
			m_NodeType = (PmtmgmpProtocol::NodeType) i.ReadU8();
			m_hopCount = i.ReadU8();
			m_ttl = i.ReadU8();
			m_metric = i.ReadLsbtohU32();
			uint8_t size = i.ReadU8();
			for (int j = 0; j < size; j++)
			{
				Mac48Address address; 
				ReadFrom(i, address);
				m_partPath.push_back(address);
			}
			return i.GetDistanceFrom(start);
		}
		uint8_t IePgen::GetInformationFieldSize() const
		{
			uint8_t retval = 6			//Source address (originator)
				+ 6						//MSECP address
				+ 4						//Path Generation Sequence Number
				+ 4						//Path Update Sequence Number
				+ 1 					//NodeType
				+ 1						//Hopcount
				+ 1						//TTL
				+ 4						//metric
				+ 1						//Part Path Node List Size
				+ m_partPath.size() * 6;//Part Path Node List
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