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

#ifndef PMTMGMP_RTABLE_H
#define PMTMGMP_RTABLE_H

#include <map>
#include "ns3/nstime.h"
#include "ns3/mac48-address.h"
#include "ns3/pmtmgmp-protocol.h"
#include "ns3/traced-value.h"

#ifndef PMTMGMP_UNUSED_MY_CODE
 ////find_if
#include <algorithm>
#include "ns3/random-variable-stream.h"
#include "ns3/pmtmgmp-tag.h"
#endif

namespace ns3 {
	namespace my11s {
#ifndef PMTMGMP_UNUSED_MY_CODE
		/*************************
		* PmtmgmpRoutePath
		************************/
		////路由表路径
		class PmtmgmpRoutePath : public Object
		{
		public:
			PmtmgmpRoutePath();
			PmtmgmpRoutePath(Mac48Address mterp, Mac48Address msecp, uint32_t seq_number, PmtmgmpProtocol::NodeType nodeType, uint8_t hopcount, uint8_t ttl, uint32_t metric);
			~PmtmgmpRoutePath();
			static TypeId GetTypeId();

			////路由路径状态 (其值用于多路径选择比较时Metric的倍率)
			enum RouteInformationStatus
			{
				////确认
				Confirmed = 1,
				////等待
				Waited = 2,
				////过期
				Expired = 3,
			};

			////候选信息
			void AddCandidateRouteInformaiton(Ptr<PmtmgmpRoutePath> path);
			void ClearCandidateRouteInformaiton();
			std::vector<Ptr<PmtmgmpRoutePath> > GetCandidateRouteInformaiton() const;
			uint8_t GetMaxCandidateNum() const;

			void SetMTERPaddress(Mac48Address address);
			void SetMSECPaddress(Mac48Address address);
			void SetMSECPindex(uint8_t index);
			void SetPathGenerationSequenceNumber(uint32_t seq_number);
			void SetNodeType(PmtmgmpProtocol::NodeType nodeType);
			void SetHopcount(uint8_t hopcount);
			void SetTTL(uint8_t ttl);
			void SetMetric(uint32_t metric);
			void SetAllMetric(uint32_t metric, uint32_t base);
			void SetInterface(uint32_t interf);
			void SetFromNode(Mac48Address address);
			void SetStatus(RouteInformationStatus status);
			void SetAcceptCandidateRouteInformaitonEvent(EventId id);
			void SetPGENsendTime();

			Mac48Address GetMTERPaddress() const;
			Mac48Address GetMSECPaddress() const;
			uint8_t GetMSECPindex() const;
			uint32_t GetPathGenerationSequenceNumber() const;
			PmtmgmpProtocol::NodeType GetNodeType() const;
			uint8_t GetHopCount() const;
			uint8_t  GetTTL() const;
			uint32_t GetMetric() const;
			uint32_t GetBaseMetric() const;
			uint32_t GetInterface() const;
			Mac48Address GetFromNode() const;
			RouteInformationStatus GetStatus() const;
			EventId GetAcceptCandidateRouteInformaitonEvent() const;
			Time GetPGENsendTime() const;
			Time GetPathRecreateDelay() const;

			void DecrementTtl();
			void IncreasePathGenerationSequenceNumber();

			////Python调用函数
			uint8_t GetCandidateRouteInformaitonSize() const;
			Ptr<PmtmgmpRoutePath> GetCandidateRouteInformaitonItem(uint8_t index) const;

			////路径信息更新自检
			bool RoutePathInforLifeCheck(Ptr<PmtmgmpRouteTable> table);
			////路径信息更新
			void RoutePathInforLifeUpdate();

			////复制类(仅复制PGEN相关属性)
			Ptr<PmtmgmpRoutePath> GetCopy();

			////度量值更新
			void IncrementMetric(uint32_t metric, double k);

