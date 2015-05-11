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
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */


#ifndef WMN_STACK_INSTALLER_H
#define WMN_STACK_INSTALLER_H
#include "ns3/wmn-point-device.h"
namespace ns3 {
/**
 * \ingroup wmn
 *
 * \brief Prototype for class, which helps to install MAC-layer
 * routing stack to ns3::WmnPointDevice
 *
 * You need to create a WmnPointDevice and attach all
 * interfaces to it, than call Install method
 */
class WmnStack : public Object
{
public:
  ///\brief Installs wmn stack. needed by helper only
  virtual bool InstallStack (Ptr<WmnPointDevice> mp) = 0;
  /// Report statistics of a given wmn point
  virtual void Report (const Ptr<WmnPointDevice> mp, std::ostream&) = 0;
  /// Reset statistics of a given wmn point
  virtual void ResetStats (const Ptr<WmnPointDevice> mp) = 0;
};
}
#endif

