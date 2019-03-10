/*
* Liam Ashdown
* Copyright (C) 2019
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SharedDefines.h"

std::string SplitString(const std::string& stringData, const std::string& tokenizer)
{
    std::size_t startPosition = stringData.find(tokenizer);

    if (stringData[startPosition + tokenizer.length()] == '=')
    {
        std::string keyString;
        for (std::size_t i = startPosition + (tokenizer.length() + 1); i < stringData.length(); i++)
        {
            if (stringData[i] == '\r')
                break;

            keyString += stringData[i];
        }

        return keyString;
    }

    return "0";
}

uint16 ConvertEndian(uint16 value)
{
    return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
}