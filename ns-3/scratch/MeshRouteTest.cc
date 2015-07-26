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
* Author: Whimsy Duke <whimsyduke@163.com>
*/

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/pmtmgmp-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/stats-module.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>

//输出到文件
//#define OUT_TO_FILE 
//无应用层模式
#define NO_APPLICATION
//测试模式
#define TEST_LOCATION


//角度转换
#define DEGREES_TO_RADIANS(__ANGLE__) ((__ANGLE__) * 0.01745329252f) // PI / 180
#define MIN_APPLICATION_TIME 120 // 仿真前路由生成时间
#define END_APPLICATION_TIME 15 // 仿真后等待时间

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("MeshRouteTest");

class MeshRouteClass
{
public:
	// 初始化测试
	MeshRouteClass();
	// 配置参数
	void Configure(int argc, char ** argv);
	// 开始测试
	int Run();

private:
	// 仿真配置
	void SimulatorSetting();
	// 创建节点及相关协议设置
	void CreateNodes();
	// 节点位置设置
	void LocateNodes();
	// 安装网络协议栈
	void InstallInternetStack();
	// 安装应用层
	void InstallApplicationRandom();
	// 数据统计模块配置
	void StatsDataSet();
	// 输出流量数据
	void FlowMonitorReport();

private:
	// 回调函数
	void CallBack_RouteDiscoveryTime(string context, Time time);
	void CallBack_ApplicationTX(string context, Ptr<const Packet> packet);
	// 输出报告
	void Report();

public:
	//子类型定义
	enum MeshProtocolType// Mesh路由协议类型
	{
		DOT11S_HWMP,// HWMP路由协议
		MY11S_PMTMGMP,// PMTMGMP路由协议
	};

	enum NodeAreaType// 节点区域形状类型
	{
		SQUARE,// 正方形
		HEXAGON,// 正六边形
	};

private:
	// 命令行配置参数
	MeshProtocolType m_ProtocolType;
	NodeAreaType m_AreaType;
	int m_Size;
	double m_RandomStart;
	uint32_t m_NumIface;
	WifiPhyStandard m_WifiPhyStandard;
	double m_Step;
	uint16_t m_ApplicationNum;
	uint32_t  m_MaxBytes;
	int m_SourceNum;
	int m_DestinationNum;
	uint16_t  m_PacketSize;
	string m_DataRate;
	double m_TotalTime;
	string m_Root;
	bool m_Pcap;

private:
	// 节点总数
	int l_NodeNum;
	// 节点容器
	NodeContainer l_Nodes;
	// 网络设备容器
	NetDeviceContainer l_NetDevices;
	MeshHelper l_Mesh;
	WmnHelper l_Pmtmgmp;
	// 接口地址:
	Ipv4InterfaceContainer l_Interfaces;
	// 流量统计
	FlowMonitorHelper l_Flowmon;
	Ptr<FlowMonitor> l_Monitor;
};

// 初始化测试
MeshRouteClass::MeshRouteClass() :
m_ProtocolType(MY11S_PMTMGMP),
m_AreaType(SQUARE),
m_Size(4),
m_RandomStart(0.1),
m_NumIface(1),
m_WifiPhyStandard(WIFI_PHY_STANDARD_80211a),
m_Step(70),
m_ApplicationNum(0),
m_MaxBytes(0),
m_SourceNum(0),
m_DestinationNum(0),
m_PacketSize(1024),
m_DataRate("150kbps"),
m_TotalTime(50),
m_Root("00:00:00:00:00:06"),
m_Pcap(false)
{
	// 链路层及网络层协议设置
	l_Mesh = MeshHelper::Default();
	l_Pmtmgmp = WmnHelper::Default();
}

