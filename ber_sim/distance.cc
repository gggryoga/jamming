#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/error-model.h"
#include "ns3/ipv4.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BerDistanceSimulation");

void PacketReceivedCallback (Ptr<OutputStreamWrapper> stream, Ptr<Node> senderNode, Ptr<Node> receiverNode, Ptr<const Packet> packet, const Address &from, const Address &to) {
    static uint32_t receivedPackets = 0;
    receivedPackets++;

    Ptr<MobilityModel> senderMobility = senderNode->GetObject<MobilityModel>();
    Ptr<MobilityModel> receiverMobility = receiverNode->GetObject<MobilityModel>();
    double distance = receiverMobility->GetDistanceFrom(senderMobility);

    *stream->GetStream () << Simulator::Now ().GetSeconds () << ","
                          << receivedPackets << ","
                          << distance << std::endl;
}

int main (int argc, char *argv[])
{
    double simulationTime = 400.0;
    double nodeSpeed = 1.0; // Speed in m/s

    CommandLine cmd;
    cmd.Parse (argc, argv);

    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetRate(1e-5);
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (simulationTime));

    UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (10000));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
    clientApps.Start (Seconds (1.0));
    clientApps.Stop (Seconds (simulationTime));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));  // Sender node position
    positionAlloc->Add (Vector (10.0, 0.0, 0.0)); // Initial position of receiver node
    mobility.SetPositionAllocator (positionAlloc);

    mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
    mobility.Install (nodes);

    Ptr<ConstantVelocityMobilityModel> mobModel = nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
    mobModel->SetVelocity(Vector(nodeSpeed, 0.0, 0.0));

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("ber_distance_simulation.csv");
    *stream->GetStream () << "Time,PacketsReceived,Distance" << std::endl;

    Ptr<UdpEchoServer> server = serverApps.Get(0)->GetObject<UdpEchoServer>();
    server->TraceConnectWithoutContext("RxWithAddresses", MakeBoundCallback(&PacketReceivedCallback, stream, nodes.Get(0), nodes.Get(1)));

    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}

