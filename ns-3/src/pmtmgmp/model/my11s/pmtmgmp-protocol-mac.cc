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

#include "ns3/wmn-wifi-interface-mac.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "my11s-mac-header.h"
#include "pmtmgmp-protocol-mac.h"
#include "pmtmgmp-tag.h"
#include "ie-my11s-preq.h"
#include "ie-my11s-prep.h"
#include "ie-my11s-rann.h"
#include "ie-my11s-perr.h"

#ifndef PMTMGMP_UNUSED_MY_CODE
#include "ie-my11s-secreq.h"
#include "ie-my11s-secrep.h"
#include "ie-my11s-secack.h"
#include "ie-my11s-pger.h"
#include "ie-my11s-pgen.h"
#include "ns3/ie-my11s-pupd.h"
#include "ns3/ie-my11s-pupgq.h"
#endif

////多播和广播选择
#ifndef PMTMGMP_UNUSED_MY_CODE
#ifndef PMTMGMP_USE_MULTICAST
#define PMTMGMP_USE_MULTICAST
#endif
#endif

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE("PmtmgmpProtocolMac");

	namespace my11s {

		PmtmgmpProtocolMac::PmtmgmpProtocolMac(uint32_t ifIndex, Ptr<PmtmgmpProtocol> protocol) :
			m_ifIndex(ifIndex), m_protocol(protocol)
		{
		}
		PmtmgmpProtocolMac::~PmtmgmpProtocolMac()
		{
		}
		void
			PmtmgmpProtocolMac::SetParent(Ptr<WmnWifiInterfaceMac> parent)
		{
			m_parent = parent;
		}

		bool
			PmtmgmpProtocolMac::ReceiveData(Ptr<Packet> packet, const WifiMacHeader & header)
		{
			NS_ASSERT(header.IsData());

			WmnHeader wmnHdr;
			PmtmgmpTag tag;
			if (packet->PeekPacketTag(tag))
			{
				NS_FATAL_ERROR("PMTMGMP tag is not supposed to be received by network");
			}

			packet->RemoveHeader(wmnHdr);
			m_stats.rxData++;
			m_stats.rxDataBytes += packet->GetSize();

			/// \todo address extension
			Mac48Address destination;
			Mac48Address source;
			switch (wmnHdr.GetAddressExt())
			{
			case 0:
				source = header.GetAddr4();
				destination = header.GetAddr3();
				break;
			default:
				NS_FATAL_ERROR(
					"6-address scheme is not yet supported and 4-address extension is not supposed to be used for data frames.");
			}
			tag.SetSeqno(wmnHdr.GetWmnSeqno());
			tag.SetTtl(wmnHdr.GetWmnTtl());
			packet->AddPacketTag(tag);

			if ((destination == Mac48Address::GetBroadcast()) && (m_protocol->DropDataFrame(wmnHdr.GetWmnSeqno(),
				source)))
			{
				return false;
			}
			return true;
		}

		bool
			PmtmgmpProtocolMac::ReceiveAction(Ptr<Packet> packet, const WifiMacHeader & header)
		{
			m_stats.rxMgt++;
			m_stats.rxMgtBytes += packet->GetSize();
			WifiActionHeader actionHdr;
			packet->RemoveHeader(actionHdr);
			if (actionHdr.GetCategory() != WifiActionHeader::MESH)
			{
				return true;
			}
			WmnInformationElementVector elements;
			packet->RemoveHeader(elements);
			std::vector<PmtmgmpProtocol::FailedDestination> failedDestinations;
			for (WmnInformationElementVector::Iterator i = elements.Begin(); i != elements.End(); i++)
			{
				if ((*i)->ElementId() == IE11S_RANN)
				{
					NS_LOG_WARN("RANN is not supported!");
				}
				if ((*i)->ElementId() == IE11S_PREQ)
				{
					Ptr<IePreq> preq = DynamicCast<IePreq>(*i);
					NS_ASSERT(preq != 0);
					m_stats.rxPreq++;
					if (preq->GetOriginatorAddress() == m_protocol->GetAddress())
					{
						continue;
					}
					if (preq->GetTtl() == 0)
					{
						continue;
					}
					preq->DecrementTtl();
					m_protocol->ReceivePreq(*preq, header.GetAddr2(), m_ifIndex, header.GetAddr3(),
						m_parent->GetLinkMetric(header.GetAddr2()));
				}
				if ((*i)->ElementId() == IE11S_PREP)
				{
					Ptr<IePrep> prep = DynamicCast<IePrep>(*i);
					NS_ASSERT(prep != 0);
					m_stats.rxPrep++;
					if (prep->GetTtl() == 0)
					{
						continue;
					}
					prep->DecrementTtl();
					m_protocol->ReceivePrep(*prep, header.GetAddr2(), m_ifIndex, header.GetAddr3(),
						m_parent->GetLinkMetric(header.GetAddr2()));
				}
				if ((*i)->ElementId() == IE11S_PERR)
				{
					Ptr<IePerr> perr = DynamicCast<IePerr>(*i);
					NS_ASSERT(perr != 0);
					m_stats.rxPerr++;
					std::vector<PmtmgmpProtocol::FailedDestination> destinations = perr->GetAddressUnitVector();
					for (std::vector<PmtmgmpProtocol::FailedDestination>::const_iterator i = destinations.begin(); i
						!= destinations.end(); i++)
					{
						failedDestinations.push_back(*i);
					}
				}
#ifndef PMTMGMP_UNUSED_MY_CODE
				if ((*i)->ElementId() == IE11S_SECREQ)
				{
					Ptr<IeSecreq> secreq = DynamicCast<IeSecreq>(*i);
					NS_ASSERT(secreq != 0);
					m_stats.rxSecreq++;
					m_protocol->ReceiveSecreq(*secreq, header.GetAddr2(), m_ifIndex, header.GetAddr3(), m_parent->GetLinkMetric(header.GetAddr2()));
				}
				if ((*i)->ElementId() == IE11S_SECREP)
				{
					Ptr<IeSecrep> secrep = DynamicCast<IeSecrep>(*i);
					NS_ASSERT(secrep != 0);
					m_stats.rxSecrep++;
					m_protocol->ReceiveSecrep(*secrep, header.GetAddr2(), m_ifIndex, header.GetAddr3(), m_parent->GetLinkMetric(header.GetAddr2()));
				}
				if ((*i)->ElementId() == IE11S_SECACK)
				{
					Ptr<IeSecack> secack = DynamicCast<IeSecack>(*i);
					NS_ASSERT(secack != 0);
					m_stats.rxSecack++;
					m_protocol->ReceiveSecack(*secack, header.GetAddr2(), m_ifIndex, header.GetAddr3(), m_parent->GetLinkMetric(header.GetAddr2()));
				}
				if ((*i)->ElementId() == IE11S_PGER)
				{
					Ptr<IePger> pger = DynamicCast<IePger>(*i);
					NS_ASSERT(pger != 0);
					m_stats.rxPger++;
					m_protocol->ReceivePger(*pger, header.GetAddr2(), m_ifIndex, header.GetAddr3(), m_parent->GetLinkMetric(header.GetAddr2()));
				}
				if ((*i)->ElementId() == IE11S_PGEN)
				{
					Ptr<IePgen> pgen = DynamicCast<IePgen>(*i);
					NS_ASSERT(pgen != 0);
					m_stats.rxPgen++;
					m_protocol->ReceivePgen(*pgen, header.GetAddr2(), m_ifIndex, header.GetAddr3(), m_parent->GetLinkMetric(header.GetAddr2()));
				}
#endif
			}
			if (failedDestinations.size() > 0)
			{
				m_protocol->ReceivePerr(failedDestinations, header.GetAddr2(), m_ifIndex, header.GetAddr3());
			}
			NS_ASSERT(packet->GetSize() == 0);
			return false;
		}

		bool
			PmtmgmpProtocolMac::Receive(Ptr<Packet> packet, const WifiMacHeader & header)
		{
			if (header.IsData())
			{
				return ReceiveData(packet, header);
			}
			else
			{
				if (header.IsAction())
				{
					return ReceiveAction(packet, header);
				}
				else
				{
					return true; // don't care
				}
			}
		}
		bool
			PmtmgmpProtocolMac::UpdateOutcomingFrame(Ptr<Packet> packet, WifiMacHeader & header, Mac48Address from,
			Mac48Address to)
		{
			if (!header.IsData())
			{
				return true;
			}
			PmtmgmpTag tag;
			bool tagExists = packet->RemovePacketTag(tag);
			if (!tagExists)
			{
				NS_FATAL_ERROR("PMTMGMP tag must exist at this point");
			}
			m_stats.txData++;
			m_stats.txDataBytes += packet->GetSize();
			WmnHeader wmnHdr;
			wmnHdr.SetWmnSeqno(tag.GetSeqno());
			wmnHdr.SetWmnTtl(tag.GetTtl());
			packet->AddHeader(wmnHdr);
			header.SetAddr1(tag.GetAddress());
			return true;
		}
		WifiActionHeader
			PmtmgmpProtocolMac::GetWifiActionHeader()
		{
			WifiActionHeader actionHdr;
			WifiActionHeader::ActionValue action;
			action.meshAction = WifiActionHeader::PATH_SELECTION;
			actionHdr.SetAction(WifiActionHeader::MESH, action);
			return actionHdr;
		}
		void
			PmtmgmpProtocolMac::SendPreq(IePreq preq)
		{
			NS_LOG_FUNCTION_NOARGS();
			std::vector<IePreq> preq_vector;
			preq_vector.push_back(preq);
			SendPreq(preq_vector);
		}
		void
			PmtmgmpProtocolMac::SendPreq(std::vector<IePreq> preq)
		{
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			for (std::vector<IePreq>::iterator i = preq.begin(); i != preq.end(); i++)
			{
				elements.AddInformationElement(Ptr<IePreq>(&(*i)));
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
			std::vector<Mac48Address> receivers = m_protocol->GetPreqReceivers(m_ifIndex);
			for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != receivers.end(); i++)
			{
				hdr.SetAddr1(*i);
				m_stats.txPreq++;
				m_stats.txMgt++;
				m_stats.txMgtBytes += packet->GetSize();
				m_parent->SendManagementFrame(packet, hdr);
			}
		}
		void
			PmtmgmpProtocolMac::RequestDestination(Mac48Address dst, uint32_t originator_seqno, uint32_t dst_seqno)
		{
			NS_LOG_FUNCTION_NOARGS();
			for (std::vector<IePreq>::iterator i = m_myPreq.begin(); i != m_myPreq.end(); i++)
			{
				if (i->IsFull())
				{
					continue;
				}
				NS_ASSERT(i->GetDestCount() > 0);
				i->AddDestinationAddressElement(m_protocol->GetDoFlag(), m_protocol->GetRfFlag(), dst, dst_seqno);
			}
			IePreq preq;
			preq.SetHopcount(0);
			preq.SetTTL(m_protocol->GetMaxTtl());
			preq.SetPreqID(m_protocol->GetNextPreqId());
			preq.SetOriginatorAddress(m_protocol->GetAddress());
			preq.SetOriginatorSeqNumber(originator_seqno);
			preq.SetLifetime(m_protocol->GetActivePathLifetime());
			preq.AddDestinationAddressElement(m_protocol->GetDoFlag(), m_protocol->GetRfFlag(), dst, dst_seqno);
			m_myPreq.push_back(preq);
			SendMyPreq();
		}
		void
			PmtmgmpProtocolMac::SendMyPreq()
		{
			NS_LOG_FUNCTION_NOARGS();
			if (m_preqTimer.IsRunning())
			{
				return;
			}
			if (m_myPreq.size() == 0)
			{
				return;
			}
			//reschedule sending PREQ
			NS_ASSERT(!m_preqTimer.IsRunning());
			m_preqTimer = Simulator::Schedule(m_protocol->GetPreqMinInterval(), &PmtmgmpProtocolMac::SendMyPreq, this);
			SendPreq(m_myPreq);
			m_myPreq.clear();
		}
		void
			PmtmgmpProtocolMac::SendPrep(IePrep prep, Mac48Address receiver)
		{
			NS_LOG_FUNCTION_NOARGS();
			//Create packet
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			elements.AddInformationElement(Ptr<IePrep>(&prep));
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr1(receiver);
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
			m_stats.txPrep++;
			m_stats.txMgt++;
			m_stats.txMgtBytes += packet->GetSize();
			m_parent->SendManagementFrame(packet, hdr);
		}
		void
			PmtmgmpProtocolMac::ForwardPerr(std::vector<PmtmgmpProtocol::FailedDestination> failedDestinations, std::vector <
			Mac48Address > receivers)
		{
			NS_LOG_FUNCTION_NOARGS();
			Ptr<Packet> packet = Create<Packet>();
			Ptr<IePerr> perr = Create <IePerr>();
			WmnInformationElementVector elements;
			for (std::vector<PmtmgmpProtocol::FailedDestination>::const_iterator i = failedDestinations.begin(); i
				!= failedDestinations.end(); i++)
			{
				if (!perr->IsFull())
				{
					perr->AddAddressUnit(*i);
				}
				else
				{
					elements.AddInformationElement(perr);
					perr->ResetPerr();
				}
			}
			if (perr->GetNumOfDest() > 0)
			{
				elements.AddInformationElement(perr);
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			if (receivers.size() >= m_protocol->GetUnicastPerrThreshold())
			{
				receivers.clear();
				receivers.push_back(Mac48Address::GetBroadcast());
			}
			//Send Management frame
			for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != receivers.end(); i++)
			{
				//
				// 64-bit Intel valgrind complains about hdr.SetAddr1 (*i).  It likes this
				// just fine.
				//
				Mac48Address address = *i;
				hdr.SetAddr1(address);
				m_stats.txPerr++;
				m_stats.txMgt++;
				m_stats.txMgtBytes += packet->GetSize();
				m_parent->SendManagementFrame(packet, hdr);
			}
		}
		void
			PmtmgmpProtocolMac::InitiatePerr(std::vector<PmtmgmpProtocol::FailedDestination> failedDestinations, std::vector <
			Mac48Address > receivers)
		{
			//All duplicates in PERR are checked here, and there is no reason to
			//check it at any athoer place
			{
				std::vector<Mac48Address>::const_iterator end = receivers.end();
				for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != end; i++)
				{
					bool should_add = true;
					for (std::vector<Mac48Address>::const_iterator j = m_myPerr.receivers.begin(); j
						!= m_myPerr.receivers.end(); j++)
					{
						if ((*i) == (*j))
						{
							should_add = false;
						}
					}
					if (should_add)
					{
						m_myPerr.receivers.push_back(*i);
					}
				}
			}
  {
	  std::vector<PmtmgmpProtocol::FailedDestination>::const_iterator end = failedDestinations.end();
	  for (std::vector<PmtmgmpProtocol::FailedDestination>::const_iterator i = failedDestinations.begin(); i != end; i++)
	  {
		  bool should_add = true;
		  for (std::vector<PmtmgmpProtocol::FailedDestination>::const_iterator j = m_myPerr.destinations.begin(); j
			  != m_myPerr.destinations.end(); j++)
		  {
			  if (((*i).destination == (*j).destination) && ((*j).seqnum > (*i).seqnum))
			  {
				  should_add = false;
			  }
		  }
		  if (should_add)
		  {
			  m_myPerr.destinations.push_back(*i);
		  }
	  }
  }
  SendMyPerr();
		}
		void
			PmtmgmpProtocolMac::SendMyPerr()
		{
			NS_LOG_FUNCTION_NOARGS();
			if (m_perrTimer.IsRunning())
			{
				return;
			}
			m_perrTimer = Simulator::Schedule(m_protocol->GetPerrMinInterval(), &PmtmgmpProtocolMac::SendMyPerr, this);
			ForwardPerr(m_myPerr.destinations, m_myPerr.receivers);
			m_myPerr.destinations.clear();
			m_myPerr.receivers.clear();
		}
		uint32_t
			PmtmgmpProtocolMac::GetLinkMetric(Mac48Address peerAddress) const
		{
			return m_parent->GetLinkMetric(peerAddress);
		}
		uint16_t
			PmtmgmpProtocolMac::GetChannelId() const
		{
			return m_parent->GetFrequencyChannel();
		}
		PmtmgmpProtocolMac::Statistics::Statistics() :
			txPreq(0), rxPreq(0), txPrep(0), rxPrep(0), txPerr(0), rxPerr(0)