// 配置参数
void MeshRouteClass::Configure(int argc, char ** argv)
{
	// 命令行配置
	CommandLine cmd;

	// 命令行参数
	int protocolType = (int)m_ProtocolType;
	int areaType = (int)m_AreaType;
	int wifiPhyStandard = (int)m_WifiPhyStandard;
	cmd.AddValue("ProtocolType", "Mesh路由协议类型。可选值：0.DOT11S_HWMP混合路由协议；1.MY11S_PMTMGMP，多先验树多网关多路径路由协议；", protocolType);
	cmd.AddValue("AreaType", "节点区域类型。可选值：0.SQUARE，正方形；1.HEXAGON，正六边形；", areaType);
	cmd.AddValue("Size", "节点区域大小的参数，决定节点数量。若为正方形，则Size代表边长，节点总数为：Size * Size；若为六边形，则Size代表边长，节点总数为：3 * (Size - 1) * Size；", m_Size);
	cmd.AddValue("RandomStartTime", "最大随机启动延迟时间。", m_RandomStart);
	cmd.AddValue("NumIface", "每个节点射频接口的数量。", m_NumIface);
	cmd.AddValue("WifiPhyStandard", "使用的Wifi物理层标准。可选值：0.WIFI_PHY_STANDARD_80211a；1.WIFI_PHY_STANDARD_80211b；2.WIFI_PHY_STANDARD_80211g；3.WIFI_PHY_STANDARD_80211_10MHZ；4.WIFI_PHY_STANDARD_80211_5MHZ；5.WIFI_PHY_STANDARD_holland；6.WIFI_PHY_STANDARD_80211n_2_4GHZ；7.WIFI_PHY_STANDARD_80211n_5GHZ；。", wifiPhyStandard);
	cmd.AddValue("Step", "节点间间隔，具体效果由节点区域类型决定。", m_Step);
	cmd.AddValue("ApplicationNum", "网络中应用层结点的数量，两个节点成出现，如果路由协议为PMTMGMP这两个结点一个作为MAP，一个作为MPP。", m_ApplicationNum);
	cmd.AddValue("MaxBytes", "最大传送总数据。", m_MaxBytes);
	cmd.AddValue("SourceNum", "源节点序号，如果不符合相关设置，自动设为0。", m_SourceNum);
	cmd.AddValue("DestinationNum", "目标点序号，如果不符合相关设置，自动设为最后一个节点。", m_DestinationNum);
	cmd.AddValue("PacketSize", "应用层包大小。", m_PacketSize);
	cmd.AddValue("DataRate", "传输速率。", m_DataRate);
	stringstream ss;
	ss << "仿真总时间,不包括前置路径生成时间(" << MIN_APPLICATION_TIME << "s)和后置完成等待时间(" << END_APPLICATION_TIME << "s)。";
	cmd.AddValue("TotalTime", ss.str(), m_TotalTime);
	cmd.AddValue("Root", "路由协议中Mesh节点的Mac地址。", m_Root);

	cmd.Parse(argc, argv);

	m_ProtocolType = (MeshProtocolType)protocolType;
	m_AreaType = (NodeAreaType)areaType;
	m_WifiPhyStandard = (WifiPhyStandard)wifiPhyStandard;
}

