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
#ifndef PMTMGMP_WIFI_PUPD_INFORMATION_ELEMENT_H
#define PMTMGMP_WIFI_PUPD_INFORMATION_ELEMENT_H

#include "ns3/mac48-address.h"
#include "ns3/wmn-information-element-vector.h"
#include "ns3/pmtmgmp-rtable.h"

namespace ns3 {
	namespace my11s {

		/*************************
		* AALMupdatePathData
		************************/
		class AALMupdatePathData
		{
		public:
			AALMupdatePathData(Buffer::Iterator i);
			AALMupdatePathData(Ptr<PmtmgmpRoutePath> path);
			~AALMupdatePathData();

			void ToBuffer(Buffer::Iterator i) const;

		private:
			uint8_t m_MSECPindex;
			uint32_t m_metric;
		};
		/*************************
		* AALMupdateTreeData
		************************/
		class AALMupdateTreeData
		{
		public:
			AALMupdateTreeData(Buffer::Iterator i);
			AALMupdateTreeData(Ptr<PmtmgmpRouteTree> tree);
			~AALMupdateTreeData();

			void ToBuffer(Buffer::Iterator i) const;
			////获取此RouteTree数据的长度
			uint8_t GetSize() const;

		private:
			Mac48Address m_MTERP;
			std::vector<AALMupdatePathData> m_tree;
		};
		/*************************
		* IePupd
		************************/
		class IePupd : public WifiInformationElement
		{
		public:
			IePupd();
			IePupd(Ptr<PmtmgmpRouteTable> table);
			~IePupd();

			// Setters for fields:
			void SetSize(uint8_t seq_number);

			// Getters for fields:
			uint8_t GetSize() const;
			std::vector<AALMupdateTreeData> GetData() const;

			// Inherited from WifiInformationElement
			virtual WifiInformationElementId ElementId() const;
			virtual void SerializeInformationField(Buffer::Iterator i) const;
			virtual uint8_t DeserializeInformationField(Buffer::Iterator i, uint8_t length);
			virtual uint8_t GetInformationFieldSize() const;
			virtual void Print(std::ostream& os) const;

		private:
			uint8_t m_size;
			std::vector<AALMupdateTreeData> m_table;
			
			friend bool operator== (const IePupd & a, const IePupd & b);

		};
		bool operator== (const IePupd & a, const IePupd & b);
		std::ostream &operator << (std::ostream &os, const IePupd &secreq);
	}
}
#endif
#endif