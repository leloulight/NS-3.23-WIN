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
#include "ie-my11s-secreq.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"


namespace ns3 {
	namespace my11s {
		IeSecreq::~IeSecreq()
		{
		}
		IeSecreq::IeSecreq()
		{

		}
		void IeSecreq::SetOriginatorAddress(Mac48Address originator_address)
		{
			m_originatorAddress = originator_address;
		}
		void IeSecreq::SetMSECPSelectIndex(uint32_t index)
		{
			m_MSECPSelectIndex = index;
		}
		Mac48Address IeSecreq::GetOriginatorAddress() const
		{
			return m_originatorAddress;
		}
		uint32_t IeSecreq::GetMSECPSelectIndex() const
		{
			return m_MSECPSelectIndex;
		}
		WifiInformationElementId IeSecreq::ElementId() const
		{
			return IE11S_SECREQ;
		}
		void IeSecreq::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " originator address  = " << m_originatorAddress << std::endl;
			os << " MSECP Select Index  = " << m_originatorAddress << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IeSecreq::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_originatorAddress);
			i.WriteHtolsbU32(m_MSECPSelectIndex);
		}
		uint8_t IeSecreq::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			ReadFrom(i, m_originatorAddress);
			m_MSECPSelectIndex = i.ReadLsbtohU32();
			return i.GetDistanceFrom(start);
		}
		uint8_t IeSecreq::GetInformationFieldSize() const
		{
			uint8_t retval = 6	//Source address (originator)
				+ 4;			//MSECP Select Index
			return retval;
		}
		bool operator== (const IeSecreq & a, const IeSecreq & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IeSecreq &secreq)
		{
			secreq.Print(os);
			return os;
		}
	}
}
#endif