// 通过配置计算基本参数
void MeshRouteClass::SimulatorSetting()
{
	//仿真总时间
	m_TotalTime += MIN_APPLICATION_TIME + END_APPLICATION_TIME;

	// 配置路径
	string l_AttributePath_PeerLink;// Peer Link
	string l_AttributePath_PeerManagementProtocol;// Peer Management Protocol
	string l_AttributePath_RouteProtocol;// Route Protocol
	string l_AttributePath_RouteProtocolPart;// 部分名称差异

	//路由协议类型
	switch (m_ProtocolType)
	{
	case MeshRouteClass::DOT11S_HWMP:
		l_AttributePath_PeerLink = "ns3::dot11s::PeerLink::";
		l_AttributePath_PeerManagementProtocol = "ns3::dot11s::PeerManagementProtocol::";
		l_AttributePath_RouteProtocol = "ns3::dot11s::HwmpProtocol::";
		l_AttributePath_RouteProtocolPart = "Dot11MeshHWMP";
		break;
	case MeshRouteClass::MY11S_PMTMGMP:
		l_AttributePath_PeerLink = "ns3::my11s::PmtmgmpPeerLink::";
		l_AttributePath_PeerManagementProtocol = "ns3::my11s::PmtmgmpPeerManagementProtocol::";
		l_AttributePath_RouteProtocol = "ns3::my11s::PmtmgmpProtocol::";
		l_AttributePath_RouteProtocolPart = "My11WmnPMTMGMP";
		break;
	default:
		break;
	}

	// 配置Peer Link参数
	Config::SetDefault(l_AttributePath_PeerLink + "MaxBeaconLoss", UintegerValue(32));
	Config::SetDefault(l_AttributePath_PeerLink + "MaxRetries", UintegerValue(8));
	Config::SetDefault(l_AttributePath_PeerLink + "MaxPacketFailure", UintegerValue(8));

	// 配置Peer Management Protocol参数
	Config::SetDefault(l_AttributePath_PeerManagementProtocol + "EnableBeaconCollisionAvoidance", BooleanValue(false));

	// 配置路由协议参数
	Config::SetDefault(l_AttributePath_RouteProtocol + l_AttributePath_RouteProtocolPart + "activePathTimeout", TimeValue(Seconds(100)));
	Config::SetDefault(l_AttributePath_RouteProtocol + l_AttributePath_RouteProtocolPart + "activeRootTimeout", TimeValue(Seconds(100)));
	Config::SetDefault(l_AttributePath_RouteProtocol + l_AttributePath_RouteProtocolPart + "maxPREQretries", UintegerValue(5));
	Config::SetDefault(l_AttributePath_RouteProtocol + "UnicastPreqThreshold", UintegerValue(10));
	Config::SetDefault(l_AttributePath_RouteProtocol + "UnicastDataThreshold", UintegerValue(5));
	Config::SetDefault(l_AttributePath_RouteProtocol + "DoFlag", BooleanValue(true));
	Config::SetDefault(l_AttributePath_RouteProtocol + "RfFlag", BooleanValue(false));
}

// 创建节点及相关协议设置
void MeshRouteClass::CreateNodes()
{
	// 节点区域类型，统计节点总数
	switch (m_AreaType)
	{
	case MeshRouteClass::SQUARE:
		l_NodeNum = m_Size * m_Size;
		break;
	case MeshRouteClass::HEXAGON:
		l_NodeNum = 3 * (m_Size - 1) * m_Size + 1;
		break;
	default:
		break;
	}

	// 创建节点
	l_Nodes.Create(l_NodeNum);

	// 配置物理层协议及信道
	double m_txpower = 18.0; // dbm
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-89.0));
	wifiPhy.Set("CcaMode1Threshold", DoubleValue(-62.0));
	wifiPhy.Set("TxGain", DoubleValue(1.0));
	wifiPhy.Set("RxGain", DoubleValue(1.0));
	wifiPhy.Set("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set("TxPowerEnd", DoubleValue(m_txpower));
	wifiPhy.Set("TxPowerStart", DoubleValue(m_txpower));
	wifiPhy.Set("RxNoiseFigure", DoubleValue(7.0));
	if (m_Pcap)	wifiPhy.EnablePcapAll(string("pmtmgmp-80211a--"));
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel", "Exponent", StringValue("2.7"));
	wifiPhy.SetChannel(wifiChannel.Create());

	switch (m_ProtocolType)
	{
	case MeshRouteClass::DOT11S_HWMP:
		l_Mesh.SetStandard(WIFI_PHY_STANDARD_80211a);
		if (!Mac48Address(m_Root.c_str()).IsBroadcast())
		{
			l_Mesh.SetStackInstaller("ns3::Dot11sStack", "Root", Mac48AddressValue(Mac48Address(m_Root.c_str())));
		}
		else
		{
			l_Mesh.SetStackInstaller("ns3::Dot11sStack");
		}
		l_Mesh.SetMacType("RandomStart", TimeValue(Seconds(m_RandomStart)));
		l_Mesh.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue(2500));
		l_Mesh.SetNumberOfInterfaces(m_NumIface);
		l_Mesh.SetSpreadInterfaceChannels(MeshHelper::ZERO_CHANNEL);
		l_NetDevices = l_Mesh.Install(wifiPhy, l_Nodes);
		break;
	case MeshRouteClass::MY11S_PMTMGMP:
		l_Pmtmgmp.SetStandard(m_WifiPhyStandard);
		if (!Mac48Address(m_Root.c_str()).IsBroadcast())
		{
			l_Pmtmgmp.SetStackInstaller("ns3::My11sStack", "Root", Mac48AddressValue(Mac48Address(m_Root.c_str())));
		}
		else
		{
			l_Pmtmgmp.SetStackInstaller("ns3::My11sStack");
		}
		l_Pmtmgmp.SetMacType("RandomStart", TimeValue(Seconds(m_RandomStart)));
		l_Pmtmgmp.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue(2500));
		l_Pmtmgmp.SetNumberOfInterfaces(m_NumIface);
		l_Pmtmgmp.SetSpreadInterfaceChannels(WmnHelper::ZERO_CHANNEL);
		l_NetDevices = l_Pmtmgmp.Install(wifiPhy, l_Nodes);
		break;
	default:
		break;
	}
}

