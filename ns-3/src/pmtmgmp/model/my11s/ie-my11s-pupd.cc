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
#include "ns3/core-module.h"

////find_if
#include <algorithm>

namespace ns3 {
	namespace my11s {
		/*************************
		* PUPDaalmPathData
		************************/
		PUPDaalmPathData::PUPDaalmPathData()
		{
		}
		PUPDaalmPathData::PUPDaalmPathData(Buffer::Iterator i)
		{
			m_MSECPindex = i.ReadU8();
			m_metric = i.ReadLsbtohU32();
		}
		PUPDaalmPathData::PUPDaalmPathData(Ptr<PmtmgmpRoutePath> path)
		{
			m_MSECPindex = path->GetMSECPindex();
			m_metric = path->GetMetric();
		}
		PUPDaalmPathData::~PUPDaalmPathData()
		{
		}
		void PUPDaalmPathData::ToBuffer(Buffer::Iterator i) const
		{
			i.WriteU8(m_MSECPindex);
			i.WriteHtolsbU32(m_metric);
		}
		uint8_t PUPDaalmPathData::GetMSECPindex() const
		{
			return m_MSECPindex;
		}
		uint32_t PUPDaalmPathData::GetMetric() const
		{
			return m_metric;
		}
		/*************************
		* PUPDaalmTreeData
		************************/
		PUPDaalmTreeData::PUPDaalmTreeData()
		{
		}
		PUPDaalmTreeData::PUPDaalmTreeData(Buffer::Iterator i)
		{
			ReadFrom(i, m_MTERPaddress);
			uint8_t j = i.ReadU8();
			for (; j > 0; j--)
			{
				m_tree.push_back(PUPDaalmPathData(i));
			}
		}
		PUPDaalmTreeData::PUPDaalmTreeData(Ptr<PmtmgmpRouteTree> tree)
		{
			m_MTERPaddress = tree->GetMTERPaddress();
			std::vector<Ptr<PmtmgmpRoutePath> > temp = tree->GetPartTree();
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = temp.begin(); iter != temp.end(); iter++)
			{
				if ((*iter)->GetStatus() == PmtmgmpRoutePath::Confirmed && Simulator::Now() - (*iter)->GetPGENsendTime() > (*iter)->GetPathRecreateDelay())
				{
					m_tree.push_back(PUPDaalmPathData(*iter));
				}
			}
		}
		PUPDaalmTreeData::~PUPDaalmTreeData()
		{
		}
		std::vector<PUPDaalmPathData> PUPDaalmTreeData::GetData() const
		{
			return m_tree;
		}
		void PUPDaalmTreeData::ToBuffer(Buffer::Iterator i) const
		{
			WriteTo(i, m_MTERPaddress);
			i.WriteU8(m_tree.size());
			for (std::vector<PUPDaalmPathData>::const_iterator iter = m_tree.begin(); iter != m_tree.end(); iter++)
			{
				iter->ToBuffer(i);
			}
		}
		////获取此RouteTree数据的长度
		uint8_t PUPDaalmTreeData::GetSize() const
		{
			return 6 + 1 + m_tree.size() * 5;
		}
		Mac48Address PUPDaalmTreeData::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		/*************************
		* IePupd
		************************/
		IePupd::IePupd(Ptr<PmtmgmpRouteTable> table, uint16_t maxPath)
		{
			uint8_t index = table->GetPUPDsendRouteTreeIndex();
			std::vector<Ptr<PmtmgmpRouteTree> > temp = table->GetRouteTable();
			if (temp.size() == 0)
			{
				IePupd();
				return;
			}
			if (index >= temp.size())
			{
				std::random_shuffle(temp.begin(), temp.end());
				index = 0;
			}
			uint16_t pathNum = 0;			
			uint16_t tempPathSize = temp[index]->GetPartTree().size();

			////至少发送一个路由树的内容
			do
			{
				if (tempPathSize != 0)
				{
					pathNum += tempPathSize;
					m_table.push_back(PUPDaalmTreeData(temp[index]));
				}
				////更新临时路由表Index
				if (index >= temp.size() - 1)
				{
					index = 0;
				}
				else
				{
					index++;
				}
				if (index == table->GetPUPDsendRouteTreeIndex())
				{
					////已经添加完整路由表数据
					break;
				}
				tempPathSize = temp[index]->GetPartTree().size();
			} while (pathNum + tempPathSize < maxPath);
			////更新路由表Index
			table->SetPUPDsendRouteTreeIndex(index);
		}
		IePupd::IePupd()
		{

		}
		IePupd::~IePupd()
		{
		}
		std::vector<PUPDaalmTreeData> IePupd::GetData() const
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
			os << " Route Path size  = " << m_table.size() << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IePupd::SerializeInformationField(Buffer::Iterator i) const
		{
			i.WriteU8(m_table.size());
			for (std::vector<PUPDaalmTreeData>::const_iterator iter = m_table.begin(); iter != m_table.end(); iter++)
			{
				iter->ToBuffer(i);
			}
		}
		uint8_t IePupd::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			uint8_t j = i.ReadU8();
			for (; j > 0; j--)
			{
				m_table.push_back(PUPDaalmTreeData(i));
			}
			return i.GetDistanceFrom(start);
		}
		uint8_t IePupd::GetInformationFieldSize() const
		{
			uint8_t retval = 1;			//Size
			for (std::vector<PUPDaalmTreeData>::const_iterator iter = m_table.begin(); iter != m_table.end(); iter++)
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