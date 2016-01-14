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

#include "mgmp-protocol.h"
#include "mgmp-protocol-mac.h"
#include "mgmp-tag.h"
#include "mgmp-rtable.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/wmn-point-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wmn-point-device.h"
#include "ns3/wmn-wifi-interface-mac.h"
#include "ns3/random-variable-stream.h"
#include "hu11s-airtime-metric.h"
#include "ie-hu11s-preq.h"
#include "ie-hu11s-prep.h"
#include "ns3/trace-source-accessor.h"
#include "ie-hu11s-perr.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MgmpProtocol");
  
namespace hu11s {

NS_OBJECT_ENSURE_REGISTERED (MgmpProtocol);
  
TypeId
MgmpProtocol::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::hu11s::MgmpProtocol")
    .SetParent<WmnL2RoutingProtocol> ()
    .SetGroupName ("Wmn")
    .AddConstructor<MgmpProtocol> ()
    .AddAttribute ( "RandomStart",
                    "Random delay at first proactive PREQ",
                    TimeValue (Seconds (0.1)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_randomStart),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "MaxQueueSize",
                    "Maximum number of packets we can store when resolving route",
                    UintegerValue (255),
                    MakeUintegerAccessor (
                      &MgmpProtocol::m_maxQueueSize),
                    MakeUintegerChecker<uint16_t> (1)
                    )
    .AddAttribute ( "Dot11WmnMGMPmaxPREQretries",
                    "Maximum number of retries before we suppose the destination to be unreachable",
                    UintegerValue (3),
                    MakeUintegerAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPmaxPREQretries),
                    MakeUintegerChecker<uint8_t> (1)
                    )
    .AddAttribute ( "Dot11WmnMGMPnetDiameterTraversalTime",
                    "Time we suppose the packet to go from one edge of the network to another",
                    TimeValue (MicroSeconds (1024*100)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPnetDiameterTraversalTime),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "Dot11WmnMGMPpreqMinInterval",
                    "Minimal interval between to successive PREQs",
                    TimeValue (MicroSeconds (1024*100)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPpreqMinInterval),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "Dot11WmnMGMPperrMinInterval",
                    "Minimal interval between to successive PREQs",
                    TimeValue (MicroSeconds (1024*100)),
                    MakeTimeAccessor (&MgmpProtocol::m_dot11WmnMGMPperrMinInterval),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "Dot11WmnMGMPactiveRootTimeout",
                    "Lifetime of poractive routing information",
                    TimeValue (MicroSeconds (1024*5000)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPactiveRootTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "Dot11WmnMGMPactivePathTimeout",
                    "Lifetime of reactive routing information",
                    TimeValue (MicroSeconds (1024*5000)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPactivePathTimeout),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "Dot11WmnMGMPpathToRootInterval",
                    "Interval between two successive proactive PREQs",
                    TimeValue (MicroSeconds (1024*2000)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPpathToRootInterval),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "Dot11WmnMGMPrannInterval",
                    "Lifetime of poractive routing information",
                    TimeValue (MicroSeconds (1024*5000)),
                    MakeTimeAccessor (
                      &MgmpProtocol::m_dot11WmnMGMPrannInterval),
                    MakeTimeChecker ()
                    )
    .AddAttribute ( "MaxTtl",
                    "Initial value of Time To Live field",
                    UintegerValue (32),
                    MakeUintegerAccessor (
                      &MgmpProtocol::m_maxTtl),
                    MakeUintegerChecker<uint8_t> (2)
                    )
    .AddAttribute ( "UnicastPerrThreshold",
                    "Maximum number of PERR receivers, when we send a PERR as a chain of unicasts",
                    UintegerValue (32),
                    MakeUintegerAccessor (
                      &MgmpProtocol::m_unicastPerrThreshold),
                    MakeUintegerChecker<uint8_t> (1)
                    )
    .AddAttribute ( "UnicastPreqThreshold",
                    "Maximum number of PREQ receivers, when we send a PREQ as a chain of unicasts",
                    UintegerValue (1),
                    MakeUintegerAccessor (
                      &MgmpProtocol::m_unicastPreqThreshold),
                    MakeUintegerChecker<uint8_t> (1)
                    )
    .AddAttribute ( "UnicastDataThreshold",
                    "Maximum number ofbroadcast receivers, when we send a broadcast as a chain of unicasts",
                    UintegerValue (1),
                    MakeUintegerAccessor (
                      &MgmpProtocol::m_unicastDataThreshold),
                    MakeUintegerChecker<uint8_t> (1)
                    )
    .AddAttribute ( "DoFlag",
                    "Destination only MGMP flag",
                    BooleanValue (false),
                    MakeBooleanAccessor (
                      &MgmpProtocol::m_doFlag),
                    MakeBooleanChecker ()
                    )
    .AddAttribute ( "RfFlag",
                    "Reply and forward flag",
                    BooleanValue (true),
                    MakeBooleanAccessor (
                      &MgmpProtocol::m_rfFlag),
                    MakeBooleanChecker ()
                    )
    .AddTraceSource ( "RouteDiscoveryTime",
                      "The time of route discovery procedure",
                      MakeTraceSourceAccessor (
                        &MgmpProtocol::m_routeDiscoveryTimeCallback),
                      "ns3::Time::TracedCallback"
                      )
  ;
  return tid;
}

