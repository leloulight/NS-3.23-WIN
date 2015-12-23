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

#include "pmtmgmp-rtable.h"

//Set Not Use AALM
//#define PMTMGMP_UNUSED_AALM 

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("PmtmgmpRtable");

	namespace my11s {

#ifndef PMTMGMP_UNUSED_MY_CODE
		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpRoutePath);
		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpRouteTree);
		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpRouteTable);
#endif
		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpRtable);

#ifndef PMTMGMP_UNUSED_MY_CODE
		/*************************
		* PmtmgmpRoutePath
		************************/
		PmtmgmpRoutePath::PmtmgmpRoutePath() :
			m_MSECPindex(0),
			m_PathGenerationSeqNumber(0),
			m_NodeType(PmtmgmpProtocol::Mesh_STA),
			m_hopCount(0),
			m_metric(0),
			m_BaseMetric(0),
			m_CandidateRouteInformaiton(std::vector<Ptr<PmtmgmpRoutePath> >()),
			m_InformationStatus(Confirmed),
			m_MaxCandidateNum(10),
			m_PMTGMGMProutePathInforLife(MicroSeconds(1024 * 200000)),
			m_PMTMGMPpathRecreateDelay(MicroSeconds(1024 * 4000)),
			m_PmtmgmpPeerLinkStatus(true)
		{
			RoutePathInforLifeUpdate();
		}

		PmtmgmpRoutePath::PmtmgmpRoutePath(Mac48Address mterp, Mac48Address msecp, uint32_t seq_number, PmtmgmpProtocol::NodeType nodeType, uint8_t hopcount, uint8_t ttl, uint32_t metric)
		{
			m_MTERPaddress = mterp;
			m_MSECPaddress = msecp;
			m_PathGenerationSeqNumber = seq_number;
			m_NodeType = nodeType;
			m_hopCount = hopcount;
			m_ttl = ttl;
			m_metric = metric;
			m_BaseMetric = metric;
			RoutePathInforLifeUpdate();
		}

		PmtmgmpRoutePath::~PmtmgmpRoutePath()
		{
		}

		TypeId PmtmgmpRoutePath::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpRoutePath")
				.SetParent<Object>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpRoutePath>()
				.AddAttribute("MaxCandidateNum",
					"Max number of Candidate Information",
					UintegerValue(10),
					MakeUintegerAccessor(
						&PmtmgmpRoutePath::m_MaxCandidateNum),
					MakeUintegerChecker<uint8_t>(0)
					)
				.AddAttribute("PMTGMGMProutePathInforLife",
					"Life of route path information.",
					TimeValue(MicroSeconds(1024 * 200000)),
					MakeTimeAccessor(
						&PmtmgmpRoutePath::m_PMTGMGMProutePathInforLife),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPpathRecreateDelay",
					"Delay for Recreate the Route Path, in Delay Time PUPGQ would not be sended",
					TimeValue(MicroSeconds(1024 * 4000)),
					MakeTimeAccessor(
						&PmtmgmpRoutePath::m_PMTMGMPpathRecreateDelay),
					MakeTimeChecker()
					)
				;
			return tid;
		}
		void PmtmgmpRoutePath::AddCandidateRouteInformaiton(Ptr<PmtmgmpRoutePath> path)
		{
			if (m_CandidateRouteInformaiton.size() <= m_MaxCandidateNum)
			{
				m_CandidateRouteInformaiton.push_back(path);
			}
			else
			{
				NS_LOG_DEBUG("Receive more candidate route informaiton .");
			}
		}
		void PmtmgmpRoutePath::ClearCandidateRouteInformaiton()
		{
			m_CandidateRouteInformaiton.clear();
		}
		std::vector<Ptr<PmtmgmpRoutePath> > PmtmgmpRoutePath::GetCandidateRouteInformaiton() const
		{
			return m_CandidateRouteInformaiton;
		}
		uint8_t PmtmgmpRoutePath::GetMaxCandidateNum() const
		{
			return m_MaxCandidateNum;
		}
		void PmtmgmpRoutePath::SetMTERPaddress(Mac48Address address)
		{
			m_MTERPaddress = address;
		}
		void PmtmgmpRoutePath::SetMSECPaddress(Mac48Address address)
		{
			m_MSECPaddress = address;
		}
		void PmtmgmpRoutePath::SetMSECPindex(uint8_t index)
		{
			m_MSECPindex = index;
		}
		void PmtmgmpRoutePath::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathGenerationSeqNumber = seq_number;
		}
		void PmtmgmpRoutePath::SetNodeType(PmtmgmpProtocol::NodeType nodeType)
		{
			m_NodeType = nodeType;
		}
		void PmtmgmpRoutePath::SetHopcount(uint8_t hopcount)
		{
			m_hopCount = hopcount;
		}
		void PmtmgmpRoutePath::SetTTL(uint8_t ttl)
		{
			m_ttl = ttl;
		}
		void PmtmgmpRoutePath::SetMetric(uint32_t metric)
		{
			m_BaseMetric = metric;
			m_metric = metric;
		}
		void PmtmgmpRoutePath::SetAllMetric(uint32_t metric, uint32_t base)
		{
			m_metric = metric;
			m_BaseMetric = base;
		}
		void PmtmgmpRoutePath::SetInterface(uint32_t interf)
		{
			m_interface = interf;
		}
		void PmtmgmpRoutePath::SetFromNode(Mac48Address address)
		{
			m_FormNode = address;
		}
		void PmtmgmpRoutePath::SetStatus(RouteInformationStatus status)
		{
			m_InformationStatus = status;
		}
		void PmtmgmpRoutePath::SetAcceptCandidateRouteInformaitonEvent(EventId id)
		{
			m_AcceptCandidateRouteInformaitonEvent = id;
		}
		void PmtmgmpRoutePath::SetPmtmgmpPeerLinkStatus(bool status)
		{
			m_PmtmgmpPeerLinkStatus = status;
		}
		void PmtmgmpRoutePath::SetPGENsendTime()
		{
			m_PMTGMGMPpgenSendTime = Simulator::Now();
		}
		Mac48Address PmtmgmpRoutePath::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		Mac48Address PmtmgmpRoutePath::GetMSECPaddress() const
		{
			return m_MSECPaddress;
		}
		uint8_t PmtmgmpRoutePath::GetMSECPindex() const
		{
			return m_MSECPindex;
		}
		uint32_t PmtmgmpRoutePath::GetPathGenerationSequenceNumber() const
		{
			return m_PathGenerationSeqNumber;
		}
		PmtmgmpProtocol::NodeType PmtmgmpRoutePath::GetNodeType() const
		{
			return m_NodeType;
		}
		uint8_t PmtmgmpRoutePath::GetHopCount() const
		{
			return m_hopCount;
		}
		uint8_t PmtmgmpRoutePath::GetTTL() const
		{
			return m_ttl;
		}
		uint32_t PmtmgmpRoutePath::GetMetric() const
		{
			return m_metric;
		}
		uint32_t PmtmgmpRoutePath::GetBaseMetric() const
		{
			return m_BaseMetric;
		}
		uint32_t PmtmgmpRoutePath::GetInterface() const
		{
			return m_interface;
		}
		Mac48Address PmtmgmpRoutePath::GetFromNode() const
		{
			return m_FormNode;
		}
		PmtmgmpRoutePath::RouteInformationStatus PmtmgmpRoutePath::GetStatus() const
		{
			return m_InformationStatus;
		}
		EventId PmtmgmpRoutePath::GetAcceptCandidateRouteInformaitonEvent() const
		{
			return m_AcceptCandidateRouteInformaitonEvent;
		}
		bool PmtmgmpRoutePath::GetPmtmgmpPeerLinkStatus() const
		{
			return m_PmtmgmpPeerLinkStatus;
		}
		Time PmtmgmpRoutePath::GetPGENsendTime() const
		{
			return m_PMTGMGMPpgenSendTime;
		}
		Time PmtmgmpRoutePath::GetPathRecreateDelay() const
		{
			return m_PMTMGMPpathRecreateDelay;
		}
		void PmtmgmpRoutePath::DecrementTtl()
		{
			m_ttl--;
			m_hopCount++;
		}
		void PmtmgmpRoutePath::IncreasePathGenerationSequenceNumber()
		{
			m_PathGenerationSeqNumber++;
		}
		////Python调用函数
		uint8_t PmtmgmpRoutePath::GetCandidateRouteInformaitonSize() const
		{
			return m_CandidateRouteInformaiton.size();
		}
		Ptr<PmtmgmpRoutePath> PmtmgmpRoutePath::GetCandidateRouteInformaitonItem(uint8_t index) const
		{
			return m_CandidateRouteInformaiton[index];
		}
		////路径信息更新自检
		bool PmtmgmpRoutePath::RoutePathInforLifeCheck(Ptr<PmtmgmpRouteTable> table)
		{
			////没有新路径（GSN增加时）不会删除旧有路径
			if (m_InformationStatus == Confirmed) return false;
			bool checker = Simulator::Now() - m_PMTGMGMProutePathInforUpdateTime > m_PMTGMGMProutePathInforLife;
			if (checker && table->GetMac48Address() == m_MSECPaddress)
			{
				table->DecreaseAsMSECPcount();
			}
			return checker;
		}
		////路径信息更新
		void PmtmgmpRoutePath::RoutePathInforLifeUpdate()
		{
			m_PMTGMGMProutePathInforUpdateTime = Simulator::Now();
		}
		////复制类(仅复制PGEN相关属性)
		Ptr<PmtmgmpRoutePath> PmtmgmpRoutePath::GetCopy()
		{
			Ptr<PmtmgmpRoutePath> copy = CreateObject<PmtmgmpRoutePath>();
			copy->SetMTERPaddress(m_MTERPaddress);
			copy->SetMSECPaddress(m_MSECPaddress);
			copy->SetMSECPindex(m_MSECPindex);
			copy->SetPathGenerationSequenceNumber(m_PathGenerationSeqNumber);
			copy->SetNodeType(m_NodeType);
			copy->SetHopcount(m_hopCount);
			copy->SetTTL(m_ttl);
			copy->SetAllMetric(m_metric, m_BaseMetric);
			copy->SetStatus(m_InformationStatus);
			copy->SetInterface(m_interface);
			return copy;
		}
		////度量值更新
		void PmtmgmpRoutePath::IncrementMetric(uint32_t metric, double k)
		{
#ifndef PMTMGMP_UNUSED_AALM
			////按公式积累计算度量，见AALM计算公式
			if (metric == 0xffffffff)
			{
				m_metric = 0xffffffff;
			}
			else if (m_BaseMetric == 0xffffffff)
			{
				m_metric = 0xffffffff;
			}
			else
			{
				//m_metric = (sqrt(m_hopCount) * k * metric + (m_hopCount - 1) * (double)m_BaseMetric) / sqrt(m_hopCount * (m_hopCount + 1));
				m_metric = (m_hopCount * k * metric + (m_hopCount - 1) * (double)m_BaseMetric) / (m_hopCount + 1);
			}
#else
			m_metric += metric;
#endif 
		}
		bool PmtmgmpRoutePath::MSECPselectRoutePathMetricCompare(Ptr<PmtmgmpRoutePath> path)
		{
			if (m_metric == path->GetMetric())
			{
				Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
				rand->SetAttribute("Min", DoubleValue(-0.5));
				rand->SetAttribute("Max", DoubleValue(1.5));
				if (rand->GetInteger(0, 1) == 1)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return m_metric > path->GetMetric();
			}
			return false;
		}
		bool PmtmgmpRoutePath::DataSendRoutePathMetricCompare(Ptr<PmtmgmpRoutePath> path)
		{
			if (path == this) return false;
			if (m_PathGenerationSeqNumber == path->GetPathGenerationSequenceNumber() && (m_metric * uint8_t(m_InformationStatus) == path->GetMetric() * uint8_t(path->GetStatus())))
			{
				Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
				rand->SetAttribute("Min", DoubleValue(-0.5));
				rand->SetAttribute("Max", DoubleValue(1.5));
				if (rand->GetInteger(0, 1) == 1)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (m_metric * uint8_t(m_InformationStatus) == path->GetMetric() * uint8_t(path->GetStatus()))
				{
					return m_PathGenerationSeqNumber < path->GetPathGenerationSequenceNumber();
				}
				else
				{
					return m_metric * uint8_t(m_InformationStatus) > path->GetMetric() * uint8_t(path->GetStatus());
				}
			}
			return false;
		}
		/*************************
		* PmtmgmpRouteTree
		************************/
		PmtmgmpRouteTree::PmtmgmpRouteTree() :
			m_tree(std::vector<Ptr<PmtmgmpRoutePath> >()),
			m_MSECPnumForMTERP(5),
			m_AcceptInformaitonDelay(MicroSeconds(1024 * 1500)),
			m_NotSelectBestRoutePathRate(3)
		{
		}

		PmtmgmpRouteTree::~PmtmgmpRouteTree()
		{
		}
		TypeId PmtmgmpRouteTree::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpRouteTree")
				.SetParent<Object>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpRouteTree>()
				.AddAttribute("MSECPnumForMTERP",
					"The number of MESCP that each MTERP can have.",
					UintegerValue(5),
					MakeUintegerAccessor(
						&PmtmgmpRouteTree::m_MSECPnumForMTERP),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("AcceptInformaitonDelay",
					"Delay for accept information.",
					TimeValue(MicroSeconds(1024 * 1500)),
					MakeTimeAccessor(
						&PmtmgmpRouteTree::m_AcceptInformaitonDelay),
					MakeTimeChecker()
					)
				.AddAttribute("NotSelectBestRoutePathRate",
					"The rate for compare selected path and the best Metric path.",
					DoubleValue(3),
					MakeUintegerAccessor(
						&PmtmgmpRouteTree::m_NotSelectBestRoutePathRate),
					MakeUintegerChecker<double>(0)
					)
				;
			return tid;
		}
		std::vector<Ptr<PmtmgmpRoutePath> > PmtmgmpRouteTree::GetPartTree()
		{
			return m_tree;
		}
		void PmtmgmpRouteTree::SetMTERPaddress(Mac48Address address)
		{
			m_MTERPaddress = address;
		}
		Mac48Address PmtmgmpRouteTree::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		Time PmtmgmpRouteTree::GetAcceptInformaitonDelay() const
		{
			return m_AcceptInformaitonDelay;
		}
		////获取MTERP、MSECP(Index)对应的Path，找不到则返回0
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTree::GetPathByMACaddress(Mac48Address msecp)
		{
			if (m_tree.size() == 0) return 0;
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = std::find_if(m_tree.begin(), m_tree.end(), PmtmgmpRouteTree_AddressFinder(msecp));
			if (iter == m_tree.end())
			{
				return 0;
			}
			else
			{
				return *iter;
			}
		}
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTree::GetPathByMACindex(uint8_t index)
		{
			if (m_tree.size() == 0) return 0;
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = std::find_if(m_tree.begin(), m_tree.end(), PmtmgmpRouteTree_IndexFinder(index));
			if (iter == m_tree.end())
			{
				return 0;
			}
			else
			{
				return *iter;
			}
		}
		////添加新路径
		void PmtmgmpRouteTree::AddNewPath(Ptr<PmtmgmpRoutePath> path)
		{
			if (m_tree.size() >= 255) NS_LOG_ERROR("Add too many path to a PmtmgmpRouteTree(MTERP:" << m_MTERPaddress << ")");
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = std::find_if(m_tree.begin(), m_tree.end(), PmtmgmpRouteTree_AddressFinder(path->GetMSECPaddress()));
			if (iter != m_tree.end())
			{
				m_tree.erase(iter);
			}
			m_tree.push_back(path);
			if (m_tree.size() > m_MSECPnumForMTERP) NS_LOG_ERROR("Too many path have add to tree");
		}
		////选择MSECP
		void PmtmgmpRouteTree::SelectMSECP()
		{
			std::vector<Ptr<PmtmgmpRoutePath> > tree = std::vector<Ptr<PmtmgmpRoutePath> >();
			uint8_t size = m_tree.size();
			if (size > m_MSECPnumForMTERP)
			{
				size = m_MSECPnumForMTERP;
			}
			for (int i = 0; i < size; i++)
			{
				std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = GetBestMSECPpath();
				Ptr<PmtmgmpRoutePath> path = *iter;
				tree.push_back(path);
				m_tree.erase(iter);
			}
			m_tree = tree;
			//std::sort(m_tree.begin(), m_tree.end(), &MSECPselectRoutePathMetricCompare);
			//if (m_tree.size() > m_MSECPnumForMTERP)
			//{
			//	std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin() + m_MSECPnumForMTERP;
			//	m_tree.erase(iter, m_tree.end());
			//}
		}
		void PmtmgmpRouteTree::AddMSECPpath(Ptr<PmtmgmpRoutePath> path)
		{
			if (m_tree.size() >= 255) NS_LOG_ERROR("Add too many path to a PmtmgmpRouteTree when add Candidate MSECP path(MTERP:" << m_MTERPaddress << ")");
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = std::find_if(m_tree.begin(), m_tree.end(), PmtmgmpRouteTree_AddressFinder(path->GetMSECPaddress()));
			if (iter != m_tree.end())
			{
				m_tree.erase(iter);
			}
			m_tree.push_back(path);
		}
		////获取当前路径的最大生成顺序号
		uint32_t PmtmgmpRouteTree::GetTreeMaxGenerationSeqNumber()
		{
			uint32_t seqNum = 0;
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin(); iter != m_tree.end(); iter++)
			{
				if ((*iter)->GetPathGenerationSequenceNumber() > seqNum)
				{
					seqNum = (*iter)->GetPathGenerationSequenceNumber();
				}
			}
			return seqNum;
		}
		////全部路径置为过期
		void PmtmgmpRouteTree::SetAllStatusExpired()
		{
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin(); iter != m_tree.end(); iter++)
			{
				(*iter)->SetStatus(PmtmgmpRoutePath::Expired);
			}
		}
		////获取重复度
		uint8_t PmtmgmpRouteTree::GetRepeatability(Mac48Address from)
		{
			std::map<Mac48Address, uint8_t>::iterator iter = m_repeatability.find(from);
			if (iter == m_repeatability.end())
			{
				return 0;
			}
			else
			{
				return iter->second;
			}
		}
		////增加重复度计量
		void PmtmgmpRouteTree::RepeatabilityIncrease(Mac48Address from)
		{
			std::map<Mac48Address, uint8_t>::iterator iter = m_repeatability.find(from);
			if (iter == m_repeatability.end())
			{
				m_repeatability.insert(std::map<Mac48Address, uint8_t>::value_type(from, 1));
			}
			else
			{
				iter->second++;
			}
		}
		////重置全部重复度
		void PmtmgmpRouteTree::RepeatabilityReset()
		{
			m_repeatability.clear();
		}
		////接受某个候选信息
		void PmtmgmpRouteTree::AcceptCandidateRouteInformaiton(Mac48Address address)
		{
			////已经接受信息
			Ptr<PmtmgmpRoutePath> path = GetPathByMACaddress(address);
			if (path == 0)
			{
				NS_LOG_DEBUG("Path information about MTERP:" << m_MTERPaddress << " MSECP:" << address << "has been Delete before the timer expired.");
				return;
			}
			if (path->GetStatus() == PmtmgmpRoutePath::Confirmed)
			{
				NS_LOG_DEBUG("Path information about MTERP:" << m_MTERPaddress << " MSECP:" << address << "has been Accept before the timer expired.");
				return;
			}

			std::vector<Ptr<PmtmgmpRoutePath> > infomation = path->GetCandidateRouteInformaiton();
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = infomation.begin();
			std::vector<Ptr<PmtmgmpRoutePath> > bests;
			uint8_t repeatability = GetRepeatability((*iter)->GetFromNode());

			////获取最小重复度且AALM最小的候选信息
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator selectIter = infomation.begin(); selectIter != infomation.end(); selectIter++)
			{
				uint8_t temp = GetRepeatability((*selectIter)->GetFromNode());
				if (temp == repeatability)
				{
					if ((*selectIter)->GetMetric() < (*iter)->GetMetric())
					{
						bests.clear();
						bests.push_back((*selectIter));
					}
					else if ((*selectIter)->GetMetric() == (*iter)->GetMetric())
					{
						bests.push_back((*selectIter));
					}
				}
				else if (temp < repeatability)
				{
					repeatability = temp;
					bests.clear();
					bests.push_back((*selectIter));
				}
			}

			Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
			rand->SetAttribute("Min", DoubleValue(0));
			rand->SetAttribute("Max", DoubleValue(bests.size() - 1));

			Ptr<PmtmgmpRoutePath> select = bests[rand->GetValue()];

			NS_LOG_DEBUG("Path information (MTERP:" << path->GetMTERPaddress() << " MSECP:" << path->GetMSECPaddress() << ") accept information from  node: " << select->GetFromNode() << " as a confirm information with the Metric of " << select->GetMetric() << ".");

			select->SetStatus(PmtmgmpRoutePath::Confirmed);
			RepeatabilityIncrease(select->GetFromNode());
			AddNewPath(select);
		}
		////获取确认状态路径的数量
		uint8_t PmtmgmpRouteTree::GetConfirmedPathSize()
		{
			uint8_t size = 0;
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin(); iter != m_tree.end(); iter++)
			{
				if ((*iter)->GetStatus() == PmtmgmpRoutePath::Confirmed) size++;
			}
			return size;
		}
		////Python调用函数
		uint8_t PmtmgmpRouteTree::GetTreeSize() const
		{
			return m_tree.size();
		}
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTree::GetTreeItem(uint8_t index) const
		{
			return m_tree[index];
		}
		uint8_t PmtmgmpRouteTree::GetRepeatabilitySize() const
		{
			return m_repeatability.size();
		}
		uint8_t PmtmgmpRouteTree::GetRepeatabilityItem(uint8_t index) const
		{
			std::map<Mac48Address, uint8_t>::const_iterator iter = m_repeatability.begin();
			for (int i = 1; i < index; i++) ++iter;
			return iter->second;
		}
		////路由树信息寿命检测
		bool PmtmgmpRouteTree::RouteTreeInforLifeCheck(Ptr<PmtmgmpRouteTable> table)
		{
			uint32_t size = m_tree.size();
			m_tree.erase(std::remove_if(m_tree.begin(), m_tree.end(), PmtmgmpRouteTree_PathLifeChecker(table)), m_tree.end());
			if (size != m_tree.size())
			{
				NS_LOG_DEBUG("Delete " << size - m_tree.size() << " paths in MTERP tree " << m_MTERPaddress);
			}
			return m_tree.size() == 0;
		}
		////数据传输最优路径获取
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTree::GetBestRoutePathForData(uint8_t index)
		{
			if (m_tree.size() == 0) return 0;
			////MSECPindex为不会分配0，说明当前没有历史路径（iter == m_tree.end()）
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = std::find_if(m_tree.begin(), m_tree.end(), PmtmgmpRouteTree_IndexFinder(index));
			if (iter == m_tree.end() || (*iter)->GetPmtmgmpPeerLinkStatus() == false)
			{
				////没找到MSECPindex的路径直接返回最优路径
				iter = GetBestRoutePathForData();
			}
			else
			{
				////比较当前使用路由度量和最优路由路径度量，选取最优路径返回
				if ((*iter)->GetMetric() >= (*(m_tree.begin()))->GetMetric() * m_NotSelectBestRoutePathRate)
				{
					iter = GetBestRoutePathForData();
				}
			}
			if (iter != m_tree.end())
			{
				return *iter;
			}
			else
			{
				return 0;
			}
		}
		std::vector<Ptr<PmtmgmpRoutePath> >::iterator PmtmgmpRouteTree::GetBestRoutePathForData()
		{
			if (m_tree.size() == 0) return m_tree.end();
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator path = m_tree.begin();
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin() + 1; iter != m_tree.end(); iter++)
			{
				if ((*path)->DataSendRoutePathMetricCompare(*iter) && (*iter)->GetPmtmgmpPeerLinkStatus() == true)
				{
					path = iter;
				}
			}
			if ((*path)->GetPmtmgmpPeerLinkStatus())
			{
				return path;
			}
			else
			{
				return m_tree.end();
			}
		}
		std::vector<Ptr<PmtmgmpRoutePath> >::iterator PmtmgmpRouteTree::GetBestMSECPpath()
		{
			if (m_tree.size() == 0) return m_tree.end();
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator path = m_tree.begin();
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin() + 1; iter != m_tree.end(); iter++)
			{
				if ((*path)->MSECPselectRoutePathMetricCompare(*iter))
				{
					path = iter;
				}
			}
			return path;
		}
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTree::GetNearestRoutePathForData()
		{
			if (m_tree.size() == 0) return 0;
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator path = m_tree.begin();
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin() + 1; iter != m_tree.end(); iter++)
			{
				if ((*path)->GetHopCount() > (*iter)->GetHopCount() && (*iter)->GetPmtmgmpPeerLinkStatus() == true)
				{
					path = iter;
				}
			}
			if ((*path)->GetPmtmgmpPeerLinkStatus())
			{
				return *path;
			}
			else
			{
				return 0;
			}
		}
		////设置路径链接状态
		void PmtmgmpRouteTree::SetPathPeerLinkStatus(Mac48Address from, bool status)
		{
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = m_tree.begin(); iter != m_tree.end(); iter++)
			{
				if ((*iter)->GetFromNode() == from)
				{
					(*iter)->SetPmtmgmpPeerLinkStatus(status);
				}
			}
		}
		/*************************
		* PmtmgmpRouteTable
		************************/
		PmtmgmpRouteTable::PmtmgmpRouteTable() :
			m_table(std::vector<Ptr<PmtmgmpRouteTree> >()),
			m_PMTGMGMProuteInforCheckPeriod(MicroSeconds(1024 * 1000)),
			m_PUPDsendRouteTreeIndex(0),
			m_MTERPtree(0),
			m_MTERPgenerationSeqNumber(0),
			m_AsMSECPcount(0),
			m_MSECPindex(0),
			m_maxQueueSize(255),
			m_currentQueueSize(0)
		{
			PMTGMGMProuteInforCheckEvent = Simulator::Schedule(m_PMTGMGMProuteInforCheckPeriod, &PmtmgmpRouteTable::RouteTableInforLifeCheck, this);
		}
		PmtmgmpRouteTable::~PmtmgmpRouteTable()
		{
		}
		TypeId PmtmgmpRouteTable::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpRouteTable")
				.SetParent<Object>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpRouteTable>()
				.AddAttribute("PMTGMGMProutePathInforCheckPeriod",
					"Life of route information.",
					TimeValue(MicroSeconds(1024 * 1000)),
					MakeTimeAccessor(
						&PmtmgmpRouteTable::m_PMTGMGMProuteInforCheckPeriod),
					MakeTimeChecker()
					)
				.AddAttribute("MaxQueueSize",
					"Maximum number of packets we can store when resolving route",
					UintegerValue(255),
					MakeUintegerAccessor(
						&PmtmgmpRouteTable::m_maxQueueSize),
					MakeUintegerChecker<uint16_t>(1)
					)
				;
			return tid;
		}
		void PmtmgmpRouteTable::InitRouteTable(Mac48Address address, PmtmgmpProtocol::NodeType type, Callback<void, PmtmgmpProtocol::NodeType> cb)
		{
			m_address = address;
			m_NodeType = type;
			m_NodeTypeCallBack = cb;
		}
		std::vector<Ptr<PmtmgmpRouteTree> > PmtmgmpRouteTable::GetRouteTable()
		{
			return m_table;
		}
		void PmtmgmpRouteTable::SetPUPDsendRouteTreeIndex(uint8_t index)
		{
			m_PUPDsendRouteTreeIndex = index;
		}
		void PmtmgmpRouteTable::SetMTERPgenerationSeqNumber(uint32_t gsn)
		{
			m_MTERPgenerationSeqNumber = gsn;
		}
		void PmtmgmpRouteTable::SetNodeType(PmtmgmpProtocol::NodeType type)
		{
			m_NodeType = type;
		}
		uint8_t PmtmgmpRouteTable::GetPUPDsendRouteTreeIndex() const
		{
			return m_PUPDsendRouteTreeIndex;
		}
		uint32_t PmtmgmpRouteTable::GetMTERPgenerationSeqNumber() const
		{
			return m_MTERPgenerationSeqNumber;
		}
		uint32_t PmtmgmpRouteTable::GetAsMSECPcount() const
		{
			return m_AsMSECPcount;
		}
		Mac48Address PmtmgmpRouteTable::GetMac48Address() const
		{
			return m_address;
		}
		void PmtmgmpRouteTable::IncreaseAsMSECPcount()
		{
			m_AsMSECPcount++;
		}
		void PmtmgmpRouteTable::DecreaseAsMSECPcount()
		{
			m_AsMSECPcount--;
		}
		////获取以当前节点为MTERP节点的路由树
		Ptr<PmtmgmpRouteTree> PmtmgmpRouteTable::GetMTERPtree()
		{
			return m_MTERPtree;
		}
		////清理MTERP节点的路由树的设置
		void PmtmgmpRouteTable::ClearMTERPtree()
		{
			m_MTERPtree = 0;
		}
		////MTERP路径清理
		void PmtmgmpRouteTable::ClearMTERProutePath()
		{
			m_MTERPgenerationSeqNumber++;
			////MTERP路由树未设置
			if (m_MTERPtree == 0) return;
			std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = std::find_if(m_table.begin(), m_table.end(), PmtmgmpRouteTable_Finder(m_address));
			if (iter != m_table.end())
			{
				m_table.erase(iter);
				m_MTERPtree = 0;
			}
		}
		////获取路由路径数量
		uint8_t PmtmgmpRouteTable::GetAllMSECPcount(Mac48Address mterp, PmtmgmpProtocol::NodeType type)
		{
			uint16_t count = 2;
			for (std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = m_table.begin(); iter != m_table.end(); iter++)
			{
				Ptr<PmtmgmpRoutePath> path = (*iter)->GetPathByMACaddress(m_address);
				if (path != 0)
				{
					if ((*iter)->GetMTERPaddress() == mterp)
					{
						count--;
					}
					else
					{
						if (path->GetNodeType() != type)
						{
							count++;
						}
						else
						{
							count += 2;
						}
					}
				}
			}
			return count;
		}
		uint8_t PmtmgmpRouteTable::GetNextMSECPindex()
		{
			m_MSECPindex++;
			return m_MSECPindex;
		}
		void PmtmgmpRouteTable::SelectMSECP()
		{
			m_MTERPgenerationSeqNumber++;
			m_MTERPtree->SelectMSECP();
		}
		////获取未接收到PGER的MTERP路由路径计数
		uint8_t PmtmgmpRouteTable::GetUnreceivedPathCount()
		{
			////当当前节点没有MTERP路径时返回0
			if (m_MTERPtree == 0) return 0;

			std::vector<Ptr<PmtmgmpRoutePath> > paths = m_MTERPtree->GetPartTree();
			uint8_t unreceivedCount = 0;
			////搜索未接收到PGER的辅助节点
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = paths.begin(); iter != paths.end(); iter++)
			{
				if ((*iter)->GetStatus() == PmtmgmpRoutePath::Waited)
				{
					unreceivedCount++;
				}
			}
			return unreceivedCount;
		}
		////获取MTERP对应的Tree
		Ptr<PmtmgmpRouteTree> PmtmgmpRouteTable::GetTreeByMACaddress(Mac48Address mterp)
		{
			if (m_table.size() == 0) return 0;
			std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = std::find_if(m_table.begin(), m_table.end(), PmtmgmpRouteTable_Finder(mterp));
			if (iter == m_table.end())
			{
				return 0;
			}
			else
			{
				return *iter;
			}
		}
		////获取MTERP、MSECP(Index)对应的Path，找不到则返回0
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTable::GetPathByMACaddress(Mac48Address mterp, Mac48Address msecp)
		{
			Ptr<PmtmgmpRouteTree> pTree = GetTreeByMACaddress(mterp);
			if (pTree == 0)
			{
				////没有找到对应的路由树
				return 0;
			}
			return pTree->GetPathByMACaddress(msecp);
		}
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTable::GetPathByMACindex(Mac48Address mterp, uint8_t index)
		{
			Ptr<PmtmgmpRouteTree> pTree = GetTreeByMACaddress(mterp);
			if (pTree == 0)
			{
				////没有找到对应的路由树
				return 0;
			}
			return pTree->GetPathByMACindex(index);
		}
		////添加新路径
		void PmtmgmpRouteTable::AddNewPath(Ptr<PmtmgmpRoutePath> path)
		{
			Ptr<PmtmgmpRouteTree> routeTree = GetTreeByMACaddress(path->GetMTERPaddress());
			if (routeTree == 0)
			{
				routeTree = CreateObject<PmtmgmpRouteTree>();
				routeTree->SetMTERPaddress(path->GetMTERPaddress());
				m_table.push_back(routeTree);
				if (routeTree->GetMTERPaddress() == m_address)
				{
					m_MTERPtree = routeTree;
				}
			}
			if (m_table.size() >= 255) NS_LOG_ERROR("Add too many path to a PmtmgmpRouteTable at " << m_address);

			routeTree->AddNewPath(path);
		}
		void PmtmgmpRouteTable::AddAsMSECPpath(Ptr<PmtmgmpRoutePath> path)
		{
			Ptr<PmtmgmpRouteTree> routeTree = GetTreeByMACaddress(path->GetMTERPaddress());
			if (routeTree == 0)
			{
				routeTree = CreateObject<PmtmgmpRouteTree>();
				routeTree->SetMTERPaddress(path->GetMTERPaddress());
				m_table.push_back(routeTree);
				if (routeTree->GetMTERPaddress() == m_address)
				{
					m_MTERPtree = routeTree;
				}
			}
			if (m_table.size() >= 255) NS_LOG_ERROR("Add too many path to a PmtmgmpRouteTable at " << m_address);

			routeTree->AddNewPath(path);
			m_AsMSECPcount++;
			if (m_AsMSECPcount != 0 && ((m_NodeType & (PmtmgmpProtocol::Mesh_Access_Point | PmtmgmpProtocol::Mesh_Portal)) == 0))
			{
				m_NodeTypeCallBack(PmtmgmpProtocol::Mesh_Secondary_Point);
			}
		}
		////添加MSECP路径(仅当前节点为MTERP)
		void PmtmgmpRouteTable::AddMSECPpath(Mac48Address mterp, Mac48Address msecp, uint8_t count, uint32_t metric, uint32_t gsn, uint8_t maxTTL)
		{
			if (m_MTERPtree == 0)
			{
				m_MTERPtree = CreateObject<PmtmgmpRouteTree>();
				m_MTERPtree->SetMTERPaddress(mterp);
				m_table.push_back(m_MTERPtree);
			}
			Ptr<PmtmgmpRoutePath> path = CreateObject<PmtmgmpRoutePath>(mterp, msecp, gsn, m_NodeType, 0, maxTTL, metric * count);
			path->SetMSECPindex(GetNextMSECPindex());
			path->SetStatus(PmtmgmpRoutePath::Waited);
			path->SetFromNode(m_address);
			path->SetPGENsendTime();
			m_MTERPtree->AddMSECPpath(path);
		}
		////获取确认状态路径的数量
		uint16_t PmtmgmpRouteTable::GetConfirmedPathSize()
		{
			uint16_t size = 0;
			for (std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = m_table.begin(); iter != m_table.end(); iter++)
			{
				size += (*iter)->GetConfirmedPathSize();
			}
			return size;
		}
		////Python调用函数
		uint8_t PmtmgmpRouteTable::GetTableSize() const
		{
			return m_table.size();
		}
		Ptr<PmtmgmpRouteTree> PmtmgmpRouteTable::GetTableItem(uint8_t index) const
		{
			return m_table[index];
		}
		////路由表信息寿命检测
		void PmtmgmpRouteTable::RouteTableInforLifeCheck()
		{
			uint32_t size = m_table.size();
			NS_LOG_DEBUG("Start life checking at node " << m_address);
			if (m_table.size() != 0)
			{
				m_table.erase(std::remove_if(m_table.begin(), m_table.end(), PmtmgmpRouteTable_PathLifeChecker(this)), m_table.end());
				if (m_AsMSECPcount == 0 && ((m_NodeType & (PmtmgmpProtocol::Mesh_Access_Point | PmtmgmpProtocol::Mesh_Portal)) == 0))
				{
					m_NodeTypeCallBack(PmtmgmpProtocol::Mesh_STA);
				}
			}
			if (size != m_table.size())
			{
				NS_LOG_DEBUG("Life checking is end, delete " << size - m_table.size() << " trees at node " << m_address);
			}
			PMTGMGMProuteInforCheckEvent = Simulator::Schedule(m_PMTGMGMProuteInforCheckPeriod, &PmtmgmpRouteTable::RouteTableInforLifeCheck, this);
		}
		////数据传输最优路径获取
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTable::GetBestRoutePathForData(Mac48Address mterp, uint8_t index)
		{
			if (m_table.size() == 0) return 0;
			std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = std::find_if(m_table.begin(), m_table.end(), PmtmgmpRouteTable_Finder(mterp));
			if (iter == m_table.end())
			{
				return 0;
			}
			else
			{
				return (*iter)->GetBestRoutePathForData(index);
			}
		}
		Ptr<PmtmgmpRoutePath> PmtmgmpRouteTable::GetNearestRoutePathForData(Mac48Address mterp)
		{
			if (m_table.size() == 0) return 0;
			std::vector<Ptr<PmtmgmpRouteTree> >::iterator iter = std::find_if(m_table.begin(), m_table.end(), PmtmgmpRouteTable_Finder(mterp));
			if (iter == m_table.end())
			{
				return 0;
			}
			else
			{
				return (*iter)->GetNearestRoutePathForData();
			}
		}
		////添加Packet数据
		bool PmtmgmpRouteTable::AddPacketToQueue(Ptr<Packet> pkt, Mac48Address src, Mac48Address dst, uint16_t protocol, uint32_t inInterface, PmtmgmpProtocol::RouteReplyCallback reply)
		{
			if (m_packets.size() > m_maxQueueSize)
			{
				return false;
			}
			PmtmgmpRouteQueuedPacket queueedPacket;
			queueedPacket.pkt = pkt;
			queueedPacket.src = src;
			queueedPacket.dst = dst;
			queueedPacket.protocol = protocol;
			queueedPacket.inInterface = inInterface;
			queueedPacket.reply = reply;
			m_currentQueueSize++;
			std::map<Mac48Address, std::vector<PmtmgmpRouteQueuedPacket> >::iterator iter = m_packets.find(dst);
			if (iter == m_packets.end())
			{
				std::vector<PmtmgmpRouteQueuedPacket> packetList = std::vector<PmtmgmpRouteQueuedPacket>();
				m_packets[dst] = packetList;
				m_packets[dst].push_back(queueedPacket);
			}
			else
			{
				iter->second.push_back(queueedPacket);
			}
			return true;
		}
		////发送列队的Packet
		void PmtmgmpRouteTable::SendQueuePackets(Mac48Address dst, PmtmgmpProtocol::Statistics * stats, std::map<Mac48Address, uint32_t> *packetSizePerPath)
		{
			std::map<Mac48Address, std::vector<PmtmgmpRouteQueuedPacket> >::iterator iter = m_packets.find(dst);
			if (iter == m_packets.end())
			{
				return;
			}
			Ptr<PmtmgmpRoutePath> next = GetBestRoutePathForData(dst, 0);
			if (next == 0)
			{
				NS_FATAL_ERROR("Can not find Path when get PGEN as MTERP " << dst << " at node " << m_address); 
				return;
			}
			std::vector<PmtmgmpRouteQueuedPacket> packetList = iter->second;
			if (packetSizePerPath->find(next->GetMTERPaddress()) == packetSizePerPath->end())
			{
				(*packetSizePerPath)[next->GetMTERPaddress()] = 0;
			}
			for (std::vector<PmtmgmpRouteQueuedPacket>::iterator select = packetList.begin(); select != packetList.end(); select++)
			{
				PmtmgmpTag tag;
				select->pkt->RemovePacketTag(tag);
				tag.SetAddress(next->GetFromNode());
				if (next->GetMSECPindex() != tag.GetMSECPindex())
				{
					tag.IncreaseChange();
				}
				select->pkt->AddPacketTag(tag);
				stats->txUnicast++;
				stats->txBytes += select->pkt->GetSize();
				(*packetSizePerPath)[next->GetFromNode()] += select->pkt->GetSize();
				select->reply(true, select->pkt, select->src, select->dst, select->protocol, next->GetInterface());
			}
		}
		////设置路径链接状态
		void PmtmgmpRouteTable::SetPathPeerLinkStatus(Mac48Address mterp, Mac48Address from, bool status)
		{
			Ptr<PmtmgmpRouteTree> pTree = GetTreeByMACaddress(mterp); 
			if (pTree != 0)
			{
				return pTree->SetPathPeerLinkStatus(from, status);
			}
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
