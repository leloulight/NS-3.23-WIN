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
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef PMTMGMP_PROTOCOL_H
#define PMTMGMP_PROTOCOL_H

#include "ns3/wmn-l2-routing-protocol.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"
#include <vector>
#include <map>
#ifndef PMTMGMP_UNUSED_MY_CODE
#include "ns3/pmtmgmp-tag.h"
#endif

namespace ns3 {
	class WmnPointDevice;
	class Packet;
	class Mac48Address;
	class UniformRandomVariable;
	namespace my11s {
		class PmtmgmpProtocolMac;
		class PmtmgmpRtable;
		class IePerr;
		class IePreq;
		class IePrep;
#ifndef PMTMGMP_UNUSED_MY_CODE
		class PmtmgmpRouteTree;
		class PmtmgmpRouteTable;
		class IeSecreq;
		class IeSecrep;
		class IeSecack;
		class IePger;
		class IePgen;
		class IePupd;
		class IePupgq;
		class PUPGQdata;
#endif
		/**
		 * \ingroup my11s
		 *
		 * \brief Hybrid wireless wmn protocol -- a routing protocol of IEEE 802.11s draft.
		 */
		class PmtmgmpProtocol : public WmnL2RoutingProtocol
		{
		public:
			static TypeId GetTypeId();
			PmtmgmpProtocol();
			~PmtmgmpProtocol();
			void DoDispose();
			/**
			 * \brief structure of unreachable destination - address and sequence number
			 */
			typedef struct
			{
				Mac48Address destination;
				uint32_t seqnum;
			} FailedDestination;
			/// Route request, inherited from WmnL2RoutingProtocol
			bool RequestRoute(uint32_t  sourceIface, const Mac48Address source, const Mac48Address destination,
				Ptr<const Packet>  packet, uint16_t  protocolType, RouteReplyCallback  routeReply);
			/// Cleanup packet from all tags
			bool RemoveRoutingStuff(uint32_t fromIface, const Mac48Address source,
				const Mac48Address destination, Ptr<Packet>  packet, uint16_t&  protocolType);
			/**
			 * \brief Install PMTMGMP on given wmn point.
			 *
			 * Installing protocol cause installing its interface MAC plugins.
			 *
			 * Also MP aggregates all installed protocols, PMTMGMP protocol can be accessed
			 * via WmnPointDevice::GetObject<my11s::PmtmgmpProtocol>();
			 */
			bool Install(Ptr<WmnPointDevice>);
			void PmtmgmpPeerLinkStatus(Mac48Address wmnPontAddress, Mac48Address peerAddress, uint32_t interface, bool status);
			///\brief This callback is used to obtain active neighbours on a given interface
			///\param cb is a callback, which returns a list of addresses on given interface (uint32_t)
			void SetNeighboursCallback(Callback<std::vector<Mac48Address>, uint32_t> cb);
			///\name Proactive PREQ mechanism:
			///\{
			void SetRoot();
			void UnsetRoot();
			///\}
			///\brief Statistics:
			void Report(std::ostream &) const;
			void ResetStats();
			/**
			 * Assign a fixed random variable stream number to the random variables
			 * used by this model.  Return the number of streams (possibly zero) that
			 * have been assigned.
			 *
			 * \param stream first stream index to use
			 * \return the number of stream indices assigned by this model
			 */
			int64_t AssignStreams(int64_t stream);

#ifndef PMTMGMP_UNUSED_MY_CODE

			struct Statistics
			{
				uint16_t txUnicast;
				uint16_t txBroadcast;
				uint32_t txBytes;
				uint16_t droppedTtl;
				uint16_t totalQueued;
				uint16_t totalDropped;
				uint16_t initiatedPreq;
				uint16_t initiatedPrep;
				uint16_t initiatedPerr;
				uint16_t initiatedSecreq;
				uint16_t initiatedSecrep;
				uint16_t initiatedSecack;
				uint16_t initiatedPger;
				uint16_t initiatedPgen;
				uint16_t initiatedPupd;
				uint16_t initiatedPupgq;

				void Print(std::ostream & os) const;
				Statistics();
			};

