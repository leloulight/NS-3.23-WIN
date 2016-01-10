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
 * 
 * This is doxygen module description, don't include 
 */
/**
 * \ingroup wmn 
 * \defgroup hu11s IEEE 802.11s draft
 * 
 * \brief IEEE 802.11s (wmn) draft standard implementation
 * 
 * Current model conforms IEEE 802.11s D3.0 draft version and includes
 * Huper Management Protocol and MGMP (routing) Protocol implementations.
 * 
 * The multi-interface wmn points are supported as an 
 * extension of IEEE draft version 3.0. Note that corresponding helper
 * creates single interface station by default.
 * \section Hu11s Overview of IEEE 802.11s
 * Implementation of 802.11s draft standard consists of two main parts:
 * Huper management protocol and MGMP - Hybrid Wireless Wmn Protocol.
 * 
 * The task of huper management protocol is the following:
 *      -open links detecting beacons and starting huper link finite
 *      state machine.
 *      -close huper links due to transmission failures or beacon loss.
 * 
 * If huper link between sender and receiver does not exist, the packet will be dropped.
 * So, the plug-in to huper management protocol is the first in the list of
 * ns3::WmnWifiInterfaceMacPlugin
 * \subsection PMP Huper management protocol
 * Huper management protocol consists of three main parts:
 *      - Protocol itself ns3::hu11s::HuperManagementProtocol, which keeps all active huper links on interfaces,
 *      handles all changes of their states and notifies a routing protocol about link failures.
 *      - MAC plug-in ns3::hu11s::HuperManagementProtocolMac which drops packet, if there is no huper link,
 *      and peek all needed information from management frames and information elements from beacons.
 *      - Huper link ns3::hu11s::HuperLink which keeps finite state machine of each huper link, keeps
 *      beacon loss counter and counter of successive transmission failures.
 *
 * Procedure of closing huper link is not described detailed in 802.11s draft standard, so in our model
 * the link may be closed by:
 *      - beacon loss (see an appropriate attribute of ns3::hu11s::HuperLink class)
 *      - transmission failure -- when a predefined number of successive packets have failed to transmit,
 *      the link will be closed.
 *
 * Also Huper management protocol is responsible for beacon collision avoidance, because it keeps
 * beacon timing elements from all neighbours.
 * Note, that HuperManagementProtocol is not attached to WmnPointDevice as a routing protocol,
 * but the structure is similar: the upper tier of protocol ns3::hu11s::HuperManagementProtocol
 * and its plug-in is  ns3::hu11s::HuperManagementProtocolMac.
 * 
 * \subsection MGMP Hybrid Wireless Wmn Protocol
 * MGMP is implemented in both modes -- reactive and proactive. Also we have implemented an ability
 * to transmit broadcast data and management frames as unicasts (see appropriate attributes).
 * This feature turns off at a station when the number of neighbours of the station is more than a threshold.
 */
 