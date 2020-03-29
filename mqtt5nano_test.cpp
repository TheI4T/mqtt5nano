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

#include <iostream>
#include <vector>
#include <string>

#include "mqtt5nano.h"

using namespace std;

/** This main() runs the tests that we have. 
 * The outout should be free of the word "FAIL".
 * We should assume that any mqtt functionality that is not in these tests is broken. 
 */

void testParsePublish();      // below
void testGenerateSubscribe(); // annoying
void testGeneratePublish();   // need go now.
void testGenerateConnect();   // declared below
void testParseMisc();         // declared below

int main()
{

    cout << "Hello World!\n";

    // we need to parse a pub and generate a pub.
    // we need to be able to generate a sub and generate a connect.
    // we need to not barf, ever ever ever, when receiving garbage.
    using namespace nano;

    testParseMisc();

    testGenerateConnect();

    testGeneratePublish();

    testGenerateSubscribe();

    testParsePublish();
}

using namespace nano;

bool parse(mqttPacketPieces &parser, mqttBuffer &parserBuffer, fount *net)
{
    bool fail = false;

    while (!net->empty())
    {

        unsigned char packetType = net->readByte();
        int len = net->getVarLenInt();

        if (len > 1024 - 4)
        {
            fail = true;
            return fail;
        }
        if (len == 0) // is legit
        {
            fail = false;
            return fail;
        }

        // now we can read the rest
        //  mqttBuffer parserBuffer;
        slice body = parserBuffer.loadFromFount(*net, len);

        fail = parser.parse(body, packetType, len);
        if (fail)
        {
            cout << "FAIL in parse \n";
            return fail;
        }
    }
    return fail;
}

void testParseMisc()
{
    using namespace nano;

    mqttBuffer testbuffer;   // to supply the fount
    mqttBuffer parserBuffer; // for use by the parser.

    mqttPacketPieces parser; // the parser.
    sliceFount net;

    const char *pubbytes = "200600000322000a";

    net.src = testbuffer.loadHexString(pubbytes);

    bool failed = parse(parser, parserBuffer, &net);

    if (failed)
    {
        cout << "FAIL of some kind \n";
    }

    pubbytes = "900400010000";

    net.src = testbuffer.loadHexString(pubbytes);

    failed = parse(parser, parserBuffer, &net);

    if (failed)
    {
        cout << "FAIL of some kind \n";
    }

    pubbytes = "40020004";

    net.src = testbuffer.loadHexString(pubbytes);

    failed = parse(parser, parserBuffer, &net);

    if (failed)
    {
        cout << "FAIL of some kind \n";
    }

    // now, all in a row.
    pubbytes = "200600000322000a90040001000040020004";

    net.src = testbuffer.loadHexString(pubbytes);

    failed = parse(parser, parserBuffer, &net);

    if (failed)
    {
        cout << "FAIL of some kind \n";
    }

    cout << "testParseMisc finished";
}

void testGenerateConnect()
{
    using namespace nano;

    slice s;
    mqttPacketPieces congen;
    congen.reset();

    mqttBuffer buffer;
    sink buffersink = buffer.getSink();

    mqttBuffer result;
    sinkDrain netDrain;
    netDrain.src = result.getSink();

    slice clientID = slice("client-id-uwjnbnegfgtwfqk");
    slice userName = slice("abc");
    slice passWord = slice("123");

    bool ok = congen.outputConnect(buffersink, &netDrain, clientID, userName, passWord);

    slice rsl = netDrain.src.getWritten();
    char tmpbuffer[1024];

    // these bytes came from gmqtt and were tested on mosquitto.
    // should be:
    const char *need = "103000044d51545405c2003c000019636c69656e742d69642d75776a6e626e65676667747766716b00036162630003313233";
    const char *got = rsl.gethexstr(tmpbuffer);
    if (strcmp(got, need) != 0)
    {
        cout << "testGenerateConnect FAIL: expected --" << need << "-- got --" << got << "--\n";
    }
}

void testGeneratePublish()
{
    using namespace nano;

    slice s;
    mqttPacketPieces pubgen;
    pubgen.reset();

    pubgen.QoS = 1;
    pubgen.packetType = CtrlPublish;

    pubgen.TopicName = slice("TEST/TIMEabcd");
    pubgen.PacketID = 3;
    pubgen.RespTopic = slice("TEST/TIMEefghijk");

    pubgen.UserKeyVal[0] = slice("key1");
    pubgen.UserKeyVal[1] = slice("val1");
    pubgen.UserKeyVal[2] = slice("key2");
    pubgen.UserKeyVal[3] = slice("val2");
    pubgen.Payload = slice("message at 2020-03-27 01:35:37.403079 c=1");

    mqttBuffer buffer;
    sink buffersink = buffer.getSink();

    mqttBuffer result;
    sinkDrain netDrain;
    netDrain.src = result.getSink();

    bool ok = pubgen.outputPubOrSub(buffersink, &netDrain);

    slice rsl = netDrain.src.getWritten();
    char tmpbuffer[1024];

    // should be:
    // same as pubbytes in testParsePublish
    const char *need = "3268000d544553542f54494d456162636400032d080010544553542f54494d4565666768696a6b2600046b657931000476616c312600046b657932000476616c326d65737361676520617420323032302d30332d32372030313a33353a33372e34303330373920633d31";
    const char *got = rsl.gethexstr(tmpbuffer);
    if (strcmp(got, need) != 0)
    {
        cout << "testGeneratePublish FAIL: expected " << need << " got " << got << "\n";
    }
}