MgmpProtocol::MgmpProtocol () :
  m_dataSeqno (1),
  m_mgmpSeqno (1),
  m_preqId (0),
  m_rtable (CreateObject<MgmpRtable> ()),
  m_randomStart (Seconds (0.1)),
  m_maxQueueSize (255),
  m_dot11WmnMGMPmaxPREQretries (3),
  m_dot11WmnMGMPnetDiameterTraversalTime (MicroSeconds (1024*100)),
  m_dot11WmnMGMPpreqMinInterval (MicroSeconds (1024*100)),
  m_dot11WmnMGMPperrMinInterval (MicroSeconds (1024*100)),
  m_dot11WmnMGMPactiveRootTimeout (MicroSeconds (1024*5000)),
  m_dot11WmnMGMPactivePathTimeout (MicroSeconds (1024*5000)),
  m_dot11WmnMGMPpathToRootInterval (MicroSeconds (1024*2000)),
  m_dot11WmnMGMPrannInterval (MicroSeconds (1024*5000)),
  m_isRoot (false),
  m_maxTtl (32),
  m_unicastPerrThreshold (32),
  m_unicastPreqThreshold (1),
  m_unicastDataThreshold (1),
  m_doFlag (false),
  m_rfFlag (false)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_coefficient = CreateObject<UniformRandomVariable> ();
}

MgmpProtocol::~MgmpProtocol ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
MgmpProtocol::DoInitialize ()
{
  m_coefficient->SetAttribute ("Max", DoubleValue (m_randomStart.GetSeconds ()));
#ifdef HUMGMP_UNUSED_MY_CODE
  if (m_isRoot)
    {
      Time randomStart = Seconds (m_coefficient->GetValue ());
      m_proactivePreqTimer = Simulator::Schedule (randomStart, &MgmpProtocol::SendProactivePreq, this);
    }
#endif
}

void
MgmpProtocol::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  for (std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.begin (); i != m_preqTimeouts.end (); i++)
    {
      i->second.preqTimeout.Cancel ();
    }
  m_proactivePreqTimer.Cancel ();
  m_preqTimeouts.clear ();
  m_lastDataSeqno.clear ();
  m_mgmpSeqnoMetricDatabase.clear ();
  m_interfaces.clear ();
  m_rqueue.clear ();
  m_rtable = 0;
  m_mp = 0;
}

