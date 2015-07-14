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
#include "ie-my11s-pger.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"


namespace ns3 {
	namespace my11s {
		IePger::~IePger()
		{
		}
		IePger::IePger()
		{

		}
		void IePger::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathGenerationSeqNumber = seq_number;
		}
		uint32_t IePger::GetPathGenerationSequenceNumber() const
		{
			return m_PathGenerationSeqNumber;
		}
		WifiInformationElementId IePger::ElementId() const
		{
			return IE11S_PGER;
		}
		void IePger::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " path generation sequence number  = " << m_PathGenerationSeqNumber << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IePger::SerializeInformationField(Buffer::Iterator i) const
		{
			i.WriteHtolsbU32(m_PathGenerationSeqNumber);
		}
		uint8_t IePger::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			m_PathGenerationSeqNumber = i.ReadLsbtohU32();
			return i.GetDistanceFrom(start);
		}
		uint8_t IePger::GetInformationFieldSize() const
		{
			uint8_t retval = 4;			//Path Generation Sequence Number
			return retval;
		}
		bool operator== (const IePger & a, const IePger & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IePger &secreq)
		{
			secreq.Print(os);
			return os;
		}
	}
}
#endif