// 节点位置设置
void MeshRouteClass::LocateNodes()
{
	MobilityHelper mobility;

	Ptr<ListPositionAllocator> initialAlloc = CreateObject<ListPositionAllocator>();
	// 节点区域类型，设置节点位置
	int r = 0, s = 0, t, u;
	double x, y;
	double fx, fy, sx, sy;
	switch (m_AreaType)
	{
	case MeshRouteClass::SQUARE:
		mobility.SetPositionAllocator("ns3::GridPositionAllocator",
			"MinX", DoubleValue(0.0),
			"MinY", DoubleValue(0.0),
			"DeltaX", DoubleValue(m_Step),
			"DeltaY", DoubleValue(m_Step),
			"GridWidth", UintegerValue(m_Size),
			"LayoutType", StringValue("RowFirst"));
		break;
	case MeshRouteClass::HEXAGON:
		initialAlloc->Add(Vector(0, 0, 0));
		for (int i = 1; i < l_NodeNum; i++)
		{
			if (s == r * 6) r++;
			s = i - r * (r - 1) * 3;
			t = (s - 1) / r;
			u = s - t * r - 1;
			if (u == 0)
			{
				fx = r * m_Step * cos(DEGREES_TO_RADIANS(60 * t));
				fy = r * m_Step * sin(DEGREES_TO_RADIANS(60 * t));
				sx = r * m_Step * cos(DEGREES_TO_RADIANS(60 * (t + 1)));
				sy = r * m_Step * sin(DEGREES_TO_RADIANS(60 * (t + 1)));
			}
			x = (sx - fx) / r;
			y = (sy - fy) / r;
			NS_LOG_INFO("R:" << i << ",s:" << s << ",t:" << t << ",u:" << u << ",r:" << r << ",R:" << x * x + y * y);
			initialAlloc->Add(Vector(fx + x * u, fy + y * u, 0.));
			mobility.SetPositionAllocator(initialAlloc);
		}
		break;
	default:
		break;
	}
	mobility.Install(l_Nodes);
}

// 安装网络协议栈
void MeshRouteClass::InstallInternetStack()
{
	// 在全部结点上安装Internet协议栈
	InternetStackHelper internetStack;
	internetStack.Install(l_Nodes);
	// 设置IP地址和设备接口
	Ipv4AddressHelper address;
	address.SetBase("192.168.1.0", "255.255.255.0");
	l_Interfaces = address.Assign(l_NetDevices);
}

