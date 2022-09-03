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

#pragma once

#include "badjson.h"

namespace knotfree
{
    struct Command // the virtual base class. Aka the interface.
    {
        Command *next;
        const char *name = "";
        const char *description = "";

        Command();
        Command(const char *name, const char *decription);
        void SetName(const char *name)
        {
            this->name = name;
        }
        void SetDescription(const char *desc)
        {
            this->description = desc;
        }
        virtual void execute(badjson::Segment *words, drain &out);
    };

    void process(badjson::Segment *words, drain &out);

}