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
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "ns3/test.h"
#include "pmp-regression.h"
#include "pmtmgmp-reactive-regression.h"
#include "pmtmgmp-proactive-regression.h"
#include "pmtmgmp-simplest-regression.h"
#include "pmtmgmp-target-flags-regression.h"

using namespace ns3;
class My11sRegressionSuite : public TestSuite
{
public:
  My11sRegressionSuite () : TestSuite ("devices-wmn-my11s-regression", SYSTEM) 
  {
    // We do not use NS_TEST_SOURCEDIR variable here since wmn/test has 
    // subdirectories
    SetDataDir (std::string ("src/wmn/test/my11s"));
    AddTestCase (new PeerManagementProtocolRegressionTest, TestCase::QUICK);
    AddTestCase (new PmtmgmpSimplestRegressionTest, TestCase::QUICK);
    AddTestCase (new PmtmgmpReactiveRegressionTest, TestCase::QUICK);
    AddTestCase (new PmtmgmpProactiveRegressionTest, TestCase::QUICK);
    AddTestCase (new PmtmgmpDoRfRegressionTest, TestCase::QUICK);
  }
} g_my11sRegressionSuite;