// 安装应用层
void MeshRouteClass::InstallApplicationRandom()
{
	// 不使用随机应用层配置
	if (m_ApplicationNum == 0)
	{
		if (m_SourceNum < 0 || m_SourceNum >= l_NodeNum) m_SourceNum = 0;
		if (m_DestinationNum < 0 || m_DestinationNum >= l_NodeNum || m_DestinationNum == m_SourceNum) m_DestinationNum = l_NodeNum - 1;
		m_SourceNum = m_Size + 1;
		m_DestinationNum = m_Size * (m_Size - 1) - 2;

		BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(l_Interfaces.GetAddress(m_DestinationNum), 49002));
		source.SetAttribute("MaxBytes", UintegerValue(m_MaxBytes));
		ApplicationContainer sourceApps = source.Install(l_Nodes.Get(m_SourceNum));
		sourceApps.Start(Seconds(MIN_APPLICATION_TIME));
		sourceApps.Stop(Seconds(m_TotalTime - END_APPLICATION_TIME));

		PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(l_Interfaces.GetAddress(m_DestinationNum), 49002));
		ApplicationContainer sinkApps = sink.Install(l_Nodes.Get(m_DestinationNum));
		sinkApps.Start(Seconds(MIN_APPLICATION_TIME));
		sinkApps.Stop(Seconds(m_TotalTime - END_APPLICATION_TIME));

		if (m_ProtocolType == MY11S_PMTMGMP)
		{
			DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(m_SourceNum)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Access_Point);
			DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(m_DestinationNum)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Portal);
		}
		return;
	}

	// 设置应用层参数
	Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(m_PacketSize));
	Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(m_DataRate));

	Ptr<UniformRandomVariable> rand_Nodes = CreateObject<UniformRandomVariable>();
	rand_Nodes->SetAttribute("Min", DoubleValue(0));
	rand_Nodes->SetAttribute("Max", DoubleValue(l_NodeNum));

	Ptr<UniformRandomVariable> rand_Port = CreateObject<UniformRandomVariable>();
	rand_Port->SetAttribute("Min", DoubleValue(49000));
	rand_Port->SetAttribute("Max", DoubleValue(49100));

	Ptr<UniformRandomVariable> rand_StarTime = CreateObject<UniformRandomVariable>();
	rand_StarTime->SetAttribute("Min", DoubleValue(MIN_APPLICATION_TIME));
	rand_StarTime->SetAttribute("Max", DoubleValue(m_TotalTime - END_APPLICATION_TIME));

	Ptr<ExponentialRandomVariable>rand_DurationTime = CreateObject<ExponentialRandomVariable>();
	rand_DurationTime->SetAttribute("Mean", DoubleValue(30));

	// 使用随机应用层配置
	for (int i = 0; i < m_ApplicationNum; i++){
		ApplicationContainer apps;
		int appStartTime = rand_StarTime->GetValue();
		int appDurationTime = rand_DurationTime->GetValue() + 1;
		int appStopTime = 0;

		// 应用起止时间
		if ((appStartTime + appDurationTime) >(m_TotalTime - 10)){
			appStopTime = m_TotalTime - 10;
		}
		else{
			appStopTime = appStartTime + appDurationTime;
		}

		// 应用连接名
		char num[2];
		char onoff[7];
		char sink[6];
		strcpy(onoff, "onoff");
		strcpy(sink, "sink");
		sprintf(num, "%d", i);
		strcat(onoff, num);
		strcat(sink, num);

		//随机选择源节点和目标结点
		int sourceNum = rand_Nodes->GetValue();
		int destinationNUM = rand_Nodes->GetValue();
		while (sourceNum == destinationNUM) // 避免选用相同节点
		{
			destinationNUM = rand_Nodes->GetValue();
		}
		uint32_t destination_Port = rand_Port->GetInteger();

		// 输出相关信息
		NS_LOG_INFO("\n\t Node " << sourceNum << " to " << destinationNUM);
		NS_LOG_INFO("\n Start_time: " << appStartTime << "s");
		NS_LOG_INFO("\n Stop_time: " << appStopTime << "s\n");

		// 定义应用层
		OnOffHelper onOffAPP("ns3::UdpSocketFactory", Address(InetSocketAddress(l_Interfaces.GetAddress(destinationNUM), destination_Port)));
		onOffAPP.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
		onOffAPP.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
		apps = onOffAPP.Install(l_Nodes.Get(sourceNum));
		apps.Start(Seconds(appStartTime));
		apps.Stop(Seconds(appStopTime));
		PacketSinkHelper sinkAPP("ns3::UdpSocketFactory", InetSocketAddress(l_Interfaces.GetAddress(destinationNUM), 49001));
		apps = sinkAPP.Install(l_Nodes.Get(destinationNUM));
		apps.Start(Seconds(1.0));
		if (m_ProtocolType == MY11S_PMTMGMP)
		{
			DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(sourceNum)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Access_Point);
			DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(destinationNUM)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Portal);
		}
	}
}

