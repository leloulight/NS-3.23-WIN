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
#include "ie-my11s-secrep.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"


namespace ns3 {
	namespace my11s {
		IeSecrep::~IeSecrep()
		{
		}
		IeSecrep::IeSecrep()
		{

		}
		void IeSecrep::SetMTERPaddress(Mac48Address originator_address)
		{
			m_MTERPaddress = originator_address;
		}
		void IeSecrep::SetCandidateMSECPaddress(Mac48Address candidateMSECP_address)
		{
			m_candidateMSECPaddress = candidateMSECP_address;
		}
		void IeSecrep::SetMSECPcount(uint8_t mTERP_number)
		{
			m_MSECPcount = mTERP_number;
		}
		void IeSecrep::SetPathGenerationSequenceNumber(uint32_t seq_number)
		{
			m_PathGenerationSeqNumber = seq_number;
		}
		Mac48Address IeSecrep::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		Mac48Address IeSecrep::GetCandidateMSECPaddress() const
		{
			return m_candidateMSECPaddress;
		}
		uint8_t IeSecrep::GetMSECPcount() const
		{
			return m_MSECPcount;
		}
		uint32_t IeSecrep::GetPathGenerationSequenceNumber() const
		{
			return m_PathGenerationSeqNumber;
		}
		WifiInformationElementId IeSecrep::ElementId() const
		{
			return IE11S_SECREP;
		}
		void IeSecrep::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " originator address               = " << m_MTERPaddress << std::endl;
			os << " candidate MSECP address          = " << m_candidateMSECPaddress << std::endl;
			os << " belong MTERP number              = " << m_MSECPcount << std::endl; 
			os << " path generation sequence number  = " << m_PathGenerationSeqNumber << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IeSecrep::SerializeInformationField(Buffer::Iterator i) const
		{
			WriteTo(i, m_MTERPaddress);
			WriteTo(i, m_candidateMSECPaddress);
			i.WriteU8(m_MSECPcount);
			i.WriteHtolsbU32(m_PathGenerationSeqNumber);
		}
		uint8_t IeSecrep::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			ReadFrom(i, m_MTERPaddress);
			ReadFrom(i, m_candidateMSECPaddress);
			m_MSECPcount = i.ReadU8();
			m_PathGenerationSeqNumber = i.ReadLsbtohU32();
			return i.GetDistanceFrom(start);
		}
		uint8_t IeSecrep::GetInformationFieldSize() const
		{
			uint8_t retval = 6	//Source address (originator)
				+ 6				//Candidate MSECP address
				+ 1				//Affiliated MTERP number
				+ 4;			//Path Generation Sequence Number
			return retval;
		}
		bool operator== (const IeSecrep & a, const IeSecrep & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IeSecrep &secreq)
		{
			secreq.Print(os);
			return os;
		}
	}
}
#endif