#ifndef PMTMGMP_UNUSED_MY_CODE
			, txSecreq(0), rxSecreq(0), txSecrep(0), rxSecrep(0), txSecack(0), rxSecack(0), txPger(0), rxPger(0), txPgen(0), rxPgen(0)
#endif
			, txMgt(0), txMgtBytes(0), rxMgt(0), rxMgtBytes(0), txData(0), txDataBytes(0), rxData(0), rxDataBytes(0)

		{
		}
		void
			PmtmgmpProtocolMac::Statistics::Print(std::ostream & os) const
		{
			os << "<Statistics "
				"txPreq= \"" << txPreq << "\"" << std::endl <<
				"txPrep=\"" << txPrep << "\"" << std::endl <<
				"txPerr=\"" << txPerr << "\"" << std::endl <<
#ifndef PMTMGMP_UNUSED_MY_CODE
				"txSecreq=\"" << txSecreq << "\"" << std::endl <<
				"txSecrep=\"" << txSecrep << "\"" << std::endl <<
				"txSecack=\"" << txSecack << "\"" << std::endl <<
				"txPger=\"" << txPger << "\"" << std::endl <<
				"txPgen=\"" << txPgen << "\"" << std::endl <<
#endif
				"rxPreq=\"" << rxPreq << "\"" << std::endl <<
				"rxPrep=\"" << rxPrep << "\"" << std::endl <<
				"rxPerr=\"" << rxPerr << "\"" << std::endl <<
#ifndef PMTMGMP_UNUSED_MY_CODE
				"rxSecreq=\"" << rxSecreq << "\"" << std::endl <<
				"rxSecrep=\"" << rxSecrep << "\"" << std::endl <<
				"rxSecack=\"" << rxSecack << "\"" << std::endl <<
				"rxPger=\"" << rxPger << "\"" << std::endl <<
				"rxPgen=\"" << rxPgen << "\"" << std::endl <<
#endif
				"txMgt=\"" << txMgt << "\"" << std::endl <<
				"txMgtBytes=\"" << txMgtBytes << "\"" << std::endl <<
				"rxMgt=\"" << rxMgt << "\"" << std::endl <<
				"rxMgtBytes=\"" << rxMgtBytes << "\"" << std::endl <<
				"txData=\"" << txData << "\"" << std::endl <<
				"txDataBytes=\"" << txDataBytes << "\"" << std::endl <<
				"rxData=\"" << rxData << "\"" << std::endl <<
				"rxDataBytes=\"" << rxDataBytes << "\"/>" << std::endl;
		}
		void
			PmtmgmpProtocolMac::Report(std::ostream & os) const
		{
			os << "<PmtmgmpProtocolMac" << std::endl <<
				"address =\"" << m_parent->GetAddress() << "\">" << std::endl;
			m_stats.Print(os);
			os << "</PmtmgmpProtocolMac>" << std::endl;
		}
		void
			PmtmgmpProtocolMac::ResetStats()
		{
			m_stats = Statistics();
		}

		int64_t
			PmtmgmpProtocolMac::AssignStreams(int64_t stream)
		{
			return m_protocol->AssignStreams(stream);
		}

