#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BerSimulation");

void SetBER(Ptr<RateErrorModel> em, double ber) {
    em->SetRate(ber);
    NS_LOG_UNCOND("BER changed to: " << ber);
}

void PacketReceivedCallback (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from, const Address &to) {
    static uint32_t receivedPackets = 0;
    receivedPackets++;
    *stream->GetStream () << Simulator::Now ().GetSeconds () << "," << receivedPackets << std::endl;
}

int main (int argc, char *argv[])
{
    double simulationTime = 400.0;

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

    Simulator::Schedule (Seconds (20.0), &SetBER, em, 1e-4);
    Simulator::Schedule (Seconds (40.0), &SetBER, em, 1e-3);
    Simulator::Schedule (Seconds (60.0), &SetBER, em, 1e-2);
    Simulator::Schedule (Seconds (80.0), &SetBER, em, 1e-1);
    Simulator::Schedule (Seconds (100.0), &SetBER, em, 1e-5);
    Simulator::Schedule (Seconds (120.0), &SetBER, em, 1e-4);
    Simulator::Schedule (Seconds (140.0), &SetBER, em, 1e-3);
    Simulator::Schedule (Seconds (160.0), &SetBER, em, 1e-2);
    Simulator::Schedule (Seconds (180.0), &SetBER, em, 1e-1);

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("received_packets.csv");
    *stream->GetStream () << "Time,PacketsReceived" << std::endl;

    Ptr<UdpEchoServer> server = serverApps.Get(0)->GetObject<UdpEchoServer>();
    server->TraceConnectWithoutContext("RxWithAddresses", MakeBoundCallback(&PacketReceivedCallback, stream));

    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