// 数据统计模块配置
void MeshRouteClass::StatsDataSet()
{
	//输出配置
	Config::Connect("/NodeList/*/DeviceList/*/RoutingProtocol/RouteDiscoveryTime", MakeCallback(&MeshRouteClass::CallBack_RouteDiscoveryTime, this));
	Config::Connect("/NodeList/*/ApplicationList/*/$ns3::BulkSendApplication/Tx", MakeCallback(&MeshRouteClass::CallBack_ApplicationTX, this));

	//Gnuplot图表输出
	string probeType = "ns3::PacketProbe";
	string tracePath = "/NodeList/*/ApplicationList/*/$ns3::BulkSendApplication/Tx";

	GnuplotHelper plotHelper;
	plotHelper.ConfigurePlot("Mesh-Route-Test-Packet-Byte-Count", "Packet Byte Count vs. Time", "Time (Seconds)", "Packet Byte Count");
	plotHelper.PlotProbe(probeType, tracePath, "OutputBytes", "Packet Byte Count", GnuplotAggregator::KEY_BELOW);

	FileHelper fileHelper;
	fileHelper.ConfigureFile("Mesh-Route-Test", FileAggregator::FORMATTED);
	fileHelper.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
	fileHelper.WriteProbe(probeType, tracePath, "OutputBytes");
}

