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
#ifndef PMTMGMP_WIFI_PGEN_INFORMATION_ELEMENT_H
#define PMTMGMP_WIFI_PGEN_INFORMATION_ELEMENT_H

#include "ns3/mac48-address.h"
#include "ns3/wmn-information-element-vector.h"
#include "ns3/pmtmgmp-protocol.h"

namespace ns3 {
	namespace my11s {
		class IePgen : public WifiInformationElement
		{
		public:
			IePgen();
			~IePgen();

			////附带路径相关函数
			void AddPartPathNode(Mac48Address address, uint8_t pathLength);
			std::vector<Mac48Address> GetPartPathNodeList();
			
			// Setters for fields:
			void SetOriginatorAddress(Mac48Address originator_address);
			void SetMSECPaddress(Mac48Address mSECP_address);
			void SetPathGenerationSequenceNumber(uint32_t seq_number);
			void SetPathUpdateSeqNumber(uint32_t seq_number);
			void SetNodeType(PmtmgmpProtocol::NodeType nodeType);
			void SetHopcount(uint8_t hopcount);
			void SetTTL(uint8_t ttl);
			void SetMetric(uint32_t metric);

			// Getters for fields:
			Mac48Address GetOriginatorAddress() const;
			Mac48Address GetMSECPaddress() const;
			uint32_t GetPathGenerationSequenceNumber() const;
			uint32_t GetPathUpdateSeqNumber() const;
			PmtmgmpProtocol::NodeType GetNodeType() const;
			uint8_t  GetHopCount() const;
			uint8_t  GetTtl() const;
			uint32_t GetMetric() const;

			/// Handle TTL and Metric:
			void  DecrementTtl();
			void  IncrementMetric(uint32_t metric, double m, uint8_t q);

			// Inherited from WifiInformationElement
			virtual WifiInformationElementId ElementId() const;
			virtual void SerializeInformationField(Buffer::Iterator i) const;
			virtual uint8_t DeserializeInformationField(Buffer::Iterator i, uint8_t length);
			virtual uint8_t GetInformationFieldSize() const;
			virtual void Print(std::ostream& os) const;
		private:
			Mac48Address m_originatorAddress;
			Mac48Address m_MSECPaddress;
			uint32_t m_PathGenerationSeqNumber;
			uint32_t m_PathUpdateSeqNumber;
			PmtmgmpProtocol::NodeType m_NodeType;
			uint8_t  m_hopCount;
			uint8_t  m_ttl;
			uint32_t m_metric;
			std::vector<Mac48Address>  m_partPath;

			friend bool operator== (const IePgen & a, const IePgen & b);
		};
		bool operator== (const IePgen & a, const IePgen & b);
		std::ostream &operator << (std::ostream &os, const IePgen &pgen);
	}
}
#endif
#endif