void testGenerateSubscribe()
{

    using namespace nano;

    slice s;
    mqttPacketPieces subscribe;
    subscribe.reset();

    subscribe.PacketID = 2;
    // add props?
    // payload is the topic
    subscribe.TopicName = slice("TEST/TIMEefghijk");
    subscribe.UserKeyVal[0] = slice("key1");
    subscribe.UserKeyVal[1] = slice("val1");
    subscribe.UserKeyVal[2] = slice("key2");
    subscribe.UserKeyVal[3] = slice("val2");
    subscribe.QoS = 1;
    subscribe.packetType = CtrlSubscribe;

    mqttBuffer buffer;
    sink buffersink = buffer.getSink();

    mqttBuffer result;
    sinkDrain netDrain;
    netDrain.src = result.getSink();

    bool ok = subscribe.outputPubOrSub(buffersink, &netDrain);

    slice rsl = netDrain.src.getWritten();
    char tmpbuffer[1024];

    //rsl.printhex();

    // should be:
    const char *need = "823000021a2600046b657931000476616c312600046b657932000476616c320010544553542f54494d4565666768696a6b01";
    const char *got = rsl.gethexstr(tmpbuffer);
    if (strcmp(got, need) != 0)
    {
        cout << "testGenerateSubscribe FAIL: expected --" << need << "-- got --" << got << "--\n";
    }
}

void testParsePublish()
{
    const char *pubbytes = "3268000d544553542f54494d456162636400032d080010544553542f54494d4565666768696a6b2600046b657931000476616c312600046b657932000476616c326d65737361676520617420323032302d30332d32372030313a33353a33372e34303330373920633d31";

    using namespace nano;

    mqttBuffer testbuffer;

    sliceFount net;
    net.src = testbuffer.loadHexString(pubbytes);

    //net.src.printhex();

    cout << "sizeof(mqttPacketPieces)=" << sizeof(mqttPacketPieces) << " ,\n";
    cout << "sizeof(UserKeyVal[8])=" << sizeof(slice[8]) << " ,\n";

    unsigned char packetType = net.readByte();
    int len = net.getVarLenInt();

    // now we can read the rest
    mqttBuffer parserBuffer;
    slice body = parserBuffer.loadFromFount(net, len);

    mqttPacketPieces parser;
    bool failed = parser.parse(body, packetType, len);

    cout << "parse result was fail=" << failed << "\n";

    // because .... it's a unit test.
    bool matched;
    const char *expected;
    char buffer[1024];
    matched = parser.TopicName.equals(expected = "TEST/TIMEabcd");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.TopicName.getCstr(buffer) << "\n";
    }
    matched = parser.Payload.equals(expected = "message at 2020-03-27 01:35:37.403079 c=1");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.Payload.getCstr(buffer) << "\n";
    }
    matched = parser.RespTopic.equals(expected = "TEST/TIMEefghijk");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.RespTopic.getCstr(buffer) << "\n";
    }

    int i = 0;
    matched = parser.UserKeyVal[i].equals(expected = "key1");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.UserKeyVal[i].getCstr(buffer) << "\n";
    }
    matched = parser.UserKeyVal[i + 1].equals(expected = "val1");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.UserKeyVal[i + 1].getCstr(buffer) << "\n";
    }
    i = 2;
    matched = parser.UserKeyVal[i].equals(expected = "key2");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.UserKeyVal[i].getCstr(buffer) << "\n";
    }
    matched = parser.UserKeyVal[i + 1].equals(expected = "val2");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.UserKeyVal[i + 1].getCstr(buffer) << "\n";
    }
    i = 4;
    matched = parser.UserKeyVal[i].equals(expected = "");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.UserKeyVal[i].getCstr(buffer) << "\n";
    }
    matched = parser.UserKeyVal[i + 1].equals(expected = "");
    if (!matched)
    {
        cout << "FAIL: expected " << expected << " got " << parser.UserKeyVal[i + 1].getCstr(buffer) << "\n";
    }

    cout << "testParsePublish finished";
}
