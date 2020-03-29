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

#include "mqtt5nano.h"

namespace nano
{

bool mqttPacketPieces::outputConnect(sink outputBuffer, drain *net,
                                     slice clientID, slice user, slice pass)
{
    // We're filling a buffer. We'll actually generate an array
    // of slices that are in the buffer that may have small gaps between them.

    //  setby caller:
    packetType = CtrlConn;
    QoS = 0;

    sink fixedHeader = outputBuffer;
    outputBuffer.writeByte(char(packetType * 16) + (QoS * 2));
    fixedHeader.end = outputBuffer.start;
    //
    outputBuffer.start += 4; // leave some space
    //
    sink varHeader = outputBuffer;
    outputBuffer.writeByte(0);// todo: shorten this.
    outputBuffer.writeByte(4);
    outputBuffer.writeByte('M');
    outputBuffer.writeByte('Q');
    outputBuffer.writeByte('T');
    outputBuffer.writeByte('T');
    outputBuffer.writeByte(5);
    outputBuffer.writeByte(0xC2); // c2 -- connect flags. user, pass, and clean start
    outputBuffer.writeByte(0);    // msb keep alive
    outputBuffer.writeByte(0x3C); // lsb keep alive
    //
    outputBuffer.writeByte(0); // length of props
                               // we could have props here. see outputPubOrSub
    varHeader.end = outputBuffer.start;
    //
    outputBuffer.start += 4; // leave some space
    //
    sink payload = outputBuffer;
    outputBuffer.writeFixedLenStr(clientID);
    outputBuffer.writeFixedLenStr(user);
    outputBuffer.writeFixedLenStr(pass);
    payload.end = outputBuffer.start;

    // now the body length
    // at the tail of fixedHeader
    int bodylen = varHeader.size() + payload.size();
    sink tmp = fixedHeader;
    tmp.start = fixedHeader.end;
    tmp.end = tmp.start + 4;
    tmp.writeVarLenInt(bodylen);
    fixedHeader.end = tmp.start;

    if (outputBuffer.empty() == true)
    {
        return true; // failed
    }
    // now, write out fixedHeader,varHeader, payload
    bool fail = false;
    fail |= net->write(fixedHeader);
    fail |= net->write(varHeader);
    fail |= net->write(payload);

    return fail;
};

// Output a mqtt5 Subscribe packet using values previously set.
// outputBuffer is a buffer. The final result is copied to 'net'.
// we don't have to buffer payload.
// NOTE: when generating Subscribe packets the topic must be in the TopicName.
bool mqttPacketPieces::outputPubOrSub(sink outputBuffer, drain *net)
{
    // We're filling a buffer. We'll actually generate an array
    // of slices that are in the buffer that may have small gaps between them.

    // setby caller:  packetType = CtrlSubscribe;

    if (packetType == CtrlSubscribe)
    {
        Payload = TopicName;
        TopicName.base = 0;
    }

    sink fixedHeader = outputBuffer;

    outputBuffer.writeByte(char(packetType * 16) + (QoS * 2));
    fixedHeader.end = outputBuffer.start;
    //
    outputBuffer.start += 4; // leave some space
    //
    sink varHeader = outputBuffer;
    if (packetType == CtrlPublish)
    {
        outputBuffer.writeFixedLenStr(TopicName);
    }
    outputBuffer.writeByte(PacketID / 16); //  .
    outputBuffer.writeByte(PacketID);
    varHeader.end = outputBuffer.start;
    //
    outputBuffer.start += 4; // leave some space
    //
    sink props = outputBuffer;
    if (RespTopic.empty() == false)
    {
        outputBuffer.writeByte(propKeyRespTopic);
        outputBuffer.writeFixedLenStr(RespTopic);
    }
    for (int i = 0; i < UserKeyVal_len(); i += 2)
    {
        if (UserKeyVal[i].empty() == false)
        {
            outputBuffer.writeByte(propKeyUserProps);
            outputBuffer.writeFixedLenStr(UserKeyVal[i]);
            outputBuffer.writeFixedLenStr(UserKeyVal[i + 1]);
        }
    }
    props.end = outputBuffer.start;

    // don't buffer the payload.
    int payloadSize = Payload.size();
    if (packetType == CtrlSubscribe)
    {
        payloadSize += 2; // for it's length bytes
        payloadSize += 1; // because subscribe adds the qos
    }
    // put the lengths in.
    // put the props len at the tail of varHeader
    int propslen = props.size();
    sink tmp = varHeader;
    tmp.start = varHeader.end;
    tmp.end = tmp.start + 4;
    tmp.writeVarLenInt(propslen);
    varHeader.end = tmp.start;

    // now the body length
    // at the tail of fixedHeader
    int bodylen = varHeader.size() + props.size() + payloadSize;
    tmp = fixedHeader;
    tmp.start = fixedHeader.end;
    tmp.end = tmp.start + 4;
    tmp.writeVarLenInt(bodylen);
    fixedHeader.end = tmp.start;

    if (outputBuffer.empty() == true)
    {
        return true; // failed
    }

    // now, write out fixedHeader,varHeader, props, payload
    bool fail = false;
    fail |= net->write(fixedHeader);
    fail |= net->write(varHeader);
    fail |= net->write(props);

    if (packetType == CtrlSubscribe)
    {
        fail |= net->writeFixedLenStr(Payload);
        fail |= net->writeByte(QoS);
    }
    else
    { // packetType == CtrlPublish
        fail |= net->write(Payload);
    }
    return fail;
};

// return a value if key found else return a 'done' slice.
slice mqttPacketPieces::findKey(const char *key)
{
    int size = UserKeyVal_len();
    for (int i = 0; i < size; i += 2)
    {
        slice s = UserKeyVal[i];
        if (s.equals(key))
        {
            return s;
        }
    }
    slice bads;
    bads.base = 0;
    return bads;
};

slice mqttBuffer::loadFromFount(fount &f, int amount)
{
    bool ok = true;
    slice extent;
    extent.start = 0;
    extent.base = buffer;
    extent.end = 0;
    for (int i = 0; i < amount; i++)
    {
        extent.end = i;
        if (f.empty() == true)
        {
            return extent;
        }
        unsigned char c = f.readByte();
        buffer[extent.end] = c;
        extent.end++;
    };
    return extent;
};

slice mqttBuffer::loadHexString(const char *hexstr)
{
    slice extent;
    extent.start = 0;
    extent.base = buffer;
    int i = 0;
    for (; i < sizeof(buffer); i++)
    {
        if (hexstr[i * 2] == 0 || hexstr[i * 2 + 1] == 0)
        {
            extent.end = i;
            return extent;
        }
        char c1 = hexstr[i * 2];
        char c2 = hexstr[i * 2 + 1];
        unsigned char v1;
        unsigned char v2;
        if (c1 >= '0' && c1 <= '9')
        {
            v1 = c1 - '0';
        }
        else
        {
            v1 = c1 - 'a' + 10;
        }
        if (c2 >= '0' && c2 <= '9')
        {
            v2 = c2 - '0';
        }
        else
        {
            v2 = c2 - 'a' + 10;
        }
        buffer[i] = (v1 * 16) + v2;
    };
    extent.end = i;
    return extent;
};

// PropKeyConsumes is to look up a code for every prop key. The code will be how many bytes to pass, in the lower
// nibble or else how many strings to pass in the upper nibble.
char PropKeyConsumes[43];

void init()
{
    PropKeyConsumes[propKeyPayloadFormatIndicator] = 0x01; // byte, Packet: Will, Publish
    PropKeyConsumes[propKeyMessageExpiryInterval] = 0x04;  // Uint (4 bytes), Packet: Will, Publish
    PropKeyConsumes[propKeyContentType] = 0x10;            // utf-8, Packet: Will, Publish
    PropKeyConsumes[propKeyRespTopic] = 0x10;              // utf-8, Packet: Will, Publish
    PropKeyConsumes[propKeyCorrelationData] = 0x10;        // binary data, Packet: Will, Publish
    PropKeyConsumes[propKeySubID] = 0x0F;                  // uint (variable bytes), Packet: Publish, Subscribe
    PropKeyConsumes[propKeySessionExpiryInterval] = 0x04;  // uint (4 bytes), Packet: Connect, ConnAck, DisConn
    PropKeyConsumes[propKeyAssignedClientID] = 0x10;       // utf-8, Packet: ConnAck
    PropKeyConsumes[propKeyServerKeepalive] = 0x02;        // uint (2 bytes), Packet: ConnAck
    PropKeyConsumes[propKeyAuthMethod] = 0x10;             // utf-8, Packet: Connect, ConnAck, Auth
    PropKeyConsumes[propKeyAuthData] = 0x10;               // binary data, Packet: Connect, ConnAck, Auth
    PropKeyConsumes[propKeyReqProblemInfo] = 0x01;         // byte, Packet: Connect
    PropKeyConsumes[propKeyWillDelayInterval] = 0x04;      // uint (4 bytes), Packet: Will
    PropKeyConsumes[propKeyReqRespInfo] = 0x01;            // byte, Packet: Connect
    PropKeyConsumes[propKeyRespInfo] = 0x10;               // utf-8, Packet: ConnAck
    PropKeyConsumes[propKeyServerRef] = 0x10;              // utf-8, Packet: ConnAck, DisConn
    PropKeyConsumes[propKeyReasonString] = 0x10;           // utf-8, Packet: ConnAck, PubAck, PubRecv, PubRel, PubComp, SubAck, UnSubAck, DisConn, Auth
    PropKeyConsumes[propKeyMaxRecv] = 0x02;                // uint (2 bytes), Packet: Connect, ConnAck
    PropKeyConsumes[propKeyMaxTopicAlias] = 0x02;          // uint (2 bytes), Packet: Connect, ConnAck
    PropKeyConsumes[propKeyTopicAlias] = 0x02;             // uint (2 bytes), Packet: Publish
    PropKeyConsumes[propKeyMaxQos] = 0x01;                 // byte, Packet: ConnAck
    PropKeyConsumes[propKeyRetainAvail] = 0x01;            // byte, Packet: ConnAck
    PropKeyConsumes[propKeyUserProps] = 0x20;              // utf-8 string pair, Packet: Connect, ConnAck, Publish, Will, PubAck, PubRecv, PubRel, PubComp, Subscribe, SubAck, UnSub, UnSubAck, DisConn, Auth
    PropKeyConsumes[propKeyMaxPacketSize] = 0x04;          // uint (4 bytes), Packet: Connect, ConnAck
    PropKeyConsumes[propKeyWildcardSubAvail] = 0x01;       // byte, Packet: ConnAck
    PropKeyConsumes[propKeySharedSubAvail] = 0x01;         // byte, Packet: ConnAck
}

unsigned char getPropertyLenCode(int i)
{
    if (PropKeyConsumes[0] == 0)
    {
        init();
    }
    if (i <= sizeof(PropKeyConsumes))
    {
        return PropKeyConsumes[i];
    }
    return 0xFF;
};

bool mqttPacketPieces::parse(slice body, unsigned char _packetType, int len)
{
    bool fail = false;
    reset();

    packetType = (_packetType >> 4);
    QoS = (_packetType > 1) & 3;

    if (packetType < CtrlConn || packetType > CtrlAuth  ){
        fail = true;
        return fail;
    }

    slice pos = body;
    //pos.printhex();
    if (packetType == CtrlPublish)
    {
        // ok. parse a pub.
        TopicName = pos.getFixedLenString();
        //TopicName.printstr();
        //pos.printhex();
        if (QoS)
        {
            PacketID = pos.getFixLenInt();
        }
        int propLen = pos.getVarLenInt();

        props.base = pos.base;
        props.start = pos.start;
        props.end = props.start + propLen;
        pos.start = props.end;

        //props.printhex();
        if (props.size())
        {
            int userIndex = 0;
            int maxUserIndex = UserKeyVal_len();
            // parse the props
            slice ptmp = props;
            while (ptmp.empty() == false)
            {
                int key = ptmp.readByte();
                if (key == propKeyRespTopic)
                {
                    RespTopic = ptmp.getFixedLenString();
                    //RespTopic.printstr("resp");
                }
                else if (key == propKeyUserProps)
                {
                    slice k = ptmp.getFixedLenString();
                    //k.printstr();
                    slice v = ptmp.getFixedLenString();
                    //v.printstr();
                    if (userIndex < maxUserIndex)
                    {
                        UserKeyVal[userIndex] = k;
                        UserKeyVal[userIndex + 1] = v;
                        userIndex += 2;
                    }
                }
                else
                {
                    // what happens now?
                    char code = getPropertyLenCode(key);
                    if (code & 0x0F)
                    {
                        if (code == char(0xFF))
                        {
                            fail = true;
                            return fail; // we're done and broken.
                        }
                        if (code == 0x0F)
                        {
                            int dummy = ptmp.getVarLenInt();
                        }
                        else
                        { // pass 'code' bytes.
                            ptmp.start += code;
                        }
                    }
                    else
                    {
                        code = code / 16;
                        // pass 'code' strings.
                        for (int i = 0; i < code; i++)
                        {
                            slice aslice = ptmp.getFixedLenString();
                        }
                    }
                }
            }
        }

        Payload = pos;
        // and we're done.
    }
    else
    {
        // worry about the body of the other packets later.
    }

    return fail;
}


void mqttPacketPieces::reset()
{
    for (int i = 0; i < UserKeyVal_len(); i++)
    {
        UserKeyVal[i].base = 0;
    }
    TopicName.base = 0;
    Payload.base = 0;
    props.base = 0;
    RespTopic.base = 0;
    QoS = 0;
}

} // namespace nano
