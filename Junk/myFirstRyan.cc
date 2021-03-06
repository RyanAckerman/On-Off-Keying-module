#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingEx2");

void SendStuff(Ptr<Socket> sock, Ipv4Address dstaddr, uint16_t port);
void BindSock(Ptr<Socket> sock, Ptr<NetDevice> netdev);
void srcSocketRecv(Ptr<Socket> Socket);
void dstSocketRecv(Ptr<Socket> Socket);

int main (int argc, char *argv[])
{

Ptr<Node> Src = CreateObject<Node> ();
Ptr<Node> Relay = CreateObject<Node> ();
Ptr<Node> Dst = CreateObject<Node> ();
NodeContainer c = NodeContainer (Src,Relay,Dst);

InternetStackHelper internet;
internet.Install(c);

PointToPointHelper p2p;
p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
p2p.SetChannelAttribute("Delay", StringValue("2ms"));
NetDeviceContainer dSrc = p2p.Install(Src , Relay);
NetDeviceContainer dDst = p2p.Install(Dst , Relay);


Ptr<NetDevice> ndRelay = dSrc.Get(1);
Ptr<NetDevice> ndRelay2 = dDst.Get(1);
Ptr<NetDevice> ndSrc = dSrc.Get(0);
Ptr<NetDevice> ndDst = dDst.Get(0);



Ipv4AddressHelper ipv4;
ipv4.SetBase("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer iSrc = ipv4.Assign(dSrc);
ipv4.SetBase("10.1.2.0", "255.255.255.0");
Ipv4InterfaceContainer iDst = ipv4.Assign(dDst);


Ptr<Ipv4> ipv4Src = Src->GetObject<Ipv4>();
Ptr<Ipv4> ipv4Relay = Relay->GetObject<Ipv4>();
Ptr<Ipv4> ipv4Dst = Dst->GetObject<Ipv4>();

Ipv4StaticRoutingHelper ipv4RoutingHelper;
Ptr<Ipv4StaticRouting> staticRoutingSrc = ipv4RoutingHelper.GetStaticRouting(ipv4Src);
Ptr<Ipv4StaticRouting> staticRoutingRelay = ipv4RoutingHelper.GetStaticRouting(ipv4Relay);
Ptr<Ipv4StaticRouting> staticRoutingDst = ipv4RoutingHelper.GetStaticRouting(ipv4Dst);

staticRoutingSrc->AddHostRouteTo(Ipv4Address("10.1.2.1"), Ipv4Address("10.1.1.2"), 1,2);
staticRoutingRelay->AddHostRouteTo(Ipv4Address("10.1.2.1"), Ipv4Address("10.1.2.1"), 1,1);
staticRoutingDst->AddHostRouteTo(Ipv4Address("10.1.1.1"), Ipv4Address("10.1.2.2"), 1,2);
staticRoutingRelay->AddHostRouteTo(Ipv4Address("10.1.1.1"), Ipv4Address("10.1.1.1"), 1,1);

Ptr<Socket> srcSocket = Socket::CreateSocket(Src,TypeId::LookupByName("ns3::UdpSocketFactory"));
srcSocket->Bind();
srcSocket->SetRecvCallback (MakeCallback(&srcSocketRecv));

Ptr<Socket> dstSocket = Socket::CreateSocket(Dst, TypeId::LookupByName("ns3::UdpSocketFactory"));
uint16_t dstport = 12345;
Ipv4Address dstaddr ("10.1.2.1");
InetSocketAddress dstsa = InetSocketAddress(dstaddr, dstport);
dstSocket->Bind(dstsa);
dstSocket->SetRecvCallback(MakeCallback(&dstSocketRecv));

AsciiTraceHelper ascii;
p2p.EnableAsciiAll(ascii.CreateFileStream ("RoutingEx2.tr"));
p2p.EnablePcapAll("RoutingEx2");

LogComponentEnableAll(LOG_PREFIX_TIME);
LogComponentEnable("RoutingEx2", LOG_LEVEL_INFO);

Simulator::Schedule(Seconds(0.1), &SendStuff,srcSocket, dstaddr, dstport);
Simulator::Run();
Simulator::Destroy();

return 0;

}



void SendStuff (Ptr<Socket> sock, Ipv4Address dstaddr, uint16_t port)
   {
     std::cout<<"SEND"<<std::endl;
     Ptr<Packet> p = Create<Packet> ();
     p->AddPaddingAtEnd (100);
     sock->SendTo (p, 0, InetSocketAddress (dstaddr,port));
     return;
   }
   
void BindSock (Ptr<Socket> sock, Ptr<NetDevice> netdev)
   {
     sock->BindToNetDevice (netdev);
     return;
   }

void srcSocketRecv(Ptr<Socket> socket)
{
 Address from;
     Ptr<Packet> packet = socket->RecvFrom (from);
     packet->RemoveAllPacketTags ();
     packet->RemoveAllByteTags ();
     NS_LOG_INFO ("Source Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());
     if (socket->GetBoundNetDevice ())
       {
         NS_LOG_INFO ("Socket was bound");
       } 
     else
       {
         NS_LOG_INFO ("Socket was not bound");
       }

}
void dstSocketRecv(Ptr<Socket> socket)
{
  Address from;
    Ptr<Packet> packet = socket->RecvFrom (from);
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
    NS_LOG_INFO ("Destination Received " << packet->GetSize () << " bytes from " << address.GetIpv4 ());
    NS_LOG_INFO ("Triggering packet back to source node's interface 1");
    SendStuff (socket, Ipv4Address ("10.1.1.1"), address.GetPort ());

}
