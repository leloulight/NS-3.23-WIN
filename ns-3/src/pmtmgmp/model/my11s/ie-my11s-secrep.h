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
#ifndef PMTMGMP_WIFI_SECREP_INFORMATION_ELEMENT_H
#define PMTMGMP_WIFI_SECREP_INFORMATION_ELEMENT_H

#include "ns3/mac48-address.h"
#include "ns3/wmn-information-element-vector.h"

namespace ns3 {
	namespace my11s {
		class IeSecrep : public WifiInformationElement
		{
		public:
			IeSecrep();
			~IeSecrep();

			// Setters for fields:
			void SetOriginatorAddress(Mac48Address originator_address);
			void SetCandidateMSECPaddress(Mac48Address candidateMSECP_address);
			void SetAffiliatedMTERPnum(uint8_t mTERP_number);
			void SetPathGenerationSequenceNumber(uint32_t seq_number);

			// Getters for fields:
			Mac48Address GetOriginatorAddress() const;
			Mac48Address GetCandidateMSECPaddress() const;
			uint8_t GetAffiliatedMTERPnum() const;
			uint32_t GetPathGenerationSequenceNumber() const;

			// Inherited from WifiInformationElement
			virtual WifiInformationElementId ElementId() const;
			virtual void SerializeInformationField(Buffer::Iterator i) const;
			virtual uint8_t DeserializeInformationField(Buffer::Iterator i, uint8_t length);
			virtual uint8_t GetInformationFieldSize() const;
			virtual void Print(std::ostream& os) const;
		private:
			Mac48Address m_originatorAddress;
			Mac48Address m_candidateMSECPaddress;
			uint8_t m_affiliatedMTERPnum;
			uint32_t m_PathGenerationSeqNumber;

			friend bool operator== (const IeSecrep & a, const IeSecrep & b);

		};
		bool operator== (const IeSecrep & a, const IeSecrep & b);
		std::ostream &operator << (std::ostream &os, const IeSecrep &secreq);
	}
}
#endif
#endif