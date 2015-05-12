/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Author: Pavel Boyko <boyko@iitp.ru>
 */
#include "ns3/test.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/mgt-headers.h"
#include "ns3/my11s-mac-header.h"
#include "ns3/pmtmgmp-rtable.h"
#include "ns3/pmtmgmp-peer-link-frame.h"
#include "ns3/ie-my11s-peer-management.h"

using namespace ns3;
using namespace my11s;

/// Built-in self test for FlameHeader
struct WmnHeaderTest : public TestCase
{
  WmnHeaderTest () :
    TestCase ("My11sWmnHeader roundtrip serialization")
  {
  }
  void DoRun ();
};

void
WmnHeaderTest::DoRun ()
{
  {
    WmnHeader a;
    a.SetAddressExt (3);
    a.SetAddr4 (Mac48Address ("11:22:33:44:55:66"));
    a.SetAddr5 (Mac48Address ("11:00:33:00:55:00"));
    a.SetAddr6 (Mac48Address ("00:22:00:44:00:66"));
    a.SetWmnTtl (122);
    a.SetWmnSeqno (321);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (a);
    WmnHeader b;
    packet->RemoveHeader (b);
    NS_TEST_ASSERT_MSG_EQ (a, b, "Wmn header roundtrip serialization works, 3 addresses");
  }
  {
    WmnHeader a;
    a.SetAddressExt (2);
    a.SetAddr5 (Mac48Address ("11:00:33:00:55:00"));
    a.SetAddr6 (Mac48Address ("00:22:00:44:00:66"));
    a.SetWmnTtl (122);
    a.SetWmnSeqno (321);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (a);
    WmnHeader b;
    packet->RemoveHeader (b);
    NS_TEST_ASSERT_MSG_EQ (a, b, "Wmn header roundtrip serialization works, 2 addresses");
  }
  {
    WmnHeader a;
    a.SetAddressExt (1);
    a.SetAddr4 (Mac48Address ("11:22:33:44:55:66"));
    a.SetWmnTtl (122);
    a.SetWmnSeqno (321);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (a);
    WmnHeader b;
    packet->RemoveHeader (b);
    NS_TEST_ASSERT_MSG_EQ (a, b, "Wmn header roundtrip serialization works, 1 address");
  }
}
//-----------------------------------------------------------------------------
/// Unit test for PmtmgmpRtable
class PmtmgmpRtableTest : public TestCase
{
public:
  PmtmgmpRtableTest ();
  virtual void DoRun ();

private:
  /// Test Add apth and lookup path;
  void TestLookup ();

  // Test add path and try to lookup after entry has expired
  void TestAddPath ();
  void TestExpire ();

  // Test add precursors and find precursor list in rtable
  void TestPrecursorAdd ();
  void TestPrecursorFind ();

private:
  Mac48Address dst;
  Mac48Address hop;
  uint32_t iface;
  uint32_t metric;
  uint32_t seqnum;
  Time expire;
  Ptr<PmtmgmpRtable> table;
  std::vector<Mac48Address> precursors;
};

PmtmgmpRtableTest::PmtmgmpRtableTest () :
  TestCase ("PMTMGMP routing table"), 
  dst ("01:00:00:01:00:01"), 
  hop ("01:00:00:01:00:03"),
  iface (8010), 
  metric (10), 
  seqnum (1), 
  expire (Seconds (10))
{
  precursors.push_back (Mac48Address ("00:10:20:30:40:50"));
  precursors.push_back (Mac48Address ("00:11:22:33:44:55"));
  precursors.push_back (Mac48Address ("00:01:02:03:04:05"));
}

void
PmtmgmpRtableTest::TestLookup ()
{
  PmtmgmpRtable::LookupResult correct (hop, iface, metric, seqnum);

  // Reactive path
  table->AddReactivePath (dst, hop, iface, metric, expire, seqnum);
  NS_TEST_EXPECT_MSG_EQ ((table->LookupReactive (dst) == correct), true, "Reactive lookup works");
  table->DeleteReactivePath (dst);
  NS_TEST_EXPECT_MSG_EQ (table->LookupReactive (dst).IsValid (), false, "Reactive lookup works");

  // Proactive
  table->AddProactivePath (metric, dst, hop, iface, expire, seqnum);
  NS_TEST_EXPECT_MSG_EQ ((table->LookupProactive () == correct), true, "Proactive lookup works");
  table->DeleteProactivePath (dst);
  NS_TEST_EXPECT_MSG_EQ (table->LookupProactive ().IsValid (), false, "Proactive lookup works");
}

void
PmtmgmpRtableTest::TestAddPath ()
{
  table->AddReactivePath (dst, hop, iface, metric, expire, seqnum);
  table->AddProactivePath (metric, dst, hop, iface, expire, seqnum);
}

void
PmtmgmpRtableTest::TestExpire ()
{
  // this is assumed to be called when path records are already expired
  PmtmgmpRtable::LookupResult correct (hop, iface, metric, seqnum);
  NS_TEST_EXPECT_MSG_EQ ((table->LookupReactiveExpired (dst) == correct), true, "Reactive expiration works");
  NS_TEST_EXPECT_MSG_EQ ((table->LookupProactiveExpired () == correct), true, "Proactive expiration works");

  NS_TEST_EXPECT_MSG_EQ (table->LookupReactive (dst).IsValid (), false, "Reactive expiration works");
  NS_TEST_EXPECT_MSG_EQ (table->LookupProactive ().IsValid (), false, "Proactive expiration works");
}

