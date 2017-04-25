///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016 F4EXB                                                      //
// written by Edouard Griffiths                                                  //
//                                                                               //
// Integer half-band FIR based interpolator and decimator                        //
// This is the double buffer variant                                             //
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

#ifndef INCLUDE_INTHALFBANDFILTER_DB_H
#define INCLUDE_INTHALFBANDFILTER_DB_H

#include <stdint.h>
#include "dsp/dsptypes.h"
#include "dsp/hbfiltertraits.h"
#include "util/export.h"

template<uint32_t HBFilterOrder>
class SDRANGEL_API IntHalfbandFilterDB {
public:
    IntHalfbandFilterDB();

	// downsample by 2, return center part of original spectrum
	bool workDecimateCenter(Sample* sample)
	{
		// insert sample into ring-buffer
	    storeSample((FixReal) sample->real(), (FixReal) sample->imag());

		switch(m_state)
		{
			case 0:
				// advance write-pointer
			    advancePointer();
				// next state
				m_state = 1;
				// tell caller we don't have a new sample
				return false;

			default:
				// save result
				doFIR(sample);
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 0;

				// tell caller we have a new sample
				return true;
		}
	}

    // upsample by 2, return center part of original spectrum - double buffer variant
    bool workInterpolateCenter(Sample* sampleIn, Sample *SampleOut)
    {
        switch(m_state)
        {
            case 0:
                // insert sample into ring-buffer
                storeSample(0, 0);
                // save result
                doFIR(SampleOut);
                // advance write-pointer
                advancePointer();
                // next state
                m_state = 1;
                // tell caller we didn't consume the sample
                return false;

            default:
                // insert sample into ring-buffer
                storeSample((FixReal) sampleIn->real(), (FixReal) sampleIn->imag());
                // save result
                doFIR(SampleOut);
                // advance write-pointer
                advancePointer();
                // next state
                m_state = 0;
                // tell caller we consumed the sample
                return true;
        }
    }

    /** Optimized upsampler by 2 not calculating FIR with inserted null samples */
    bool workInterpolateCenterOptimized(Sample* sampleIn, Sample *SampleOut)
    {
        switch(m_state)
        {
        case 0:
            // return the middle peak
            SampleOut->setReal(m_samplesDB[m_ptr + (HBFIRFilterTraits<HBFilterOrder>::hbOrder/4) - 1][0]);
            SampleOut->setImag(m_samplesDB[m_ptr + (HBFIRFilterTraits<HBFilterOrder>::hbOrder/4) - 1][1]);
            m_state = 1;  // next state
            return false; // tell caller we didn't consume the sample

        default:
            // calculate with non null samples
            doInterpolateFIR(SampleOut);

            // insert sample into ring double buffer
            m_samplesDB[m_ptr][0] = sampleIn->real();
            m_samplesDB[m_ptr][1] = sampleIn->imag();
            m_samplesDB[m_ptr + HBFIRFilterTraits<HBFilterOrder>::hbOrder/2][0] = sampleIn->real();
            m_samplesDB[m_ptr + HBFIRFilterTraits<HBFilterOrder>::hbOrder/2][1] = sampleIn->imag();

            // advance pointer
            if (m_ptr < (HBFIRFilterTraits<HBFilterOrder>::hbOrder/2) - 1) {
                m_ptr++;
            } else {
                m_ptr = 0;
            }

            m_state = 0; // next state
            return true; // tell caller we consumed the sample
        }
    }

	bool workDecimateCenter(qint32 *x, qint32 *y)
	{
		// insert sample into ring-buffer
	    storeSample(*x, *y);

		switch(m_state)
		{
			case 0:
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 1;
				// tell caller we don't have a new sample
				return false;

			default:
				// save result
				doFIR(x, y);
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 0;
				// tell caller we have a new sample
				return true;
		}
	}