			////用于RoutePath按度量排序(优先新路径（GSN大），同GSN优先小Metric，同Metric优先MSECP较小的)
			bool MSECPselectRoutePathMetricCompare(Ptr<PmtmgmpRoutePath> path);
			bool DataSendRoutePathMetricCompare(Ptr<PmtmgmpRoutePath> path);

		private:
			Mac48Address m_MTERPaddress;
			Mac48Address m_MSECPaddress;
			uint8_t m_MSECPindex;
			uint32_t m_PathGenerationSeqNumber;
			PmtmgmpProtocol::NodeType m_NodeType;
			uint8_t  m_hopCount;
			uint8_t  m_ttl;
			uint32_t m_metric;
			uint32_t m_BaseMetric;
			uint32_t m_interface;

			////候选路由信息列表
			std::vector<Ptr<PmtmgmpRoutePath> >  m_CandidateRouteInformaiton;
			RouteInformationStatus m_InformationStatus;
			uint8_t m_MaxCandidateNum;

			////来源节点MAC
			Mac48Address m_FormNode;

			////延迟等待接受路径信息事件
			EventId m_AcceptCandidateRouteInformaitonEvent;

			////路由表路径寿命
			Time m_PMTGMGMProutePathInforLife;

			////路由表路径刷新时刻
			Time m_PMTGMGMProutePathInforUpdateTime;

			////路由路径补充生成（重发PGEN）时刻
			Time m_PMTGMGMPpgenSendTime;

			////路由路径补充生成延迟
			Time m_PMTMGMPpathRecreateDelay;
		};

		/*************************
		* PmtmgmpRouteTree
		************************/
		////路由表树
		class PmtmgmpRouteTree : public Object
		{
		public:
			PmtmgmpRouteTree();
			~PmtmgmpRouteTree();
			static TypeId GetTypeId();

			std::vector<Ptr<PmtmgmpRoutePath> > GetPartTree();

			void SetMTERPaddress(Mac48Address address);

			Mac48Address GetMTERPaddress() const;
			Time GetAcceptInformaitonDelay() const;

			////获取MTERP、MSECP(Index)对应的Path，找不到则返回0
			Ptr<PmtmgmpRoutePath> GetPathByMACaddress(Mac48Address msecp);
			Ptr<PmtmgmpRoutePath> GetPathByMACindex(uint8_t index);

			////添加新路径
			void AddNewPath(Ptr<PmtmgmpRoutePath> path);
			////选择MSECP(仅MTERP路由树)
			void SelectMSECP();
			////添加MSECP路径(仅当前节点为MTERP)
			void AddMSECPpath(Ptr<PmtmgmpRoutePath> path);
			////获取当前路径的最大生成顺序号
			uint32_t GetTreeMaxGenerationSeqNumber();
			////全部路径置为过期
			void SetAllStatusExpired();
			////获取重复度
			uint8_t GetRepeatability(Mac48Address from);
			////增加重复度计量
			void RepeatabilityIncrease(Mac48Address from);
			////重置全部重复度
			void RepeatabilityReset();
			////接受某个候选信息
			void AcceptCandidateRouteInformaiton(Mac48Address address);
			////获取确认状态路径的数量
			uint8_t GetConfirmedPathSize();

			////Python调用函数
			uint8_t GetTreeSize() const;
			Ptr<PmtmgmpRoutePath> GetTreeItem(uint8_t index) const;
			uint8_t GetRepeatabilitySize() const;
			uint8_t GetRepeatabilityItem(uint8_t index) const;

			////路由树信息寿命检测
			bool RouteTreeInforLifeCheck(Ptr<PmtmgmpRouteTable> table);

			////数据传输最优路径获取
			Ptr<PmtmgmpRoutePath> GetBestRoutePathForData(uint8_t index);
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator GetBestRoutePathForData();
			std::vector<Ptr<PmtmgmpRoutePath> >::iterator GetBestMSECPpath();


