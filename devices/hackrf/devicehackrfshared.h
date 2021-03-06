///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 Edouard Griffiths, F4EXB                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef DEVICES_HACKRF_DEVICEHACKRFSHARED_H_
#define DEVICES_HACKRF_DEVICEHACKRFSHARED_H_

#include "util/message.h"
#include "export.h"

class DEVICES_API DeviceHackRFShared
{
public:
    class MsgConfigureFrequencyDelta : public Message
    {
        MESSAGE_CLASS_DECLARATION

    public:
        int getFrequencyDelta() const { return m_frequencyDelta; }

        static MsgConfigureFrequencyDelta *create(int frequencyDelta)
        {
            return new MsgConfigureFrequencyDelta(frequencyDelta);
        }

    private:
        int m_frequencyDelta;

        MsgConfigureFrequencyDelta(int frequencyDelta) :
            Message(),
            m_frequencyDelta(frequencyDelta)
        { }
    };

    static const float  m_sampleFifoLengthInSeconds;
    static const int    m_sampleFifoMinSize;
};


#endif /* DEVICES_HACKRF_DEVICEHACKRFSHARED_H_ */
