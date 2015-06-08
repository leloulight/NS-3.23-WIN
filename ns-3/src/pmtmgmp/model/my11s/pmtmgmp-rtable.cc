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

#include "ns3/object.h"
#include "ns3/assert.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/log.h"
#ifndef PMTMGMP_UNUSED_MY_CODE
 ////find_if
#include <algorithm>
#endif

#include "pmtmgmp-rtable.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("PmtmgmpRtable");

	namespace my11s {

		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpRtable);

#ifndef PMTMGMP_UNUSED_MY_CODE
		/*************************
		* PmtmgmpRPpath
		************************/
		PmtmgmpRPpath::PmtmgmpRPpath() :
			m_PMTMGMPpathNodeListNum(2)
		{
		}

		PmtmgmpRPpath::~PmtmgmpRPpath()
		{
		}
		TypeId PmtmgmpRPpath::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpRPpath")
				.SetParent<Object>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpRPpath>()
				.AddAttribute("PMTMGMPpgenNodeListNum",
					"Maximum number of the last part of the path that PGEN have generationed, which is used for computing the repeatability of path. t need define the same as \"ns3::my11s::PmtmgmpProtocol::MSECPnumForMTERP\"",
					UintegerValue(4),
					MakeUintegerAccessor(
						&PmtmgmpRPpath::m_PMTMGMPpathNodeListNum),
					MakeUintegerChecker<uint8_t>(0)
					)
				;
			return tid;
		}
		std::vector<Mac48Address> PmtmgmpRPpath::GetPartPath() const
		{
			return m_partPath;
		}
		void PmtmgmpRPpath::SetMTERPaddress(Mac48Address address)
		{
			m_MTERPaddress = address;
		}
		void PmtmgmpRPpath::SetMSECPaddress(Mac48Address address)
		{
			m_MSECPaddress = address;
		}
		void PmtmgmpRPpath::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathGenerationSeqNumber = seq_number;
		}
		void PmtmgmpRPpath::SetPathUpdateSeqNumber(uint32_t seq_number)
		{
			m_PathUpdateSeqNumber = seq_number;
		}
		void PmtmgmpRPpath::SetNodeType(PmtmgmpProtocol::NodeType nodeType)
		{
			m_NodeType = nodeType;
		}
		void PmtmgmpRPpath::SetHopcount(uint8_t hopcount)
		{
			m_hopCount = hopcount;
		}
		void PmtmgmpRPpath::SetMetric(double metric)
		{
			m_metric = metric;
		}
		Mac48Address PmtmgmpRPpath::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		Mac48Address PmtmgmpRPpath::GetMSECPaddress() const
		{
			return m_MSECPaddress;
		}
		uint32_t PmtmgmpRPpath::GetPathGenerationSequenceNumber() const
		{
			return m_PathGenerationSeqNumber;
		}
		uint32_t PmtmgmpRPpath::GetPathUpdateSeqNumber() const
		{
			return m_PathUpdateSeqNumber;
		}
		PmtmgmpProtocol::NodeType PmtmgmpRPpath::GetNodeType() const
		{
			return m_NodeType;
		}
		uint8_t PmtmgmpRPpath::GetHopCount() const
		{
			return m_hopCount;
		}
		double PmtmgmpRPpath::GetMetric() const
		{
			return m_metric;
		}
		////获取路径重复度
		uint8_t PmtmgmpRPpath::GetPartPathRepeatability(PmtmgmpRPpath compare)
		{
			uint8_t Repeatability = 0;
			for (std::vector<Mac48Address>::iterator selectIter = m_partPath.begin(); selectIter != m_partPath.end(); selectIter++)
			{
				std::vector<Mac48Address>::iterator iter = std::find_if(compare.GetPartPath().begin(), compare.GetPartPath().end(), PmtmgmpRPpath_Finder(*selectIter));
				if (iter == compare.GetPartPath().end())
				{
					Repeatability++;
				}
			}
			return Repeatability;
		}
		////添加节点
		void PmtmgmpRPpath::AddPartPathNode(Mac48Address address)
		{
			while (m_partPath.size() >= m_PMTMGMPpathNodeListNum)
			{
				m_partPath.erase(m_partPath.begin());
			}
			m_partPath.push_back(address);
		}
		////获取路径结点地址
		Mac48Address PmtmgmpRPpath::GetNodeMac(int8_t index)
		{
			if (index < m_partPath.size())
			{
				return m_partPath[index];
			}
			else
			{
				NS_LOG_ERROR("Out of size.");
			}
		}
		////获取路径最大结点数量
		uint8_t PmtmgmpRPpath::GetMaxNodeListNum()
		{
			return m_PMTMGMPpathNodeListNum;
		}
		////获取路径当前结点数量
		uint8_t PmtmgmpRPpath::GetCurrentNodeListNum()
		{
			return m_partPath.size();
		}
		////复制类
		Ptr<PmtmgmpRPpath> PmtmgmpRPpath::GetCopy()
		{
			Ptr<PmtmgmpRPpath> copy = CreateObject<PmtmgmpRPpath>();
			copy->SetMTERPaddress(m_MTERPaddress);
			copy->SetMSECPaddress(m_MTERPaddress);
			copy->SetPathGenerationSequenceNumber(m_PathGenerationSeqNumber);
			copy->SetPathUpdateSeqNumber(m_PathUpdateSeqNumber);
			copy->SetNodeType(m_NodeType);
			copy->SetHopcount(m_hopCount);
			copy->SetMetric(m_metric);
			for (std::vector<Mac48Address>::iterator selectIter = m_partPath.begin(); selectIter != m_partPath.end(); selectIter++)
			{
				copy->AddPartPathNode(*selectIter);
			}
			return copy;
		}
		/*************************
		* PmtmgmpRPtree
		************************/
		PmtmgmpRPtree::PmtmgmpRPtree() :
			m_MSECPnumForMTERP(2)
		{
		}

		PmtmgmpRPtree::~PmtmgmpRPtree()
		{
		}
		TypeId PmtmgmpRPtree::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpRPtree")
				.SetParent<Object>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpRPtree>()
				.AddAttribute("MSECPnumForMTERP",
					"The number of MESCP that each MTERP can have.",
					UintegerValue(2),
					MakeUintegerAccessor(
						&PmtmgmpRPtree::m_MSECPnumForMTERP),
					MakeUintegerChecker<uint8_t>(1)
					)
				;
			return tid;
		}
		std::vector<Ptr<PmtmgmpRPpath>> PmtmgmpRPtree::GetPartTree()
		{
			return m_partTree;
		}
		void PmtmgmpRPtree::SetMTERPaddress(Mac48Address address)
		{
			m_MTERPaddress = address;
		}
		Mac48Address PmtmgmpRPtree::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		////获取MTERP、MSECP对应的Path
		Ptr<PmtmgmpRPpath> PmtmgmpRPtree::GetPathByMACaddress(Mac48Address mterp, Mac48Address msecp)
		{
			std::vector<Ptr<PmtmgmpRPpath>>::iterator iter = std::find_if(m_partTree.begin(), m_partTree.end(), PmtmgmpRPtree_Finder(mterp, msecp));
			if (iter == m_partTree.end())
			{
				return 0;
				//NS_LOG_ERROR("Can not find the path with MTERP is " << mterp << ",MSECP is " << msecp);
			}
			else
			{
				return *iter;
			}
		}
		std::vector<Ptr<PmtmgmpRPpath>> PmtmgmpRPtree::GetBestPath()
		{
			if (m_partTree.size() == 0) NS_LOG_ERROR("There is no path in this tree.");
			std::vector<Ptr<PmtmgmpRPpath>>::iterator bestIter = m_partTree.begin();
			std::vector<Ptr<PmtmgmpRPpath>> bestPaths;
			bestPaths.push_back(*bestIter);
			for (std::vector<Ptr<PmtmgmpRPpath>>::iterator iter = m_partTree.begin(); iter != m_partTree.end(); iter++)
			{
				if ((*bestIter)->GetMetric() < (*iter)->GetMetric())
				{
					bestPaths.clear();
					bestPaths.push_back(*iter);
					bestIter == iter;
				}
				else if ((*bestIter)->GetMetric() == (*iter)->GetMetric())
				{
					bestPaths.push_back(*iter);
				}
			}
			return bestPaths;
		}
		////添加新路径
		void PmtmgmpRPtree::AddNewPath(Ptr<PmtmgmpRPpath> path)
		{
			std::vector<Ptr<PmtmgmpRPpath>>::iterator exist = std::find_if(m_partTree.begin(), m_partTree.end(), PmtmgmpRPtree_Finder(path->GetMTERPaddress(), path->GetMSECPaddress()));
			if (exist == m_partTree.end())
			{
				m_partTree.erase(exist);
			}
			m_partTree.push_back(path->GetCopy());
			if (m_partTree.size() > m_MSECPnumForMTERP) NS_LOG_ERROR("Too many path have add to tree");
		}
		////获取当前路径的最大生成顺序号
		uint32_t PmtmgmpRPtree::GetTreeMaxGenerationSeqNumber()
		{
			uint32_t seqNum = 0;
			for (std::vector<Ptr<PmtmgmpRPpath>>::iterator iter = m_partTree.begin(); iter != m_partTree.end(); iter++)
			{
				if ((*iter)->GetPathGenerationSequenceNumber() > seqNum)
				{
					seqNum = (*iter)->GetPathGenerationSequenceNumber();
				}
			}
			return seqNum;
		}
		/*************************
		* PmtmgmpRPRouteTable
		************************/
		PmtmgmpRPRouteTable::PmtmgmpRPRouteTable()
		{
		}
		PmtmgmpRPRouteTable::~PmtmgmpRPRouteTable()
		{
		}
		std::vector<Ptr<PmtmgmpRPtree>> PmtmgmpRPRouteTable::GetRouteTable()
		{
			return m_partTable;
		}
		////获取MTERP对应的Tree
		Ptr<PmtmgmpRPtree> PmtmgmpRPRouteTable::GetTreeByMACaddress(Mac48Address mterp)
		{
			std::vector<Ptr<PmtmgmpRPtree>>::iterator iter = std::find_if(m_partTable.begin(), m_partTable.end(), PmtmgmpRProuteTable_Finder(mterp));
			if (iter == m_partTable.end())
			{
				return 0;
				//NS_LOG_ERROR("Can not find the tree with MTERP is " << mterp);
			}
			else
			{
				return *iter;
			}
		}
		////获取MTERP、MSECP对应的Path
		Ptr<PmtmgmpRPpath> PmtmgmpRPRouteTable::GetPathByMACaddress(Mac48Address mterp, Mac48Address msecp)
		{
			Ptr<PmtmgmpRPtree> pTree = GetTreeByMACaddress(mterp);
			if (pTree == 0)
			{
				////没有找到对应的路由树
				return 0;
			}
			return pTree->GetPathByMACaddress(mterp, msecp);
		}
		////添加新路径
		void PmtmgmpRPRouteTable::AddNewPath(Ptr<PmtmgmpRPpath> path)
		{
			Ptr<PmtmgmpRPtree> pTree = GetTreeByMACaddress(path->GetMTERPaddress());
			if (pTree == 0)
			{
				pTree = CreateObject<PmtmgmpRPtree>();
				m_partTable.push_back(pTree);
			}
			
			pTree->AddNewPath(path);
		}