#ifndef PMTMGMP_UNUSED_MY_CODE
		////发送SECREQ
		void PmtmgmpProtocolMac::SendSecreq(IeSecreq secreq)
		{
			NS_LOG_FUNCTION_NOARGS();
			std::vector<IeSecreq> secreq_vector;
			secreq_vector.push_back(secreq);
			SendSecreq(secreq_vector);
		}
		void PmtmgmpProtocolMac::SendSecreq(std::vector<IeSecreq> secreq)
		{
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			for (std::vector<IeSecreq>::iterator i = secreq.begin(); i != secreq.end(); i++)
			{
				elements.AddInformationElement(Ptr<IeSecreq>(&(*i)));
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
#ifdef PMTMGMP_USE_MULTICAST
			////注释为多播，使用为广播
			std::vector<Mac48Address> receivers = m_protocol->GetBroadcastReceivers(m_ifIndex);
			for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != receivers.end(); i++)
			{
				hdr.SetAddr1(*i);
				m_stats.txSecreq++;
				m_stats.txMgt++;
				m_stats.txMgtBytes += packet->GetSize();
				m_parent->SendManagementFrame(packet, hdr);
			}
#else
			hdr.SetAddr1(Mac48Address::GetBroadcast());
			m_stats.txSecreq++;
			m_stats.txMgt++;
			m_stats.txMgtBytes += packet->GetSize();
			m_parent->SendManagementFrame(packet, hdr);
#endif
		}
		////发送SECREP
		void PmtmgmpProtocolMac::SendSecrep(IeSecrep secrep, Mac48Address receiver)
		{
			NS_LOG_FUNCTION_NOARGS();
			std::vector<IeSecrep> secrep_vector;
			secrep_vector.push_back(secrep);
			SendSecrep(secrep_vector, receiver);
		}
		void PmtmgmpProtocolMac::SendSecrep(std::vector<IeSecrep> secrep, Mac48Address receiver)
		{
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			for (std::vector<IeSecrep>::iterator i = secrep.begin(); i != secrep.end(); i++)
			{
				elements.AddInformationElement(Ptr<IeSecrep>(&(*i)));
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
			hdr.SetAddr1(receiver);
			m_stats.txSecrep++;
			m_stats.txMgt++;
			m_stats.txMgtBytes += packet->GetSize();
			m_parent->SendManagementFrame(packet, hdr);
		}
		////发送SECACK
		void PmtmgmpProtocolMac::SendSecack(IeSecack secack, Mac48Address receiver)
		{
			NS_LOG_FUNCTION_NOARGS();
			std::vector<IeSecack> secack_vector;
			secack_vector.push_back(secack);
			SendSecack(secack_vector, receiver);
		}
		void PmtmgmpProtocolMac::SendSecack(std::vector<IeSecack> secack, Mac48Address receiver)
		{
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			for (std::vector<IeSecack>::iterator i = secack.begin(); i != secack.end(); i++)
			{
				elements.AddInformationElement(Ptr<IeSecack>(&(*i)));
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
			hdr.SetAddr1(receiver);
			m_stats.txSecack++;
			m_stats.txMgt++;
			m_stats.txMgtBytes += packet->GetSize();
			m_parent->SendManagementFrame(packet, hdr);
		}
		////发送PGER
		void PmtmgmpProtocolMac::SendPger(IePger pger, Mac48Address receiver)
		{
			NS_LOG_FUNCTION_NOARGS();
			std::vector<IePger> pger_vector;
			pger_vector.push_back(pger);
			SendPger(pger_vector, receiver);
		}
		void PmtmgmpProtocolMac::SendPger(std::vector<IePger> pger, Mac48Address receiver)
		{
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			for (std::vector<IePger>::iterator i = pger.begin(); i != pger.end(); i++)
			{
				elements.AddInformationElement(Ptr<IePger>(&(*i)));
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
			hdr.SetAddr1(receiver);
			m_stats.txPger++;
			m_stats.txMgt++;
			m_stats.txMgtBytes += packet->GetSize();
			m_parent->SendManagementFrame(packet, hdr);
		}
		////发送PGEN
		void PmtmgmpProtocolMac::SendPgen(IePgen pgen)
		{
			NS_LOG_FUNCTION_NOARGS();
			std::vector<IePgen> pgen_vector;
			pgen_vector.push_back(pgen);
			SendPgen(pgen_vector);
		}
		void PmtmgmpProtocolMac::SendPgen(std::vector<IePgen> pgen)
		{
			Ptr<Packet> packet = Create<Packet>();
			WmnInformationElementVector elements;
			for (std::vector<IePgen>::iterator i = pgen.begin(); i != pgen.end(); i++)
			{
				elements.AddInformationElement(Ptr<IePgen>(&(*i)));
			}
			packet->AddHeader(elements);
			packet->AddHeader(GetWifiActionHeader());
			//create 802.11 header:
			WifiMacHeader hdr;
			hdr.SetAction();
			hdr.SetDsNotFrom();
			hdr.SetDsNotTo();
			hdr.SetAddr2(m_parent->GetAddress());
			hdr.SetAddr3(m_protocol->GetAddress());
			//Send Management frame
#ifdef PMTMGMP_USE_MULTICAST
			////注释为多播，使用为广播
			std::vector<Mac48Address> receivers = m_protocol->GetBroadcastReceivers(m_ifIndex);
			for (std::vector<Mac48Address>::const_iterator i = receivers.begin(); i != receivers.end(); i++)
			{
				hdr.SetAddr1(*i);
				m_stats.txPgen++;
				m_stats.txMgt++;
				m_stats.txMgtBytes += packet->GetSize();
				m_parent->SendManagementFrame(packet, hdr);
			}
#else
			hdr.SetAddr1(Mac48Address::GetBroadcast());
			m_stats.txPgen++;
			m_stats.txMgt++;
			m_stats.txMgtBytes += packet->GetSize();
			m_parent->SendManagementFrame(packet, hdr);
#endif
		}
#endif
	} // namespace my11s
} // namespace ns3