	// downsample by 2, return lower half of original spectrum
	bool workDecimateLowerHalf(Sample* sample)
	{
		switch(m_state)
		{
			case 0:
				// insert sample into ring-buffer
			    storeSample((FixReal) -sample->imag(), (FixReal) sample->real());
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 1;
				// tell caller we don't have a new sample
				return false;

			case 1:
				// insert sample into ring-buffer
                storeSample((FixReal) -sample->real(), (FixReal) -sample->imag());
				// save result
				doFIR(sample);
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 2;
				// tell caller we have a new sample
				return true;

			case 2:
				// insert sample into ring-buffer
                storeSample((FixReal) sample->imag(), (FixReal) -sample->real());
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 3;
				// tell caller we don't have a new sample
				return false;

			default:
				// insert sample into ring-buffer
                storeSample((FixReal) sample->real(), (FixReal) sample->imag());
				// save result
				doFIR(sample);
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 0;
				// tell caller we have a new sample
				return true;
		}
	}

    // upsample by 2, from lower half of original spectrum - double buffer variant
    bool workInterpolateLowerHalf(Sample* sampleIn, Sample *sampleOut)
    {
        Sample s;

        switch(m_state)
        {
        case 0:
            // insert sample into ring-buffer
            storeSample(0, 0);

            // save result
            doFIR(&s);
            sampleOut->setReal(s.imag());
            sampleOut->setImag(-s.real());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 1;

            // tell caller we didn't consume the sample
            return false;

        case 1:
            // insert sample into ring-buffer
            storeSample((FixReal) sampleIn->real(), (FixReal) sampleIn->imag());

            // save result
            doFIR(&s);
            sampleOut->setReal(-s.real());
            sampleOut->setImag(-s.imag());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 2;

            // tell caller we consumed the sample
            return true;

        case 2:
            // insert sample into ring-buffer
            storeSample(0, 0);

            // save result
            doFIR(&s);
            sampleOut->setReal(-s.imag());
            sampleOut->setImag(s.real());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 3;

            // tell caller we didn't consume the sample
            return false;

        default:
            // insert sample into ring-buffer
            storeSample((FixReal) sampleIn->real(), (FixReal) sampleIn->imag());

            // save result
            doFIR(&s);
            sampleOut->setReal(s.real());
            sampleOut->setImag(s.imag());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 0;

            // tell caller we consumed the sample
            return true;
        }
    }

	// downsample by 2, return upper half of original spectrum
	bool workDecimateUpperHalf(Sample* sample)
	{
		switch(m_state)
		{
			case 0:
				// insert sample into ring-buffer
	            storeSample((FixReal) sample->imag(), (FixReal) -sample->real());
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 1;
				// tell caller we don't have a new sample
				return false;

			case 1:
				// insert sample into ring-buffer
                storeSample((FixReal) -sample->real(), (FixReal) -sample->imag());
				// save result
				doFIR(sample);
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 2;
				// tell caller we have a new sample
				return true;

			case 2:
				// insert sample into ring-buffer
                storeSample((FixReal) -sample->imag(), (FixReal) sample->real());
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 3;
				// tell caller we don't have a new sample
				return false;

			default:
				// insert sample into ring-buffer
                storeSample((FixReal) sample->real(), (FixReal) sample->imag());
				// save result
				doFIR(sample);
				// advance write-pointer
                advancePointer();
				// next state
				m_state = 0;
				// tell caller we have a new sample
				return true;
		}
	}

    // upsample by 2, move original spectrum to upper half - double buffer variant
    bool workInterpolateUpperHalf(Sample* sampleIn, Sample *sampleOut)
    {
        Sample s;

        switch(m_state)
        {
        case 0:
            // insert sample into ring-buffer
            storeSample(0, 0);

            // save result
            doFIR(&s);
            sampleOut->setReal(-s.imag());
            sampleOut->setImag(s.real());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 1;

            // tell caller we didn't consume the sample
            return false;

        case 1:
            // insert sample into ring-buffer
            storeSample((FixReal) sampleIn->real(), (FixReal) sampleIn->imag());

            // save result
            doFIR(&s);
            sampleOut->setReal(-s.real());
            sampleOut->setImag(-s.imag());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 2;

            // tell caller we consumed the sample
            return true;

        case 2:
            // insert sample into ring-buffer
            storeSample(0, 0);

            // save result
            doFIR(&s);
            sampleOut->setReal(s.imag());
            sampleOut->setImag(-s.real());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 3;

            // tell caller we didn't consume the sample
            return false;

        default:
            // insert sample into ring-buffer
            storeSample((FixReal) sampleIn->real(), (FixReal) sampleIn->imag());

            // save result
            doFIR(&s);
            sampleOut->setReal(s.real());
            sampleOut->setImag(s.imag());

            // advance write-pointer
            advancePointer();

            // next state
            m_state = 0;

            // tell caller we consumed the sample
            return true;
        }
    }

