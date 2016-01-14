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

#include "pmtmgmp-protocol.h"
#include "pmtmgmp-protocol-mac.h"
#include "pmtmgmp-tag.h"
#include "pmtmgmp-rtable.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/wmn-point-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wmn-point-device.h"
#include "ns3/wmn-wifi-interface-mac.h"
#include "ns3/random-variable-stream.h"
#include "pmtmgmp-airtime-metric.h"
#include "ie-my11s-preq.h"
#include "ie-my11s-prep.h"
#include "ns3/trace-source-accessor.h"
#include "ie-my11s-perr.h"

#ifndef PMTMGMP_UNUSED_MY_CODE
///Comment Below Code To Use Path Check
//#define PMTMGMP_UNUSED_REPEAT_PATH
////find_if
#include <algorithm>

#include "ns3/ie-my11s-secreq.h"
#include "ns3/ie-my11s-secrep.h"
#include "ns3/ie-my11s-secack.h"
#include "ns3/ie-my11s-pger.h"
#include "ns3/ie-my11s-pgen.h"
#include "ns3/ie-my11s-pupd.h"
#include "ns3/ie-my11s-pupgq.h"

#endif

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("PmtmgmpProtocol");

	namespace my11s {

		NS_OBJECT_ENSURE_REGISTERED(PmtmgmpProtocol);

		TypeId
			PmtmgmpProtocol::GetTypeId()
		{
			static TypeId tid = TypeId("ns3::my11s::PmtmgmpProtocol")
				.SetParent<WmnL2RoutingProtocol>()
				.SetGroupName("Wmn")
				.AddConstructor<PmtmgmpProtocol>()
				.AddAttribute("RandomStart",
					"Random delay at first proactive PREQ",
					TimeValue(Seconds(0.1)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_randomStart),
					MakeTimeChecker()
					)
				.AddAttribute("MaxQueueSize",
					"Maximum number of packets we can store when resolving route",
					UintegerValue(255),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_maxQueueSize),
					MakeUintegerChecker<uint16_t>(1)
					)
				.AddAttribute("My11WmnPMTMGMPmaxPREQretries",
					"Maximum number of retries before we suppose the destination to be unreachable",
					UintegerValue(3),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPmaxPREQretries),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("My11WmnPMTMGMPnetDiameterTraversalTime",
					"Time we suppose the packet to go from one edge of the network to another",
					TimeValue(MicroSeconds(1024 * 100)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPnetDiameterTraversalTime),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPpreqMinInterval",
					"Minimal interval between to successive PREQs",
					TimeValue(MicroSeconds(1024 * 100)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPpreqMinInterval),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPperrMinInterval",
					"Minimal interval between to successive PREQs",
					TimeValue(MicroSeconds(1024 * 100)),
					MakeTimeAccessor(&PmtmgmpProtocol::m_My11WmnPMTMGMPperrMinInterval),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPactiveRootTimeout",
					"Lifetime of poractive routing information",
					TimeValue(MicroSeconds(1024 * 5000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPactiveRootTimeout),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPactivePathTimeout",
					"Lifetime of reactive routing information",
					TimeValue(MicroSeconds(1024 * 5000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPactivePathTimeout),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPpathToRootInterval",
					"Interval between two successive proactive PREQs",
					TimeValue(MicroSeconds(1024 * 2000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPpathToRootInterval),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPrannInterval",
					"Lifetime of poractive routing information",
					TimeValue(MicroSeconds(1024 * 5000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPrannInterval),
					MakeTimeChecker()
					)
				.AddAttribute("MaxTtl",
					"Initial value of Time To Live field",
					UintegerValue(32),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_maxTtl),
					MakeUintegerChecker<uint8_t>(2)
					)
				.AddAttribute("UnicastPerrThreshold",
					"Maximum number of PERR receivers, when we send a PERR as a chain of unicasts",
					UintegerValue(32),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_unicastPerrThreshold),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("UnicastPreqThreshold",
					"Maximum number of PREQ receivers, when we send a PREQ as a chain of unicasts",
					UintegerValue(1),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_unicastPreqThreshold),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("UnicastDataThreshold",
					"Maximum number ofbroadcast receivers, when we send a broadcast as a chain of unicasts",
					UintegerValue(1),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_unicastDataThreshold),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("DoFlag",
					"Destination only PMTMGMP flag",
					BooleanValue(false),
					MakeBooleanAccessor(
						&PmtmgmpProtocol::m_doFlag),
					MakeBooleanChecker()
					)
				.AddAttribute("RfFlag",
					"Reply and forward flag",
					BooleanValue(true),
					MakeBooleanAccessor(
						&PmtmgmpProtocol::m_rfFlag),
					MakeBooleanChecker()
					)
				.AddTraceSource("RouteDiscoveryTime",
					"The time of route discovery procedure",
					MakeTraceSourceAccessor(
						&PmtmgmpProtocol::m_routeDiscoveryTimeCallback),
					"ns3::Time::TracedCallback"
					)
#ifndef PMTMGMP_UNUSED_MY_CODE
				.AddAttribute("My11WmnPMTMGMPsecStartDelayTime",
					"Delay time of start MSECP search",
					TimeValue(MicroSeconds(1024 * 958)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPsecStartDelayTime),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPsecInterval",
					"Interval MSECP search, it also the interval of path Generation",
					TimeValue(MicroSeconds(1024 * 100000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPsecInterval),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPsecSetTime",
					"Delay time of waiting for accept SECREP",
					TimeValue(MicroSeconds(1024 * 2000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPsecSetTime),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTGMGMPpgerWaitTime",
					"Delay time of waiting for receive PGER",
					TimeValue(MicroSeconds(1024 * 2000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPpgerWaitTime),
					MakeTimeChecker()
					)
				.AddAttribute("My11WmnPMTMGMPpathMetricUpdatePeriod",
					"Period for Update the Metric of Route Path, also the Period for PUDP",
					TimeValue(MicroSeconds(1024 * 6000)),
					MakeTimeAccessor(
						&PmtmgmpProtocol::m_My11WmnPMTMGMPpathMetricUpdatePeriod),
					MakeTimeChecker()
					)
				.AddAttribute("PMTMGMPmterpAALMmagnification",
					"The magnification of the value as M in MTERP node when Compute AALM.",
					DoubleValue(4),
					MakeDoubleAccessor(
						&PmtmgmpProtocol::m_PMTMGMPmterpAALMmagnification),
					MakeDoubleChecker<double>(1)
					)
				.AddAttribute("PMTMGMPmsecpAALMmagnification",
					"The magnification of the value as M in MSECP node when Compute AALM.",
					DoubleValue(3),
					MakeDoubleAccessor(
						&PmtmgmpProtocol::m_PMTMGMPmsecpAALMmagnification),
					MakeDoubleChecker<double>(1)
					)
				.AddAttribute("MaxRoutePathPerPUPD",
					"Max Route Path in each PUPD, same times it may be more then this value.",
					UintegerValue(15),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_MaxRoutePathPerPUPD),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("MaxRoutePathPerPUPGQ",
					"Max Route Path in each PUDGQ, same times it may be more then this value.",
					UintegerValue(6),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_MaxRoutePathPerPUPGQ),
					MakeUintegerChecker<uint8_t>(1)
					)
				.AddAttribute("MaxPathChange",
					"Max Route Path count can change when a packet in its way.",
					UintegerValue(16),
					MakeUintegerAccessor(
						&PmtmgmpProtocol::m_MaxPathChange),
					MakeUintegerChecker<uint8_t>(1)
					)
#endif
				;
			return tid;
		}

		PmtmgmpProtocol::PmtmgmpProtocol() :
			m_dataSeqno(1),
			m_pmtmgmpSeqno(1),
			m_preqId(0),
			m_rtable(CreateObject<PmtmgmpRtable>()),
			m_randomStart(Seconds(0.1)),
			m_maxQueueSize(255),
			m_My11WmnPMTMGMPmaxPREQretries(3),
			m_My11WmnPMTMGMPnetDiameterTraversalTime(MicroSeconds(1024 * 100)),
			m_My11WmnPMTMGMPpreqMinInterval(MicroSeconds(1024 * 100)),
			m_My11WmnPMTMGMPperrMinInterval(MicroSeconds(1024 * 100)),
			m_My11WmnPMTMGMPactiveRootTimeout(MicroSeconds(1024 * 5000)),
			m_My11WmnPMTMGMPactivePathTimeout(MicroSeconds(1024 * 5000)),
			m_My11WmnPMTMGMPpathToRootInterval(MicroSeconds(1024 * 2000)),
			m_My11WmnPMTMGMPrannInterval(MicroSeconds(1024 * 5000)),
			m_isRoot(false),
			m_maxTtl(32),
			m_unicastPerrThreshold(32),
			m_unicastPreqThreshold(1),
			m_unicastDataThreshold(1),
			m_doFlag(false),
			m_rfFlag(false),
#ifndef PMTMGMP_UNUSED_MY_CODE
			m_NodeType(Mesh_STA),
			m_My11WmnPMTMGMPsecStartDelayTime(MicroSeconds(1024 * 958)),
			m_My11WmnPMTMGMPsecInterval(MicroSeconds(1024 * 100000)),
			m_My11WmnPMTMGMPsecSetTime(MicroSeconds(1024 * 2000)),
			m_My11WmnPMTMGMPpgerWaitTime(MicroSeconds(1024 * 2000)),
			m_My11WmnPMTMGMPpathMetricUpdatePeriod(MicroSeconds(1024 * 6000)),
			m_PathGenerationSeqNumber(0),
			m_PMTMGMPmterpAALMmagnification(4),
			m_PMTMGMPmsecpAALMmagnification(3),
			m_RouteTable(CreateObject<PmtmgmpRouteTable>()),
			m_MaxRoutePathPerPUPD(15),
			m_MaxRoutePathPerPUPGQ(6),
#ifdef PMTMGMP_TAG_INFOR_ATTACH
			m_MaxPathChange(10),
			m_SendIndex(0)
#else
			m_MaxPathChange(16)
#endif
#endif
		{
			NS_LOG_FUNCTION_NOARGS();
			m_coefficient = CreateObject<UniformRandomVariable>();
		}

		PmtmgmpProtocol::~PmtmgmpProtocol()
		{
			NS_LOG_FUNCTION_NOARGS();
		}

		void
			PmtmgmpProtocol::DoInitialize()
		{
			m_coefficient->SetAttribute("Max", DoubleValue(m_randomStart.GetSeconds()));
			Time randomStart = m_My11WmnPMTMGMPsecStartDelayTime + Seconds(m_coefficient->GetValue());
#ifdef PMTMGMP_UNUSED_MY_CODE
			if (m_isRoot)
			{
				m_proactivePreqTimer = Simulator::Schedule(randomStart, &PmtmgmpProtocol::SendProactivePreq, this);
			}
#endif
#ifndef PMTMGMP_UNUSED_MY_CODE
			if (IsMTERP())
			{
				m_MSECPsearchStartEventTimer = Simulator::Schedule(randomStart, &PmtmgmpProtocol::MSECPSearch, this);
			}
			////PUPD���ڣ���ʼ�ӳ���MSECP������ͬ
			m_PUPDperiodEventTimer = Simulator::Schedule(randomStart + m_My11WmnPMTMGMPpathMetricUpdatePeriod, &PmtmgmpProtocol::PUPDperiod, this);
#endif
		}

		void
			PmtmgmpProtocol::DoDispose()
		{
			NS_LOG_FUNCTION_NOARGS();
			for (std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.begin(); i != m_preqTimeouts.end(); i++)
			{
				i->second.preqTimeout.Cancel();
			}
			m_proactivePreqTimer.Cancel();
			m_preqTimeouts.clear();
			m_lastDataSeqno.clear();
			m_pmtmgmpSeqnoMetricDatabase.clear();
			m_interfaces.clear();
			m_rqueue.clear();
			m_rtable = 0;
			m_mp = 0;

#ifndef PMTMGMP_UNUSED_MY_CODE
			m_RouteTable = 0;
#endif
		}

#ifdef PMTMGMP_UNUSED_MY_CODE
		bool
			PmtmgmpProtocol::RequestRoute(
				uint32_t sourceIface,
				const Mac48Address source,
				const Mac48Address destination,
				Ptr<const Packet> constPacket,
				uint16_t protocolType, //ethrnet 'Protocol' field
				WmnL2RoutingProtocol::RouteReplyCallback routeReply
				)
		{
			Ptr <Packet> packet = constPacket->Copy();
			PmtmgmpTag tag;
			if (sourceIface == GetWmnPoint()->GetIfIndex())
			{
				// packet from level 3
				if (packet->PeekPacketTag(tag))
				{
					NS_FATAL_ERROR("PMTMGMP tag has come with a packet from upper layer. This must not occur...");
				}
				//Filling TAG:
				if (destination == Mac48Address::GetBroadcast())
				{
					tag.SetSeqno(m_dataSeqno++);
				}
				tag.SetTtl(m_maxTtl);
			}
			else
			{
				if (!packet->RemovePacketTag(tag))
				{
					NS_FATAL_ERROR("PMTMGMP tag is supposed to be here at this point.");
				}
				tag.DecrementTtl();
				if (tag.GetTtl() == 0)
				{
					m_stats.droppedTtl++;
					return false;
				}
			}
			if (destination == Mac48Address::GetBroadcast())
			{
				m_stats.txBroadcast++;
				m_stats.txBytes += packet->GetSize();
				//channel IDs where we have already sent broadcast:
				std::vector<uint16_t> channels;
				for (PmtmgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin(); plugin != m_interfaces.end(); plugin++)
				{
					bool shouldSend = true;
					for (std::vector<uint16_t>::const_iterator chan = channels.begin(); chan != channels.end(); chan++)
					{
						if ((*chan) == plugin->second->GetChannelId())
						{
							shouldSend = false;
						}
					}
					if (!shouldSend)
					{
						continue;
					}
					channels.push_back(plugin->second->GetChannelId());
					std::vector<Mac48Address> receivers = GetBroadcastReceivers(plugin->first);
					for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != receivers.end(); i++)
					{
						Ptr<Packet> packetCopy = packet->Copy();
						//
						// 64-bit Intel valgrind complains about tag.SetAddress (*i).  It
						// likes this just fine.
						//
						Mac48Address address = *i;
						tag.SetAddress(address);
						packetCopy->AddPacketTag(tag);
						routeReply(true, packetCopy, source, destination, protocolType, plugin->first);
					}
				}
			}
			else
			{
				return ForwardUnicast(sourceIface, source, destination, packet, protocolType, routeReply, tag.GetTtl());
			}
			return true;
		}
#else
		bool
			PmtmgmpProtocol::RequestRoute(
				uint32_t sourceIface,
				const Mac48Address source,
				const Mac48Address destination,
				Ptr<const Packet> constPacket,
				uint16_t protocolType, //ethrnet 'Protocol' field
				WmnL2RoutingProtocol::RouteReplyCallback routeReply
				)
		{
			Ptr <Packet> packet = constPacket->Copy();
			PmtmgmpTag tag;
			if (sourceIface == GetWmnPoint()->GetIfIndex())
			{
				// packet from level 3
				if (packet->PeekPacketTag(tag))
				{
					NS_FATAL_ERROR("PMTMGMP tag has come with a packet from upper layer. This must not occur...");
				}
				tag.SetTtl(m_maxTtl);
				//Filling TAG:
				if (destination == Mac48Address::GetBroadcast())
				{
					tag.SetSeqno(m_dataSeqno++);
					NS_LOG_DEBUG("Start ARP Packet at " << m_address << " from " << source << " to " << destination);
				}
#ifdef PMTMGMP_TAG_INFOR_ATTACH
				else
				{
					NS_LOG_DEBUG("Start Packet at " << m_address << " from " << source << " to " << destination << " while tag index is " << tag.GetSendIndex());
					tag.SetSendIndex(m_SendIndex);
					m_SendIndex++;
				}
#endif
			}
			else
			{
				if (!packet->RemovePacketTag(tag))
				{
					NS_FATAL_ERROR("PMTMGMP tag is supposed to be here at this point.");
				}
				tag.DecrementTtl();
				if (tag.GetTtl() == 0)
				{
					NS_LOG_DEBUG("TTL is 0 " << " at " << m_address << " from " << source << " to " << destination);
					m_stats.droppedTtl++;
					return false;
				}
			}
			if (destination == Mac48Address::GetBroadcast())
			{
				NS_LOG_DEBUG("Receive ARP Packet at " << m_address << " from " << source << " to " << destination);
				m_stats.txBroadcast++;
				m_stats.txBytes += packet->GetSize();
				//channel IDs where we have already sent broadcast:
				std::vector<uint16_t> channels;
				for (PmtmgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin(); plugin != m_interfaces.end(); plugin++)
				{
					bool shouldSend = true;
					for (std::vector<uint16_t>::const_iterator chan = channels.begin(); chan != channels.end(); chan++)
					{
						if ((*chan) == plugin->second->GetChannelId())
						{
							shouldSend = false;
						}
					}
					if (!shouldSend)
					{
						continue;
					}
					channels.push_back(plugin->second->GetChannelId());
					std::vector<Mac48Address> receivers = GetBroadcastReceivers(plugin->first);
					for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != receivers.end(); i++)
					{
						Ptr<Packet> packetCopy = packet->Copy();
						//
						// 64-bit Intel valgrind complains about tag.SetAddress (*i).  It
						// likes this just fine.
						//
						Mac48Address address = *i;
						tag.SetAddress(address);
						packetCopy->AddPacketTag(tag);
						routeReply(true, packetCopy, source, destination, protocolType, plugin->first);
					}
				}
			}
			else
			{
#ifdef PMTMGMP_TAG_INFOR_ATTACH
				NS_LOG_DEBUG("Receive Packet at " << m_address << " from " << source << " to " << destination << " while tag is (Address:" << tag.GetAddress() << ", SN:" << tag.GetSeqno() << ", MSECPindex:" << (uint32_t)tag.GetMSECPindex() << ", TTL:" << (uint32_t)tag.GetTtl() << ", Change:" << (uint32_t)tag.GetChangePath() << ", Index:" << (uint32_t)tag.GetSendIndex() << ")");
#else
				NS_LOG_DEBUG("Receive Packet at " << m_address << " from " << source << " to " << destination << " while tag is (Address:" << tag.GetAddress() << ", SN:" << tag.GetSeqno() << ", MSECPindex:" << (uint32_t)tag.GetMSECPindex() << ", TTL:" << (uint32_t)tag.GetTtl() << ", Change:" << (uint32_t)tag.GetChangePath() << ")");
#endif
				//return ForwardUnicast(sourceIface, source, destination, packet, protocolType, routeReply, tag.GetTtl());
				Ptr<PmtmgmpRoutePath> next;
				if (tag.GetChangePath() > m_MaxPathChange)
				{
					NS_LOG_DEBUG("Reach maximum change, and  can not find Next Hop Node by MSECPindex " << (uint32_t)tag.GetMSECPindex() << " at Node " << m_address);
					////�ѵ������·��ת������
					next = m_RouteTable->GetNearestRoutePathForData(destination);
				}
				else
				{
					next = m_RouteTable->GetBestRoutePathForData(destination, tag.GetMSECPindex());
				}
				if (next != 0)
				{
					if (next->GetMSECPindex() != tag.GetMSECPindex())
					{
						tag.IncreaseChange();
					}
					tag.SetAddress(next->GetFromNode());
					tag.SetMSECPindex(next->GetMSECPindex());
					packet->AddPacketTag(tag);

<<<<<<< HEAD
					NS_LOG_DEBUG("Get next hop " << next->GetFromNode() << " at " << m_address << " to " << destination << " from " << source);
=======
					NS_LOG_DEBUG("Get next hop " << next->GetFromNode() << " at " << m_address << " from " << source << " to " << destination << " as MSECPindex: " << (uint32_t)next->GetMSECPindex() << " ,Path Status is " << next->GetPmtmgmpPeerLinkStatus() << ", Path Change is " << (uint32_t) tag.GetChangePath() << ", Hopcount is " << (uint32_t)(m_maxTtl - tag.GetTtl()) << ", Packet is " << packet);
>>>>>>> refs/remotes/Whimsyduke/humgmp

					// reply immediately :
					routeReply(true, packet, source, destination, protocolType, next->GetInterface());
					m_stats.txUnicast++;
					m_stats.txBytes += packet->GetSize();
					if (m_PacketSizePerPath.find(next->GetFromNode()) == m_PacketSizePerPath.end())
					{
						m_PacketSizePerPath[next->GetFromNode()] = packet->GetSize();
					}
					else
					{
						m_PacketSizePerPath[next->GetFromNode()] += packet->GetSize();
					}
					return true;
				}

				if (sourceIface != GetWmnPoint()->GetIfIndex())
				{
					//Start path error procedure:
					NS_LOG_DEBUG("Must Send PERR at " << m_address << " from " << source << " to " << destination);
					m_stats.totalDropped++;
					return false;
				}

				NS_LOG_DEBUG("No Route Path here now. at " << m_address << " from " << source << " to " << destination);
				if (m_RouteTable->AddPacketToQueue(packet, source, destination, protocolType, sourceIface, routeReply))
				{
					m_stats.totalQueued++;
					return true;
				}
				else
				{
					m_stats.totalDropped++;
					return false;
				}
			}
			return true;
		}
#endif
		bool
			PmtmgmpProtocol::RemoveRoutingStuff(uint32_t fromIface, const Mac48Address source,
				const Mac48Address destination, Ptr<Packet>  packet, uint16_t&  protocolType)
		{
			PmtmgmpTag tag;
			if (!packet->RemovePacketTag(tag))
			{
				NS_FATAL_ERROR("PMTMGMP tag must exist when packet received from the network");
			}
			return true;
		}
		bool
			PmtmgmpProtocol::ForwardUnicast(uint32_t  sourceIface, const Mac48Address source, const Mac48Address destination,
				Ptr<Packet>  packet, uint16_t  protocolType, RouteReplyCallback  routeReply, uint32_t ttl)
		{
			NS_ASSERT(destination != Mac48Address::GetBroadcast());
			PmtmgmpRtable::LookupResult result = m_rtable->LookupReactive(destination);
			NS_LOG_DEBUG("Requested src = " << source << ", dst = " << destination << ", I am " << GetAddress() << ", RA = " << result.retransmitter);
			if (result.retransmitter == Mac48Address::GetBroadcast())
			{
				result = m_rtable->LookupProactive();
			}
			PmtmgmpTag tag;
			tag.SetAddress(result.retransmitter);
			tag.SetTtl(ttl);
			//seqno and metric is not used;
			packet->AddPacketTag(tag);
			if (result.retransmitter != Mac48Address::GetBroadcast())
			{
				//reply immediately:
				routeReply(true, packet, source, destination, protocolType, result.ifIndex);
				m_stats.txUnicast++;
				m_stats.txBytes += packet->GetSize();
				return true;
			}
			if (sourceIface != GetWmnPoint()->GetIfIndex())
			{
				//Start path error procedure:
				NS_LOG_DEBUG("Must Send PERR");
				result = m_rtable->LookupReactiveExpired(destination);
				//1.  Lookup expired reactive path. If exists - start path error
				//    procedure towards a next hop of this path
				//2.  If there was no reactive path, we lookup expired proactive
				//    path. If exist - start path error procedure towards path to
				//    root
				if (result.retransmitter == Mac48Address::GetBroadcast())
				{
					result = m_rtable->LookupProactiveExpired();
				}
				if (result.retransmitter != Mac48Address::GetBroadcast())
				{
					std::vector<FailedDestination> destinations = m_rtable->GetUnreachableDestinations(result.retransmitter);
					InitiatePathError(MakePathError(destinations));
				}
				m_stats.totalDropped++;
				return false;
			}
			//Request a destination:
			result = m_rtable->LookupReactiveExpired(destination);
			if (ShouldSendPreq(destination))
			{
				uint32_t originator_seqno = GetNextPmtmgmpSeqno();
				uint32_t dst_seqno = 0;
				if (result.retransmitter != Mac48Address::GetBroadcast())
				{
					dst_seqno = result.seqnum;
				}
				m_stats.initiatedPreq++;
				for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
				{
					i->second->RequestDestination(destination, originator_seqno, dst_seqno);
				}
			}
			QueuedPacket pkt;
			pkt.pkt = packet;
			pkt.dst = destination;
			pkt.src = source;
			pkt.protocol = protocolType;
			pkt.reply = routeReply;
			pkt.inInterface = sourceIface;
			if (QueuePacket(pkt))
			{
				m_stats.totalQueued++;
				return true;
			}
			else
			{
				m_stats.totalDropped++;
				return false;
			}
		}
		void
			PmtmgmpProtocol::ReceivePreq(IePreq preq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			preq.IncrementMetric(metric);
			//acceptance cretirea:
			std::map<Mac48Address, std::pair<uint32_t, uint32_t> >::const_iterator i = m_pmtmgmpSeqnoMetricDatabase.find(
				preq.GetOriginatorAddress());
			bool freshInfo(true);
			if (i != m_pmtmgmpSeqnoMetricDatabase.end())
			{
				if ((int32_t)(i->second.first - preq.GetOriginatorSeqNumber()) > 0)
				{
					return;
				}
				if (i->second.first == preq.GetOriginatorSeqNumber())
				{
					freshInfo = false;
					if (i->second.second <= preq.GetMetric())
					{
						return;
					}
				}
			}
			m_pmtmgmpSeqnoMetricDatabase[preq.GetOriginatorAddress()] =
				std::make_pair(preq.GetOriginatorSeqNumber(), preq.GetMetric());
			NS_LOG_DEBUG("I am " << GetAddress() << "Accepted preq from address" << from << ", preq:" << preq);
			std::vector<Ptr<DestinationAddressUnit> > destinations = preq.GetDestinationList();
			//Add reactive path to originator:
			if (
				(freshInfo) ||
				(
					(m_rtable->LookupReactive(preq.GetOriginatorAddress()).retransmitter == Mac48Address::GetBroadcast()) ||
					(m_rtable->LookupReactive(preq.GetOriginatorAddress()).metric > preq.GetMetric())
					)
				)
			{
				m_rtable->AddReactivePath(
					preq.GetOriginatorAddress(),
					from,
				interface,
					preq.GetMetric(),
					MicroSeconds(preq.GetLifetime() * 1024),
					preq.GetOriginatorSeqNumber()
					);
				ReactivePathResolved(preq.GetOriginatorAddress());
			}
			if (
				(m_rtable->LookupReactive(fromMp).retransmitter == Mac48Address::GetBroadcast()) ||
				(m_rtable->LookupReactive(fromMp).metric > metric)
				)
			{
				m_rtable->AddReactivePath(
					fromMp,
					from,
				interface,
					metric,
					MicroSeconds(preq.GetLifetime() * 1024),
					preq.GetOriginatorSeqNumber()
					);
				ReactivePathResolved(fromMp);
			}
			for (std::vector<Ptr<DestinationAddressUnit> >::const_iterator i = destinations.begin(); i != destinations.end(); i++)
			{
				if ((*i)->GetDestinationAddress() == Mac48Address::GetBroadcast())
				{
					//only proactive PREQ contains destination
					//address as broadcast! Proactive preq MUST
					//have destination count equal to 1 and
					//per destination flags DO and RF
					NS_ASSERT(preq.GetDestCount() == 1);
					NS_ASSERT(((*i)->IsDo()) && ((*i)->IsRf()));
					//Add proactive path only if it is the better then existed
					//before
					if (
						((m_rtable->LookupProactive()).retransmitter == Mac48Address::GetBroadcast()) ||
						((m_rtable->LookupProactive()).metric > preq.GetMetric())
						)
					{
						m_rtable->AddProactivePath(
							preq.GetMetric(),
							preq.GetOriginatorAddress(),
							from,
						interface,
							MicroSeconds(preq.GetLifetime() * 1024),
							preq.GetOriginatorSeqNumber()
							);
						ProactivePathResolved();
					}
					if (!preq.IsNeedNotPrep())
					{
						SendPrep(
							GetAddress(),
							preq.GetOriginatorAddress(),
							from,
							(uint32_t)0,
							preq.GetOriginatorSeqNumber(),
							GetNextPmtmgmpSeqno(),
							preq.GetLifetime(),
						interface
							);
					}
					break;
				}
				if ((*i)->GetDestinationAddress() == GetAddress())
				{
					SendPrep(
						GetAddress(),
						preq.GetOriginatorAddress(),
						from,
						(uint32_t)0,
						preq.GetOriginatorSeqNumber(),
						GetNextPmtmgmpSeqno(),
						preq.GetLifetime(),
					interface
						);
					NS_ASSERT(m_rtable->LookupReactive(preq.GetOriginatorAddress()).retransmitter != Mac48Address::GetBroadcast());
					preq.DelDestinationAddressElement((*i)->GetDestinationAddress());
					continue;
				}
				//check if can answer:
				PmtmgmpRtable::LookupResult result = m_rtable->LookupReactive((*i)->GetDestinationAddress());
				if ((!((*i)->IsDo())) && (result.retransmitter != Mac48Address::GetBroadcast()))
				{
					//have a valid information and can answer
					uint32_t lifetime = result.lifetime.GetMicroSeconds() / 1024;
					if ((lifetime > 0) && ((int32_t)(result.seqnum - (*i)->GetDestSeqNumber()) >= 0))
					{
						SendPrep(
							(*i)->GetDestinationAddress(),
							preq.GetOriginatorAddress(),
							from,
							result.metric,
							preq.GetOriginatorSeqNumber(),
							result.seqnum,
							lifetime,
						interface
							);
						m_rtable->AddPrecursor((*i)->GetDestinationAddress(), interface, from,
							MicroSeconds(preq.GetLifetime() * 1024));
						if ((*i)->IsRf())
						{
							(*i)->SetFlags(true, false, (*i)->IsUsn()); //DO = 1, RF = 0
						}
						else
						{
							preq.DelDestinationAddressElement((*i)->GetDestinationAddress());
							continue;
						}
					}
				}
			}
			//check if must retransmit:
			if (preq.GetDestCount() == 0)
			{
				return;
			}
			//Forward PREQ to all interfaces:
			NS_LOG_DEBUG("I am " << GetAddress() << "retransmitting PREQ:" << preq);
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				i->second->SendPreq(preq);
			}
		}
		void
			PmtmgmpProtocol::ReceivePrep(IePrep prep, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			prep.IncrementMetric(metric);
			//acceptance cretirea:
			std::map<Mac48Address, std::pair<uint32_t, uint32_t> >::const_iterator i = m_pmtmgmpSeqnoMetricDatabase.find(
				prep.GetOriginatorAddress());
			bool freshInfo(true);
			uint32_t sequence = prep.GetDestinationSeqNumber();
			if (i != m_pmtmgmpSeqnoMetricDatabase.end())
			{
				if ((int32_t)(i->second.first - sequence) > 0)
				{
					return;
				}
				if (i->second.first == sequence)
				{
					freshInfo = false;
				}
			}
			m_pmtmgmpSeqnoMetricDatabase[prep.GetOriginatorAddress()] = std::make_pair(sequence, prep.GetMetric());
			//update routing info
			//Now add a path to destination and add precursor to source
			NS_LOG_DEBUG("I am " << GetAddress() << ", received prep from " << prep.GetOriginatorAddress() << ", receiver was:" << from);
			PmtmgmpRtable::LookupResult result = m_rtable->LookupReactive(prep.GetDestinationAddress());
			//Add a reactive path only if seqno is fresher or it improves the
			//metric
			if (
				(freshInfo) ||
				(
					((m_rtable->LookupReactive(prep.GetOriginatorAddress())).retransmitter == Mac48Address::GetBroadcast()) ||
					((m_rtable->LookupReactive(prep.GetOriginatorAddress())).metric > prep.GetMetric())
					)
				)
			{
				m_rtable->AddReactivePath(
					prep.GetOriginatorAddress(),
					from,
				interface,
					prep.GetMetric(),
					MicroSeconds(prep.GetLifetime() * 1024),
					sequence);
				m_rtable->AddPrecursor(prep.GetDestinationAddress(), interface, from,
					MicroSeconds(prep.GetLifetime() * 1024));
				if (result.retransmitter != Mac48Address::GetBroadcast())
				{
					m_rtable->AddPrecursor(prep.GetOriginatorAddress(), interface, result.retransmitter,
						result.lifetime);
				}
				ReactivePathResolved(prep.GetOriginatorAddress());
			}
			if (
				((m_rtable->LookupReactive(fromMp)).retransmitter == Mac48Address::GetBroadcast()) ||
				((m_rtable->LookupReactive(fromMp)).metric > metric)
				)
			{
				m_rtable->AddReactivePath(
					fromMp,
					from,
				interface,
					metric,
					MicroSeconds(prep.GetLifetime() * 1024),
					sequence);
				ReactivePathResolved(fromMp);
			}
			if (prep.GetDestinationAddress() == GetAddress())
			{
				NS_LOG_DEBUG("I am " << GetAddress() << ", resolved " << prep.GetOriginatorAddress());
				return;
			}
			if (result.retransmitter == Mac48Address::GetBroadcast())
			{
				return;
			}
			//Forward PREP
			PmtmgmpProtocolMacMap::const_iterator prep_sender = m_interfaces.find(result.ifIndex);
			NS_ASSERT(prep_sender != m_interfaces.end());
			prep_sender->second->SendPrep(prep, result.retransmitter);
		}
		void
			PmtmgmpProtocol::ReceivePerr(std::vector<FailedDestination> destinations, Mac48Address from, uint32_t interface, Mac48Address fromMp)
		{
			//Acceptance cretirea:
			NS_LOG_DEBUG("I am " << GetAddress() << ", received PERR from " << from);
			std::vector<FailedDestination> retval;
			PmtmgmpRtable::LookupResult result;
			for (unsigned int i = 0; i < destinations.size(); i++)
			{
				result = m_rtable->LookupReactiveExpired(destinations[i].destination);
				if (!(
					(result.retransmitter != from) ||
					(result.ifIndex != interface) ||
					((int32_t)(result.seqnum - destinations[i].seqnum) > 0)
					))
				{
					retval.push_back(destinations[i]);
				}
			}
			if (retval.size() == 0)
			{
				return;
			}
			ForwardPathError(MakePathError(retval));
		}
		void
			PmtmgmpProtocol::SendPrep(
				Mac48Address src,
				Mac48Address dst,
				Mac48Address retransmitter,
				uint32_t initMetric,
				uint32_t originatorDsn,
				uint32_t destinationSN,
				uint32_t lifetime,
				uint32_t interface)
		{
			IePrep prep;
			prep.SetHopcount(0);
			prep.SetTtl(m_maxTtl);
			prep.SetDestinationAddress(dst);
			prep.SetDestinationSeqNumber(destinationSN);
			prep.SetLifetime(lifetime);
			prep.SetMetric(initMetric);
			prep.SetOriginatorAddress(src);
			prep.SetOriginatorSeqNumber(originatorDsn);
			PmtmgmpProtocolMacMap::const_iterator prep_sender = m_interfaces.find(interface);
			NS_ASSERT(prep_sender != m_interfaces.end());
			prep_sender->second->SendPrep(prep, retransmitter);
			m_stats.initiatedPrep++;
		}
		bool
			PmtmgmpProtocol::Install(Ptr<WmnPointDevice> mp)
		{
			m_mp = mp;
			std::vector<Ptr<NetDevice> > interfaces = mp->GetInterfaces();
			for (std::vector<Ptr<NetDevice> >::const_iterator i = interfaces.begin(); i != interfaces.end(); i++)
			{
				// Checking for compatible net device
				Ptr<WifiNetDevice> wifiNetDev = (*i)->GetObject<WifiNetDevice>();
				if (wifiNetDev == 0)
				{
					return false;
				}
				Ptr<WmnWifiInterfaceMac>  mac = wifiNetDev->GetMac()->GetObject<WmnWifiInterfaceMac>();
				if (mac == 0)
				{
					return false;
				}
				// Installing plugins:
				Ptr<PmtmgmpProtocolMac> pmtmgmpMac = Create<PmtmgmpProtocolMac>(wifiNetDev->GetIfIndex(), this);
				m_interfaces[wifiNetDev->GetIfIndex()] = pmtmgmpMac;
				mac->InstallPlugin(pmtmgmpMac);
				//Installing airtime link metric:
				Ptr<PmtmgmpAirtimeLinkMetricCalculator> metric = CreateObject <PmtmgmpAirtimeLinkMetricCalculator>();
				mac->SetLinkMetricCallback(MakeCallback(&PmtmgmpAirtimeLinkMetricCalculator::CalculateMetric, metric));
			}
			mp->SetRoutingProtocol(this);
			// Wmn point aggregates all installed protocols
			mp->AggregateObject(this);
			m_address = Mac48Address::ConvertFrom(mp->GetAddress()); // address;
#ifndef PMTMGMP_UNUSED_MY_CODE
																	 ////ͬ��·�ɱ��е�ַ
			m_RouteTable->InitRouteTable(m_address, m_NodeType, MakeCallback(&PmtmgmpProtocol::SetNodeType, this));
#endif
			return true;
		}
		void
			PmtmgmpProtocol::PmtmgmpPeerLinkStatus(Mac48Address wmnPointAddress, Mac48Address peerAddress, uint32_t interface, bool status)
		{
#ifndef PMTMGMP_UNUSED_MY_CODE
			m_RouteTable->SetPathPeerLinkStatus(wmnPointAddress, peerAddress, status);
			if (status)
			{
				NS_LOG_DEBUG("Status of PeerLink to " << peerAddress << " is True at " << m_address);
			}
			else
			{
				NS_LOG_DEBUG("Status of PeerLink to " << peerAddress << " is False at " << m_address);
			}
#else
			if (status)
			{
				return;
			}
			std::vector<FailedDestination> destinations = m_rtable->GetUnreachableDestinations(peerAddress);
			InitiatePathError(MakePathError(destinations));
#endif
		}
		void
			PmtmgmpProtocol::SetNeighboursCallback(Callback<std::vector<Mac48Address>, uint32_t> cb)
		{
			m_neighboursCallback = cb;
		}
		bool
			PmtmgmpProtocol::DropDataFrame(uint32_t seqno, Mac48Address source)
		{
			if (source == GetAddress())
			{
				return true;
			}
			std::map<Mac48Address, uint32_t, std::less<Mac48Address> >::const_iterator i = m_lastDataSeqno.find(source);
			if (i == m_lastDataSeqno.end())
			{
				m_lastDataSeqno[source] = seqno;
			}
			else
			{
				if ((int32_t)(i->second - seqno) >= 0)
				{
					return true;
				}
				m_lastDataSeqno[source] = seqno;
			}
			return false;
		}
		PmtmgmpProtocol::PathError
			PmtmgmpProtocol::MakePathError(std::vector<FailedDestination> destinations)
		{
			PathError retval;
			//PmtmgmpRtable increments a sequence number as written in 11B.9.7.2
			retval.receivers = GetPerrReceivers(destinations);
			if (retval.receivers.size() == 0)
			{
				return retval;
			}
			m_stats.initiatedPerr++;
			for (unsigned int i = 0; i < destinations.size(); i++)
			{
				retval.destinations.push_back(destinations[i]);
				m_rtable->DeleteReactivePath(destinations[i].destination);
			}
			return retval;
		}
		void
			PmtmgmpProtocol::InitiatePathError(PathError perr)
		{
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				std::vector<Mac48Address> receivers_for_interface;
				for (unsigned int j = 0; j < perr.receivers.size(); j++)
				{
					if (i->first == perr.receivers[j].first)
					{
						receivers_for_interface.push_back(perr.receivers[j].second);
					}
				}
				i->second->InitiatePerr(perr.destinations, receivers_for_interface);
			}
		}
		void
			PmtmgmpProtocol::ForwardPathError(PathError perr)
		{
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				std::vector<Mac48Address> receivers_for_interface;
				for (unsigned int j = 0; j < perr.receivers.size(); j++)
				{
					if (i->first == perr.receivers[j].first)
					{
						receivers_for_interface.push_back(perr.receivers[j].second);
					}
				}
				i->second->ForwardPerr(perr.destinations, receivers_for_interface);
			}
		}

		std::vector<std::pair<uint32_t, Mac48Address> >
			PmtmgmpProtocol::GetPerrReceivers(std::vector<FailedDestination> failedDest)
		{
			PmtmgmpRtable::PrecursorList retval;
			for (unsigned int i = 0; i < failedDest.size(); i++)
			{
				PmtmgmpRtable::PrecursorList precursors = m_rtable->GetPrecursors(failedDest[i].destination);
				m_rtable->DeleteReactivePath(failedDest[i].destination);
				m_rtable->DeleteProactivePath(failedDest[i].destination);
				for (unsigned int j = 0; j < precursors.size(); j++)
				{
					retval.push_back(precursors[j]);
				}
			}
			//Check if we have dublicates in retval and precursors:
			for (unsigned int i = 0; i < retval.size(); i++)
			{
				for (unsigned int j = i + 1; j < retval.size(); j++)
				{
					if (retval[i].second == retval[j].second)
					{
						retval.erase(retval.begin() + j);
					}
				}
			}
			return retval;
		}
		std::vector<Mac48Address>
			PmtmgmpProtocol::GetPreqReceivers(uint32_t interface)
		{
			std::vector<Mac48Address> retval;
			if (!m_neighboursCallback.IsNull())
			{
				retval = m_neighboursCallback(interface);
			}
			if ((retval.size() >= m_unicastPreqThreshold) || (retval.size() == 0))
			{
				retval.clear();
				retval.push_back(Mac48Address::GetBroadcast());
			}
			return retval;
		}
		std::vector<Mac48Address>
			PmtmgmpProtocol::GetBroadcastReceivers(uint32_t interface)
		{
			std::vector<Mac48Address> retval;
			if (!m_neighboursCallback.IsNull())
			{
				retval = m_neighboursCallback(interface);
			}
			if ((retval.size() >= m_unicastDataThreshold) || (retval.size() == 0))
			{
				retval.clear();
				retval.push_back(Mac48Address::GetBroadcast());
			}
			return retval;
		}

		bool
			PmtmgmpProtocol::QueuePacket(QueuedPacket packet)
		{
			if (m_rqueue.size() > m_maxQueueSize)
			{
				return false;
			}
			m_rqueue.push_back(packet);
			return true;
		}

		PmtmgmpProtocol::QueuedPacket
			PmtmgmpProtocol::DequeueFirstPacketByDst(Mac48Address dst)
		{
			QueuedPacket retval;
			retval.pkt = 0;
			for (std::vector<QueuedPacket>::iterator i = m_rqueue.begin(); i != m_rqueue.end(); i++)
			{
				if ((*i).dst == dst)
				{
					retval = (*i);
					m_rqueue.erase(i);
					break;
				}
			}
			return retval;
		}

		PmtmgmpProtocol::QueuedPacket
			PmtmgmpProtocol::DequeueFirstPacket()
		{
			QueuedPacket retval;
			retval.pkt = 0;
			if (m_rqueue.size() != 0)
			{
				retval = m_rqueue[0];
				m_rqueue.erase(m_rqueue.begin());
			}
			return retval;
		}

		void
			PmtmgmpProtocol::ReactivePathResolved(Mac48Address dst)
		{
			std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.find(dst);
			if (i != m_preqTimeouts.end())
			{
				m_routeDiscoveryTimeCallback(Simulator::Now() - i->second.whenScheduled);
			}

			PmtmgmpRtable::LookupResult result = m_rtable->LookupReactive(dst);
			NS_ASSERT(result.retransmitter != Mac48Address::GetBroadcast());
			//Send all packets stored for this destination
			QueuedPacket packet = DequeueFirstPacketByDst(dst);
			while (packet.pkt != 0)
			{
				//set RA tag for retransmitter:
				PmtmgmpTag tag;
				packet.pkt->RemovePacketTag(tag);
				tag.SetAddress(result.retransmitter);
				packet.pkt->AddPacketTag(tag);
				m_stats.txUnicast++;
				m_stats.txBytes += packet.pkt->GetSize();
				packet.reply(true, packet.pkt, packet.src, packet.dst, packet.protocol, result.ifIndex);

				packet = DequeueFirstPacketByDst(dst);
			}
		}
		void
			PmtmgmpProtocol::ProactivePathResolved()
		{
			//send all packets to root
			PmtmgmpRtable::LookupResult result = m_rtable->LookupProactive();
			NS_ASSERT(result.retransmitter != Mac48Address::GetBroadcast());
			QueuedPacket packet = DequeueFirstPacket();
			while (packet.pkt != 0)
			{
				//set RA tag for retransmitter:
				PmtmgmpTag tag;
				if (!packet.pkt->RemovePacketTag(tag))
				{
					NS_FATAL_ERROR("PMTMGMP tag must be present at this point");
				}
				tag.SetAddress(result.retransmitter);
				packet.pkt->AddPacketTag(tag);
				m_stats.txUnicast++;
				m_stats.txBytes += packet.pkt->GetSize();
				packet.reply(true, packet.pkt, packet.src, packet.dst, packet.protocol, result.ifIndex);

				packet = DequeueFirstPacket();
			}
		}

		bool
			PmtmgmpProtocol::ShouldSendPreq(Mac48Address dst)
		{
			std::map<Mac48Address, PreqEvent>::const_iterator i = m_preqTimeouts.find(dst);
			if (i == m_preqTimeouts.end())
			{
				m_preqTimeouts[dst].preqTimeout = Simulator::Schedule(
					Time(m_My11WmnPMTMGMPnetDiameterTraversalTime * 2),
					&PmtmgmpProtocol::RetryPathDiscovery, this, dst, 1);
				m_preqTimeouts[dst].whenScheduled = Simulator::Now();
				return true;
			}
			return false;
		}
		void
			PmtmgmpProtocol::RetryPathDiscovery(Mac48Address dst, uint8_t numOfRetry)
		{
			PmtmgmpRtable::LookupResult result = m_rtable->LookupReactive(dst);
			if (result.retransmitter == Mac48Address::GetBroadcast())
			{
				result = m_rtable->LookupProactive();
			}
			if (result.retransmitter != Mac48Address::GetBroadcast())
			{
				std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.find(dst);
				NS_ASSERT(i != m_preqTimeouts.end());
				m_preqTimeouts.erase(i);
				return;
			}
			if (numOfRetry > m_My11WmnPMTMGMPmaxPREQretries)
			{
				QueuedPacket packet = DequeueFirstPacketByDst(dst);
				//purge queue and delete entry from retryDatabase
				while (packet.pkt != 0)
				{
					m_stats.totalDropped++;
					packet.reply(false, packet.pkt, packet.src, packet.dst, packet.protocol, PmtmgmpRtable::MAX_METRIC);
					packet = DequeueFirstPacketByDst(dst);
				}
				std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.find(dst);
				NS_ASSERT(i != m_preqTimeouts.end());
				m_routeDiscoveryTimeCallback(Simulator::Now() - i->second.whenScheduled);
				m_preqTimeouts.erase(i);
				return;
			}
			numOfRetry++;
			uint32_t originator_seqno = GetNextPmtmgmpSeqno();
			uint32_t dst_seqno = m_rtable->LookupReactiveExpired(dst).seqnum;
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				i->second->RequestDestination(dst, originator_seqno, dst_seqno);
			}
			m_preqTimeouts[dst].preqTimeout = Simulator::Schedule(
				Time((2 * (numOfRetry + 1)) *  m_My11WmnPMTMGMPnetDiameterTraversalTime),
				&PmtmgmpProtocol::RetryPathDiscovery, this, dst, numOfRetry);
		}
		//Proactive PREQ routines:
		void
			PmtmgmpProtocol::SetRoot()
		{
			NS_LOG_DEBUG("ROOT IS: " << m_address);
			m_isRoot = true;
		}
		void
			PmtmgmpProtocol::UnsetRoot()
		{
			m_proactivePreqTimer.Cancel();
		}
		void
			PmtmgmpProtocol::SendProactivePreq()
		{
			IePreq preq;
			//By default: must answer
			preq.SetHopcount(0);
			preq.SetTTL(m_maxTtl);
			preq.SetLifetime(m_My11WmnPMTMGMPactiveRootTimeout.GetMicroSeconds() / 1024);
			//\attention: do not forget to set originator address, sequence
			//number and preq ID in PMTMGMP-MAC plugin
			preq.AddDestinationAddressElement(true, true, Mac48Address::GetBroadcast(), 0);
			preq.SetOriginatorAddress(GetAddress());
			preq.SetPreqID(GetNextPreqId());
			preq.SetOriginatorSeqNumber(GetNextPmtmgmpSeqno());
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				i->second->SendPreq(preq);
			}
			m_proactivePreqTimer = Simulator::Schedule(m_My11WmnPMTMGMPpathToRootInterval, &PmtmgmpProtocol::SendProactivePreq, this);
		}
		bool
			PmtmgmpProtocol::GetDoFlag()
		{
			return m_doFlag;
		}
		bool
			PmtmgmpProtocol::GetRfFlag()
		{
			return m_rfFlag;
		}
		Time
			PmtmgmpProtocol::GetPreqMinInterval()
		{
			return m_My11WmnPMTMGMPpreqMinInterval;
		}
		Time
			PmtmgmpProtocol::GetPerrMinInterval()
		{
			return m_My11WmnPMTMGMPperrMinInterval;
		}
		uint8_t
			PmtmgmpProtocol::GetMaxTtl()
		{
			return m_maxTtl;
		}
		uint32_t
			PmtmgmpProtocol::GetNextPreqId()
		{
			m_preqId++;
			return m_preqId;
		}
		uint32_t
			PmtmgmpProtocol::GetNextPmtmgmpSeqno()
		{
			m_pmtmgmpSeqno++;
			return m_pmtmgmpSeqno;
		}
		uint32_t
			PmtmgmpProtocol::GetActivePathLifetime()
		{
			return m_My11WmnPMTMGMPactivePathTimeout.GetMicroSeconds() / 1024;
		}
		uint8_t
			PmtmgmpProtocol::GetUnicastPerrThreshold()
		{
			return m_unicastPerrThreshold;
		}
		Mac48Address
			PmtmgmpProtocol::GetAddress()
		{
			return m_address;
		}
		//Statistics:
		PmtmgmpProtocol::Statistics::Statistics() :
			txUnicast(0),
			txBroadcast(0),
			txBytes(0),
			droppedTtl(0),
			totalQueued(0),
			totalDropped(0),
			initiatedPreq(0),
			initiatedPrep(0),
			initiatedPerr(0)
