// Copyright 2020 Alan Tracey Wootton
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef NANO_MQTT_H // include guard
#define NANO_MQTT_H

#include <stdint.h>
#include "slices.h"

namespace nano
{

// After we parse a Pub/Sub packet we'll end up with a collection
// of slices for the various parts.
// Since publish is a superset of the other packets we can use this struct.
// to construct all the packets.
// Note that mqttPacketPieces does not own a buffer.
// sizeof(mqttPacketPieces) is 100 bytes built by Arduino. slices are 8 bytes.
struct mqttPacketPieces
{
    slice TopicName;
    slice Payload;

    slice RespTopic;     // one prop
    slice UserKeyVal[8]; // user props. 4 pair max. no hash table here.
    // ignoring the rest of the props.

    uint16_t PacketID; // not a nonce
    char QoS;          // used by sub, parsed
    unsigned char packetType;

    slice props;       // the whole properties block.

    // zero the slices before parse
    void reset();

    bool parse(slice body, unsigned char packetType, int len);

    // uses outputBuffer for assembly and then writes it to net.
    bool outputPubOrSub(sink outputBuffer, drain *net);

    bool outputConnect(sink outputBuffer, drain *net,
                       slice clientID, slice user, slice pass);

    // return a value if key found else return a 'done' slice.
    slice findKey(const char *key);

    int UserKeyVal_len()
    {
        return *(&UserKeyVal + 1) - UserKeyVal;
    }

    // what about username and password for connect?
    // slice *username()
    // {
    //     return &UserKeyVal[0];
    // }
    // slice *password()
    // {
    //     return &UserKeyVal[1];
    // }
};

struct mqttBuffer 
{
private:
    char buffer[1024];

public:
    slice getSlice()
    {
        slice s;
        s.base = buffer;
        s.start = 0;
        s.end = sizeof(buffer);
        return s;
    }

    sink getSink()
    {
        sink s;
        s.base = buffer;
        s.start = 0;
        s.end = sizeof(buffer);
        return s;
    };

    slice loadHexString(const char *hexstr);   // return slice
    slice loadFromFount(fount &f, int amount); // return slice
};

const char CtrlConn = 1;      // Connect
const char CtrlConnAck = 2;   // connect ack
const char CtrlPublish = 3;   // Publish
const char CtrlPubAck = 4;    // publish ack
const char CtrlPubRecv = 5;   // publish received
const char CtrlPubRel = 6;    // publish release
const char CtrlPubComp = 7;   // publish complete
const char CtrlSubscribe = 8; // subscribe
const char CtrlSubAck = 9;    // subscribe ack
const char CtrlUnSub = 10;    // unsubscribe
const char CtrlUnSubAck = 11; // unsubscribe ack
const char CtrlPingReq = 12;  // ping request
const char CtrlPingResp = 13; // ping response
const char CtrlDisConn = 14;  // disconnect
const char CtrlAuth = 15;     // authentication (since MQTT 5)

unsigned char getPropertyLenCode(int i);

enum PropKeyType
{
    propKeyPayloadFormatIndicator = 1, // byte, Packet: Will, Publish
    propKeyMessageExpiryInterval = 2,  // Uint (4 bytes), Packet: Will, Publish
    propKeyContentType = 3,            // utf-8, Packet: Will, Publish
    propKeyRespTopic = 8,              // utf-8, Packet: Will, Publish
    propKeyCorrelationData = 9,        // binary data, Packet: Will, Publish
    propKeySubID = 11,                 // uint (variable bytes), Packet: Publish, Subscribe
    propKeySessionExpiryInterval = 17, // uint (4 bytes), Packet: Connect, ConnAck, DisConn
    propKeyAssignedClientID = 18,      // utf-8, Packet: ConnAck
    propKeyServerKeepalive = 19,       // uint (2 bytes), Packet: ConnAck
    propKeyAuthMethod = 21,            // utf-8, Packet: Connect, ConnAck, Auth
    propKeyAuthData = 22,              // binary data, Packet: Connect, ConnAck, Auth
    propKeyReqProblemInfo = 23,        // byte, Packet: Connect
    propKeyWillDelayInterval = 24,     // uint (4 bytes), Packet: Will
    propKeyReqRespInfo = 25,           // byte, Packet: Connect
    propKeyRespInfo = 26,              // utf-8, Packet: ConnAck
    propKeyServerRef = 28,             // utf-8, Packet: ConnAck, DisConn
    propKeyReasonString = 31,          // utf-8, Packet: ConnAck, PubAck, PubRecv, PubRel, PubComp, SubAck, UnSubAck, DisConn, Auth
    propKeyMaxRecv = 33,               // uint (2 bytes), Packet: Connect, ConnAck
    propKeyMaxTopicAlias = 34,         // uint (2 bytes), Packet: Connect, ConnAck
    propKeyTopicAlias = 35,            // uint (2 bytes), Packet: Publish
    propKeyMaxQos = 36,                // byte, Packet: ConnAck
    propKeyRetainAvail = 37,           // byte, Packet: ConnAck
    propKeyUserProps = 38,             // utf-8 string pair, Packet: Connect, ConnAck, Publish, Will, PubAck, PubRecv, PubRel, PubComp, Subscribe, SubAck, UnSub, UnSubAck, DisConn, Auth
    propKeyMaxPacketSize = 39,         // uint (4 bytes), Packet: Connect, ConnAck
    propKeyWildcardSubAvail = 40,      // byte, Packet: ConnAck
    propKeySubIDAvail = 41,            // byte, Packet: ConnAck
    propKeySharedSubAvail = 42,        // byte, Packet: ConnAck
};

} // namespace nano

#endif // NANO_MQTT_H