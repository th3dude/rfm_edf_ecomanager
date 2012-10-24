#include "consts.h"
#include "Logger.h"
#include "CcTx.h"

/**************************
 * CcTrx                  *
 **************************/

CcTrx::CcTrx(): id(ID_INVALID) {}


CcTrx::CcTrx(const id_t& _id): id(_id) {}


CcTrx::~CcTrx() {}


void CcTrx::print() const
{
    Serial.print(id);
}


/*************************
 * CcTx                  *
 *************************/

CcTx::CcTx():CcTrx() { init(); }


CcTx::CcTx(const id_t& _id): CcTrx(_id) { init(); }


void CcTx::init()
{
    /* This init() function could be avoided if
     * we use C++11, but I can't think of any way to
     * force the Arduino IDE to use C++11.
     * http://stackoverflow.com/questions/308276/c-call-constructor-from-constructor */
    eta = 0xFFFFFFFF;
    num_periods = 0;
    last_seen = 0;
}


CcTx::~CcTx() {}


void CcTx::print()
{
    Serial.print("{id: ");
    Serial.print(id);
    Serial.print(", last_seen: ");
    Serial.print(last_seen);
    Serial.print(", num_periods_missed: ");
    Serial.print(num_periods);
    Serial.print(", eta: ");
    Serial.print(eta);
    Serial.print(", sample_period: ");
    Serial.print(sample_period.get_av());
    Serial.print("}");
}


void CcTx::update(const RXPacket& packet)
{
    if (last_seen != 0) {
        uint16_t new_sample_period;

        // update sample period for this Sensor
        // (seems to vary a little between sensors)
        log(DEBUG, "CC_TX ID=%lu, old sample_period=%u", id, sample_period.get_av());

        new_sample_period = (packet.get_timecode() - last_seen) / num_periods;

        // Check the new sample_period is sane
        if (new_sample_period > 5700 && new_sample_period < 6300) {
            log(DEBUG, "Adding new_sample_period %u", new_sample_period);
            sample_period.add_sample(new_sample_period);
        }

        log(DEBUG, "CC_TX ID=%lu, new sample_period=%u", id, sample_period.get_av());
    }
	eta = packet.get_timecode() + sample_period.get_av() - CC_TX_WINDOW_OPEN;
	num_periods = 1;
	last_seen = packet.get_timecode();
}


const id_t& CcTx::get_eta()
{
    // Sanity-check ETA to make sure it's in the future.
    // TODO: remove if this is never used
    if (eta < millis() &&
            eta+SAMPLE_PERIOD < millis()+SAMPLE_PERIOD) /* one possible reason
            for eta < millis() is that millis() has rolled over.
            If millis() has rolled over
            then eta+SAMPLE_PERIOD > millis()+SAMPLE_PERIOD.  In other words, only
            set eta to 0xFFFFFFFF if the fact that eta < millis cannot be explained
            by roll-over.  We want to let roll-over do its thing.  */
    {
        log(DEBUG, "eta %lu < millis() %lu", eta, millis());
        eta = 0xFFFFFFFF;
    }
	return eta;
}


void CcTx::missing()
{
	eta += sample_period.get_av();
	num_periods++;

	log(INFO, "id:%lu is missing. New ETA=%lu, num_periods missed=%u", id, eta, num_periods);
}


/******************************
 * CcTxArray                  *
 ******************************/

void CcTxArray::next()
{
    for (index_t j=0; j<size; j++) {
        if (data[j].get_eta() < current().get_eta()) {
            i = j;
        }
    }
    log(DEBUG, "Next expected CC_TX: ID=%lu, ETA=%lu",
            current().id, current().get_eta());
}


void CcTxArray::print_name() const
{
    Serial.print(" CC_TX");
}


/******************************
 * CcTrxArray                 *
 ******************************/

void CcTrxArray::next()
{
    i++;
    if (i==size) i=0;
}


void CcTrxArray::print_name() const
{
    Serial.print(" CC_TRX");
}