bool
MgmpProtocol::RequestRoute (
  uint32_t sourceIface,
  const Mac48Address source,
  const Mac48Address destination,
  Ptr<const Packet> constPacket,
  uint16_t protocolType, //ethrnet 'Protocol' field
  WmnL2RoutingProtocol::RouteReplyCallback routeReply
  )
{
  Ptr <Packet> packet = constPacket->Copy ();
  MgmpTag tag;
  if (sourceIface == GetWmnPoint ()->GetIfIndex ())
    {
      // packet from level 3
      if (packet->PeekPacketTag (tag))
        {
          NS_FATAL_ERROR ("MGMP tag has come with a packet from upper layer. This must not occur...");
        }
      //Filling TAG:
      if (destination == Mac48Address::GetBroadcast ())
        {
          tag.SetSeqno (m_dataSeqno++);
        }
      tag.SetTtl (m_maxTtl);
    }
  else
    {
      if (!packet->RemovePacketTag (tag))
        {
          NS_FATAL_ERROR ("MGMP tag is supposed to be here at this point.");
        }
      tag.DecrementTtl ();
      if (tag.GetTtl () == 0)
        {
          m_stats.droppedTtl++;
          return false;
        }
    }
  if (destination == Mac48Address::GetBroadcast ())
    {
      m_stats.txBroadcast++;
      m_stats.txBytes += packet->GetSize ();
      //channel IDs where we have already sent broadcast:
      std::vector<uint16_t> channels;
      for (MgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin (); plugin != m_interfaces.end (); plugin++)
        {
          bool shouldSend = true;
          for (std::vector<uint16_t>::const_iterator chan = channels.begin (); chan != channels.end (); chan++)
            {
              if ((*chan) == plugin->second->GetChannelId ())
                {
                  shouldSend = false;
                }
            }
          if (!shouldSend)
            {
              continue;
            }
          channels.push_back (plugin->second->GetChannelId ());
          std::vector<Mac48Address> receivers = GetBroadcastReceivers (plugin->first);
          for (std::vector<Mac48Address>::const_iterator i = receivers.begin (); i != receivers.end (); i++)
            {
              Ptr<Packet> packetCopy = packet->Copy ();
              //
              // 64-bit Intel valgrind complains about tag.SetAddress (*i).  It
              // likes this just fine.
              //
              Mac48Address address = *i;
              tag.SetAddress (address);
              packetCopy->AddPacketTag (tag);
              routeReply (true, packetCopy, source, destination, protocolType, plugin->first);
            }
        }
    }
  else
    {
      return ForwardUnicast (sourceIface, source, destination, packet, protocolType, routeReply, tag.GetTtl ());
    }
  return true;
}
bool
MgmpProtocol::RemoveRoutingStuff (uint32_t fromIface, const Mac48Address source,
                                  const Mac48Address destination, Ptr<Packet>  packet, uint16_t&  protocolType)
{
  MgmpTag tag;
  if (!packet->RemovePacketTag (tag))
    {
      NS_FATAL_ERROR ("MGMP tag must exist when packet received from the network");
    }
  return true;
}
bool
MgmpProtocol::ForwardUnicast (uint32_t  sourceIface, const Mac48Address source, const Mac48Address destination,
                              Ptr<Packet>  packet, uint16_t  protocolType, RouteReplyCallback  routeReply, uint32_t ttl)
{
  NS_ASSERT (destination != Mac48Address::GetBroadcast ());
  MgmpRtable::LookupResult result = m_rtable->LookupReactive (destination);
  NS_LOG_DEBUG ("Requested src = "<<source<<", dst = "<<destination<<", I am "<<GetAddress ()<<", RA = "<<result.retransmitter);
  if (result.retransmitter == Mac48Address::GetBroadcast ())
    {
      result = m_rtable->LookupProactive ();
    }
  MgmpTag tag;
  tag.SetAddress (result.retransmitter);
  tag.SetTtl (ttl);
  //seqno and metric is not used;
  packet->AddPacketTag (tag);
  if (result.retransmitter != Mac48Address::GetBroadcast ())
    {
      //reply immediately:
      routeReply (true, packet, source, destination, protocolType, result.ifIndex);
      m_stats.txUnicast++;
      m_stats.txBytes += packet->GetSize ();
      return true;
    }
  if (sourceIface != GetWmnPoint ()->GetIfIndex ())
    {
      //Start path error procedure:
      NS_LOG_DEBUG ("Must Send PERR");
      result = m_rtable->LookupReactiveExpired (destination);
      //1.  Lookup expired reactive path. If exists - start path error
      //    procedure towards a next hop of this path
      //2.  If there was no reactive path, we lookup expired proactive
      //    path. If exist - start path error procedure towards path to
      //    root
      if (result.retransmitter == Mac48Address::GetBroadcast ())
        {
          result = m_rtable->LookupProactiveExpired ();
        }
      if (result.retransmitter != Mac48Address::GetBroadcast ())
        {
          std::vector<FailedDestination> destinations = m_rtable->GetUnreachableDestinations (result.retransmitter);
          InitiatePathError (MakePathError (destinations));
        }
      m_stats.totalDropped++;
      return false;
    }
  //Request a destination:
  result = m_rtable->LookupReactiveExpired (destination);
  if (ShouldSendPreq (destination))
    {
      uint32_t originator_seqno = GetNextMgmpSeqno ();
      uint32_t dst_seqno = 0;
      if (result.retransmitter != Mac48Address::GetBroadcast ())
        {
          dst_seqno = result.seqnum;
        }
      m_stats.initiatedPreq++;
      for (MgmpProtocolMacMap::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
        {
          i->second->RequestDestination (destination, originator_seqno, dst_seqno);
        }
    }
  QueuedPacket pkt;
  pkt.pkt = packet;
  pkt.dst = destination;
  pkt.src = source;
  pkt.protocol = protocolType;
  pkt.reply = routeReply;
  pkt.inInterface = sourceIface;
  if (QueuePacket (pkt))
    {
      m_stats.totalQueued++;
      return true;
    }
  else
    {
      m_stats.totalDropped++;
      return false;
    }
}
void
MgmpProtocol::ReceivePreq (IePreq preq, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
{
  preq.IncrementMetric (metric);
  //acceptance cretirea:
  std::map<Mac48Address, std::pair<uint32_t, uint32_t> >::const_iterator i = m_mgmpSeqnoMetricDatabase.find (
      preq.GetOriginatorAddress ());
  bool freshInfo (true);
  if (i != m_mgmpSeqnoMetricDatabase.end ())
    {
      if ((int32_t)(i->second.first - preq.GetOriginatorSeqNumber ())  > 0)
        {
          return;
        }
      if (i->second.first == preq.GetOriginatorSeqNumber ())
        {
          freshInfo = false;
          if (i->second.second <= preq.GetMetric ())
            {
              return;
            }
        }
    }
  m_mgmpSeqnoMetricDatabase[preq.GetOriginatorAddress ()] =
    std::make_pair (preq.GetOriginatorSeqNumber (), preq.GetMetric ());
  NS_LOG_DEBUG ("I am " << GetAddress () << "Accepted preq from address" << from << ", preq:" << preq);
  std::vector<Ptr<DestinationAddressUnit> > destinations = preq.GetDestinationList ();
  //Add reactive path to originator:
  if (
    (freshInfo) ||
    (
      (m_rtable->LookupReactive (preq.GetOriginatorAddress ()).retransmitter == Mac48Address::GetBroadcast ()) ||
      (m_rtable->LookupReactive (preq.GetOriginatorAddress ()).metric > preq.GetMetric ())
    )
    )
    {
      m_rtable->AddReactivePath (
        preq.GetOriginatorAddress (),
        from,
        interface,
        preq.GetMetric (),
        MicroSeconds (preq.GetLifetime () * 1024),
        preq.GetOriginatorSeqNumber ()
        );
      ReactivePathResolved (preq.GetOriginatorAddress ());
    }
  if (
    (m_rtable->LookupReactive (fromMp).retransmitter == Mac48Address::GetBroadcast ()) ||
    (m_rtable->LookupReactive (fromMp).metric > metric)
    )
    {
      m_rtable->AddReactivePath (
        fromMp,
        from,
        interface,
        metric,
        MicroSeconds (preq.GetLifetime () * 1024),
        preq.GetOriginatorSeqNumber ()
        );
      ReactivePathResolved (fromMp);
    }
  for (std::vector<Ptr<DestinationAddressUnit> >::const_iterator i = destinations.begin (); i != destinations.end (); i++)
    {
      if ((*i)->GetDestinationAddress () == Mac48Address::GetBroadcast ())
        {
          //only proactive PREQ contains destination
          //address as broadcast! Proactive preq MUST
          //have destination count equal to 1 and
          //per destination flags DO and RF
          NS_ASSERT (preq.GetDestCount () == 1);
          NS_ASSERT (((*i)->IsDo ()) && ((*i)->IsRf ()));
          //Add proactive path only if it is the better then existed
          //before
          if (
            ((m_rtable->LookupProactive ()).retransmitter == Mac48Address::GetBroadcast ()) ||
            ((m_rtable->LookupProactive ()).metric > preq.GetMetric ())
            )
            {
              m_rtable->AddProactivePath (
                preq.GetMetric (),
                preq.GetOriginatorAddress (),
                from,
                interface,
                MicroSeconds (preq.GetLifetime () * 1024),
                preq.GetOriginatorSeqNumber ()
                );
              ProactivePathResolved ();
            }
          if (!preq.IsNeedNotPrep ())
            {
              SendPrep (
                GetAddress (),
                preq.GetOriginatorAddress (),
                from,
                (uint32_t)0,
                preq.GetOriginatorSeqNumber (),
                GetNextMgmpSeqno (),
                preq.GetLifetime (),
                interface
                );
            }
          break;
        }
      if ((*i)->GetDestinationAddress () == GetAddress ())
        {
          SendPrep (
            GetAddress (),
            preq.GetOriginatorAddress (),
            from,
            (uint32_t)0,
            preq.GetOriginatorSeqNumber (),
            GetNextMgmpSeqno (),
            preq.GetLifetime (),
            interface
            );
          NS_ASSERT (m_rtable->LookupReactive (preq.GetOriginatorAddress ()).retransmitter != Mac48Address::GetBroadcast ());
          preq.DelDestinationAddressElement ((*i)->GetDestinationAddress ());
          continue;
        }
      //check if can answer:
      MgmpRtable::LookupResult result = m_rtable->LookupReactive ((*i)->GetDestinationAddress ());
      if ((!((*i)->IsDo ())) && (result.retransmitter != Mac48Address::GetBroadcast ()))
        {
          //have a valid information and can answer
          uint32_t lifetime = result.lifetime.GetMicroSeconds () / 1024;
          if ((lifetime > 0) && ((int32_t)(result.seqnum - (*i)->GetDestSeqNumber ()) >= 0))
            {
              SendPrep (
                (*i)->GetDestinationAddress (),
                preq.GetOriginatorAddress (),
                from,
                result.metric,
                preq.GetOriginatorSeqNumber (),
                result.seqnum,
                lifetime,
                interface
                );
              m_rtable->AddPrecursor ((*i)->GetDestinationAddress (), interface, from,
                                      MicroSeconds (preq.GetLifetime () * 1024));
              if ((*i)->IsRf ())
                {
                  (*i)->SetFlags (true, false, (*i)->IsUsn ()); //DO = 1, RF = 0
                }
              else
                {
                  preq.DelDestinationAddressElement ((*i)->GetDestinationAddress ());
                  continue;
                }
            }
        }
    }
  //check if must retransmit:
  if (preq.GetDestCount () == 0)
    {
      return;
    }
  //Forward PREQ to all interfaces:
  NS_LOG_DEBUG ("I am " << GetAddress () << "retransmitting PREQ:" << preq);
  for (MgmpProtocolMacMap::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      i->second->SendPreq (preq);
    }
}
void
MgmpProtocol::ReceivePrep (IePrep prep, Mac48Address from, uint32_t interface, Mac48Address fromMp, uint32_t metric)
{
  prep.IncrementMetric (metric);
  //acceptance cretirea:
  std::map<Mac48Address, std::pair<uint32_t, uint32_t> >::const_iterator i = m_mgmpSeqnoMetricDatabase.find (
      prep.GetOriginatorAddress ());
  bool freshInfo (true);
  uint32_t sequence = prep.GetDestinationSeqNumber ();
  if (i != m_mgmpSeqnoMetricDatabase.end ())
    {
      if ((int32_t)(i->second.first - sequence) > 0)
        {
          return;
        }
      if (i->second.first == sequence)
        {
          freshInfo = false;
        }
    }
  m_mgmpSeqnoMetricDatabase[prep.GetOriginatorAddress ()] = std::make_pair (sequence, prep.GetMetric ());
  //update routing info
  //Now add a path to destination and add precursor to source
  NS_LOG_DEBUG ("I am " << GetAddress () << ", received prep from " << prep.GetOriginatorAddress () << ", receiver was:" << from);
  MgmpRtable::LookupResult result = m_rtable->LookupReactive (prep.GetDestinationAddress ());
  //Add a reactive path only if seqno is fresher or it improves the
  //metric
  if (
    (freshInfo) ||
    (
      ((m_rtable->LookupReactive (prep.GetOriginatorAddress ())).retransmitter == Mac48Address::GetBroadcast ()) ||
      ((m_rtable->LookupReactive (prep.GetOriginatorAddress ())).metric > prep.GetMetric ())
    )
    )
    {
      m_rtable->AddReactivePath (
        prep.GetOriginatorAddress (),
        from,
        interface,
        prep.GetMetric (),
        MicroSeconds (prep.GetLifetime () * 1024),
        sequence);
      m_rtable->AddPrecursor (prep.GetDestinationAddress (), interface, from,
                              MicroSeconds (prep.GetLifetime () * 1024));
      if (result.retransmitter != Mac48Address::GetBroadcast ())
        {
          m_rtable->AddPrecursor (prep.GetOriginatorAddress (), interface, result.retransmitter,
                                  result.lifetime);
        }
      ReactivePathResolved (prep.GetOriginatorAddress ());
    }
  if (
    ((m_rtable->LookupReactive (fromMp)).retransmitter == Mac48Address::GetBroadcast ()) ||
    ((m_rtable->LookupReactive (fromMp)).metric > metric)
    )
    {
      m_rtable->AddReactivePath (
        fromMp,
        from,
        interface,
        metric,
        MicroSeconds (prep.GetLifetime () * 1024),
        sequence);
      ReactivePathResolved (fromMp);
    }
  if (prep.GetDestinationAddress () == GetAddress ())
    {
      NS_LOG_DEBUG ("I am "<<GetAddress ()<<", resolved "<<prep.GetOriginatorAddress ());
      return;
    }
  if (result.retransmitter == Mac48Address::GetBroadcast ())
    {
      return;
    }
  //Forward PREP
  MgmpProtocolMacMap::const_iterator prep_sender = m_interfaces.find (result.ifIndex);
  NS_ASSERT (prep_sender != m_interfaces.end ());
  prep_sender->second->SendPrep (prep, result.retransmitter);
}
void
MgmpProtocol::ReceivePerr (std::vector<FailedDestination> destinations, Mac48Address from, uint32_t interface, Mac48Address fromMp)
{
  //Acceptance cretirea:
  NS_LOG_DEBUG ("I am "<<GetAddress ()<<", received PERR from "<<from);
  std::vector<FailedDestination> retval;
  MgmpRtable::LookupResult result;
  for (unsigned int i = 0; i < destinations.size (); i++)
    {
      result = m_rtable->LookupReactiveExpired (destinations[i].destination);
      if (!(
            (result.retransmitter != from) ||
            (result.ifIndex != interface) ||
            ((int32_t)(result.seqnum - destinations[i].seqnum) > 0)
            ))
        {
          retval.push_back (destinations[i]);
        }
    }
  if (retval.size () == 0)
    {
      return;
    }
  ForwardPathError (MakePathError (retval));
}
void
MgmpProtocol::SendPrep (
  Mac48Address src,
  Mac48Address dst,
  Mac48Address retransmitter,
  uint32_t initMetric,
  uint32_t originatorDsn,
  uint32_t destinationSN,
  uint32_t lifetime,
  uint32_t interface)
{
  IePrep prep;
  prep.SetHopcount (0);
  prep.SetTtl (m_maxTtl);
  prep.SetDestinationAddress (dst);
  prep.SetDestinationSeqNumber (destinationSN);
  prep.SetLifetime (lifetime);
  prep.SetMetric (initMetric);
  prep.SetOriginatorAddress (src);
  prep.SetOriginatorSeqNumber (originatorDsn);
  MgmpProtocolMacMap::const_iterator prep_sender = m_interfaces.find (interface);
  NS_ASSERT (prep_sender != m_interfaces.end ());
  prep_sender->second->SendPrep (prep, retransmitter);
  m_stats.initiatedPrep++;
}
bool
MgmpProtocol::Install (Ptr<WmnPointDevice> mp)
{
  m_mp = mp;
  std::vector<Ptr<NetDevice> > interfaces = mp->GetInterfaces ();
  for (std::vector<Ptr<NetDevice> >::const_iterator i = interfaces.begin (); i != interfaces.end (); i++)
    {
      // Checking for compatible net device
      Ptr<WifiNetDevice> wifiNetDev = (*i)->GetObject<WifiNetDevice> ();
      if (wifiNetDev == 0)
        {
          return false;
        }
      Ptr<WmnWifiInterfaceMac>  mac = wifiNetDev->GetMac ()->GetObject<WmnWifiInterfaceMac> ();
      if (mac == 0)
        {
          return false;
        }
      // Installing plugins:
      Ptr<MgmpProtocolMac> mgmpMac = Create<MgmpProtocolMac> (wifiNetDev->GetIfIndex (), this);
      m_interfaces[wifiNetDev->GetIfIndex ()] = mgmpMac;
      mac->InstallPlugin (mgmpMac);
      //Installing airtime link metric:
      Ptr<AirtimeLinkMetricCalculator> metric = CreateObject <AirtimeLinkMetricCalculator> ();
      mac->SetLinkMetricCallback (MakeCallback (&AirtimeLinkMetricCalculator::CalculateMetric, metric));
    }
  mp->SetRoutingProtocol (this);
  // Wmn point aggregates all installed protocols
  mp->AggregateObject (this);
  m_address = Mac48Address::ConvertFrom (mp->GetAddress ()); // address;
  return true;
}
void
MgmpProtocol::HuperLinkStatus (Mac48Address wmnPointAddress, Mac48Address huperAddress, uint32_t interface, bool status)
{
  if (status)
    {
      return;
    }
  std::vector<FailedDestination> destinations = m_rtable->GetUnreachableDestinations (huperAddress);
  InitiatePathError (MakePathError (destinations));
}
void
MgmpProtocol::SetNeighboursCallback (Callback<std::vector<Mac48Address>, uint32_t> cb)
{
  m_neighboursCallback = cb;
}
bool
MgmpProtocol::DropDataFrame (uint32_t seqno, Mac48Address source)
{
  if (source == GetAddress ())
    {
      return true;
    }
  std::map<Mac48Address, uint32_t,std::less<Mac48Address> >::const_iterator i = m_lastDataSeqno.find (source);
  if (i == m_lastDataSeqno.end ())
    {
      m_lastDataSeqno[source] = seqno;
    }
  else
    {
      if ((int32_t)(i->second - seqno)  >= 0)
        {
          return true;
        }
      m_lastDataSeqno[source] = seqno;
    }
  return false;
}
MgmpProtocol::PathError
MgmpProtocol::MakePathError (std::vector<FailedDestination> destinations)
{
  PathError retval;
  //MgmpRtable increments a sequence number as written in 11B.9.7.2
  retval.receivers = GetPerrReceivers (destinations);
  if (retval.receivers.size () == 0)
    {
      return retval;
    }
  m_stats.initiatedPerr++;
  for (unsigned int i = 0; i < destinations.size (); i++)
    {
      retval.destinations.push_back (destinations[i]);
      m_rtable->DeleteReactivePath (destinations[i].destination);
    }
  return retval;
}
void
MgmpProtocol::InitiatePathError (PathError perr)
{
  for (MgmpProtocolMacMap::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      std::vector<Mac48Address> receivers_for_interface;
      for (unsigned int j = 0; j < perr.receivers.size (); j++)
        {
          if (i->first == perr.receivers[j].first)
            {
              receivers_for_interface.push_back (perr.receivers[j].second);
            }
        }
      i->second->InitiatePerr (perr.destinations, receivers_for_interface);
    }
}
void
MgmpProtocol::ForwardPathError (PathError perr)
{
  for (MgmpProtocolMacMap::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      std::vector<Mac48Address> receivers_for_interface;
      for (unsigned int j = 0; j < perr.receivers.size (); j++)
        {
          if (i->first == perr.receivers[j].first)
            {
              receivers_for_interface.push_back (perr.receivers[j].second);
            }
        }
      i->second->ForwardPerr (perr.destinations, receivers_for_interface);
    }
}

std::vector<std::pair<uint32_t, Mac48Address> >
MgmpProtocol::GetPerrReceivers (std::vector<FailedDestination> failedDest)
{
  MgmpRtable::PrecursorList retval;
  for (unsigned int i = 0; i < failedDest.size (); i++)
    {
      MgmpRtable::PrecursorList precursors = m_rtable->GetPrecursors (failedDest[i].destination);
      m_rtable->DeleteReactivePath (failedDest[i].destination);
      m_rtable->DeleteProactivePath (failedDest[i].destination);
      for (unsigned int j = 0; j < precursors.size (); j++)
        {
          retval.push_back (precursors[j]);
        }
    }
  //Check if we have dublicates in retval and precursors:
  for (unsigned int i = 0; i < retval.size (); i++)
    {
      for (unsigned int j = i+1; j < retval.size (); j++)
        {
          if (retval[i].second == retval[j].second)
            {
              retval.erase (retval.begin () + j);
            }
        }
    }
  return retval;
}
std::vector<Mac48Address>
MgmpProtocol::GetPreqReceivers (uint32_t interface)
{
  std::vector<Mac48Address> retval;
  if (!m_neighboursCallback.IsNull ())
    {
      retval = m_neighboursCallback (interface);
    }
  if ((retval.size () >= m_unicastPreqThreshold) || (retval.size () == 0))
    {
      retval.clear ();
      retval.push_back (Mac48Address::GetBroadcast ());
    }
  return retval;
}
std::vector<Mac48Address>
MgmpProtocol::GetBroadcastReceivers (uint32_t interface)
{
  std::vector<Mac48Address> retval;
  if (!m_neighboursCallback.IsNull ())
    {
      retval = m_neighboursCallback (interface);
    }
  if ((retval.size () >= m_unicastDataThreshold) || (retval.size () == 0))
    {
      retval.clear ();
      retval.push_back (Mac48Address::GetBroadcast ());
    }
  return retval;
}

bool
MgmpProtocol::QueuePacket (QueuedPacket packet)
{
  if (m_rqueue.size () > m_maxQueueSize)
    {
      return false;
    }
  m_rqueue.push_back (packet);
  return true;
}

MgmpProtocol::QueuedPacket
MgmpProtocol::DequeueFirstPacketByDst (Mac48Address dst)
{
  QueuedPacket retval;
  retval.pkt = 0;
  for (std::vector<QueuedPacket>::iterator i = m_rqueue.begin (); i != m_rqueue.end (); i++)
    {
      if ((*i).dst == dst)
        {
          retval = (*i);
          m_rqueue.erase (i);
          break;
        }
    }
  return retval;
}

MgmpProtocol::QueuedPacket
MgmpProtocol::DequeueFirstPacket ()
{
  QueuedPacket retval;
  retval.pkt = 0;
  if (m_rqueue.size () != 0)
    {
      retval = m_rqueue[0];
      m_rqueue.erase (m_rqueue.begin ());
    }
  return retval;
}

void
MgmpProtocol::ReactivePathResolved (Mac48Address dst)
{
  std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.find (dst);
  if (i != m_preqTimeouts.end ())
    {
      m_routeDiscoveryTimeCallback (Simulator::Now () - i->second.whenScheduled);
    }

  MgmpRtable::LookupResult result = m_rtable->LookupReactive (dst);
  NS_ASSERT (result.retransmitter != Mac48Address::GetBroadcast ());
  //Send all packets stored for this destination
  QueuedPacket packet = DequeueFirstPacketByDst (dst);
  while (packet.pkt != 0)
    {
      //set RA tag for retransmitter:
      MgmpTag tag;
      packet.pkt->RemovePacketTag (tag);
      tag.SetAddress (result.retransmitter);
      packet.pkt->AddPacketTag (tag);
      m_stats.txUnicast++;
      m_stats.txBytes += packet.pkt->GetSize ();
      packet.reply (true, packet.pkt, packet.src, packet.dst, packet.protocol, result.ifIndex);

      packet = DequeueFirstPacketByDst (dst);
    }
}
void
MgmpProtocol::ProactivePathResolved ()
{
  //send all packets to root
  MgmpRtable::LookupResult result = m_rtable->LookupProactive ();
  NS_ASSERT (result.retransmitter != Mac48Address::GetBroadcast ());
  QueuedPacket packet = DequeueFirstPacket ();
  while (packet.pkt != 0)
    {
      //set RA tag for retransmitter:
      MgmpTag tag;
      if (!packet.pkt->RemovePacketTag (tag))
        {
          NS_FATAL_ERROR ("MGMP tag must be present at this point");
        }
      tag.SetAddress (result.retransmitter);
      packet.pkt->AddPacketTag (tag);
      m_stats.txUnicast++;
      m_stats.txBytes += packet.pkt->GetSize ();
      packet.reply (true, packet.pkt, packet.src, packet.dst, packet.protocol, result.ifIndex);

      packet = DequeueFirstPacket ();
    }
}

bool
MgmpProtocol::ShouldSendPreq (Mac48Address dst)
{
  std::map<Mac48Address, PreqEvent>::const_iterator i = m_preqTimeouts.find (dst);
  if (i == m_preqTimeouts.end ())
    {
      m_preqTimeouts[dst].preqTimeout = Simulator::Schedule (
          Time (m_dot11WmnMGMPnetDiameterTraversalTime * 2),
          &MgmpProtocol::RetryPathDiscovery, this, dst, 1);
      m_preqTimeouts[dst].whenScheduled = Simulator::Now ();
      return true;
    }
  return false;
}
void
MgmpProtocol::RetryPathDiscovery (Mac48Address dst, uint8_t numOfRetry)
{
  MgmpRtable::LookupResult result = m_rtable->LookupReactive (dst);
  if (result.retransmitter == Mac48Address::GetBroadcast ())
    {
      result = m_rtable->LookupProactive ();
    }
  if (result.retransmitter != Mac48Address::GetBroadcast ())
    {
      std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.find (dst);
      NS_ASSERT (i != m_preqTimeouts.end ());
      m_preqTimeouts.erase (i);
      return;
    }
  if (numOfRetry > m_dot11WmnMGMPmaxPREQretries)
    {
      QueuedPacket packet = DequeueFirstPacketByDst (dst);
      //purge queue and delete entry from retryDatabase
      while (packet.pkt != 0)
        {
          m_stats.totalDropped++;
          packet.reply (false, packet.pkt, packet.src, packet.dst, packet.protocol, MgmpRtable::MAX_METRIC);
          packet = DequeueFirstPacketByDst (dst);
        }
      std::map<Mac48Address, PreqEvent>::iterator i = m_preqTimeouts.find (dst);
      NS_ASSERT (i != m_preqTimeouts.end ());
      m_routeDiscoveryTimeCallback (Simulator::Now () - i->second.whenScheduled);
      m_preqTimeouts.erase (i);
      return;
    }
  numOfRetry++;
  uint32_t originator_seqno = GetNextMgmpSeqno ();
  uint32_t dst_seqno = m_rtable->LookupReactiveExpired (dst).seqnum;
  for (MgmpProtocolMacMap::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      i->second->RequestDestination (dst, originator_seqno, dst_seqno);
    }
  m_preqTimeouts[dst].preqTimeout = Simulator::Schedule (
      Time ((2 * (numOfRetry + 1)) *  m_dot11WmnMGMPnetDiameterTraversalTime),
      &MgmpProtocol::RetryPathDiscovery, this, dst, numOfRetry);
}
//Proactive PREQ routines:
void
MgmpProtocol::SetRoot ()
{
  NS_LOG_DEBUG ("ROOT IS: " << m_address);
  m_isRoot = true;
}
void
MgmpProtocol::UnsetRoot ()
{
  m_proactivePreqTimer.Cancel ();
}
void
MgmpProtocol::SendProactivePreq ()
{
  IePreq preq;
  //By default: must answer
  preq.SetHopcount (0);
  preq.SetTTL (m_maxTtl);
  preq.SetLifetime (m_dot11WmnMGMPactiveRootTimeout.GetMicroSeconds () /1024);
  //\attention: do not forget to set originator address, sequence
  //number and preq ID in MGMP-MAC plugin
  preq.AddDestinationAddressElement (true, true, Mac48Address::GetBroadcast (), 0);
  preq.SetOriginatorAddress (GetAddress ());
  preq.SetPreqID (GetNextPreqId ());
  preq.SetOriginatorSeqNumber (GetNextMgmpSeqno ());
  for (MgmpProtocolMacMap::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      i->second->SendPreq (preq);
    }
  m_proactivePreqTimer = Simulator::Schedule (m_dot11WmnMGMPpathToRootInterval, &MgmpProtocol::SendProactivePreq, this);
}
bool
MgmpProtocol::GetDoFlag ()
{
  return m_doFlag;
}
bool
MgmpProtocol::GetRfFlag ()
{
  return m_rfFlag;
}
Time
MgmpProtocol::GetPreqMinInterval ()
{
  return m_dot11WmnMGMPpreqMinInterval;
}
Time
MgmpProtocol::GetPerrMinInterval ()
{
  return m_dot11WmnMGMPperrMinInterval;
}
uint8_t
MgmpProtocol::GetMaxTtl ()
{
  return m_maxTtl;
}
uint32_t
MgmpProtocol::GetNextPreqId ()
{
  m_preqId++;
  return m_preqId;
}
uint32_t
MgmpProtocol::GetNextMgmpSeqno ()
{
  m_mgmpSeqno++;
  return m_mgmpSeqno;
}
uint32_t
MgmpProtocol::GetActivePathLifetime ()
{
  return m_dot11WmnMGMPactivePathTimeout.GetMicroSeconds () / 1024;
}
uint8_t
MgmpProtocol::GetUnicastPerrThreshold ()
{
  return m_unicastPerrThreshold;
}
void MgmpProtocol::ReceiveRann(IePreq preq, Mac48Address from, uint32_t, Mac48Address fromMp, uint32_t metric)
{
}
Mac48Address
MgmpProtocol::GetAddress ()
{
  return m_address;
}
//Statistics:
MgmpProtocol::Statistics::Statistics () :
  txUnicast (0),
  txBroadcast (0),
  txBytes (0),
  droppedTtl (0),
  totalQueued (0),
  totalDropped (0),
  initiatedPreq (0),
  initiatedPrep (0),
  initiatedPerr (0)
{
}
void MgmpProtocol::Statistics::Print (std::ostream & os) const
{
  os << "<Statistics "
  "txUnicast=\"" << txUnicast << "\" "
  "txBroadcast=\"" << txBroadcast << "\" "
  "txBytes=\"" << txBytes << "\" "
  "droppedTtl=\"" << droppedTtl << "\" "
  "totalQueued=\"" << totalQueued << "\" "
  "totalDropped=\"" << totalDropped << "\" "
  "initiatedPreq=\"" << initiatedPreq << "\" "
  "initiatedPrep=\"" << initiatedPrep << "\" "
  "initiatedPerr=\"" << initiatedPerr << "\"/>" << std::endl;
}
void
MgmpProtocol::Report (std::ostream & os) const
{
  os << "<Mgmp "
  "address=\"" << m_address << "\"" << std::endl <<
  "maxQueueSize=\"" << m_maxQueueSize << "\"" << std::endl <<
  "Dot11WmnMGMPmaxPREQretries=\"" << (uint16_t)m_dot11WmnMGMPmaxPREQretries << "\"" << std::endl <<
  "Dot11WmnMGMPnetDiameterTraversalTime=\"" << m_dot11WmnMGMPnetDiameterTraversalTime.GetSeconds () << "\"" << std::endl <<
  "Dot11WmnMGMPpreqMinInterval=\"" << m_dot11WmnMGMPpreqMinInterval.GetSeconds () << "\"" << std::endl <<
  "Dot11WmnMGMPperrMinInterval=\"" << m_dot11WmnMGMPperrMinInterval.GetSeconds () << "\"" << std::endl <<
  "Dot11WmnMGMPactiveRootTimeout=\"" << m_dot11WmnMGMPactiveRootTimeout.GetSeconds () << "\"" << std::endl <<
  "Dot11WmnMGMPactivePathTimeout=\"" << m_dot11WmnMGMPactivePathTimeout.GetSeconds () << "\"" << std::endl <<
  "Dot11WmnMGMPpathToRootInterval=\"" << m_dot11WmnMGMPpathToRootInterval.GetSeconds () << "\"" << std::endl <<
  "Dot11WmnMGMPrannInterval=\"" << m_dot11WmnMGMPrannInterval.GetSeconds () << "\"" << std::endl <<
  "isRoot=\"" << m_isRoot << "\"" << std::endl <<
  "maxTtl=\"" << (uint16_t)m_maxTtl << "\"" << std::endl <<
  "unicastPerrThreshold=\"" << (uint16_t)m_unicastPerrThreshold << "\"" << std::endl <<
  "unicastPreqThreshold=\"" << (uint16_t)m_unicastPreqThreshold << "\"" << std::endl <<
  "unicastDataThreshold=\"" << (uint16_t)m_unicastDataThreshold << "\"" << std::endl <<
  "doFlag=\"" << m_doFlag << "\"" << std::endl <<
  "rfFlag=\"" << m_rfFlag << "\">" << std::endl;
  m_stats.Print (os);
  for (MgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin (); plugin != m_interfaces.end (); plugin++)
    {
      plugin->second->Report (os);
    }
  os << "</Mgmp>" << std::endl;
}
void
MgmpProtocol::ResetStats ()
{
  m_stats = Statistics ();
  for (MgmpProtocolMacMap::const_iterator plugin = m_interfaces.begin (); plugin != m_interfaces.end (); plugin++)
    {
      plugin->second->ResetStats ();
    }
}

int64_t
MgmpProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_coefficient->SetStream (stream);
  return 1;
}

MgmpProtocol::QueuedPacket::QueuedPacket () :
  pkt (0),
  protocol (0),
  inInterface (0)
{
}
} // namespace hu11s
} // namespace ns3