#ifndef PMTMGMP_UNUSED_MY_CODE
			,
			initiatedSecreq(0),
			initiatedSecrep(0),
			initiatedSecack(0),
			initiatedPger(0),
			initiatedPgen(0),
			initiatedPupd(0),
			initiatedPupgq(0)
#endif
		{
		}
		void PmtmgmpProtocol::Statistics::Print(std::ostream & os) const
		{
			os << "<Statistics "
				"txUnicast=\"" << txUnicast << "\" "
				"txBroadcast=\"" << txBroadcast << "\" "
				"txBytes=\"" << txBytes << "\" "
				"droppedTtl=\"" << droppedTtl << "\" "
				"totalQueued=\"" << totalQueued << "\" "
				"totalDropped=\"" << totalDropped << "\" "
				"initiatedPreq=\"" << initiatedPreq << "\" "
				"initiatedPrep=\"" << initiatedPrep << "\" "
#ifdef PMTMGMP_UNUSED_MY_CODE
				"initiatedPerr=\"" << initiatedPerr << "\"/>" << std::endl;
#else
				"initiatedPerr=\"" << initiatedPerr << "\" "
				"initiatedSecreq=\"" << initiatedSecreq << "\" "
				"initiatedSecrep=\"" << initiatedSecrep << "\" "
				"initiatedSecack=\"" << initiatedSecack << "\" "
				"initiatedPger=\"" << initiatedPger << "\" "
				"initiatedPgen=\"" << initiatedPgen << "\" "
				"initiatedPupd=\"" << initiatedPupd << "\" "
				"initiatedPupgq=\"" << initiatedPupgq << "\"/>" << std::endl;
#endif
		}
		void
			PmtmgmpProtocol::Report(std::ostream & os) const
		{
#ifndef PMTMGMP_UNUSED_MY_CODE
			std::string typeName;
			switch (m_NodeType)
			{
			case Mesh_STA:
				typeName = "Mesh_STA";
				break;
			case Mesh_Access_Point:
				typeName = "Mesh_Access_Point";
				break;
			case Mesh_Portal:
				typeName = "Mesh_Portal";
				break;
			case Mesh_Secondary_Point:
				typeName = "Mesh_Secondary_Point";
				break;
			default:
				break;
			}
			std::string name[] = { "Mesh_STA", "Mesh_Access_Point", "Mesh_Portal", "Mesh_Secondary_Point" };
			os << "<Pmtmgmp "
				"address=\"" << m_address << "\"" << std::endl <<
				"NodeType=\"" << typeName << "\"" << std::endl <<
				"My11WmnPMTMGMPsecStartDelayTime\"" << m_My11WmnPMTMGMPsecStartDelayTime << "\"" << std::endl <<
				"My11WmnPMTMGMPsecInterval\"" << m_My11WmnPMTMGMPsecInterval << "\"" << std::endl <<
				"My11WmnPMTMGMPsecSetTime\"" << m_My11WmnPMTMGMPsecSetTime << "\"" << std::endl <<
				"My11WmnPMTMGMPpgerWaitTime\"" << m_My11WmnPMTMGMPpgerWaitTime << "\"" << std::endl <<
				"My11WmnPMTMGMPpathMetricUpdatePeriod\"" << m_My11WmnPMTMGMPpathMetricUpdatePeriod << "\"" << std::endl <<
				"PMTMGMPmterpAALMmagnification\"" << m_PMTMGMPmterpAALMmagnification << "\"" << std::endl <<
				"PMTMGMPmsecpAALMmagnification\"" << m_PMTMGMPmsecpAALMmagnification << "\"" << std::endl <<
				"MaxRoutePathPerPUPD\"" << (uint32_t)m_MaxRoutePathPerPUPD << "\"" << std::endl <<
				"MaxRoutePathPerPUPGQ\"" << (uint32_t)m_MaxRoutePathPerPUPGQ << "\">" << std::endl;

#else
			os << "<Pmtmgmp "
				"address=\"" << m_address << "\"" << std::endl <<
				"maxQueueSize=\"" << m_maxQueueSize << "\"" << std::endl <<
				"My11WmnPMTMGMPmaxPREQretries=\"" << (uint16_t)m_My11WmnPMTMGMPmaxPREQretries << "\"" << std::endl <<
				"My11WmnPMTMGMPnetDiameterTraversalTime=\"" << m_My11WmnPMTMGMPnetDiameterTraversalTime.GetSeconds() << "\"" << std::endl <<
				"My11WmnPMTMGMPpreqMinInterval=\"" << m_My11WmnPMTMGMPpreqMinInterval.GetSeconds() << "\"" << std::endl <<
				"My11WmnPMTMGMPperrMinInterval=\"" << m_My11WmnPMTMGMPperrMinInterval.GetSeconds() << "\"" << std::endl <<
				"My11WmnPMTMGMPactiveRootTimeout=\"" << m_My11WmnPMTMGMPactiveRootTimeout.GetSeconds() << "\"" << std::endl <<
				"My11WmnPMTMGMPactivePathTimeout=\"" << m_My11WmnPMTMGMPactivePathTimeout.GetSeconds() << "\"" << std::endl <<
				"My11WmnPMTMGMPpathToRootInterval=\"" << m_My11WmnPMTMGMPpathToRootInterval.GetSeconds() << "\"" << std::endl <<
				"My11WmnPMTMGMPrannInterval=\"" << m_My11WmnPMTMGMPrannInterval.GetSeconds() << "\"" << std::endl <<
				"isRoot=\"" << m_isRoot << "\"" << std::endl <<
				"maxTtl=\"" << (uint16_t)m_maxTtl << "\"" << std::endl <<
				"unicastPerrThreshold=\"" << (uint16_t)m_unicastPerrThreshold << "\"" << std::endl <<
				"unicastPreqThreshold=\"" << (uint16_t)m_unicastPreqThreshold << "\"" << std::endl <<
				"unicastDataThreshold=\"" << (uint16_t)m_unicastDataThreshold << "\"" << std::endl <<
				"doFlag=\"" << m_doFlag << "\"" << std::endl <<
				"rfFlag=\"" << m_rfFlag << "\">" << std::endl;
#endif
			m_stats.Print(os);
			for (PmtmgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin(); plugin != m_interfaces.end(); plugin++)
			{
				plugin->second->Report(os);
			}
			os << "</Pmtmgmp>" << std::endl;
		}
		void
			PmtmgmpProtocol::ResetStats()
		{
			m_stats = Statistics();
			for (PmtmgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin(); plugin != m_interfaces.end(); plugin++)
			{
				plugin->second->ResetStats();
			}
		}

		int64_t
			PmtmgmpProtocol::AssignStreams(int64_t stream)
		{
			NS_LOG_FUNCTION(this << stream);
			m_coefficient->SetStream(stream);
			return 1;
		}

		PmtmgmpProtocol::QueuedPacket::QueuedPacket() :
			pkt(0),
			protocol(0),
			inInterface(0)
		{
		}
