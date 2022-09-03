
// Copyright 2022 Alan Tracey Wootton
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
//#include <vector>
//#include <stdio.h>
#include <string>
//#include <string.h> // has strcmp

#include "commandLine.h"

using namespace std;
using namespace badjson;

struct cout_drain : drain
{
    bool writeByte(char c) override
    {
        cout << c;
        bool failed = false;
        return failed;
    };
};
cout_drain outputter;

struct test1Struct : Command
{    
    void execute(badjson::Segment *words, drain &out) override
    {
        out.write("test 1 ");
    }
};

int main()
{
    cout << "hello command tests\n";

    test1Struct cmd1;   
    cmd1.SetName("test1");
    cmd1.SetDescription("test1 [args] output 'test1' ignore the args");

    const char *test = "test1 command line";
    ResultsTriplette res = Chop(test, strlen(test));

    process(res.segment,outputter);

    delete res.segment;// very important
}