		private:
			////路由表树搜索器
			struct PmtmgmpRouteTree_AddressFinder
			{
				PmtmgmpRouteTree_AddressFinder(Mac48Address msecp) :m_msecp(msecp) {};
				bool operator()(Ptr<PmtmgmpRoutePath> p)
				{
					return m_msecp == p->GetMSECPaddress();
				}
				Mac48Address m_msecp;
			};
			struct PmtmgmpRouteTree_IndexFinder
			{
				PmtmgmpRouteTree_IndexFinder(uint8_t index) :m_MSECPindex(index) {};
				bool operator()(Ptr<PmtmgmpRoutePath> p)
				{
					return m_MSECPindex == p->GetMSECPindex();
				}
				uint8_t m_MSECPindex;
			};
			////路由表树路径寿命检测器
			struct PmtmgmpRouteTree_PathLifeChecker
			{
				PmtmgmpRouteTree_PathLifeChecker(Ptr<PmtmgmpRouteTable> table) :m_table(table) {};
				bool operator()(Ptr<PmtmgmpRoutePath> p)
				{
					return p->RoutePathInforLifeCheck(m_table);
				}
				Ptr<PmtmgmpRouteTable> m_table;
			};
		private:
			std::vector<Ptr<PmtmgmpRoutePath> >  m_tree;
			Mac48Address m_MTERPaddress;

			////终端节点可拥有的辅助节点数量
			uint8_t m_MSECPnumForMTERP;

			////路径重复度计量
			std::map<Mac48Address, uint8_t> m_repeatability;

			////延迟时间
			Time m_AcceptInformaitonDelay;

			////非当前使用路径最优路径比较倍率
			uint8_t m_NotSelectBestRoutePathRate;
		};
		/*************************
		* PmtmgmpRouteTable
		************************/
		////路由表
		class PmtmgmpRouteTable : public Object
		{
		public:
			PmtmgmpRouteTable();
			~PmtmgmpRouteTable();

			static TypeId GetTypeId();

			////初始化RouteTable
			void InitRouteTable(Mac48Address address, PmtmgmpProtocol::NodeType type, Callback<void, PmtmgmpProtocol::NodeType> cb);

			std::vector<Ptr<PmtmgmpRouteTree> > GetRouteTable();

			void SetPUPDsendRouteTreeIndex(uint8_t index);
			void SetMTERPgenerationSeqNumber(uint32_t gsn);
			void SetNodeType(PmtmgmpProtocol::NodeType nodeType);

			uint8_t GetPUPDsendRouteTreeIndex() const;
			uint32_t GetMTERPgenerationSeqNumber() const;
			uint32_t GetAsMSECPcount() const;
			Mac48Address GetMac48Address() const;

			////MSECP处理函数
			void IncreaseAsMSECPcount();
			void DecreaseAsMSECPcount();

			////获取以当前节点为MTERP节点的路由树
			Ptr<PmtmgmpRouteTree> GetMTERPtree();
			////清理MTERP节点的路由树的设置
			void ClearMTERPtree();
			////MTERP路径清理
			void ClearMTERProutePath();
			////获取路由路径数量(存在当前MTERP-1，不同NodeType+1，相同NodeType+2)
			uint8_t GetAllMSECPcount(Mac48Address mterp, PmtmgmpProtocol::NodeType type);
			////获取下一个MSECPindex
			uint8_t GetNextMSECPindex();
			////选择MSECP
			void SelectMSECP();
			////获取未接收到PGER的MTERP路由路径计数
			uint8_t GetUnreceivedPathCount();

			////获取MTERP对应的Tree，找不到则返回0
			Ptr<PmtmgmpRouteTree> GetTreeByMACaddress(Mac48Address mterp);
			////获取MTERP、MSECP(Index)对应的Path，找不到则返回0
			Ptr<PmtmgmpRoutePath> GetPathByMACaddress(Mac48Address mterp, Mac48Address msecp);
			Ptr<PmtmgmpRoutePath> GetPathByMACindex(Mac48Address mterp, uint8_t index);

