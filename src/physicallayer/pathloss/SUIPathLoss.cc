/***************************************************************************
 * author:      Konrad Polys, Krzysztof Grochla
 *
 * copyright:   (c) 2013 The Institute of Theoretical and Applied Informatics
 *                       of the Polish Academy of Sciences, Project
 *                       LIDER/10/194/L-3/11/ supported by NCBIR
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************/

#include "SUIPathLoss.h"

using namespace physicallayer;

Register_Class(SUIPathLoss);

SUIPathLoss::SUIPathLoss() :
    ht(m(0)),
    hr(m(0)),
    a(0),
    b(0),
    c(0),
    d(0),
    s(0)
{
}

void SUIPathLoss::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        ht = m(par("TransmiterAntennaHigh"));
        hr = m(par("ReceiverAntennaHigh"));
        /**
         * Terrain A - Highest path loss. Dense populated urban area.
         * Terrain B - Intermediate path loss. Suburban area.
         * Terrain C - Minimum path loss. Flat areas or rural with light vegetation.
         */
        const char *terrain = par("terrain").stringValue();
        if (!strcmp(terrain, "TerrainA"))
        { a = 4.6; b = 0.0075; c = 12.6; d = 10.8; s = 10.6; }
        else if (!strcmp(terrain, "TerrainB"))
        { a = 4.0; b = 0.0065; c = 17.1; d = 10.8; s = 9.6;  }
        else if (!strcmp(terrain, "TerrainC"))
        { a = 3.6; b = 0.0050; c = 20.0; d = 20.0; s = 8.2;  }
        else
            throw cRuntimeError("Unknown terrain");
    }
}

void SUIPathLoss::printToStream(std::ostream &stream) const
{
    stream << "SUI path loss, "
           << "transmitter antenna high = " << ht << ", "
           << "receiver antenna high = " << hr;
}

double SUIPathLoss::computePathLoss(mps propagationSpeed, Hz carrierFrequency, m distance) const
{
    m R = distance;
    m R0 = m(100.0);
    m lambda = propagationSpeed / carrierFrequency;
    double L = 0.0;         // [dBm]
    double f = carrierFrequency.get() / 1000000000.0; // [GHz]
    double alpha = 0.0;
    double gamma = a - b * ht.get() + c / ht.get();
    double Xf = 6 * log10(f / 2);
    double Xh = -d * log10(hr.get() / 2);
    m R0p = R0 * pow(10.0, -((Xf + Xh) / (10 * gamma)));
    if (R > R0p)
    {
        alpha = 20 * log10(unit(4 * M_PI * R0p / lambda).get());
        L = alpha + 10 * gamma * log10(unit(R / R0).get()) + Xf + Xh + s;
    }
    else
    {
        L = 20 * log10(unit(4 * M_PI * R / lambda).get()) + s;
    }
    return FWMath::dB2fraction(-L);
}