#ifndef PMTMGMP_UNUSED_MY_CODE
		////��ȡMAC
		Mac48Address PmtmgmpProtocol::GetMacAddress()
		{
			return m_address;
		}
		////��ȡЭ���·�ɱ�
		Ptr<PmtmgmpRouteTable> PmtmgmpProtocol::GetPmtmgmpRouteTable()
		{
			return m_RouteTable;
		}
		////�ն˽ڵ㷢�� 
		void PmtmgmpProtocol::MSECPSearch()
		{
			NS_LOG_DEBUG("MSECP Search start at " << m_address);
			////��ʼ��MTERP·����
			m_RouteTable->ClearMTERProutePath();

			SendSECREQ();
			////MSECP����
			m_MSECPsearchPeriodEventTimer = Simulator::Schedule(m_My11WmnPMTMGMPsecInterval, &PmtmgmpProtocol::MSECPSearch, this);
		}
		////����SECREQ
		void PmtmgmpProtocol::SendSECREQ()
		{
			NS_LOG_DEBUG("Send SECREQ at " << m_address);
			IeSecreq secreq;
			secreq.SetMTERPaddress(m_address);
			secreq.SetPathGenerationSequenceNumber(m_RouteTable->GetMTERPgenerationSeqNumber());
			secreq.SetNodeType(m_NodeType);
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				m_stats.initiatedSecreq++;
				i->second->SendSECREQ(secreq);
			}
			////�ӳٵȴ�����ѡ��MSECP
			m_MSECPsetEventTimer = Simulator::Schedule(m_My11WmnPMTMGMPsecSetTime, &PmtmgmpProtocol::SelectMSECP, this, m_RouteTable->GetMTERPgenerationSeqNumber());
		}
		////����SECREQ
		void PmtmgmpProtocol::ReceiveSECREQ(IeSecreq secreq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			if (IsMTERP())
			{
				NS_LOG_DEBUG("Receive SECREQ in MTERP from " << from << " at interface " << interface << " while metric is " << metric << " at " << m_address << " GSN is " << secreq.GetPathGenerationSequenceNumber() << " node type is " << secreq.GetNodeType());
			}
			else
			{
				NS_LOG_DEBUG("Receive SECREQ from " << from << " at interface " << interface << " while metric is " << metric << " at " << m_address << " GSN is " << secreq.GetPathGenerationSequenceNumber() << " node type is " << secreq.GetNodeType());
				SendSECREP(secreq.GetMTERPaddress(), secreq.GetPathGenerationSequenceNumber(), secreq.GetNodeType());
			}
		}
		////����SECREP
		void PmtmgmpProtocol::SendSECREP(Mac48Address receiver, uint32_t index, NodeType type)
		{
			NS_LOG_DEBUG("Send SECREP to " << receiver << " at " << m_address);
			IeSecrep secrep;
			secrep.SetMTERPaddress(receiver);
			secrep.SetCandidateMSECPaddress(m_address);
			secrep.SetMSECPcount(m_RouteTable->GetAllMSECPcount(receiver, type));
			secrep.SetPathGenerationSequenceNumber(index);
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				m_stats.initiatedSecrep++;
				i->second->SendSECREP(secrep, receiver);
			}
		}
		////����SECREP
		void PmtmgmpProtocol::ReceiveSECREP(IeSecrep secrep, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			NS_LOG_DEBUG("Receive SECREP from " << from << " at interface " << interface << " while metric is " << metric << " at " << m_address);

			if ((secrep.GetPathGenerationSequenceNumber() != m_RouteTable->GetMTERPgenerationSeqNumber()) || (secrep.GetMTERPaddress() != m_address))
			{
				return;////����SECREP���ᱻ����
			}
			////������ѡ·��·��
			m_RouteTable->AddMSECPpath(secrep.GetMTERPaddress(), secrep.GetCandidateMSECPaddress(), secrep.GetMSECPcount(), metric, secrep.GetPathGenerationSequenceNumber(), m_maxTtl);
		}
		////���úͻ�ȡ��ǰ�ڵ�����
		void PmtmgmpProtocol::SetNodeType(NodeType type)
		{
			m_NodeType = type;
			m_RouteTable->SetNodeType(type);
		}
		PmtmgmpProtocol::NodeType PmtmgmpProtocol::GetNodeType()
		{
			return m_NodeType;
		}
		////��ȡAALMʱ���õ�m��ֵ
		uint8_t PmtmgmpProtocol::GetValueMForAALM()
		{
			if (m_NodeType == Mesh_Access_Point || m_NodeType == Mesh_Portal)
			{
				return m_PMTMGMPmterpAALMmagnification;
			}
			if (m_NodeType == Mesh_Secondary_Point)
			{
				return m_PMTMGMPmterpAALMmagnification;
			}
			return 1;
		}
		////��ȡÿ����������֡��·�����͵�������
		std::map<Mac48Address, uint32_t> PmtmgmpProtocol::GetPacketSizePerPath()
		{
			return m_PacketSizePerPath;
		}
		////Python֧�ֺ���
		uint32_t PmtmgmpProtocol::GetPacketSizePerPathCount()
		{
			return m_PacketSizePerPath.size();
		}
		Mac48Address PmtmgmpProtocol::GetPacketSizePerPathFirst(uint8_t index)
		{
			std::map<Mac48Address, uint32_t>::iterator iter = m_PacketSizePerPath.begin();
			for (uint8_t i = 0; i < index; i++)
			{
				iter++;
			}
			return iter->first;
		}
		uint32_t PmtmgmpProtocol::GetPacketSizePerPathSecond(uint8_t index)
		{
			std::map<Mac48Address, uint32_t>::iterator iter = m_PacketSizePerPath.begin();
			for (uint8_t i = 0; i < index; i++)
			{
				iter++;
			}
			return iter->second;
		}
		uint32_t PmtmgmpProtocol::GetPacketSizePerPathSecondByMac(Mac48Address first)
		{
			if (m_PacketSizePerPath.find(first) != m_PacketSizePerPath.end())
			{
				return m_PacketSizePerPath.find(first)->second;
			}
			else
			{
				return 0;
			}
		}
		////��֤Э����������Ƿ�Ϊ�ն˽ڵ�MTERP
		bool PmtmgmpProtocol::IsMTERP()
		{
			return (m_NodeType & (Mesh_Access_Point | Mesh_Portal)) != 0;
		}
		////�ӳ�����SECREP��Ϣ��ѡȡ�ɽ��ܵ�MSECP
		void PmtmgmpProtocol::SelectMSECP(uint32_t gsn)
		{
			if (gsn != m_RouteTable->GetMTERPgenerationSeqNumber())
			{
				////�µ�·�����ɿ�ʼ�������˹���
				return;
			}

			////Ч���Ƿ��յ�SECREP����δ�յ����ط�SECREQ
			if (m_RouteTable->GetMTERPtree() == 0)
			{
				NS_LOG_DEBUG("Select MSECP: receive SECREP " << " at " << m_address << ", Research.");
				MSECPSearch();
			}
			else
			{
				NS_LOG_DEBUG("Select MSECP: receive SECREP " << m_RouteTable->GetMTERPtree()->GetPartTree().size() << " at " << m_address);
				if (m_RouteTable->SelectMSECP())
				{

					///Ϊɸѡ���Ľ������SECACK
					SendSECACK();

					////�ӳٵȴ�����PGER
					m_PGERwaitEventTimer = Simulator::Schedule(m_My11WmnPMTMGMPpgerWaitTime, &PmtmgmpProtocol::WaitForRecivePGER, this);
				}
				else
				{
					NS_LOG_DEBUG("Select MSECP: receive SECREP " << m_RouteTable->GetMTERPtree()->GetPartTree().size() << " at " << m_address << ", Research.");
					MSECPSearch();
				}
			}
		}
		////����SECACK
		void PmtmgmpProtocol::SendSECACK()
		{
			std::vector<Ptr<PmtmgmpRoutePath> > paths = m_RouteTable->GetMTERPtree()->GetPartTree();
			for (std::vector<Ptr<PmtmgmpRoutePath> >::iterator iter = paths.begin(); iter != paths.end(); iter++)
			{
				IeSecack secack = IeSecack(*iter);
				secack.SetMSECPindex((*iter)->GetMSECPindex());
				if ((*iter)->GetStatus() == PmtmgmpRoutePath::Waited)
				{
					for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
					{
						NS_LOG_DEBUG("Send SECACK at " << m_address << " to " << (*iter)->GetMSECPaddress() << " MSECPindex:" << uint32_t((*iter)->GetMSECPindex()));
						m_stats.initiatedSecack++;
						i->second->SendSECACK(secack, (*iter)->GetMSECPaddress());
					}
				}
			}
		}
		////����SECACK
		void PmtmgmpProtocol::ReceiveSECACK(IeSecack secack, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			Ptr<PmtmgmpRoutePath> path = m_RouteTable->GetPathByMACaddress(secack.GetMTERPaddress(), secack.GetMSECPaddress());
			if (path != 0)
			{
				if (path->GetPathGenerationSequenceNumber() > secack.GetPathGenerationSequenceNumber())
				{
					NS_LOG_DEBUG("Receive SECACK have expired from " << from << " at interface " << interface << " while metric is " << metric << " at " << m_address);
					return;
				}
			}

			NS_LOG_DEBUG("Receive SECACK from " << from << " at interface " << interface << " while metric is " << metric << " at " << m_address);

			////��ȡPGEN����
			Ptr<PmtmgmpRoutePath> pathCopy = secack.GetPathInfo()->GetCopy();

			////PGEN��Ϣ����
			pathCopy->DecrementTtl();
			pathCopy->IncrementMetric(metric, GetValueMForAALM());

			pathCopy->SetFromNode(from);
			pathCopy->SetPGENsendTime();
			pathCopy->IncreasePathGenerationSequenceNumber();
			pathCopy->SetInterface(interface);
			m_RouteTable->AddAsMSECPpath(pathCopy);

			SendPGER(pathCopy->GetPathGenerationSequenceNumber(), pathCopy->GetMTERPaddress());
			SendPGEN(IePgen(pathCopy));
		}
		////����PGER
		void PmtmgmpProtocol::SendPGER(uint32_t gsn, Mac48Address address)
		{
			IePger pger;
			pger.SetPathGenerationSequenceNumber(gsn);

			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				NS_LOG_DEBUG("Send PGER at " << m_address << " to " << address);
				m_stats.initiatedPger++;
				i->second->SendPGER(pger, address);
			}
		}
		////��ʱ����PGER
		void PmtmgmpProtocol::WaitForRecivePGER()
		{
			uint8_t count = m_RouteTable->GetUnreceivedPathCount();
			if (count == 0)
			{
				NS_LOG_DEBUG("Unreceived MSECP's number " << (uint32_t)count << " at " << m_address);
				return;
			}
			else
			{
				SendSECACK();
				////�ٴ��ӳٵȴ�����PGER
				m_PGERwaitEventTimer = Simulator::Schedule(m_My11WmnPMTMGMPpgerWaitTime, &PmtmgmpProtocol::WaitForRecivePGER, this);
			}
		}
		////����PGER
		void PmtmgmpProtocol::ReceivePGER(IePger pger, Mac48Address from, uint32_t, Mac48Address fromMp, uint32_t metric)
		{
			Ptr<PmtmgmpRouteTree> mterpTree = m_RouteTable->GetMTERPtree();
			Ptr<PmtmgmpRoutePath> path = mterpTree->GetPathByMACaddress(from);
			if (path == 0)
			{
				NS_LOG_DEBUG("Receive PGER but can`t find the path(MTERP:" << m_address << " ,MSECP:" << from << " at " << m_address);
				return;
			}
			if (m_RouteTable->GetMTERPgenerationSeqNumber() != pger.GetPathGenerationSequenceNumber())
			{
				NS_LOG_DEBUG("PGER have expired at " << m_address << " while GSN of this MTERP node is " << m_RouteTable->GetMTERPgenerationSeqNumber() << " and the GSN of PGER is " << pger.GetPathGenerationSequenceNumber());
				return;
			}
			path->SetStatus(PmtmgmpRoutePath::Confirmed);
			NS_LOG_DEBUG("Receive PGER at " << m_address << " while GSN of this MTERP node is " << m_RouteTable->GetMTERPgenerationSeqNumber() << " and the GSN of PGER is " << pger.GetPathGenerationSequenceNumber());
		}
		////ת��pgen
		void PmtmgmpProtocol::SendPGEN(IePgen pgen)
		{
			NS_LOG_DEBUG("Send PGEN " << " at node " << m_address << " while metric is " << pgen.GetMetric() << ", origination is " << pgen.GetMTERPaddress() << ", GSN is " << pgen.GetPathGenerationSequenceNumber());
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				m_stats.initiatedPgen++;
				i->second->SendPGEN(pgen);
			}
		}
		////����PGEN
		void PmtmgmpProtocol::ReceivePGEN(IePgen pgen, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			////������ʼ�ڵ�
			if (pgen.GetMTERPaddress() == m_address)
			{
				NS_LOG_DEBUG("Receive PGEN from " << from << " at node " << m_address << " , this is the MTERP, return.");
				return;
			}
			if (pgen.GetMSECPaddress() == m_address)
			{
				NS_LOG_DEBUG("Receive PGEN from " << from << " at node " << m_address << " , this is the MSECP, return.");
				return;
			}

			////��ȡPGEN����
			Ptr<PmtmgmpRoutePath> pathCopy = pgen.GetPathInfo()->GetCopy();
			pathCopy->SetInterface(interface);

			////PGEN��Ϣ����
			pathCopy->DecrementTtl();
			pathCopy->IncrementMetric(metric, GetValueMForAALM());

			pathCopy->SetFromNode(from);
			pathCopy->SetPGENsendTime();

#ifndef PMTMGMP_UNUSED_REPEAT_PATH //ʹ��·���ظ�Ч��
			////·�ɱ��Ƿ����PGEN��Դ��Ӧ·��
			Ptr<PmtmgmpRouteTree> routeTree = m_RouteTable->GetTreeByMACaddress(pathCopy->GetMTERPaddress());
			if (routeTree == 0)
			{
				////��ǰ·�ɱ��в���������PGENԴ�ڵ��·��
				pathCopy->SetStatus(PmtmgmpRoutePath::Confirmed);
				m_RouteTable->AddNewPath(pathCopy);
				routeTree = m_RouteTable->GetTreeByMACaddress(pathCopy->GetMTERPaddress());
				routeTree->RepeatabilityIncrease(from);
			}
			else
			{
				////PGEN��ȫ��·����GSN�Ƚ�
				uint32_t GenSeqNum = pathCopy->GetPathGenerationSequenceNumber();
				if (GenSeqNum > routeTree->GetTreeMaxGenerationSeqNumber())
				{
					////��ǰ·�ɱ���û����ͬ����˳��ŵļ�¼��Ϊ��·������ĵ�һ��PGEN��
					routeTree->SetAllStatusExpired();
					pathCopy->SetStatus(PmtmgmpRoutePath::Confirmed);
					routeTree->AddNewPath(pathCopy);
					routeTree->RepeatabilityIncrease(from);
				}
				else if (GenSeqNum == routeTree->GetTreeMaxGenerationSeqNumber())
				{
					////PGEN����˳��ŵ���·�ɱ����������˳���

					////·�����ظ�Ч��
					if (routeTree->GetRepeatability(from) == 0)
					{
						////Ч��ͨ������������
						Ptr<PmtmgmpRoutePath> existPath = routeTree->GetPathByMACaddress(pathCopy->GetMSECPaddress());
						////·�ɱ���·������Ч��	
						if (existPath == 0)
						{
							////��ǰ·�ɱ��в����ڸ����ڵ��Ӧ��·��
							pathCopy->SetStatus(PmtmgmpRoutePath::Confirmed);
							routeTree->AddNewPath(pathCopy);
							routeTree->RepeatabilityIncrease(from);

						}
						else
						{
							if (existPath->GetPathGenerationSequenceNumber() > GenSeqNum)
							{
								//����·���ظ���
								pathCopy->SetStatus(PmtmgmpRoutePath::Confirmed);
								routeTree->RepeatabilityIncrease(from);
								routeTree->AddNewPath(pathCopy);
							}
							else
							{
								////PGEN����·��������Ϣ��ȷ��
								NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " has been Confirmed");
								return;
							}
						}
					}
					else
					{
						////Ч��δͨ��
						Ptr<PmtmgmpRoutePath> existPath = routeTree->GetPathByMACaddress(pathCopy->GetMSECPaddress());

						if (existPath == 0)
						{
							Ptr<PmtmgmpRoutePath> newPath = pathCopy->GetCopy();

							////������·����Ϊδ����״̬
							newPath->SetStatus(PmtmgmpRoutePath::Waited);
							newPath->SetPathGenerationSequenceNumber(GenSeqNum - 1);
							newPath->AddCandidateRouteInformaiton(pathCopy);
							newPath->SetFromNode(from);
							newPath->SetAcceptCandidateRouteInformaitonEvent(Simulator::Schedule(routeTree->GetAcceptInformaitonDelay(), &PmtmgmpRouteTree::AcceptCandidateRouteInformaiton, routeTree, pathCopy->GetMSECPaddress()));
							newPath->SetInterface(interface);
							routeTree->AddNewPath(newPath);
							NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " start waiting");
							return;
						}
						else
						{
							switch (existPath->GetStatus())
							{
							case PmtmgmpRoutePath::Expired:
								if (GenSeqNum == existPath->GetPathGenerationSequenceNumber())
								{
									NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " has been Expired");
								}
								else
								{
									existPath->SetStatus(PmtmgmpRoutePath::Waited);
									existPath->AddCandidateRouteInformaiton(pathCopy);
									existPath->SetAcceptCandidateRouteInformaitonEvent(Simulator::Schedule(routeTree->GetAcceptInformaitonDelay(), &PmtmgmpRouteTree::AcceptCandidateRouteInformaiton, routeTree, pathCopy->GetMSECPaddress()));
									NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " start waiting");
								}
								return;
							case PmtmgmpRoutePath::Waited:
								if (GenSeqNum == existPath->GetPathGenerationSequenceNumber())
								{
									NS_LOG_ERROR("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " , GSN is equal, path status can not be Waited.");
								}
								else
								{
									existPath->AddCandidateRouteInformaiton(pathCopy);
									NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " waits for Confirm");
								}
								return;
							case PmtmgmpRoutePath::Confirmed:
								if (GenSeqNum == existPath->GetPathGenerationSequenceNumber())
								{
									////PGEN����·��������Ϣ��ȷ��
									NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " has been Confirmed");
								}
								else
								{
									existPath->SetStatus(PmtmgmpRoutePath::Waited);
									existPath->AddCandidateRouteInformaiton(pathCopy);
									existPath->SetAcceptCandidateRouteInformaitonEvent(Simulator::Schedule(routeTree->GetAcceptInformaitonDelay(), &PmtmgmpRouteTree::AcceptCandidateRouteInformaiton, routeTree, pathCopy->GetMSECPaddress()));
									NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " start waiting");
								}
								return;
							}
						}
					}
				}
				else
				{
					////����PGEN������
					NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " has been Expired");
					return;
				}
			}
