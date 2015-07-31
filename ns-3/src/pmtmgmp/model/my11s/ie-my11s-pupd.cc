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
#include "ie-my11s-pupd.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"

////find_if
#include <algorithm>

namespace ns3 {
	namespace my11s {
		/*************************
		* AALMupdatePathData
		************************/
		AALMupdatePathData::AALMupdatePathData(Buffer::Iterator i)
		{
			m_MSECPindex = i.ReadU8();
			m_metric = i.ReadLsbtohU32();
		}
		AALMupdatePathData::AALMupdatePathData(Ptr<PmtmgmpRoutePath> path)
		{
			m_MSECPindex = path->GetMSECPindex();
			m_metric = path->GetMetric();
		}
		AALMupdatePathData::~AALMupdatePathData()
		{
		}
		void AALMupdatePathData::ToBuffer(Buffer::Iterator i) const
		{
			i.WriteU8(m_MSECPindex);
			i.WriteHtolsbU32(m_metric);
		}
		/*************************
		* AALMupdateTreeData
		************************/
		AALMupdateTreeData::AALMupdateTreeData(Buffer::Iterator i)
		{
			ReadFrom(i, m_MTERP);
			uint8_t j = i.ReadU8();
			for (; j > 0; j--)
			{
				m_tree.push_back(AALMupdatePathData(i));
			}
		}
		AALMupdateTreeData::AALMupdateTreeData(Ptr<PmtmgmpRouteTree> tree)
		{
			m_MTERP = tree->GetMTERPaddress();
			std::vector<Ptr<PmtmgmpRoutePath> > temp = tree->GetPartTree();
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = temp.begin(); iter != temp.end(); iter++)
			{
				if ((*iter)->GetStatus() == PmtmgmpRoutePath::Confirmed)
				{
					m_tree.push_back(AALMupdatePathData(*iter));
				}
			}
		}
		AALMupdateTreeData::~AALMupdateTreeData()
		{
		}
		void AALMupdateTreeData::ToBuffer(Buffer::Iterator i) const
		{
			WriteTo(i, m_MTERP);
			uint8_t j = m_tree.size();
			i.WriteU8(m_tree.size());
			for (std::vector<AALMupdatePathData>::const_iterator iter = m_tree.begin(); iter != m_tree.end(); iter++)
			{
				iter->ToBuffer(i);
			}
		}
		////获取此RouteTree数据的长度
		uint8_t AALMupdateTreeData::GetSize() const
		{
			return 6 + 1 + m_tree.size() * 5;
		}
		/*************************
		* IePupd
		************************/
		IePupd::IePupd(Ptr<PmtmgmpRouteTable> table)
		{
			uint8_t index = table->GetPUPDsendRouteTreeIndex();
			std::vector<Ptr<PmtmgmpRouteTree> > temp = table->GetRouteTable();
			if (index > temp.size())
			{
				std::random_shuffle(temp.begin(), temp.end());
				index = 0;
			}
			m_size = 0;
			for (std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = temp.begin(); iter != temp.end(); iter++)
			{
				if ((*iter)->GetTreeSize() != 0)
				{
					m_table.push_back(AALMupdateTreeData(*iter));
					m_size += 1;
				}
			}			
		}
		IePupd::IePupd()
		{

		}
		IePupd::~IePupd()
		{
		}
		void IePupd::SetSize(uint8_t seq_number)
		{
			m_size = seq_number;
		}
		uint8_t IePupd::GetSize() const
		{
			return m_size;
		}
		std::vector<AALMupdateTreeData> IePupd::GetData() const
		{
			return m_table;
		}
		WifiInformationElementId IePupd::ElementId() const
		{
			return IE11S_PUPD;
		}
		void IePupd::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " path generation sequence number  = " << m_size << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IePupd::SerializeInformationField(Buffer::Iterator i) const
		{
			i.WriteU8(m_size);
			for (std::vector<AALMupdateTreeData>::const_iterator iter = m_table.begin(); iter != m_table.end(); iter++)
			{
				iter->ToBuffer(i);
			}
		}
		uint8_t IePupd::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			m_size = i.ReadU8();
			uint8_t j = m_size;
			for (; j > 0; j--)
			{
				m_table.push_back(AALMupdateTreeData(i));
			}
			return i.GetDistanceFrom(start);
		}
		uint8_t IePupd::GetInformationFieldSize() const
		{
			uint8_t retval = 1;			//Path Generation Sequence Number
			for (std::vector<AALMupdateTreeData>::const_iterator iter = m_table.begin(); iter != m_table.end(); iter++)
			{
				retval += iter->GetSize();
			}
			return retval;
		}
		bool operator== (const IePupd & a, const IePupd & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IePupd &secreq)
		{
			secreq.Print(os);
			return os;
		}
	}
}
#endif