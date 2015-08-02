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
#include "ie-my11s-pupgq.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/packet.h"


namespace ns3 {
	namespace my11s {
		/*************************
		* PUPGQdata
		************************/
		PUPGQdata::PUPGQdata(Buffer::Iterator i)
		{
			ReadFrom(i, m_MTERPaddress);
			m_MSECPindex = i.ReadU8();
		}
		PUPGQdata::PUPGQdata(Mac48Address mterp, uint8_t index)
		{
			m_MTERPaddress = mterp;
			m_MSECPindex = index;
		}
		PUPGQdata::~PUPGQdata()
		{
		}
		void PUPGQdata::SetMTERPaddress(Mac48Address address)
		{
			m_MTERPaddress = address;
		}
		void PUPGQdata::SetMSECPindex(uint8_t index)
		{
			m_MSECPindex = index;
		}
		Mac48Address PUPGQdata::GetMTERPaddress() const
		{
			return m_MTERPaddress;
		}
		uint8_t PUPGQdata::GetMSECPindex() const
		{
			return m_MSECPindex;
		}
		void PUPGQdata::ToBuffer(Buffer::Iterator i) const
		{
			WriteTo(i, m_MTERPaddress);
			i.WriteU8(m_MSECPindex);
		}
		/*************************
		* IePupgq
		************************/
		IePupgq::IePupgq()
		{
		}
		IePupgq::IePupgq(std::vector<PUPGQdata> list)
		{
			m_PathList = list;
		}
		IePupgq::~IePupgq()
		{
		}
		std::vector<PUPGQdata> IePupgq::GetPathList() const
		{
			return m_PathList;
		}
		WifiInformationElementId IePupgq::ElementId() const
		{
			return IE11S_PUPGQ;
		}
		void IePupgq::Print(std::ostream &os) const
		{
			os << std::endl << "<information_element id=" << ElementId() << ">" << std::endl;
			os << " Route Path size  = " << m_PathList.size() << std::endl;
			os << "</information_element>" << std::endl;
		}
		void IePupgq::SerializeInformationField(Buffer::Iterator i) const
		{
			i.WriteU8(m_PathList.size());
			for (std::vector<PUPGQdata>::const_iterator iter = m_PathList.begin(); iter != m_PathList.end(); iter++)
			{
				iter->ToBuffer(i);
			}
		}
		uint8_t IePupgq::DeserializeInformationField(Buffer::Iterator start, uint8_t length)
		{
			Buffer::Iterator i = start;
			uint8_t j = i.ReadU8();
			for (; j > 0; j--)
			{
				m_PathList.push_back(PUPGQdata(i));
			}
			return i.GetDistanceFrom(start);
		}
		uint8_t IePupgq::GetInformationFieldSize() const
		{
			uint8_t retval = 1 			//Size
				+ m_PathList.size() * 7;
			return retval;
		}
		bool operator== (const IePupgq & a, const IePupgq & b)
		{
			return true;
		}
		std::ostream &
			operator << (std::ostream &os, const IePupgq &secreq)
		{
			secreq.Print(os);
			return os;
		}
	}
}
#endif