// 输出流量数据
void MeshRouteClass::FlowMonitorReport()
{
	// 定义统计应用的变量
	int k = 0;
	int totaltxPackets = 0;
	int totalrxPackets = 0;
	double totaltxbytes = 0;
	double totalrxbytes = 0;

	double totaldelay = 0;
	double totalrxbitrate = 0;
	double difftx, diffrx;
	double pdf_value, rxbitrate_value, txbitrate_value, delay_value;
	double pdf_total, rxbitrate_total, delay_total;
	// 输出每个流量数据
	l_Monitor->CheckForLostPackets();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(l_Flowmon.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = l_Monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
		difftx = i->second.timeLastTxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
		diffrx = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstRxPacket.GetSeconds();
		pdf_value = (double)i->second.rxPackets / (double)i->second.txPackets * 100;
		txbitrate_value = (double)i->second.txBytes * 8 / 1024 / difftx;
		if (i->second.rxPackets != 0){
			rxbitrate_value = (double)i->second.rxPackets * m_PacketSize * 8 / 1024 / diffrx;
			delay_value = (double)i->second.delaySum.GetSeconds() / (double)i->second.rxPackets;
		}
		else{
			rxbitrate_value = 0;
			delay_value = 0;
		}
		// 数据流量
		if ((!t.destinationAddress.IsSubnetDirectedBroadcast("255.255.255.0")))
		{
			//输出数据流量
			k++;
			NS_LOG_INFO("\nFlow " << k << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n");
			NS_LOG_INFO("PDF: " << pdf_value << " %\n");
			NS_LOG_INFO("Tx Packets: " << i->second.txPackets << "\n");
			NS_LOG_INFO("Rx Packets: " << i->second.rxPackets << "\n");
			NS_LOG_INFO("Lost Packets: " << i->second.lostPackets << "\n");
			NS_LOG_INFO("Dropped Packets: " << i->second.packetsDropped.size() << "\n");
			NS_LOG_INFO("PDF: " << pdf_value << " %\n");
			NS_LOG_INFO("Average delay: " << delay_value << "s\n");
			NS_LOG_INFO("Rx bitrate: " << rxbitrate_value << " kbps\n");
			NS_LOG_INFO("Tx bitrate: " << txbitrate_value << " kbps\n\n");
			// 统计平均值
			totaltxPackets += i->second.txPackets;
			totaltxbytes += i->second.txBytes;
			totalrxPackets += i->second.rxPackets;
			totaldelay += i->second.delaySum.GetSeconds();
			totalrxbitrate += rxbitrate_value;
			totalrxbytes += i->second.rxBytes;
		}
	}
	// 平均全部结点数据
	if (totaltxPackets != 0){
		pdf_total = (double)totalrxPackets / (double)totaltxPackets * 100;
	}
	else{
		pdf_total = 0;
	}
	if (totalrxPackets != 0){
		rxbitrate_total = totalrxbitrate;
		delay_total = (double)totaldelay / (double)totalrxPackets;
	}
	else{
		rxbitrate_total = 0;
		delay_total = 0;
	}
	// 输出全部结点数据
	NS_LOG_INFO("\nTotal PDF: " << pdf_total << " %\n");
	NS_LOG_INFO("Total Rx bitrate: " << rxbitrate_total << " kbps\n");
	NS_LOG_INFO("Total Delay: " << delay_total << " s\n");
	// 输出全部结点数据到文件
	std::ostringstream os;
	os << "1_HWMP_PDF.txt";
	std::ofstream of(os.str().c_str(), ios::out | ios::app);
	of << pdf_total << "\n";
	of.close();
	std::ostringstream os2;
	os2 << "1_HWMP_Delay.txt";
	std::ofstream of2(os2.str().c_str(), ios::out | ios::app);
	of2 << delay_total << "\n";
	of2.close();
	std::ostringstream os3;
	os3 << "1_HWMP_Throu.txt";
	std::ofstream of3(os3.str().c_str(), ios::out | ios::app);
	of3 << rxbitrate_total << "\n";
	of3.close();
	Simulator::Destroy();
}

// 开始测试
int MeshRouteClass::Run()
{
	// 通过配置计算基本参数
	SimulatorSetting();
	// 创建节点及相关协议设置
	CreateNodes();
	// 节点位置设置
#ifndef TEST_LOCATION
	LocateNodes();
#else
	MobilityHelper mobility;

	Ptr<ListPositionAllocator> initialAlloc = CreateObject<ListPositionAllocator>();
	initialAlloc->Add(Vector(0, 0, 0));
	mobility.SetPositionAllocator(initialAlloc);
	initialAlloc->Add(Vector(84, 84, 0.));
	mobility.SetPositionAllocator(initialAlloc);
	initialAlloc->Add(Vector(84, -84, 0.));
	mobility.SetPositionAllocator(initialAlloc);
	for (int i = 3; i < l_NodeNum; i++)
	{
		initialAlloc->Add(Vector(120 * (i - 3) + 168, 0, 0.));
		mobility.SetPositionAllocator(initialAlloc);
	}
	mobility.Install(l_Nodes);
#endif
	// 安装网络协议栈
	InstallInternetStack();
	// 安装应用层
#ifndef NO_APPLICATION
	InstallApplicationRandom();
#else
	if (m_SourceNum < 0 || m_SourceNum >= l_NodeNum) m_SourceNum = 0;
	if (m_DestinationNum < 0 || m_DestinationNum >= l_NodeNum || m_DestinationNum == m_SourceNum) m_DestinationNum = l_NodeNum - 1;
	m_SourceNum = m_Size + 1;
	m_DestinationNum = m_Size * (m_Size - 1) - 2;
	if (m_ProtocolType == MY11S_PMTMGMP)
	{
		/*DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(m_SourceNum)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Access_Point);*/
		DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(0)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Access_Point);
		/*DynamicCast<my11s::PmtmgmpProtocol>(DynamicCast<WmnPointDevice>(l_Nodes.Get(m_DestinationNum)->GetDevice(0))->GetRoutingProtocol())->SetNodeType(my11s::PmtmgmpProtocol::Mesh_Portal);*/
	}
#endif
	// 数据统计模块配置
	void StatsDataSet();

	//Ptr<WmnPointDevice> wpd = DynamicCast<WmnPointDevice>(l_Nodes.Get(0)->GetDevice(0));
	//Ptr<my11s::PmtmgmpProtocol> pp = DynamicCast<my11s::PmtmgmpProtocol>(wpd->GetRoutingProtocol());

	//流量监测
	l_Monitor = l_Flowmon.InstallAll();

	float timeStart = clock();
	Simulator::Schedule(Seconds(timeStart), &MeshRouteClass::Report, this);
	Simulator::Stop(Seconds(m_TotalTime));
	Simulator::Run();
	Simulator::Destroy();

	// 输出流量数据
	FlowMonitorReport();
	return 0;
}