			////添加新路径
			void AddNewPath(Ptr<PmtmgmpRoutePath> path);
			////添加MSECP新路径(仅当前节点为MSECP)
			void AddAsMSECPpath(Ptr<PmtmgmpRoutePath> path);
			////添加MSECP路径(仅当前节点为MTERP)
			void AddMSECPpath(Mac48Address mterp, Mac48Address msecp, uint8_t count, uint32_t metric, uint32_t gsn, uint8_t maxTTL);

			////获取确认状态路径的数量
			uint16_t GetConfirmedPathSize();

			////Python调用函数
			uint8_t GetTableSize() const;
			Ptr<PmtmgmpRouteTree> GetTableItem(uint8_t index) const;

			////路由表信息寿命检测
			void RouteTableInforLifeCheck();

			////数据传输最优路径获取
			Ptr<PmtmgmpRoutePath> GetBestRoutePathForData(Mac48Address mterp, uint8_t index);

			////添加Packet数据
			bool AddPacketToQueue(Ptr<Packet> pkt, Mac48Address src, Mac48Address dst, uint16_t protocol, uint32_t inInterface, PmtmgmpProtocol::RouteReplyCallback reply);

			////发送列队的Packet
			void SendQueuePackets(Mac48Address dst, PmtmgmpProtocol::Statistics *stats);

		private:
			////路由表路径搜索器
			struct PmtmgmpRouteTable_Finder
			{
				PmtmgmpRouteTable_Finder(Mac48Address address) :m_address(address) {};
				bool operator()(Ptr<PmtmgmpRouteTree> p)
				{
					return m_address == p->GetMTERPaddress();
				}
				Mac48Address m_address;
			};
			////路由表寿命检测器
			struct PmtmgmpRouteTable_PathLifeChecker
			{
				PmtmgmpRouteTable_PathLifeChecker(Ptr<PmtmgmpRouteTable> table) :m_table(table) {};
				bool operator()(Ptr<PmtmgmpRouteTree> p)
				{
					bool checker = p->RouteTreeInforLifeCheck(m_table);
					if (checker && m_table->GetMac48Address() == p->GetMTERPaddress())
					{
						m_table->ClearMTERPtree();
					}
					return checker;
				}
				Ptr<PmtmgmpRouteTable> m_table;
			};

			////Packet数据
			struct PmtmgmpRouteQueuedPacket
			{
				Ptr<Packet> pkt; ///< the packet
				Mac48Address src; ///< src address
				Mac48Address dst; ///< dst address
				uint16_t protocol; ///< protocol number
				uint32_t inInterface; ///< incoming device interface ID. (if packet has come from upper layers, this is Wmn point ID)
				PmtmgmpProtocol::RouteReplyCallback reply; ///< how to reply
			};
		private:
			std::vector<Ptr<PmtmgmpRouteTree> >  m_table;

			////PMTMGMP的数据
			Mac48Address m_address;
			PmtmgmpProtocol::NodeType m_NodeType;

			////路由表检测间隔
			Time m_PMTGMGMProuteInforCheckPeriod;

			////路由表检测事件
			EventId PMTGMGMProuteInforCheckEvent;

			////PUPD发送的RouteTree的序号
			uint8_t m_PUPDsendRouteTreeIndex;

			////以当前节点为MTERP节点的路由树
			Ptr<PmtmgmpRouteTree> m_MTERPtree;
			////作为MTERP的生成顺序号GSN
			uint32_t m_MTERPgenerationSeqNumber;
			////设置NodeType回调
			Callback<void, PmtmgmpProtocol::NodeType> m_NodeTypeCallBack;
			////此节点作为MSECP的计数
			uint8_t m_AsMSECPcount;
			////MSECPindex分配标记(从1开始)
			uint8_t m_MSECPindex;

