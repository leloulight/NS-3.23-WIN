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
 * \defgroup my11s IEEE 802.11s draft
 * 
 * \brief IEEE 802.11s (wmn) draft standard implementation
 * 
 * Current model conforms IEEE 802.11s D3.0 draft version and includes
 * Peer Management Protocol and PMTMGMP (routing) Protocol implementations.
 * 
 * The multi-interface wmn points are supported as an 
 * extension of IEEE draft version 3.0. Note that corresponding helper
 * creates single interface station by default.
 * \section My11s Overview of IEEE 802.11s
 * Implementation of 802.11s draft standard consists of two main parts:
 * Peer management protocol and PMTMGMP - Hybrid Wireless Wmn Protocol.
 * 
 * The task of peer management protocol is the following:
 *      -open links detecting beacons and starting peer link finite
 *      state machine.
 *      -close peer links due to transmission failures or beacon loss.
 * 
 * If peer link between sender and receiver does not exist, the packet will be dropped.
 * So, the plug-in to peer management protocol is the first in the list of
 * ns3::WmnWifiInterfaceMacPlugin
 * \subsection PMP Peer management protocol
 * Peer management protocol consists of three main parts:
 *      - Protocol itself ns3::my11s::PmtmgmpPeerManagementProtocol, which keeps all active peer links on interfaces,
 *      handles all changes of their states and notifies a routing protocol about link failures.
 *      - MAC plug-in ns3::my11s::PmtmgmpPeerManagementProtocolMac which drops packet, if there is no peer link,
 *      and peek all needed information from management frames and information elements from beacons.
 *      - Peer link ns3::my11s::PmtmgmpPeerLink which keeps finite state machine of each peer link, keeps
 *      beacon loss counter and counter of successive transmission failures.
 *
 * Procedure of closing peer link is not described detailed in 802.11s draft standard, so in our model
 * the link may be closed by:
 *      - beacon loss (see an appropriate attribute of ns3::my11s::PmtmgmpPeerLink class)
 *      - transmission failure -- when a predefined number of successive packets have failed to transmit,
 *      the link will be closed.
 *
 * Also Peer management protocol is responsible for beacon collision avoidance, because it keeps
 * beacon timing elements from all neighbours.
 * Note, that PmtmgmpPeerManagementProtocol is not attached to WmnPointDevice as a routing protocol,
 * but the structure is similar: the upper tier of protocol ns3::my11s::PmtmgmpPeerManagementProtocol
 * and its plug-in is  ns3::my11s::PmtmgmpPeerManagementProtocolMac.
 * 
 * \subsection PMTMGMP Hybrid Wireless Wmn Protocol
 * PMTMGMP is implemented in both modes -- reactive and proactive. Also we have implemented an ability
 * to transmit broadcast data and management frames as unicasts (see appropriate attributes).
 * This feature turns off at a station when the number of neighbours of the station is more than a threshold.
 */