    void myDecimate(const Sample* sample1, Sample* sample2)
    {
        storeSample((FixReal) sample1->real(), (FixReal) sample1->imag());
        advancePointer();

        storeSample((FixReal) sample2->real(), (FixReal) sample2->imag());
        doFIR(sample2);
        advancePointer();
    }

    void myDecimate(qint32 x1, qint32 y1, qint32 *x2, qint32 *y2)
    {
        storeSample(x1, y1);
        advancePointer();

        storeSample(*x2, *y2);
        doFIR(x2, y2);
        advancePointer();
    }

    void myInterpolate(Sample* sample1, Sample* sample2)
    {
        storeSample((FixReal) sample1->real(), (FixReal) sample1->imag());
        doFIR(sample1);
        advancePointer();

        storeSample(0, 0);
        doFIR(sample2);
        advancePointer();
    }

    void myInterpolate(qint32 *x1, qint32 *y1, qint32 *x2, qint32 *y2)
    {
        storeSample(*x1, *y1);
        doFIR(x1, y1);
        advancePointer();

        storeSample(0, 0);
        doFIR(x2, y2);
        advancePointer();
    }

    /** Optimized upsampler by 2 not calculating FIR with inserted null samples */
    void myInterpolateOptimized(qint32 *x1, qint32 *y1, qint32 *x2, qint32 *y2)
    {
        // insert sample into ring double buffer
        m_samplesDB[m_ptr][0] = *x1;
        m_samplesDB[m_ptr][1] = *y1;
        m_samplesDB[m_ptr + HBFIRFilterTraits<HBFilterOrder>::hbOrder/2][0] = *x1;
        m_samplesDB[m_ptr + HBFIRFilterTraits<HBFilterOrder>::hbOrder/2][1] = *y1;

        // advance pointer
        if (m_ptr < (HBFIRFilterTraits<HBFilterOrder>::hbOrder/2) - 1) {
            m_ptr++;
        } else {
            m_ptr = 0;
        }

        // first output sample calculated with the middle peak
        *x1 = m_samplesDB[m_ptr + (HBFIRFilterTraits<HBFilterOrder>::hbOrder/4) - 1][0];
        *y1 = m_samplesDB[m_ptr + (HBFIRFilterTraits<HBFilterOrder>::hbOrder/4) - 1][1];

        // second sample calculated with the filter
        doInterpolateFIR(x2, y2);
    }

protected:
	qint32 m_samplesDB[2*(HBFIRFilterTraits<HBFilterOrder>::hbOrder - 1)][2]; // double buffer technique
	int m_ptr;
	int m_size;
	int m_state;

    void storeSample(const FixReal& sampleI, const FixReal& sampleQ)
    {
        m_samplesDB[m_ptr][0] = sampleI;
        m_samplesDB[m_ptr][1] = sampleQ;
        m_samplesDB[m_ptr + m_size][0] = sampleI;
        m_samplesDB[m_ptr + m_size][1] = sampleQ;
    }

    void storeSample(qint32 x, qint32 y)
    {
        m_samplesDB[m_ptr][0] = x;
        m_samplesDB[m_ptr][1] = y;
        m_samplesDB[m_ptr + m_size][0] = x;
        m_samplesDB[m_ptr + m_size][1] = y;
    }

    void advancePointer()
    {
        m_ptr = m_ptr + 1 < m_size ? m_ptr + 1: 0;
    }