// 回调函数
void MeshRouteClass::CallBack_RouteDiscoveryTime(string context, Time time)
{
	NS_LOG_INFO("RouteDiscoveryTime:" << time.GetSeconds() << std::endl);
}
void MeshRouteClass::CallBack_ApplicationTX(string context, Ptr<const Packet> packet)
{
	NS_LOG_INFO("Packet Size:" << packet->GetSize() << std::endl);
}

//输出报告
void MeshRouteClass::Report()
{
	unsigned n(0);
	for (NetDeviceContainer::Iterator i = l_NetDevices.Begin();
		i != l_NetDevices.End(); ++i, ++n)
	{
		std::ostringstream os;
		//os << "mp-report1-" << n << ".xml";
		os << "mp-report.xml";
		std::ofstream of;
		of.open(os.str().c_str(), ios::out | ios::app);
		if (!of.is_open())
		{
			NS_LOG_INFO("Error: Can't open file " << os.str() << "\n");
			return;
		}
		switch (m_ProtocolType)
		{
		case MeshRouteClass::DOT11S_HWMP:
			l_Mesh.Report(*i, of);
			break;
		case MeshRouteClass::MY11S_PMTMGMP:
			l_Pmtmgmp.Report(*i, of);
			break;
		default:
			break;
		}
		of.close();
	}
	n = 0;
}
int main(int argc, char *argv[])
{
#ifdef OUT_TO_FILE
#ifdef WIN32
	ofstream logfile("E:\\Ns-3.23-log.log");
	std::clog.rdbuf(logfile.rdbuf());
#endif
#endif

	//LogComponentEnableAll((LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_ALL));

	LogComponentEnable("PmtmgmpProtocol", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	//LogComponentEnable("WmnPointDevice", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	LogComponentEnable("PmtmgmpRtable", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	//LogComponentEnable("WmnWifiInterfaceMac", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	//LogComponentEnable("PmtmgmpProtocolMac", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	//LogComponentEnable("PmtmgmpPeerManagementProtocol", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	//LogComponentEnable("HwmpProtocol", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));
	//LogComponentEnable("MeshPointDevice", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));

	//LogComponentEnable("BulkSendApplication", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));

	LogComponentEnable("MeshRouteTest", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_ALL));

	PacketMetadata::Enable();
	MeshRouteClass t;
	t.Configure(argc, argv);
	return t.Run();
}