#else
			Ptr<PmtmgmpRouteTree> routeTree = m_RouteTable->GetTreeByMACaddress(pathCopy->GetMTERPaddress());
			if (routeTree != 0)
			{
				Ptr<PmtmgmpRoutePath> existPath = routeTree->GetPathByMACaddress(pathCopy->GetMSECPaddress());
				if (existPath != 0)
				{
					if (existPath->GetStatus() == PmtmgmpRoutePath::Confirmed && pathCopy->GetPathGenerationSequenceNumber() == existPath->GetPathGenerationSequenceNumber())
					{
						//·����Ϣ�����ҵ�ǰ��������ɣ�����PGEN
						return;
					}
				}
			}
			pathCopy->SetStatus(PmtmgmpRoutePath::Confirmed);
			m_RouteTable->AddNewPath(pathCopy);
#endif
			m_RouteTable->SendQueuePackets(pathCopy->GetMTERPaddress(), &m_stats, &m_PacketSizePerPath);
			if (pathCopy->GetTTL() > 0)
			{
				////ת��PGEN
				NS_LOG_DEBUG("Receive PGEN (MTERP:" << pathCopy->GetMTERPaddress() << " MSECP:" << pathCopy->GetMSECPaddress() << ") from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", GSN is " << pathCopy->GetPathGenerationSequenceNumber() << " has been Transmit");
				SendPGEN(IePgen(pathCopy));
			}
		}
		////AALM����
		void PmtmgmpProtocol::PUPDperiod()
		{
			if (m_RouteTable->GetConfirmedPathSize() != 0)
			{
				NS_LOG_DEBUG("Period for PUPD at " << m_address << " , do send.");
				SendPUPD();
			}
			else
			{
				NS_LOG_DEBUG("Period for PUPD at " << m_address << " , do not send.");
			}
			m_PUPDperiodEventTimer = Simulator::Schedule(m_My11WmnPMTMGMPpathMetricUpdatePeriod, &PmtmgmpProtocol::PUPDperiod, this);
		}
		////����PUPD
		void PmtmgmpProtocol::SendPUPD()
		{
			IePupd pupd = IePupd(m_RouteTable, m_MaxRoutePathPerPUPD, m_My11WmnPMTMGMPsecInterval);

			////PUPD����·���б�Ϊ��ʱ������PUPD
			if (pupd.GetData().size() == 0)
			{
				NS_LOG_DEBUG("PUPD is empty at " << m_address << " do not send.");
				return;
			}

			NS_LOG_DEBUG("Send PUPD at " << m_address << ", its size is " << uint32_t(pupd.GetInformationFieldSize()));
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				m_stats.initiatedPupd++;
				i->second->SendPUPD(pupd);
			}
		}
		////����PUPD
		void PmtmgmpProtocol::ReceivePUPD(IePupd pupd, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			std::vector<PUPGQdata> recreateList;
			std::vector<PUPDaalmTreeData> data = pupd.GetData();
			////AALM���²�������
			NS_LOG_DEBUG("Receive PUPD  from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric);
			for (std::vector<PUPDaalmTreeData>::iterator iter = data.begin(); iter != data.end(); iter++)
			{
				std::vector<PUPDaalmPathData> tempData = iter->GetData();
				Mac48Address mterp = iter->GetMTERPaddress();
				for (std::vector<PUPDaalmPathData>::iterator select = tempData.begin(); select != tempData.end(); select++)
				{
					Ptr<PmtmgmpRoutePath> find = m_RouteTable->GetPathByMACindex(mterp, select->GetMSECPindex());
					if (find == 0)
					{
						////·�ɱ��в����ڸ�����Ϣ�е�·��·������PUPD�����߲��Ǵ�·����MTERP�������
						if (recreateList.size() < m_MaxRoutePathPerPUPGQ && mterp != from)
						{
							recreateList.push_back(PUPGQdata(mterp, select->GetMSECPindex()));
						}
						NS_LOG_DEBUG("PUPD  Infor: MTERP(" << mterp << "),MSECPindex(" << uint32_t(select->GetMSECPindex()) << "), RecreateListSize(" << recreateList.size() << ")");
					}
					else if (find->GetFromNode() == from)
					{
						////·�ɱ��д���ָ��·��·����������PUPD�ķ����ߣ�����Metric
						uint32_t old = find->GetMetric();
						find->SetMetric(select->GetMetric());
						find->IncrementMetric(metric, GetValueMForAALM());
<<<<<<< HEAD
						NS_LOG_DEBUG("PUPD  Infor: MTERP(" << mterp << "),MSECPindex(" << uint32_t(select->GetMSECPindex()) << "), Metric(From " << old << " to " << find->GetMetric() << " , Base:" << find-> GetBaseMetric()<< " Step:" << metric << ")");
=======
						NS_LOG_DEBUG("PUPD  Infor: MTERP(" << mterp << "),MSECPindex(" << uint32_t(select->GetMSECPindex()) << "), Metric(From " << old << " to " << find->GetMetric() << " , Base:" << find->GetBaseMetric() << " Step:" << metric << ")");
						m_RouteTable->SendQueuePackets(find->GetMTERPaddress(), &m_stats, &m_PacketSizePerPath);
>>>>>>> refs/remotes/Whimsyduke/humgmp
					}
					else
					{
						////·�ɱ��д���ָ��·��·���Ҳ���������PUPD�ķ����ߣ������κ�����
						m_RouteTable->SendQueuePackets(find->GetMTERPaddress(), &m_stats, &m_PacketSizePerPath);
						continue;
					}
				}
			}
			if (recreateList.size() != 0)
			{
				SendPUPGQ(from, recreateList);
			}
			NS_LOG_DEBUG("Receive PUPD, it is PUPGQ  contain " << recreateList.size() << " Route Path at " << m_address);
		}
		////����PUPGQ
		void PmtmgmpProtocol::SendPUPGQ(Mac48Address receiver, std::vector<PUPGQdata> list)
		{
			NS_LOG_DEBUG("Send PUPGQ to " << receiver << " at " << m_address);
			IePupgq pupgq = IePupgq(list);
			for (PmtmgmpProtocolMacMap::const_iterator i = m_interfaces.begin(); i != m_interfaces.end(); i++)
			{
				m_stats.initiatedPupgq++;
				i->second->SendPUPGQ(pupgq, receiver);
			}
		}
		////����PUPGQ
		void PmtmgmpProtocol::ReceivePUPGQ(IePupgq pupgq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
		{
			int sendTimes = 0;
			std::vector<PUPGQdata> temp = pupgq.GetPathList();
			for (std::vector<PUPGQdata>::iterator iter = temp.begin(); iter != temp.end(); iter++)
			{
				Ptr<PmtmgmpRoutePath> path = m_RouteTable->GetPathByMACindex(iter->GetMTERPaddress(), iter->GetMSECPindex());
				if (path != 0)
				{
					if (path->GetStatus() == PmtmgmpRoutePath::Confirmed)
					{
						////���ͼ������PUPD���ͼ��������������η���
						if (Simulator::Now() - path->GetPGENsendTime() > m_My11WmnPMTMGMPpathMetricUpdatePeriod)
							path->SetPGENsendTime();
						SendPGEN(IePgen(path->GetCopy()));
						sendTimes++;
					}
				}
			}
			NS_LOG_DEBUG("Receive PUPGQ  from " << from << " at node " << m_address << " at interface " << interface << " while metric is " << metric << ", and PUPGQ  contain " << sendTimes << " Route Path.");
		}
#endif
	} // namespace my11s
} // namespace ns3