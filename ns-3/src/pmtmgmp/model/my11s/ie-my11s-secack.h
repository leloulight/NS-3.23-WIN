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
#ifndef PMTMGMP_WIFI_SECACK_INFORMATION_ELEMENT_H
#define PMTMGMP_WIFI_SECACK_INFORMATION_ELEMENT_H

#include "ns3/mac48-address.h"
#include "ns3/wmn-information-element-vector.h"
#include "ns3/pmtmgmp-protocol.h"

namespace ns3 {
	namespace my11s {
		class IeSecack : public WifiInformationElement
		{
		public:
			IeSecack();
			~IeSecack();

			// Setters for fields:
			void SetMTERPaddress(Mac48Address originator_address);
			void SetPathGenerationSequenceNumber(uint32_t seq_number);
			void SetPathUpdateSeqNumber(uint32_t seq_number);
			void SetNodeType(PmtmgmpProtocol::NodeType nodeType);
			void SetMSECPindex(uint8_t index);

			// Getters for fields:
			Mac48Address GetMTERPaddress() const;
			uint32_t GetPathGenerationSequenceNumber() const;
			uint32_t GetPathUpdateSeqNumber() const;
			PmtmgmpProtocol::NodeType GetNodeType() const;
			uint8_t GetMSECPindex() const;

			// Inherited from WifiInformationElement
			virtual WifiInformationElementId ElementId() const;
			virtual void SerializeInformationField(Buffer::Iterator i) const;
			virtual uint8_t DeserializeInformationField(Buffer::Iterator i, uint8_t length);
			virtual uint8_t GetInformationFieldSize() const;
			virtual void Print(std::ostream& os) const;
		private:
			Mac48Address m_MTERPaddress;
			uint32_t m_PathGenerationSeqNumber;
			uint32_t m_PathUpdateSeqNumber;
			PmtmgmpProtocol::NodeType m_NodeType;
			uint8_t m_MSECPindex;

			friend bool operator== (const IeSecack & a, const IeSecack & b);

		};
		bool operator== (const IeSecack & a, const IeSecack & b);
		std::ostream &operator << (std::ostream &os, const IeSecack &secack);
	}
}
#endif
#endif