void
PmtmgmpRtableTest::TestPrecursorAdd ()
{
  for (std::vector<Mac48Address>::const_iterator i = precursors.begin (); i != precursors.end (); i++)
    {
      table->AddPrecursor (dst, iface, *i, Seconds (100));
      // Check that duplicates are filtered
      table->AddPrecursor (dst, iface, *i, Seconds (100));
    }
}

void
PmtmgmpRtableTest::TestPrecursorFind ()
{
  PmtmgmpRtable::PrecursorList precursorList = table->GetPrecursors (dst);
  NS_TEST_EXPECT_MSG_EQ (precursors.size (), precursorList.size (), "Precursors size works");
  for (unsigned i = 0; i < precursors.size (); i++)
    {
      NS_TEST_EXPECT_MSG_EQ (precursorList[i].first, iface, "Precursors lookup works");
      NS_TEST_EXPECT_MSG_EQ (precursorList[i].second, precursors[i], "Precursors lookup works");
    }
}

void
PmtmgmpRtableTest::DoRun ()
{
  table = CreateObject<PmtmgmpRtable> ();

  Simulator::Schedule (Seconds (0), &PmtmgmpRtableTest::TestLookup, this);
  Simulator::Schedule (Seconds (1), &PmtmgmpRtableTest::TestAddPath, this);
  Simulator::Schedule (Seconds (2), &PmtmgmpRtableTest::TestPrecursorAdd, this);
  Simulator::Schedule (expire + Seconds (2), &PmtmgmpRtableTest::TestExpire, this);
  Simulator::Schedule (expire + Seconds (3), &PmtmgmpRtableTest::TestPrecursorFind, this);

  Simulator::Run ();
  Simulator::Destroy ();
}
//-----------------------------------------------------------------------------
/// Built-in self test for PeerLinkFrameStart
struct PeerLinkFrameStartTest : public TestCase
{
  PeerLinkFrameStartTest () :
    TestCase ("PmtmgmpPeerLinkFrames (open, confirm, close) unit tests")
  {
  }
  virtual void DoRun ();
};

void
PeerLinkFrameStartTest::DoRun ()
{
  {
    PeerLinkFrameStart a;
    PeerLinkFrameStart::PlinkFrameStartFields fields;
    fields.subtype = (uint8_t)(WifiActionHeader::PMTMGMP_PEER_LINK_OPEN);
    fields.capability = 0;
    fields.aid = 101;
    fields.reasonCode = 12;
    fields.wmnId = IeWmnId ("qwertyuiop");
    a.SetPlinkFrameStart (fields);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (a);
    PeerLinkFrameStart b;
    b.SetPlinkFrameSubtype ((uint8_t)(WifiActionHeader::PMTMGMP_PEER_LINK_OPEN));
    packet->RemoveHeader (b);
    NS_TEST_EXPECT_MSG_EQ (a, b, "PMTMGMP_PEER_LINK_OPEN works");
  }
  {
    PeerLinkFrameStart a;
    PeerLinkFrameStart::PlinkFrameStartFields fields;
    fields.subtype = (uint8_t)(WifiActionHeader::PMTMGMP_PEER_LINK_CONFIRM);
    fields.capability = 0;
    fields.aid = 1234;
    fields.reasonCode = 12;
    fields.wmnId = IeWmnId ("qwerty");
    a.SetPlinkFrameStart (fields);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (a);
    PeerLinkFrameStart b;
    b.SetPlinkFrameSubtype ((uint8_t)(WifiActionHeader::PMTMGMP_PEER_LINK_CONFIRM));
    packet->RemoveHeader (b);
    NS_TEST_EXPECT_MSG_EQ (a, b, "PMTMGMP_PEER_LINK_CONFIRM works");
  }
  {
    PeerLinkFrameStart a;
    PeerLinkFrameStart::PlinkFrameStartFields fields;
    fields.subtype = (uint8_t)(WifiActionHeader::PMTMGMP_PEER_LINK_CLOSE);
    fields.capability = 0;
    fields.aid = 10;
    fields.wmnId = IeWmnId ("qqq");
    fields.reasonCode = 12;
    a.SetPlinkFrameStart (fields);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (a);
    PeerLinkFrameStart b;
    b.SetPlinkFrameSubtype ((uint8_t)(WifiActionHeader::PMTMGMP_PEER_LINK_CLOSE));
    packet->RemoveHeader (b);
    NS_TEST_EXPECT_MSG_EQ (a, b, "PMTMGMP_PEER_LINK_CLOSE works");
  }
}
//-----------------------------------------------------------------------------
class My11sTestSuite : public TestSuite
{
public:
  My11sTestSuite ();
};

My11sTestSuite::My11sTestSuite ()
  : TestSuite ("devices-wmn-my11s", UNIT)
{
  AddTestCase (new WmnHeaderTest, TestCase::QUICK);
  AddTestCase (new PmtmgmpRtableTest, TestCase::QUICK);
  AddTestCase (new PeerLinkFrameStartTest, TestCase::QUICK);
}

static My11sTestSuite g_my11sTestSuite;