    void doFIR(Sample* sample)
    {
        int a = m_ptr + m_size; // tip pointer
        int b = m_ptr + 1; // tail pointer
        qint32 iAcc = 0;
        qint32 qAcc = 0;

        for (int i = 0; i < HBFIRFilterTraits<HBFilterOrder>::hbOrder / 4; i++)
        {
            iAcc += (m_samplesDB[a][0] + m_samplesDB[b][0]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            qAcc += (m_samplesDB[a][1] + m_samplesDB[b][1]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            a -= 2;
            b += 2;
        }

        iAcc += ((qint32)m_samplesDB[b-1][0]) << (HBFIRFilterTraits<HBFilterOrder>::hbShift - 1);
        qAcc += ((qint32)m_samplesDB[b-1][1]) << (HBFIRFilterTraits<HBFilterOrder>::hbShift - 1);

        sample->setReal(iAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1));
        sample->setImag(qAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1));
    }

    void doFIR(qint32 *x, qint32 *y)
    {
        int a = m_ptr + m_size; // tip pointer
        int b = m_ptr + 1; // tail pointer
        qint32 iAcc = 0;
        qint32 qAcc = 0;

        for (int i = 0; i < HBFIRFilterTraits<HBFilterOrder>::hbOrder / 4; i++)
        {
            iAcc += (m_samplesDB[a][0] + m_samplesDB[b][0]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            qAcc += (m_samplesDB[a][1] + m_samplesDB[b][1]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            a -= 2;
            b += 2;
        }

        iAcc += ((qint32)m_samplesDB[b-1][0]) << (HBFIRFilterTraits<HBFilterOrder>::hbShift - 1);
        qAcc += ((qint32)m_samplesDB[b-1][1]) << (HBFIRFilterTraits<HBFilterOrder>::hbShift - 1);

        *x = iAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1); // HB_SHIFT incorrect do not loose the gained bit
        *y = qAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1);
    }

    void doInterpolateFIR(Sample* sample)
    {
        qint16 a = m_ptr;
        qint16 b = m_ptr + (HBFIRFilterTraits<HBFilterOrder>::hbOrder / 2) - 1;

        // go through samples in buffer
        qint32 iAcc = 0;
        qint32 qAcc = 0;

        for (int i = 0; i < HBFIRFilterTraits<HBFilterOrder>::hbOrder / 4; i++)
        {
            iAcc += (m_samplesDB[a][0] + m_samplesDB[b][0]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            qAcc += (m_samplesDB[a][1] + m_samplesDB[b][1]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            a++;
            b--;
        }

        sample->setReal(iAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1));
        sample->setImag(qAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1));
    }

    void doInterpolateFIR(qint32 *x, qint32 *y)
    {
        qint16 a = m_ptr;
        qint16 b = m_ptr + (HBFIRFilterTraits<HBFilterOrder>::hbOrder / 2) - 1;

        // go through samples in buffer
        qint32 iAcc = 0;
        qint32 qAcc = 0;

        for (int i = 0; i < HBFIRFilterTraits<HBFilterOrder>::hbOrder / 4; i++)
        {
            iAcc += (m_samplesDB[a][0] + m_samplesDB[b][0]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            qAcc += (m_samplesDB[a][1] + m_samplesDB[b][1]) * HBFIRFilterTraits<HBFilterOrder>::hbCoeffs[i];
            a++;
            b--;
        }

        *x = iAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1);
        *y = qAcc >> (HBFIRFilterTraits<HBFilterOrder>::hbShift -1);
    }
};

template<uint32_t HBFilterOrder>
IntHalfbandFilterDB<HBFilterOrder>::IntHalfbandFilterDB()
{
    m_size = HBFIRFilterTraits<HBFilterOrder>::hbOrder - 1;

    for (int i = 0; i < m_size; i++)
    {
        m_samplesDB[i][0] = 0;
        m_samplesDB[i][1] = 0;
    }

    m_ptr = 0;
    m_state = 0;
}

#endif // INCLUDE_INTHALFBANDFILTER_DB_H