#endif
		/*************************
		* PmtmgmpRtable
		************************/
		TypeId
			PmtmgmpRtable::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpRtable")
				.SetParent<Object>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpRtable>();
			return tid;
		}
		PmtmgmpRtable::PmtmgmpRtable()
		{
			DeleteProactivePath();
		}
		PmtmgmpRtable::~PmtmgmpRtable()
		{
		}
		void
			PmtmgmpRtable::DoDispose()
		{
			m_routes.clear();
		}
		void
			PmtmgmpRtable::AddReactivePath(Mac48Address destination, Mac48Address retransmitter, uint32_t interface,
				uint32_t metric, Time lifetime, uint32_t seqnum)
		{
			std::map<Mac48Address, ReactiveRoute>::iterator i = m_routes.find(destination);
			if (i == m_routes.end())
			{
				ReactiveRoute newroute;
				m_routes[destination] = newroute;
			}
			i = m_routes.find(destination);
			NS_ASSERT(i != m_routes.end());
			i->second.retransmitter = retransmitter;
			i->second.interface = interface;
			i->second.metric = metric;
			i->second.whenExpire = Simulator::Now() + lifetime;
			i->second.seqnum = seqnum;
		}
		void
			PmtmgmpRtable::AddProactivePath(uint32_t metric, Mac48Address root, Mac48Address retransmitter,
				uint32_t interface, Time lifetime, uint32_t seqnum)
		{
			m_root.root = root;
			m_root.retransmitter = retransmitter;
			m_root.metric = metric;
			m_root.whenExpire = Simulator::Now() + lifetime;
			m_root.seqnum = seqnum;
			m_root.interface = interface;
		}
		void
			PmtmgmpRtable::AddPrecursor(Mac48Address destination, uint32_t precursorInterface,
				Mac48Address precursorAddress, Time lifetime)
		{
			Precursor precursor;
			precursor.interface = precursorInterface;
			precursor.address = precursorAddress;
			precursor.whenExpire = Simulator::Now() + lifetime;
			std::map<Mac48Address, ReactiveRoute>::iterator i = m_routes.find(destination);
			if (i != m_routes.end())
			{
				bool should_add = true;
				for (unsigned int j = 0; j < i->second.precursors.size(); j++)
				{
					//NB: Only one active route may exist, so do not check
					//interface ID, just address
					if (i->second.precursors[j].address == precursorAddress)
					{
						should_add = false;
						i->second.precursors[j].whenExpire = precursor.whenExpire;
						break;
					}
				}
				if (should_add)
				{
					i->second.precursors.push_back(precursor);
				}
			}
		}
		void
			PmtmgmpRtable::DeleteProactivePath()
		{
			m_root.precursors.clear();
			m_root.interface = INTERFACE_ANY;
			m_root.metric = MAX_METRIC;
			m_root.retransmitter = Mac48Address::GetBroadcast();
			m_root.seqnum = 0;
			m_root.whenExpire = Simulator::Now();
		}
		void
			PmtmgmpRtable::DeleteProactivePath(Mac48Address root)
		{
			if (m_root.root == root)
			{
				DeleteProactivePath();
			}
		}
		void
			PmtmgmpRtable::DeleteReactivePath(Mac48Address destination)
		{
			std::map<Mac48Address, ReactiveRoute>::iterator i = m_routes.find(destination);
			if (i != m_routes.end())
			{
				m_routes.erase(i);
			}
		}
		PmtmgmpRtable::LookupResult
			PmtmgmpRtable::LookupReactive(Mac48Address destination)
		{
			std::map<Mac48Address, ReactiveRoute>::iterator i = m_routes.find(destination);
			if (i == m_routes.end())
			{
				return LookupResult();
			}
			if ((i->second.whenExpire < Simulator::Now()) && (i->second.whenExpire != Seconds(0)))
			{
				NS_LOG_DEBUG("Reactive route has expired, sorry.");
				return LookupResult();
			}
			return LookupReactiveExpired(destination);
		}
		PmtmgmpRtable::LookupResult
			PmtmgmpRtable::LookupReactiveExpired(Mac48Address destination)
		{
			std::map<Mac48Address, ReactiveRoute>::iterator i = m_routes.find(destination);
			if (i == m_routes.end())
			{
				return LookupResult();
			}
			return LookupResult(i->second.retransmitter, i->second.interface, i->second.metric, i->second.seqnum,
				i->second.whenExpire - Simulator::Now());
		}
		PmtmgmpRtable::LookupResult
			PmtmgmpRtable::LookupProactive()
		{
			if (m_root.whenExpire < Simulator::Now())
			{
				NS_LOG_DEBUG("Proactive route has expired and will be deleted, sorry.");
				DeleteProactivePath();
			}
			return LookupProactiveExpired();
		}
		PmtmgmpRtable::LookupResult
			PmtmgmpRtable::LookupProactiveExpired()
		{
			return LookupResult(m_root.retransmitter, m_root.interface, m_root.metric, m_root.seqnum,
				m_root.whenExpire - Simulator::Now());
		}
		std::vector<PmtmgmpProtocol::FailedDestination>
			PmtmgmpRtable::GetUnreachableDestinations(Mac48Address peerAddress)
		{
			PmtmgmpProtocol::FailedDestination dst;
			std::vector<PmtmgmpProtocol::FailedDestination> retval;
			for (std::map<Mac48Address, ReactiveRoute>::iterator i = m_routes.begin(); i != m_routes.end(); i++)
			{
				if (i->second.retransmitter == peerAddress)
				{
					dst.destination = i->first;
					i->second.seqnum++;
					dst.seqnum = i->second.seqnum;
					retval.push_back(dst);
				}
			}
			//Lookup a path to root
			if (m_root.retransmitter == peerAddress)
			{
				dst.destination = m_root.root;
				dst.seqnum = m_root.seqnum;
				retval.push_back(dst);
			}
			return retval;
		}
		PmtmgmpRtable::PrecursorList
			PmtmgmpRtable::GetPrecursors(Mac48Address destination)
		{
			//We suppose that no duplicates here can be
			PrecursorList retval;
			std::map<Mac48Address, ReactiveRoute>::iterator route = m_routes.find(destination);
			if (route != m_routes.end())
			{
				for (std::vector<Precursor>::const_iterator i = route->second.precursors.begin();
				i != route->second.precursors.end(); i++)
				{
					if (i->whenExpire > Simulator::Now())
					{
						retval.push_back(std::make_pair(i->interface, i->address));
					}
				}
			}
			return retval;
		}
		bool
			PmtmgmpRtable::LookupResult::operator== (const PmtmgmpRtable::LookupResult & o) const
		{
			return (retransmitter == o.retransmitter && ifIndex == o.ifIndex && metric == o.metric && seqnum
				== o.seqnum);
		}
		PmtmgmpRtable::LookupResult::LookupResult(Mac48Address r, uint32_t i, uint32_t m, uint32_t s, Time l) :
			retransmitter(r), ifIndex(i), metric(m), seqnum(s), lifetime(l)
		{
		}
		bool
			PmtmgmpRtable::LookupResult::IsValid() const
		{
			return !(retransmitter == Mac48Address::GetBroadcast() && ifIndex == INTERFACE_ANY && metric == MAX_METRIC
				&& seqnum == 0);
		}
	} // namespace my11s
} // namespace ns3
