//
// Copyright (C) 2014 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_IEEE80211OFDMRECEIVER_H
#define __INET_IEEE80211OFDMRECEIVER_H

#include "inet/physicallayer/common/layered/SignalPacketModel.h"
#include "inet/physicallayer/contract/IRadioMedium.h"
#include "inet/physicallayer/base/APSKModulationBase.h"
#include "inet/physicallayer/ieee80211/layered/Ieee80211ConvolutionalCode.h"
#include "inet/physicallayer/base/SNIRReceiverBase.h"
#include "inet/physicallayer/contract/layered/IDecoder.h"
#include "inet/physicallayer/contract/layered/IDemodulator.h"
#include "inet/physicallayer/contract/layered/IPulseFilter.h"
#include "inet/physicallayer/contract/layered/IAnalogDigitalConverter.h"
#include "inet/physicallayer/contract/IErrorModel.h"
#include "inet/physicallayer/contract/layered/ILayeredErrorModel.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211OFDMMode.h"

namespace inet {
namespace physicallayer {

class INET_API Ieee80211OFDMReceiver : public SNIRReceiverBase
{
    public:
        enum LevelOfDetail
        {
            PACKET_DOMAIN,
            BIT_DOMAIN,
            SYMBOL_DOMAIN,
            SAMPLE_DOMAIN,
        };

    protected:
        LevelOfDetail levelOfDetail;
        mutable const Ieee80211OFDMMode *mode = nullptr;
        const ILayeredErrorModel *errorModel = nullptr;
        const IDecoder *dataDecoder = nullptr;
        const IDecoder *signalDecoder = nullptr;
        const IDemodulator *dataDemodulator = nullptr;
        const IDemodulator *signalDemodulator = nullptr;
        const IPulseFilter *pulseFilter = nullptr;
        const IAnalogDigitalConverter *analogDigitalConverter = nullptr;

        W energyDetection;
        W sensitivity;
        Hz carrierFrequency;
        Hz bandwidth;
        Hz channelSpacing;
        double snirThreshold;
        bool isCompliant;

    protected:
        virtual void initialize(int stage);

        /* Analog and sample domain */
        const IReceptionAnalogModel *createAnalogModel(const LayeredTransmission *transmission, const ISNIR *snir) const;
        const IReceptionSampleModel *createSampleModel(const LayeredTransmission *transmission, const ISNIR *snir) const;

        /* Symbol domain */
        const IReceptionSymbolModel *createSignalFieldSymbolModel(const IReceptionSymbolModel *symbolModel) const;
        const IReceptionSymbolModel *createDataFieldSymbolModel(const IReceptionSymbolModel *symbolModel) const;
        const IReceptionSymbolModel *createSymbolModel(const LayeredTransmission *transmission, const ISNIR *snir) const;
        const IReceptionSymbolModel *createCompleteSymbolModel(const IReceptionSymbolModel *signalFieldSymbolModel, const IReceptionSymbolModel *dataFieldSymbolModel) const;

        /* Bit domain */
        const IReceptionBitModel *createSignalFieldBitModel(const IReceptionBitModel *bitModel, const IReceptionSymbolModel *symbolModel) const;
        const IReceptionBitModel *createDataFieldBitModel(const IReceptionBitModel *bitModel, const IReceptionSymbolModel *symbolModel, const IReceptionPacketModel *signalFieldPacketModel, const IReceptionBitModel *signalFieldBitModel) const;
        const IReceptionBitModel *createBitModel(const LayeredTransmission* transmission, const ISNIR* snir) const;
        const IReceptionBitModel *createCompleteBitModel(const IReceptionBitModel *signalFieldBitModel, const IReceptionBitModel *dataFieldBitModel) const;

        /* Packet domain */
        const IReceptionPacketModel *createSignalFieldPacketModel(const IReceptionBitModel *signalFieldBitModel) const;
        const IReceptionPacketModel *createDataFieldPacketModel(const IReceptionBitModel *signalFieldBitModel, const IReceptionBitModel *dataFieldBitModel, const IReceptionPacketModel *signalFieldPacketModel) const;
        const IReceptionPacketModel *createPacketModel(const LayeredTransmission* transmission, const ISNIR* snir) const;
        const IReceptionPacketModel *createCompletePacketModel(const IReceptionPacketModel *signalFieldPacketModel, const IReceptionPacketModel *dataFieldPacketModel) const;

        const Ieee80211OFDMMode *computeMode(Hz bandwidth) const;
        uint8_t getRate(const BitVector *serializedPacket) const;
        unsigned int getSignalFieldLength(const BitVector *signalField) const;
        unsigned int calculatePadding(unsigned int dataFieldLengthInBits, const APSKModulationBase *modulation, double codeRate) const;
        double getCodeRateFromDecoderModule(const IDecoder *decoder) const;

    public:
        virtual ~Ieee80211OFDMReceiver();

        bool computeIsReceptionPossible(const IListening *listening, const IReception *reception) const;
        const IListeningDecision* computeListeningDecision(const IListening* listening, const IInterference* interference) const;
        const IListening* createListening(const IRadio* radio, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition) const;
        virtual const IReceptionDecision *computeReceptionDecision(const IListening *listening, const IReception *reception, const IInterference *interference) const;
};

} /* namespace physicallayer */
} /* namespace inet */

#endif /* __INET_IEEE80211OFDMRECEIVER_H */