			////��ȡMAC
			Mac48Address GetMacAddress();
			////��ȡЭ���·�ɱ�
			Ptr<PmtmgmpRouteTable> GetPmtmgmpRouteTable();
			////PMTMGMPЭ���нڵ������ö��
			enum NodeType
			{
				Mesh_STA = 1,
				Mesh_Access_Point = 2,
				Mesh_Portal = 4,
				Mesh_Secondary_Point = 8,
			};
			////���úͻ�ȡ��ǰ�ڵ�����
			void SetNodeType(NodeType nodeType);
			NodeType GetNodeType();
			////��ȡAALM����ʱ���õ�m��ֵ
			uint8_t GetValueMForAALM();

			////��ȡÿ����������֡��·�����͵�������
			std::map<Mac48Address, uint32_t> GetPacketSizePerPath();
			////Python֧�ֺ���
			uint32_t GetPacketSizePerPathCount();
			Mac48Address GetPacketSizePerPathFirst(uint8_t index);
			uint32_t GetPacketSizePerPathSecond(uint8_t index);
			uint32_t GetPacketSizePerPathSecondByMac(Mac48Address first);
#endif

		private:
			friend class PmtmgmpProtocolMac;

			virtual void DoInitialize();

			PmtmgmpProtocol& operator= (const PmtmgmpProtocol &);
			PmtmgmpProtocol(const PmtmgmpProtocol &);

			/**
			 * \brief Structure of path error: IePerr and list of receivers:
			 * interfaces and MAC address
			 */
			struct PathError
			{
				std::vector<FailedDestination> destinations; ///< destination list: Mac48Address and sequence number
				std::vector<std::pair<uint32_t, Mac48Address> > receivers; ///< list of PathError receivers (in case of unicast PERR)
			};
			/// Packet waiting its routing information
			struct QueuedPacket
			{
				Ptr<Packet> pkt; ///< the packet
				Mac48Address src; ///< src address
				Mac48Address dst; ///< dst address
				uint16_t protocol; ///< protocol number
				uint32_t inInterface; ///< incoming device interface ID. (if packet has come from upper layers, this is Wmn point ID)
				RouteReplyCallback reply; ///< how to reply

				QueuedPacket();
			};
			typedef std::map<uint32_t, Ptr<PmtmgmpProtocolMac> > PmtmgmpProtocolMacMap;
			/// Like RequestRoute, but for unicast packets
			bool ForwardUnicast(uint32_t  sourceIface, const Mac48Address source, const Mac48Address destination,
				Ptr<Packet>  packet, uint16_t  protocolType, RouteReplyCallback  routeReply, uint32_t ttl);

