
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

#include "commandLine.h"

#include <string.h> // has strcmp

namespace knotfree
{
    Command *head = 0;

    Command::Command(const char *name, const char *decription) : name(name), description(decription)
    {
        next = head;
        head = this;
    }

    Command::Command()
    {
        next = head;
        head = this;
    }

    void Command::execute(badjson::Segment *words, drain &out)
    {
    }

    void process(badjson::Segment *words, drain &out)
    {
        char wordBuffer[64];
        char *cP = &wordBuffer[0];
        int amt = sizeof(wordBuffer);
        sink have(cP, amt);
        words->Raw(have);
        slice theIncomingWord(have);

        Command *cmdP = head;
        while (cmdP != nullptr)
        {
            if (theIncomingWord.equals(cmdP->name))
            {
                cmdP->execute(words, out);
                break;
            }
            cmdP = cmdP->next;
        }
    }

}