			////待发送包
			uint16_t m_maxQueueSize;
			uint16_t m_currentQueueSize;
			std::map<Mac48Address, std::vector<PmtmgmpRouteQueuedPacket> > m_packets;
		};
#endif
		/*************************
		* PmtmgmpRtable
		************************/
		/**
		 * \ingroup my11s
		 *
		 * \brief Routing table for PMTMGMP -- 802.11s routing protocol
		 */
		class PmtmgmpRtable : public Object
		{
		public:
			/// Means all interfaces
			const static uint32_t INTERFACE_ANY = 0xffffffff;
			/// Maximum (the best?) path metric
			const static uint32_t MAX_METRIC = 0xffffffff;

			/// Route lookup result, return type of LookupXXX methods
			struct LookupResult
			{
				Mac48Address retransmitter;
				uint32_t ifIndex;
				uint32_t metric;
				uint32_t seqnum;
				Time lifetime;
				LookupResult(Mac48Address r = Mac48Address::GetBroadcast(),
					uint32_t i = INTERFACE_ANY,
					uint32_t m = MAX_METRIC,
					uint32_t s = 0,
					Time l = Seconds(0.0));
				/// True for valid route
				bool IsValid() const;
				/// Compare route lookup results, used by tests
				bool operator== (const LookupResult & o) const;
			};
			/// Path precursor = {MAC, interface ID}
			typedef std::vector<std::pair<uint32_t, Mac48Address> > PrecursorList;

		public:
			static TypeId GetTypeId();
			PmtmgmpRtable();
			~PmtmgmpRtable();
			void DoDispose();

			///\name Add/delete paths
			//\{
			void AddReactivePath(
				Mac48Address destination,
				Mac48Address retransmitter,
				uint32_t interface,
				uint32_t metric,
				Time  lifetime,
				uint32_t seqnum
				);
			void AddProactivePath(
				uint32_t metric,
				Mac48Address root,
				Mac48Address retransmitter,
				uint32_t interface,
				Time  lifetime,
				uint32_t seqnum
				);
			void AddPrecursor(Mac48Address destination, uint32_t precursorInterface, Mac48Address precursorAddress, Time lifetime);
			PrecursorList GetPrecursors(Mac48Address destination);
			void DeleteProactivePath();
			void DeleteProactivePath(Mac48Address root);
			void DeleteReactivePath(Mac48Address destination);
			//\}

			///\name Lookup
			//\{
			/// Lookup path to destination
			LookupResult LookupReactive(Mac48Address destination);
			/// Return all reactive paths, including expired
			LookupResult LookupReactiveExpired(Mac48Address destination);
			/// Find proactive path to tree root. Note that calling this method has side effect of deleting expired proactive path
			LookupResult LookupProactive();
			/// Return all proactive paths, including expired
			LookupResult LookupProactiveExpired();
			//\}

			/// When peer link with a given MAC-address fails - it returns list of unreachable destination addresses
			std::vector<PmtmgmpProtocol::FailedDestination> GetUnreachableDestinations(Mac48Address peerAddress);

		private:
			/// Route found in reactive mode
			struct Precursor
			{
				Mac48Address address;
				uint32_t interface;
				Time whenExpire;
			};
			struct ReactiveRoute
			{
				Mac48Address retransmitter;
				uint32_t interface;
				uint32_t metric;
				Time whenExpire;
				uint32_t seqnum;
				std::vector<Precursor> precursors;
			};
			/// Route fond in proactive mode
			struct ProactiveRoute
			{
				Mac48Address root;
				Mac48Address retransmitter;
				uint32_t interface;
				uint32_t metric;
				Time whenExpire;
				uint32_t seqnum;
				std::vector<Precursor> precursors;
			};

			/// List of routes
			std::map<Mac48Address, ReactiveRoute>  m_routes;
			/// Path to proactive tree root MP
			ProactiveRoute  m_root;
		};
	} // namespace my11s
} // namespace ns3
#endif