			///\name Interaction with PMTMGMP MAC plugin
			//\{
			void ReceivePreq(IePreq preq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			void ReceivePrep(IePrep prep, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			void ReceivePerr(std::vector<FailedDestination>, Mac48Address from, uint32_t interface, Mac48Address fromMp);
			void SendPrep(
				Mac48Address src,
				Mac48Address dst,
				Mac48Address retransmitter,
				uint32_t initMetric,
				uint32_t originatorDsn,
				uint32_t destinationSN,
				uint32_t lifetime,
				uint32_t interface);
			/**
			 * \brief forms a path error information element when list of destination fails on a given interface
			 * \attention removes all entries from routing table!
			 */
			PathError MakePathError(std::vector<FailedDestination> destinations);
			///\brief Forwards a received path error
			void ForwardPathError(PathError perr);
			///\brief Passes a self-generated PERR to interface-plugin
			void InitiatePathError(PathError perr);
			/// \return list of addresses where a PERR should be sent to
			std::vector<std::pair<uint32_t, Mac48Address> > GetPerrReceivers(std::vector<FailedDestination> failedDest);

			/// \return list of addresses where a PREQ should be sent to
			std::vector<Mac48Address> GetPreqReceivers(uint32_t interface);
			/// \return list of addresses where a broadcast should be
			//retransmitted
			std::vector<Mac48Address> GetBroadcastReceivers(uint32_t interface);
			/**
			 * \brief MAC-plugin asks whether the frame can be dropped. Protocol automatically updates seqno.
			 *
			 * \return true if frame can be dropped
			 * \param seqno is the sequence number of source
			 * \param source is the source address
			 */
			bool DropDataFrame(uint32_t seqno, Mac48Address source);
			//\}
			/// Route discovery time:
			TracedCallback<Time> m_routeDiscoveryTimeCallback;
			///\name Methods related to Queue/Dequeue procedures
			///\{
			bool QueuePacket(QueuedPacket packet);
			QueuedPacket  DequeueFirstPacketByDst(Mac48Address dst);
			QueuedPacket  DequeueFirstPacket();
			void ReactivePathResolved(Mac48Address dst);
			void ProactivePathResolved();
			///\}
			///\name Methods responsible for path discovery retry procedure:
			///\{
			/**
			 * \brief checks when the last path discovery procedure was started for a given destination.
			 *
			 * If the retry counter has not achieved the maximum level - preq should not be sent
			 */
			bool  ShouldSendPreq(Mac48Address dst);

			/**
			 * \brief Generates PREQ retry when retry timeout has expired and route is still unresolved.
			 *
			 * When PREQ retry has achieved the maximum level - retry mechanism should be canceled
			 */
			void  RetryPathDiscovery(Mac48Address dst, uint8_t numOfRetry);
			/// Proactive Preq routines:
			void SendProactivePreq();
			///\}
			///\return address of WmnPointDevice
			Mac48Address GetAddress();
			///\name Methods needed by PmtmgmpMacLugin to access protocol parameters:
			///\{
			bool GetDoFlag();
			bool GetRfFlag();
			Time GetPreqMinInterval();
			Time GetPerrMinInterval();
			uint8_t GetMaxTtl();
			uint32_t GetNextPreqId();
			uint32_t GetNextPmtmgmpSeqno();
			uint32_t GetActivePathLifetime();
			uint8_t GetUnicastPerrThreshold();
			///\}
#ifndef PMTMGMP_UNUSED_MY_CODE
			////�ն˽������㷢�� 
			void MSECPSearch();
			////����SECREQ
			void SendSECREQ();
			////����SECREQ
			void ReceiveSECREQ(IeSecreq secreq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			////����SECREP
			void SendSECREP(Mac48Address receiver, uint32_t index, NodeType type);
			////����SECREP
			void ReceiveSECREP(IeSecrep secrep, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			////��֤Э����������Ƿ�Ϊ�ն˽ڵ�MTERP
			bool IsMTERP();
			////�ӳ�����SECREP��Ϣ��ѡȡ�ɽ��ܵ�MSECP
			void SelectMSECP(uint32_t gsn);
			////����SECACK
			void SendSECACK();
			////����SECACK
			void ReceiveSECACK(IeSecack secack, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			////����PGER
			void SendPGER(uint32_t gsn, Mac48Address address);
			////��ʱ����PGER
			void WaitForRecivePGER();
			////����PGER
			void ReceivePGER(IePger pger, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			////ת��Pgen
			void SendPGEN(IePgen pgen);
			////����PGEN
			void ReceivePGEN(IePgen pgen, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			////AALM����
			void PUPDperiod();
			////����PUPD
			void SendPUPD();
			////����PUPD
			void ReceivePUPD(IePupd pupd, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
			////����PUPGQ
			void SendPUPGQ(Mac48Address receiver, std::vector<PUPGQdata> list);
			////����PUPD
			void ReceivePUPGQ(IePupgq pupgq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric);
#endif
		private:
#ifdef PMTMGMP_UNUSED_MY_CODE
			///\name Statistics:
			///\{
			struct Statistics
			{
				uint16_t txUnicast;
				uint16_t txBroadcast;
				uint32_t txBytes;
				uint16_t droppedTtl;
				uint16_t totalQueued;
				uint16_t totalDropped;
				uint16_t initiatedPreq;
				uint16_t initiatedPrep;
				uint16_t initiatedPerr;

				void Print(std::ostream & os) const;
				Statistics();
			};
#endif
			Statistics m_stats;
			///\}
			PmtmgmpProtocolMacMap m_interfaces;
			Mac48Address m_address;
			uint32_t m_dataSeqno;
			uint32_t m_pmtmgmpSeqno;
			uint32_t m_preqId;
			///\name Sequence number filters
			///\{
			/// Data sequence number database
			std::map<Mac48Address, uint32_t> m_lastDataSeqno;
			/// keeps PMTMGMP seqno (first in pair) and PMTMGMP metric (second in pair) for each address
			std::map<Mac48Address, std::pair<uint32_t, uint32_t> > m_pmtmgmpSeqnoMetricDatabase;
			///\}

			/// Routing table
			Ptr<PmtmgmpRtable> m_rtable;

			///\name Timers:
			//\{
			struct PreqEvent {
				EventId preqTimeout;
				Time whenScheduled;
			};
			std::map<Mac48Address, PreqEvent> m_preqTimeouts;
			EventId m_proactivePreqTimer;
			/// Random start in Proactive PREQ propagation
			Time m_randomStart;
			///\}
			/// Packet Queue
			std::vector<QueuedPacket> m_rqueue;

			/// \name PMTMGMP-protocol parameters
			/// These are all Aattributes
			/// \{
			uint16_t m_maxQueueSize;
			uint8_t m_My11WmnPMTMGMPmaxPREQretries;
			Time m_My11WmnPMTMGMPnetDiameterTraversalTime;
			Time m_My11WmnPMTMGMPpreqMinInterval;
			Time m_My11WmnPMTMGMPperrMinInterval;
			Time m_My11WmnPMTMGMPactiveRootTimeout;
			Time m_My11WmnPMTMGMPactivePathTimeout;
			Time m_My11WmnPMTMGMPpathToRootInterval;
			Time m_My11WmnPMTMGMPrannInterval;
			bool m_isRoot;
			uint8_t m_maxTtl;
			uint8_t m_unicastPerrThreshold;
			uint8_t m_unicastPreqThreshold;
			uint8_t m_unicastDataThreshold;
			bool m_doFlag;
			bool m_rfFlag;
			///\}

			/// Random variable for random start time
			Ptr<UniformRandomVariable> m_coefficient;
			Callback <std::vector<Mac48Address>, uint32_t> m_neighboursCallback;
#ifndef PMTMGMP_UNUSED_MY_CODE
			EventId m_MSECPsearchStartEventTimer;
			EventId m_MSECPsearchPeriodEventTimer;
			EventId m_MSECPsetEventTimer;
			EventId m_PGERwaitEventTimer;
			EventId m_PUPDperiodEventTimer;
			////�ڵ���������
			NodeType m_NodeType;
			////MSECP������ʼ�ӳ�
			Time m_My11WmnPMTMGMPsecStartDelayTime;
			////MSECP�������
			Time m_My11WmnPMTMGMPsecInterval;
			////SECREQ�����ӳ�
			Time m_My11WmnPMTMGMPsecSetTime;
			////PGER�ȴ�ʱ��
			Time m_My11WmnPMTMGMPpgerWaitTime;
			////·�ɶ�����������
			Time m_My11WmnPMTMGMPpathMetricUpdatePeriod;
			////MSECPѡ����ţ����㿪ʼ������������ż���رա���
			uint32_t m_PathGenerationSeqNumber;
			////MTERP���۴���ʱ�����Ӷ�������
			double m_PMTMGMPmterpAALMmagnification;
			////MSECP���۴���ʱ�����Ӷ�������
			double m_PMTMGMPmsecpAALMmagnification;
			////Pmtmgmp·�ɱ�
			Ptr<PmtmgmpRouteTable> m_RouteTable;
			////ÿ��PUPD���Է��͵�·��·���������
			uint8_t m_MaxRoutePathPerPUPD;
			////ÿ��PUPGQ���Է��͵�·��·���������
			uint8_t m_MaxRoutePathPerPUPGQ;
			////���·��ת������
			uint8_t m_MaxPathChange;
#ifdef PMTMGMP_TAG_INFOR_ATTACH
			uint32_t m_SendIndex;
#endif
			////ÿ����������֡��·�����͵�������
			std::map<Mac48Address, uint32_t> m_PacketSizePerPath;
#endif
		};
	} // namespace my11s
} // namespace ns3
#endif