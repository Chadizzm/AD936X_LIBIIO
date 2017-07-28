/*
 * Author: Rodrigo Rolim Mendes de Alencar <alencar.fmce@imbel.gov.br>
 *
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of AD936X_LIBIIO.
 *
 * AD936X_LIBIIO is based on REDHAWK USRP_UHD, thus it is released under the same
 * Copyright and the same License.
 *
 * AD936X_LIBIIO is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * AD936X_LIBIIO is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "AD936X_LIBIIO.h"

static double min_CENTER_FREQ;
static double max_CENTER_FREQ;
static double max_BANDWIDTH;
static double max_SAMPLE_RATE;

PREPARE_LOGGING(AD936X_LIBIIO_i)

AD936X_LIBIIO_i::AD936X_LIBIIO_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    AD936X_LIBIIO_base(devMgr_ior, id, lbl, sftwrPrfl){
	construct();
}

AD936X_LIBIIO_i::AD936X_LIBIIO_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    AD936X_LIBIIO_base(devMgr_ior, id, lbl, sftwrPrfl, compDev){
	construct();
}

AD936X_LIBIIO_i::AD936X_LIBIIO_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    AD936X_LIBIIO_base(devMgr_ior, id, lbl, sftwrPrfl, capacities){
	construct();
}

AD936X_LIBIIO_i::AD936X_LIBIIO_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    AD936X_LIBIIO_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev){
	construct();
}

AD936X_LIBIIO_i::~AD936X_LIBIIO_i(){
	if (rx_buffer_lock.cond != NULL)
		delete rx_buffer_lock.cond;
	if (rx_buffer_lock.mutex != NULL)
		delete rx_buffer_lock.mutex;
	if (tx_buffer_lock.cond != NULL)
		delete tx_buffer_lock.cond;
	if (tx_buffer_lock.mutex != NULL)
		delete tx_buffer_lock.mutex;

	for (size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id++) {
		if (ad936x_tuners[tuner_id].lock.cond != NULL)
			delete ad936x_tuners[tuner_id].lock.cond;
		if (ad936x_tuners[tuner_id].lock.mutex != NULL)
			delete ad936x_tuners[tuner_id].lock.mutex;
	}
}

/***********************************************************************************
 this function is invoked in the constructors
***********************************************************************************/
void AD936X_LIBIIO_i::construct() {
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	receive_service_thread = NULL;
	transmit_service_thread = NULL;

	global_group_id = "AD936X_GROUP_ID_NOT_SET";

	// initialize rf info packets w/ very large ranges
	extendedRFInfoPkt rfPort;
	rfPort.rfinfo_pkt.rf_center_freq = 50e9; // 50 GHz
	rfPort.rfinfo_pkt.rf_bandwidth = 100e9; // 100 GHz, makes range 0 Hz to 100 GHz
	rfPort.rfinfo_pkt.if_center_freq = 0; // 0 Hz, no up/down converter

	//RX1
	rfPort.tuner_id = 0;
	rfPort.port = "RX1A_0";
	rfPort.rf_port_select = "A_BALANCED";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_RX_1_A";
	RX_info_port.push_back(rfPort);
	rfPort.tuner_id = 0;
	rfPort.port = "RX1B_1";
	rfPort.rf_port_select = "B_BALANCED";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_RX_1_B";
	RX_info_port.push_back(rfPort);
	//TX1
	rfPort.tuner_id = 1;
	rfPort.port = "TX1A_0";
	rfPort.rf_port_select = "A";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_TX_1_A";
	TX_info_port.push_back(rfPort);
	rfPort.tuner_id = 1;
	rfPort.port = "TX1B_1";
	rfPort.rf_port_select = "B";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_TX_1_B";
	TX_info_port.push_back(rfPort);

	//RX2
	rfPort.tuner_id = 2;
	rfPort.port = "RX2A_2";
	rfPort.rf_port_select = "A_BALANCED";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_RX_2_A";
	RX_info_port.push_back(rfPort);
	rfPort.tuner_id = 2;
	rfPort.port = "RX2B_3";
	rfPort.rf_port_select = "B_BALANCED";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_RX_2_B";
	RX_info_port.push_back(rfPort);
	//TX2
	rfPort.tuner_id = 3;
	rfPort.port = "TX2A_2";
	rfPort.rf_port_select = "A";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_TX_2_A";
	TX_info_port.push_back(rfPort);
	rfPort.tuner_id = 3;
	rfPort.port = "TX2B_3";
	rfPort.rf_port_select = "B";
	rfPort.rfinfo_pkt.rf_flow_id = "AD936X_TX_2_B";
	TX_info_port.push_back(rfPort);
}

void AD936X_LIBIIO_i::constructor(){

	addPropertyListener(target_device, this, &AD936X_LIBIIO_i::targetDeviceChanged);
	addPropertyListener(global_settings, this, &AD936X_LIBIIO_i::globalSettingsChanged);
	addPropertyListener(receive_chain, this, &AD936X_LIBIIO_i::receiveChainChanged);
	addPropertyListener(transmit_chain, this, &AD936X_LIBIIO_i::transmitChainChanged);
	addPropertyListener(global_group_id, this, &AD936X_LIBIIO_i::deviceGroupIdChanged);

	try{
		initAD936x();
	}catch(...){
		LOG_WARN(AD936X_LIBIIO_i,"CAUGHT EXCEPTION WHEN INITIALIZING AD936x. WAITING 1 SECOND AND TRYING AGAIN");
		sleep(1);
		try {
			initAD936x();
		} catch(...) {
			LOG_INFO(AD936X_LIBIIO_i,"Could not init AD936x with current target_device configuration.");
		}
	}

	//Ignoring initial setup for global_settings, receive_chain and transmit_chain
	updateGroupId(global_group_id);

	start();
}

// Receive thread
int AD936X_LIBIIO_i::serviceFunctionReceive(){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	size_t nsamples;
	ssize_t refilled;
	BULKIO::PrecisionUTCTime rx_time;

	enqueued_scoped_lock buf_lock(rx_buffer_lock);

	if(rx_buffer){

		refilled = iio_buffer_refill(rx_buffer);
		rx_time = bulkio::time::utils::now();

		if(refilled > 0){
			nsamples = (size_t) refilled/iio_buffer_step(rx_buffer);
			assert(nsamples == buffer_size);

			for(size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over RX tuners

				if (getControlAllocationId(tuner_id).empty())//Check to see if channel is allocated
					continue;

				enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);

				if (getControlAllocationId(tuner_id).empty())//Check to make sure channel is allocated still
					continue;

				if(frontend_tuner_status[tuner_id].enabled){

					tunerReceive(tuner_id, &ad936x_tuners[tuner_id].buffer[0]);

					std::string stream_id = getStreamId(tuner_id);
					if (ad936x_tuners[tuner_id].update_sri){
						LOG_DEBUG(AD936X_LIBIIO_i,"AD936X_LIBIIO_i::serviceFunctionReceive|creating SRI for tuner: "<<tuner_id<<" with stream id: "<< stream_id);
						BULKIO::StreamSRI sri = create(stream_id, frontend_tuner_status[tuner_id]);
						sri.mode = 1; // complex
						dataShortRX_out->pushSRI(sri);
						ad936x_tuners[tuner_id].update_sri = false;
					}

					ad936x_tuners[tuner_id].time = rx_time;

					if(dataShortRX_out->isActive()){
						dataShortRX_out->pushPacket(ad936x_tuners[tuner_id].buffer, ad936x_tuners[tuner_id].time, false, stream_id);
					}
				}
			}
			refilled = 0;
			return NORMAL;

		}else{
			return NOOP;
		}
	}
	return NOOP;
}

// Transmit Thread
int AD936X_LIBIIO_i::serviceFunctionTransmit(){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	size_t nsamples;
	ssize_t pushed;
	enqueued_scoped_lock buf_lock(tx_buffer_lock);

	if(tx_buffer){

		bulkio::InShortStream stream = dataShortTX_in->getCurrentStream(0);

		if(!stream){
			return NOOP;
		}

		bulkio::ShortDataBlock block = stream.read(buffer_size);

		if(!block){
			LOG_DEBUG(AD936X_LIBIIO_i,"AD936X_LIBIIO_i::serviceFunctionTransmit| stream has ended.");
			return NOOP;
		}

		if (block.inputQueueFlushed())
			LOG_WARN(AD936X_LIBIIO_i,"Input Queue Flushed");

		if(!block.complex()){
			LOG_ERROR(AD936X_LIBIIO_i,"AD936X device requires complex data.  Real data type received.");
			return NOOP;
		}

		for(size_t tuner_id = 1; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over TX tuners

			if (getControlAllocationId(tuner_id).empty())//Check to see if channel is allocated
				continue;

			enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);

			if (getControlAllocationId(tuner_id).empty())//Check to make sure channel is allocated still
				continue;

			if(frontend_tuner_status[tuner_id].enabled){
				tunerTransmit(tuner_id, block);
			}
		}

		pushed = iio_buffer_push(tx_buffer);
		nsamples = (size_t) pushed/iio_buffer_step(tx_buffer);
		assert(nsamples == buffer_size);

		return NORMAL;
	}
    
    return NOOP;
}

// Initialize Receive and Transmit Threads
void AD936X_LIBIIO_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
    LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    AD936X_LIBIIO_base::start();
    dataShortTX_in->unblock();

    // Create threads
    try{

        {
            exclusive_lock lock(receive_service_thread_lock);
            if (receive_service_thread == NULL) {

                receive_service_thread = new MultiProcessThread<AD936X_LIBIIO_i> (this, &AD936X_LIBIIO_i::serviceFunctionReceive, 0.001);
                receive_service_thread->start();
            }
        }
        {
            exclusive_lock lock(transmit_service_thread_lock);
            if (transmit_service_thread == NULL) {

                transmit_service_thread = new MultiProcessThread<AD936X_LIBIIO_i> (this, &AD936X_LIBIIO_i::serviceFunctionTransmit, 0.001);
                transmit_service_thread->start();
            }
        }

    } catch (...) {
        stop();
        throw;
    }

}

// Stop the Device releasing the threads
void AD936X_LIBIIO_i::stop() throw (CORBA::SystemException, CF::Resource::StopError) {
    LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    dataShortTX_in->block();

    {
        exclusive_lock lock(receive_service_thread_lock);
        // release the child thread (if it exists)
        if (receive_service_thread != 0) {

            if (!receive_service_thread->release(2)) {
                throw CF::Resource::StopError(CF::CF_NOTSET,"Receive processing thread did not die");
            }
            delete receive_service_thread;
            receive_service_thread = 0;
        }
    }

    {
        exclusive_lock lock(transmit_service_thread_lock);
        // release the child thread (if it exists)
        if (transmit_service_thread != 0) {

            if (!transmit_service_thread->release(2)) {
                throw CF::Resource::StopError(CF::CF_NOTSET,"Transmit processing thread did not die");
            }
            delete transmit_service_thread;
            transmit_service_thread = 0;
        }
    }

    // iterate through tuners to disable any enabled tuners
    for (size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id++) {
		deviceDisable(frontend_tuner_status[tuner_id], tuner_id);
    }

    AD936X_LIBIIO_base::stop();
}

///////////////////////////////
//   CONFIGURE CALLBACKS     //
///////////////////////////////

void AD936X_LIBIIO_i::targetDeviceChanged(const target_device_struct& old_value, const target_device_struct& new_value){
    LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|target_device.type=" << target_device.type);
    LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|target_device.name=" << target_device.name);
    LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|target_device.serial=" << target_device.serial);
    LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|target_device.context_uri=" << target_device.context_uri);

    if(started()){
        LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|device has been started, must stop before initialization");
        stop();
    } else {
        LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|device has not been started, continue with initialization");
    }

    { // scope for prop_lock
        exclusive_lock lock(prop_lock);

        try{
        	initAD936x();
        }catch(...){
            LOG_WARN(AD936X_LIBIIO_i,"CAUGHT EXCEPTION WHEN INITIALIZING AD936X. WAITING 1 SECOND AND TRYING AGAIN");
            sleep(1);
            initAD936x();
        }
    } // end scope for prop_lock

    if(!started()){
        LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|device is not started, must start device after initialization");
        start();
    } else {
        LOG_DEBUG(AD936X_LIBIIO_i,"targetDeviceChanged|device was already started after initialization, not calling start again");
    }
}

void AD936X_LIBIIO_i::globalSettingsChanged(const global_settings_struct& old_value, const global_settings_struct& new_value){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	long long aux;

	// This feature does not exist on the AD9363
	if(target_device.type != "ad9363" && new_value.dcxo_tune_coarse != old_value.dcxo_tune_coarse){
		iio_device_attr_write_longlong(phy,"dcxo_tune_coarse",(long long)new_value.dcxo_tune_coarse);
		iio_device_attr_read_longlong(phy,"dcxo_tune_coarse", &aux);
		global_settings.dcxo_tune_coarse = (CORBA::Long)aux;
	}

	// This feature does not exist on the AD9363
	if(target_device.type != "ad9363" && new_value.dcxo_tune_fine != old_value.dcxo_tune_fine){
		iio_device_attr_write_longlong(phy,"dcxo_tune_fine",(long long)new_value.dcxo_tune_fine);
		iio_device_attr_read_longlong(phy,"dcxo_tune_fine", &aux);
		global_settings.dcxo_tune_fine = (CORBA::Long)aux;
	}

	if(new_value.xo_correction != old_value.xo_correction){
		iio_device_attr_write_longlong(phy,"xo_correction",(long long)new_value.xo_correction);
		iio_device_attr_read_longlong(phy,"xo_correction", &aux);
		global_settings.xo_correction = (CORBA::Long)aux;
	}

	if(new_value.filter_fir_en != old_value.filter_fir_en){
		iio_channel_attr_write_bool(iio_device_find_channel(phy, "out", false),"voltage_filter_fir_en", new_value.filter_fir_en);
		iio_channel_attr_read_bool(iio_device_find_channel(phy, "out", false),"voltage_filter_fir_en", &global_settings.filter_fir_en);
		if(global_settings.filter_fir_en){
			{
				enqueued_scoped_lock rx_buf_lock(rx_buffer_lock);
				updateReceiveSampleRate(0);
				updateReceiveBandwidth(0);
			}
			{
				enqueued_scoped_lock tx_buf_lock(tx_buffer_lock);
				updateTransmitSampleRate(0);
				updateTransmitBandwidth(0);
			}
		}
	}

	if(new_value.filter_fir_config != old_value.filter_fir_config){
		if(!loadFIRFilter(new_value.filter_fir_config)){
			LOG_ERROR(AD936X_LIBIIO_i,"Cannot read file: "<<new_value.filter_fir_config);
		}
		iio_channel_attr_write_bool(iio_device_find_channel(phy, "out", false),"voltage_filter_fir_en", false);
		iio_channel_attr_read_bool(iio_device_find_channel(phy, "out", false),"voltage_filter_fir_en", &global_settings.filter_fir_en);
	}

}

void AD936X_LIBIIO_i::receiveChainChanged(const receive_chain_struct& old_value, const receive_chain_struct& new_value){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);

	enqueued_scoped_lock buf_lock(rx_buffer_lock);

	if(new_value.rf_bandwidth != old_value.rf_bandwidth)
		updateReceiveBandwidth(new_value.rf_bandwidth);

	if(new_value.frequency != old_value.frequency)
		updateReceiveFrequency(new_value.frequency);

	if(new_value.sampling_frequency != old_value.sampling_frequency)
		updateReceiveSampleRate(new_value.sampling_frequency);

	// This feature does not exist on the AD9363
	if(target_device.type != "ad9363" && new_value.rx_lo_external != old_value.rx_lo_external)
		iio_channel_attr_write_bool(rx_LO,"external", new_value.rx_lo_external);

	if(new_value.rf_port_select != old_value.rf_port_select)
		updateReceivePort(new_value.rf_port_select);

	if(new_value.quadrature_tracking_en != old_value.quadrature_tracking_en)
		iio_channel_attr_write_bool(ad936x_tuners[0].config,"quadrature_tracking_en", new_value.quadrature_tracking_en);

	if(new_value.rf_dc_offset_tracking_en != old_value.rf_dc_offset_tracking_en)
		iio_channel_attr_write_bool(ad936x_tuners[0].config,"rf_dc_offset_tracking_en", new_value.rf_dc_offset_tracking_en);

	if(new_value.bb_dc_offset_tracking_en != old_value.bb_dc_offset_tracking_en)
		iio_channel_attr_write_bool(ad936x_tuners[0].config,"bb_dc_offset_tracking_en", new_value.bb_dc_offset_tracking_en);

	if(new_value.rx1_hardwaregain != old_value.rx1_hardwaregain){
		iio_channel_attr_write_longlong(ad936x_tuners[0].config,"hardwaregain",(long long)new_value.rx1_hardwaregain);
		iio_channel_attr_read_double(ad936x_tuners[0].config,"hardwaregain", &receive_chain.rx1_hardwaregain);
	}

	if(new_value.rx1_gain_control_mode != old_value.rx1_gain_control_mode)
		iio_channel_attr_write(ad936x_tuners[0].config, "gain_control_mode", new_value.rx1_gain_control_mode.c_str());

	if(!isAD9364 && new_value.rx2_hardwaregain != old_value.rx2_hardwaregain){
		iio_channel_attr_write_longlong(ad936x_tuners[2].config,"hardwaregain",(long long)new_value.rx2_hardwaregain);
		iio_channel_attr_read_double(ad936x_tuners[2].config,"hardwaregain", &receive_chain.rx2_hardwaregain);
	}

	if(!isAD9364 && new_value.rx2_gain_control_mode != old_value.rx2_gain_control_mode)
		iio_channel_attr_write(ad936x_tuners[2].config, "gain_control_mode", new_value.rx2_gain_control_mode.c_str());
}

void AD936X_LIBIIO_i::transmitChainChanged(const transmit_chain_struct& old_value, const transmit_chain_struct& new_value){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);

	enqueued_scoped_lock buf_lock(tx_buffer_lock);

	if(new_value.rf_bandwidth != old_value.rf_bandwidth)
		updateTransmitBandwidth(new_value.rf_bandwidth);

	if(new_value.frequency != old_value.frequency)
		updateTransmitFrequency(new_value.frequency);

	if(new_value.sampling_frequency != old_value.sampling_frequency)
		updateTransmitSampleRate(new_value.sampling_frequency);

	// This feature does not exist on the AD9363
	if(target_device.type != "ad9363" && new_value.tx_lo_external != old_value.tx_lo_external)
		iio_channel_attr_write_bool(tx_LO,"external", new_value.tx_lo_external);

	if(new_value.rf_port_select != old_value.rf_port_select)
		updateTransmitPort(new_value.rf_port_select);

	if(new_value.tx1_hardwaregain != old_value.tx1_hardwaregain){
		iio_channel_attr_write_longlong(ad936x_tuners[1].config,"hardwaregain",(long long)new_value.tx1_hardwaregain);
		iio_channel_attr_read_double(ad936x_tuners[1].config,"hardwaregain", &transmit_chain.tx1_hardwaregain);
	}

	if(!isAD9364 && new_value.tx2_hardwaregain != old_value.tx2_hardwaregain){
		iio_channel_attr_write_longlong(ad936x_tuners[3].config,"hardwaregain",(long long)new_value.tx2_hardwaregain);
		iio_channel_attr_read_double(ad936x_tuners[3].config,"hardwaregain", &transmit_chain.tx2_hardwaregain);
	}
}

void AD936X_LIBIIO_i::deviceGroupIdChanged(std::string old_value, std::string new_value){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);

	updateGroupId(new_value);
}

/*************************************************************
Functions supporting tuning allocation
*************************************************************/
void AD936X_LIBIIO_i::deviceEnable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id );

	bool prev_enabled = frontend_tuner_status[tuner_id].enabled;

	if(frontend_tuner_status[tuner_id].tuner_type == "TX"){

		enqueued_scoped_lock buf_lock(tx_buffer_lock);
		enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);

		iio_channel_enable(ad936x_tuners[tuner_id].inphase);
		iio_channel_enable(ad936x_tuners[tuner_id].quadrature);

		if (tx_buffer)
			destroyTransmitBuffer();

		createTransmitBuffer();

		if(!tx_buffer){
			iio_channel_disable(ad936x_tuners[tuner_id].inphase);
			iio_channel_disable(ad936x_tuners[tuner_id].quadrature);
			LOG_ERROR(AD936X_LIBIIO_i,"Could not create TX buffer! Failed enable.");
			return;
		}

		extendedRFInfoPkt *info_port = NULL;
		for(size_t i = 0; i < TX_info_port.size(); i++){
			if(TX_info_port[i].tuner_id == tuner_id && TX_info_port[i].rf_port_select == frontend_tuner_status[tuner_id].rf_port_select){
				TX_info_port[i].rfinfo_pkt.rf_center_freq = frontend_tuner_status[tuner_id].center_frequency;
				TX_info_port[i].rfinfo_pkt.if_center_freq = frontend_tuner_status[tuner_id].center_frequency;
				TX_info_port[i].rfinfo_pkt.rf_bandwidth = frontend_tuner_status[tuner_id].bandwidth;
				info_port = &TX_info_port[i];
				break;
			}
		}
		if (info_port==NULL) {
			iio_channel_disable(ad936x_tuners[tuner_id].inphase);
			iio_channel_disable(ad936x_tuners[tuner_id].quadrature);
			LOG_ERROR(AD936X_LIBIIO_i,"tunerEnable|tuner_id=" << tuner_id << "No matching RFInfo port found!! Failed enable.");
			return;
		}

		if(!prev_enabled){
			LOG_DEBUG(AD936X_LIBIIO_i,"tunerEnable|tuner_id=" << tuner_id << "Sending updated rfinfo_pkt: port="<<info_port->port
			                    <<" rf_center_freq="<<info_port->rfinfo_pkt.rf_center_freq
			                    <<" if_center_freq="<<info_port->rfinfo_pkt.if_center_freq
			                    <<" bandwidth="<<info_port->rfinfo_pkt.rf_bandwidth);
			if(info_port->port == "TX1A_0")
				TX1A_0->rfinfo_pkt(info_port->rfinfo_pkt);
			else if(info_port->port == "TX1B_1")
				TX1B_1->rfinfo_pkt(info_port->rfinfo_pkt);
			else if(!isAD9364 && info_port->port == "TX2A_2")
				TX2A_2->rfinfo_pkt(info_port->rfinfo_pkt);
			else if(!isAD9364 && info_port->port == "TX2B_3")
				TX2B_3->rfinfo_pkt(info_port->rfinfo_pkt);

			ad936x_tuners[tuner_id].update_sri = false;
		}

		frontend_tuner_status[tuner_id].enabled = true;

	}else{ //RX

		enqueued_scoped_lock buf_lock(rx_buffer_lock);
		enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);

		iio_channel_enable(ad936x_tuners[tuner_id].inphase);
		iio_channel_enable(ad936x_tuners[tuner_id].quadrature);

		if (rx_buffer)
			destroyReceiveBuffer();

		createReceiveBuffer();

		if(!rx_buffer){
			iio_channel_disable(ad936x_tuners[tuner_id].inphase);
			iio_channel_disable(ad936x_tuners[tuner_id].quadrature);
			LOG_ERROR(AD936X_LIBIIO_i,"Could not start RX buffer! Failed enable.");
			return;
		}

		std::string stream_id = getStreamId(tuner_id);

		if(!prev_enabled){
			LOG_DEBUG(AD936X_LIBIIO_i,"AD936X_LIBIIO_i::tunerEnable|creating SRI for tuner: "<<tuner_id<<" with stream id: "<< stream_id);
			BULKIO::StreamSRI sri = create(stream_id, frontend_tuner_status[tuner_id]);
			sri.mode = 1; // complex
			dataShortRX_out->pushSRI(sri);
			ad936x_tuners[tuner_id].update_sri = false;
		}

		frontend_tuner_status[tuner_id].enabled = true;
	}

	LOG_DEBUG(AD936X_LIBIIO_i,"deviceEnable| "<<frontend_tuner_status[tuner_id].tuner_type<<" tuner_id = " << tuner_id << " Enabled!");
}

void AD936X_LIBIIO_i::deviceDisable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);

	if(frontend_tuner_status[tuner_id].tuner_type == "TX"){
		enqueued_scoped_lock buf_lock(tx_buffer_lock);
		enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);
		iio_channel_disable(ad936x_tuners[tuner_id].inphase);
		iio_channel_disable(ad936x_tuners[tuner_id].quadrature);
		destroyTransmitBuffer();
		if(frontend_tuner_status[(tuner_id+2)%ad936x_tuners.size()].enabled){
			createTransmitBuffer();
		}
		ad936x_tuners[tuner_id].update_sri = false;
		frontend_tuner_status[tuner_id].enabled = false;
	}else if(frontend_tuner_status[tuner_id].tuner_type == "RX_DIGITIZER"){
		enqueued_scoped_lock buf_lock(rx_buffer_lock);
		enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);
		iio_channel_disable(ad936x_tuners[tuner_id].inphase);
		iio_channel_disable(ad936x_tuners[tuner_id].quadrature);
		destroyReceiveBuffer();
		if(frontend_tuner_status[(tuner_id+2)%ad936x_tuners.size()].enabled){
			createReceiveBuffer();
		}
		ad936x_tuners[tuner_id].update_sri = false;
		frontend_tuner_status[tuner_id].enabled = false;
	}
	LOG_DEBUG(AD936X_LIBIIO_i,"deviceDisable| "<<frontend_tuner_status[tuner_id].tuner_type<<" tuner_id = " << tuner_id << " Disabled!");
}

bool AD936X_LIBIIO_i::deviceSetTuning(const frontend::frontend_tuner_allocation_struct &request, frontend_tuner_status_struct_struct &fts, size_t tuner_id){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);

	extendedRFInfoPkt *info_port = NULL;

	if(fts.tuner_type == "RX_DIGITIZER"){

		for(size_t i = 0; i < RX_info_port.size(); i++){
			if(RX_info_port[i].tuner_id == tuner_id && RX_info_port[i].rf_port_select == fts.rf_port_select){
				info_port = &RX_info_port[i];
				break;
			}
		}
		if (info_port==NULL) {
			LOG_ERROR(AD936X_LIBIIO_i,"deviceSetTuning|tuner_id="<<tuner_id<<" rf_port_select="<<fts.rf_port_select<<". No matching RFInfo port found!! Failed allocation.");
			throw CF::Device::InvalidState("No matching RFInfo port found! Device must be in an invalid state.");
			return false;
		}

		{ // scope for prop_lock
			exclusive_lock lock(prop_lock);

			try {
				if(!frontend::validateRequestVsDevice(request, info_port->rfinfo_pkt, true, min_CENTER_FREQ, max_CENTER_FREQ,
						max_BANDWIDTH, max_SAMPLE_RATE)){
					throw FRONTEND::BadParameterException("INVALID REQUEST -- falls outside of analog input or device capabilities");
				}
			} catch(FRONTEND::BadParameterException& e){
				LOG_INFO(AD936X_LIBIIO_i,"deviceSetTuning|BadParameterException - " << e.msg);
				return false;
			}

		} // end scope for prop_lock

		enqueued_scoped_lock buf_lock(rx_buffer_lock);

		// configure hardware
		updateReceiveFrequency(request.center_frequency);
		updateReceiveBandwidth(request.bandwidth);
		updateReceiveSampleRate(request.sample_rate);

		// update tolerance
		fts.bandwidth_tolerance = request.bandwidth_tolerance;
		fts.sample_rate_tolerance = request.sample_rate_tolerance;

		std::string stream_id = getStreamId(tuner_id);

		matchAllocationIdToStreamId(request.allocation_id, stream_id, "dataShortRX_out");
		LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << "Updated dataShortRX_out connection table with streamID: "<<stream_id<<" for tuner_id=" << tuner_id);

		ad936x_tuners[tuner_id].update_sri = true;

	}else if(fts.tuner_type == "TX"){

		for(size_t i = 0; i < TX_info_port.size(); i++){
			if(TX_info_port[i].tuner_id == tuner_id && TX_info_port[i].rf_port_select == fts.rf_port_select){
				info_port = &TX_info_port[i];
				break;
			}
		}
		if (info_port==NULL) {
			LOG_ERROR(AD936X_LIBIIO_i,"deviceSetTuning|tuner_id="<<tuner_id<<" rf_port_select="<<fts.rf_port_select<<". No matching RFInfo port found!! Failed allocation.");
			throw CF::Device::InvalidState("No matching RFInfo port found! Device must be in an invalid state.");
			return false;
		}

		{ // scope for prop_lock
			exclusive_lock lock(prop_lock);

			try {
				if(!frontend::validateRequestVsDevice(request, info_port->rfinfo_pkt, true, min_CENTER_FREQ, max_CENTER_FREQ,
						max_BANDWIDTH, max_SAMPLE_RATE)){
					throw FRONTEND::BadParameterException("INVALID REQUEST -- falls outside of analog input or device capabilities");
				}
			} catch(FRONTEND::BadParameterException& e){
				LOG_INFO(AD936X_LIBIIO_i,"deviceSetTuning|BadParameterException - " << e.msg);
				return false;
			}

		} // end scope for prop_lock

		enqueued_scoped_lock buf_lock(tx_buffer_lock);

		// configure hardware
		updateTransmitFrequency(request.center_frequency);
		updateTransmitBandwidth(request.bandwidth);
		updateTransmitSampleRate(request.sample_rate);

		// update tolerance
		fts.bandwidth_tolerance = request.bandwidth_tolerance;
		fts.sample_rate_tolerance = request.sample_rate_tolerance;

	}else {
        LOG_ERROR(AD936X_LIBIIO_i,"deviceSetTuning|Invalid tuner type. Must be RX_DIGITIZER or TX");
        throw FRONTEND::BadParameterException("deviceSetTuning|Invalid tuner type. Must be RX_DIGITIZER or TX");
    }

    return true;
}
bool AD936X_LIBIIO_i::deviceDeleteTuning(frontend_tuner_status_struct_struct &fts, size_t tuner_id) {
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);

	fts.center_frequency = 0.0;
	fts.sample_rate = 0.0;
	fts.bandwidth = 0.0;

	fts.stream_id.clear();

    return true;
}

/*************************************************************
Functions servicing the tuner control port
*************************************************************/
std::string AD936X_LIBIIO_i::getTunerType(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].tuner_type;
}

bool AD936X_LIBIIO_i::getTunerDeviceControl(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if (getControlAllocationId(idx) == allocation_id)
        return true;
    return false;
}

std::string AD936X_LIBIIO_i::getTunerGroupId(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].group_id;
}

std::string AD936X_LIBIIO_i::getTunerRfFlowId(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].rf_flow_id;
}

void AD936X_LIBIIO_i::setTunerCenterFrequency(const std::string& allocation_id, double freq){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (freq<0) throw FRONTEND::BadParameterException();

    if(frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER"){
    	enqueued_scoped_lock buf_lock(rx_buffer_lock);
    	updateReceiveFrequency(freq);
    }else if(frontend_tuner_status[idx].tuner_type == "TX"){
    	enqueued_scoped_lock buf_lock(tx_buffer_lock);
    	updateTransmitFrequency(freq);
    }
}

double AD936X_LIBIIO_i::getTunerCenterFrequency(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].center_frequency;
}

void AD936X_LIBIIO_i::setTunerBandwidth(const std::string& allocation_id, double bw){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (bw<0) throw FRONTEND::BadParameterException();

    if(frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER"){
		enqueued_scoped_lock buf_lock(rx_buffer_lock);
		updateReceiveBandwidth(bw);
	}else if(frontend_tuner_status[idx].tuner_type == "TX"){
		enqueued_scoped_lock buf_lock(tx_buffer_lock);
		updateTransmitBandwidth(bw);
	}
}

double AD936X_LIBIIO_i::getTunerBandwidth(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].bandwidth;
}

void AD936X_LIBIIO_i::setTunerAgcEnable(const std::string& allocation_id, bool enable){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    throw FRONTEND::NotSupportedException("setTunerAgcEnable not supported");
}

bool AD936X_LIBIIO_i::getTunerAgcEnable(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
    throw FRONTEND::NotSupportedException("getTunerAgcEnable not supported");
}

void AD936X_LIBIIO_i::setTunerGain(const std::string& allocation_id, float gain){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	long idx = getTunerMapping(allocation_id);
	if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");

	iio_channel_attr_write_longlong(ad936x_tuners[idx].config,"hardwaregain",(long long)gain);
	if(frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER"){
		if(idx == 0)
			iio_channel_attr_read_double(ad936x_tuners[idx].config,"hardwaregain", &receive_chain.rx1_hardwaregain);
		else if(idx == 2)
			iio_channel_attr_read_double(ad936x_tuners[idx].config,"hardwaregain", &receive_chain.rx2_hardwaregain);
	}else if(frontend_tuner_status[idx].tuner_type == "TX"){
		if(idx == 1)
			iio_channel_attr_read_double(ad936x_tuners[idx].config,"hardwaregain", &transmit_chain.tx1_hardwaregain);
		else if(idx == 3)
			iio_channel_attr_read_double(ad936x_tuners[idx].config,"hardwaregain", &transmit_chain.tx2_hardwaregain);
	}
}

float AD936X_LIBIIO_i::getTunerGain(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	double gain;
	long idx = getTunerMapping(allocation_id);
	if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");

	iio_channel_attr_read_double(ad936x_tuners[idx].config,"hardwaregain", &gain);
	return (float)gain;
}

void AD936X_LIBIIO_i::setTunerReferenceSource(const std::string& allocation_id, long source){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	long idx = getTunerMapping(allocation_id);
	if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");

	if(target_device.type == "ad9363"){
		throw FRONTEND::NotSupportedException("setTunerReferenceSource not supported for AD9363");
	}

	if(frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER"){
		iio_channel_attr_write_bool(rx_LO,"external",(source)? true:false);
		iio_channel_attr_read_bool(rx_LO,"external", &receive_chain.rx_lo_external);
	}else if(frontend_tuner_status[idx].tuner_type == "TX"){
		iio_channel_attr_write_bool(tx_LO,"external",(source)? true:false);
		iio_channel_attr_read_bool(tx_LO,"external", &transmit_chain.tx_lo_external);
	}
}

long AD936X_LIBIIO_i::getTunerReferenceSource(const std::string& allocation_id){
	LOG_DEBUG(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	long idx = getTunerMapping(allocation_id);
	if(target_device.type == "ad9363"){
		throw FRONTEND::NotSupportedException("getTunerReferenceSource not supported for AD9363");
	}
	if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
	if(frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER"){
		return (receive_chain.rx_lo_external)? 1:0;
	}else if(frontend_tuner_status[idx].tuner_type == "TX"){
		return (transmit_chain.tx_lo_external)? 1:0;
	}
	return 0;
}

void AD936X_LIBIIO_i::setTunerEnable(const std::string& allocation_id, bool enable) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());

    if(enable)
    	deviceEnable(frontend_tuner_status[idx], idx);
    else
    	deviceDisable(frontend_tuner_status[idx], idx);
}

bool AD936X_LIBIIO_i::getTunerEnable(const std::string& allocation_id) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].enabled;
}

void AD936X_LIBIIO_i::setTunerOutputSampleRate(const std::string& allocation_id, double sr) {
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx))
        throw FRONTEND::FrontendException(("ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    if (sr<0) throw FRONTEND::BadParameterException();

    if(frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER"){
		enqueued_scoped_lock buf_lock(rx_buffer_lock);
		updateReceiveSampleRate(sr);
	}else if(frontend_tuner_status[idx].tuner_type == "TX"){
		enqueued_scoped_lock buf_lock(tx_buffer_lock);
		updateTransmitSampleRate(sr);
	}
}

double AD936X_LIBIIO_i::getTunerOutputSampleRate(const std::string& allocation_id){
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::FrontendException("Invalid allocation id");
    return frontend_tuner_status[idx].sample_rate;
}

std::string AD936X_LIBIIO_i::getStreamId(size_t tuner_id) {
    if (tuner_id >= ad936x_tuners.size())
        return "ERR: INVALID TUNER ID";
    if (frontend_tuner_status[tuner_id].stream_id.empty()){
        std::ostringstream id;
        id<<"tuner_freq_"<<long(frontend_tuner_status[tuner_id].center_frequency)<<"_Hz_"<<frontend::uuidGenerator();
        frontend_tuner_status[tuner_id].stream_id = id.str();
        ad936x_tuners[tuner_id].update_sri = true;
    }
    return frontend_tuner_status[tuner_id].stream_id;
}
/*************************************************************
Functions servicing the RFInfo port(s)
- port_name is the port over which the call was received
*************************************************************/
std::string AD936X_LIBIIO_i::get_rf_flow_id(const std::string& port_name){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " port_name=" << port_name);

	if(port_name.substr(0,2) == "RX"){
		extendedRFInfoPkt *rx_port = &RX_info_port[(int)(port_name.at(5)-'0')];
		exclusive_lock lock(prop_lock);
		return rx_port->rfinfo_pkt.rf_flow_id;
	}else if(port_name.substr(0,2) == "TX"){
		extendedRFInfoPkt *tx_port = &TX_info_port[(int)(port_name.at(5)-'0')];
		exclusive_lock lock(prop_lock);
		return tx_port->rfinfo_pkt.rf_flow_id;
	}else{
		LOG_WARN(AD936X_LIBIIO_i, "get_rf_flow_id|Unknown port name: " << port_name);
		return "";
	}
}

void AD936X_LIBIIO_i::set_rf_flow_id(const std::string& port_name, const std::string& id){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " port_name=" << port_name << " id=" << id);

	if(port_name.substr(0,2) == "RX"){
		extendedRFInfoPkt *rx_port = &RX_info_port[(int)(port_name.at(5)-'0')];
		exclusive_lock lock(prop_lock);
		rx_port->rfinfo_pkt.rf_flow_id = id;
		updateRfFlowId(rx_port);
	}else if(port_name.substr(0,2) == "TX"){
		extendedRFInfoPkt *tx_port = &TX_info_port[(int)(port_name.at(5)-'0')];
		exclusive_lock lock(prop_lock);
		tx_port->rfinfo_pkt.rf_flow_id = id;
		updateRfFlowId(tx_port);
	}else{
		LOG_WARN(AD936X_LIBIIO_i, "set_rf_flow_id|Unknown port name: " << port_name);
	}
}

frontend::RFInfoPkt AD936X_LIBIIO_i::get_rfinfo_pkt(const std::string& port_name){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " port_name=" << port_name);

    frontend::RFInfoPkt tmp;
    extendedRFInfoPkt *rf_info;

    if(port_name.substr(0,2) == "RX"){
    	rf_info = &RX_info_port[(int)(port_name.at(5)-'0')];
	}else if(port_name.substr(0,2) == "TX"){
		rf_info = &TX_info_port[(int)(port_name.at(5)-'0')];
	}else{
		LOG_WARN(AD936X_LIBIIO_i, "get_rfinfo_pkt|Unknown port name: " << port_name);
		return tmp;
	}
    exclusive_lock lock(prop_lock);
    tmp.rf_flow_id = rf_info->rfinfo_pkt.rf_flow_id;
	tmp.rf_center_freq = rf_info->rfinfo_pkt.rf_center_freq;
	tmp.rf_bandwidth = rf_info->rfinfo_pkt.rf_bandwidth;
	tmp.if_center_freq = rf_info->rfinfo_pkt.if_center_freq;
	tmp.spectrum_inverted = rf_info->rfinfo_pkt.spectrum_inverted;
	tmp.sensor.collector = rf_info->rfinfo_pkt.sensor.collector;
	tmp.sensor.mission = rf_info->rfinfo_pkt.sensor.mission;
	tmp.sensor.rx = rf_info->rfinfo_pkt.sensor.rx;
	tmp.sensor.antenna.description = rf_info->rfinfo_pkt.sensor.antenna.description;
	tmp.sensor.antenna.name = rf_info->rfinfo_pkt.sensor.antenna.name;
	tmp.sensor.antenna.size = rf_info->rfinfo_pkt.sensor.antenna.size;
	tmp.sensor.antenna.type = rf_info->rfinfo_pkt.sensor.antenna.type;
	tmp.sensor.feed.name = rf_info->rfinfo_pkt.sensor.feed.name;
	tmp.sensor.feed.polarization = rf_info->rfinfo_pkt.sensor.feed.polarization;
	tmp.sensor.feed.freq_range.max_val = rf_info->rfinfo_pkt.sensor.feed.freq_range.max_val;
	tmp.sensor.feed.freq_range.min_val = rf_info->rfinfo_pkt.sensor.feed.freq_range.min_val;
	tmp.sensor.feed.freq_range.values.resize(rf_info->rfinfo_pkt.sensor.feed.freq_range.values.size());
	for (unsigned int i=0; i<rf_info->rfinfo_pkt.sensor.feed.freq_range.values.size(); i++) {
		tmp.sensor.feed.freq_range.values[i] = rf_info->rfinfo_pkt.sensor.feed.freq_range.values[i];
	}
	return tmp;
}

void AD936X_LIBIIO_i::set_rfinfo_pkt(const std::string& port_name, const frontend::RFInfoPkt &pkt){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " port_name=" << port_name << " pkt.rf_flow_id=" << pkt.rf_flow_id);

	extendedRFInfoPkt *rf_info;

	if(port_name.substr(0,2) == "RX"){
		rf_info = &RX_info_port[(int)(port_name.at(5)-'0')];
	}else if(port_name.substr(0,2) == "TX"){
		rf_info = &TX_info_port[(int)(port_name.at(5)-'0')];
	}else{
		LOG_WARN(AD936X_LIBIIO_i, "set_rfinfo_pkt|Unknown port name: " << port_name);
		return;
	}

	exclusive_lock lock(prop_lock);
	rf_info->rfinfo_pkt.rf_flow_id = pkt.rf_flow_id;
	rf_info->rfinfo_pkt.rf_center_freq = pkt.rf_center_freq;
	rf_info->rfinfo_pkt.rf_bandwidth = pkt.rf_bandwidth;
	rf_info->rfinfo_pkt.if_center_freq = pkt.if_center_freq;
	rf_info->rfinfo_pkt.spectrum_inverted = pkt.spectrum_inverted;
	rf_info->rfinfo_pkt.sensor.collector = pkt.sensor.collector;
	rf_info->rfinfo_pkt.sensor.mission = pkt.sensor.mission;
	rf_info->rfinfo_pkt.sensor.rx = pkt.sensor.rx;
	rf_info->rfinfo_pkt.sensor.antenna.description = pkt.sensor.antenna.description;
	rf_info->rfinfo_pkt.sensor.antenna.name = pkt.sensor.antenna.name;
	rf_info->rfinfo_pkt.sensor.antenna.size = pkt.sensor.antenna.size;
	rf_info->rfinfo_pkt.sensor.antenna.type = pkt.sensor.antenna.type;
	rf_info->rfinfo_pkt.sensor.feed.name = pkt.sensor.feed.name;
	rf_info->rfinfo_pkt.sensor.feed.polarization = pkt.sensor.feed.polarization;
	rf_info->rfinfo_pkt.sensor.feed.freq_range.max_val = pkt.sensor.feed.freq_range.max_val;
	rf_info->rfinfo_pkt.sensor.feed.freq_range.min_val = pkt.sensor.feed.freq_range.min_val;
	rf_info->rfinfo_pkt.sensor.feed.freq_range.values.resize(pkt.sensor.feed.freq_range.values.size());
	for (unsigned int i=0; i<pkt.sensor.feed.freq_range.values.size(); i++) {
		rf_info->rfinfo_pkt.sensor.feed.freq_range.values[i] = pkt.sensor.feed.freq_range.values[i];
	}
	updateRfFlowId(rf_info);
}

void AD936X_LIBIIO_i::updateRfFlowId(extendedRFInfoPkt *rf_info){
    LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);

    enqueued_scoped_lock tuner_lock(ad936x_tuners[rf_info->tuner_id].lock);
	frontend_tuner_status[rf_info->tuner_id].rf_flow_id = rf_info->rfinfo_pkt.rf_flow_id;
	ad936x_tuners[rf_info->tuner_id].update_sri = true;
}

void AD936X_LIBIIO_i::updateGroupId(const std::string &group){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__ << " group=" << group);

	for(size_t tuner_id = 0; tuner_id < frontend_tuner_status.size(); tuner_id++){
		enqueued_scoped_lock tuner_lock(ad936x_tuners[tuner_id].lock);
		frontend_tuner_status[tuner_id].group_id = group;
	}
}

/*************************************************************
	interface with ad936x device
*************************************************************/
void AD936X_LIBIIO_i::initAD936x() throw (CF::PropertySet::InvalidConfiguration) {
    LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);

    unsigned int nb_channels, i;

    try{
    	if(target_device.type == "ad9361" || target_device.type == "ad9364"){
    		min_CENTER_FREQ = 70000000; 	// 70 MHz
    		max_CENTER_FREQ = 6000000000; 	//  6 GHz
    		max_BANDWIDTH = 56000000; 		// 56 MHz
    		max_SAMPLE_RATE = 20000000; 	// assume a limit?
    	}else if(target_device.type == "ad9363"){
    		min_CENTER_FREQ = 325000000; 	//325 MHz
			max_CENTER_FREQ = 3800000000; 	//3.8 GHz
			max_BANDWIDTH = 20000000; 		// 20 MHz
			max_SAMPLE_RATE = 20000000; 	// assume a limit?
    	}

    	isAD9364 = (target_device.type == "ad9364")? true : false;

    	context = getContext(target_device.context_uri);
    	rx_device = iio_context_find_device(context, "cf-ad9361-lpc");
    	tx_device = iio_context_find_device(context, "cf-ad9361-dds-core-lpc");
    	phy = iio_context_find_device(context, "ad9361-phy");

    	if(!rx_device || !tx_device || !phy){
			LOG_ERROR(AD936X_LIBIIO_i,"Could not find iio device in the context!");
			throw CF::PropertySet::InvalidConfiguration();
		}

    	// First disable all channels
		nb_channels = iio_device_get_channels_count(rx_device);
		for (i = 0; i < nb_channels; i++)
			iio_channel_disable(iio_device_get_channel(rx_device, i));

		nb_channels = iio_device_get_channels_count(tx_device);
		for (i = 0; i < nb_channels; i++)
			iio_channel_disable(iio_device_get_channel(tx_device, i));

    	const size_t max_payload_size    = (size_t) (bulkio::Const::MAX_TRANSFER_BYTES * .3);
    	buffer_size = size_t((max_payload_size/sizeof(short))/1024)*1024;

    	rx_LO = iio_device_find_channel(phy, "altvoltage0", true);
    	tx_LO = iio_device_find_channel(phy, "altvoltage1", true);
    	if(!rx_LO || !tx_LO){
			LOG_ERROR(AD936X_LIBIIO_i,"Could not find RX/TX LO iio channels!");
			throw CF::PropertySet::InvalidConfiguration();
		}

    	//iio_channel_attr_write_bool(iio_device_find_channel(phy, "out", false),"voltage_filter_fir_en", true);

    	if(isAD9364){
			ad936x_tuners.resize(2);
			AD936X_LIBIIO_base::setNumChannels(2);
    	}else{
			ad936x_tuners.resize(4);
			AD936X_LIBIIO_base::setNumChannels(4);
    	}

    	ad936x_tuners[0].inphase = iio_device_find_channel(rx_device, "voltage0", false);
    	ad936x_tuners[0].quadrature = iio_device_find_channel(rx_device, "voltage1", false);
    	ad936x_tuners[0].config = iio_device_find_channel(phy, "voltage0", false);
    	ad936x_tuners[0].buffer.resize(2*buffer_size);

    	ad936x_tuners[1].inphase = iio_device_find_channel(tx_device, "voltage0", true);
		ad936x_tuners[1].quadrature = iio_device_find_channel(tx_device, "voltage1", true);
		ad936x_tuners[1].config = iio_device_find_channel(phy, "voltage0", true);
		ad936x_tuners[1].buffer.resize(0); // we don't need this buffer, at least for now

    	if(!ad936x_tuners[0].inphase || !ad936x_tuners[0].quadrature || !ad936x_tuners[0].config
    			|| !ad936x_tuners[1].inphase || !ad936x_tuners[1].quadrature || !ad936x_tuners[1].config){
			LOG_ERROR(AD936X_LIBIIO_i,"Could not find iio channels!");
			throw CF::PropertySet::InvalidConfiguration();
		}

    	if(!isAD9364){
    		ad936x_tuners[2].inphase = iio_device_find_channel(rx_device, "voltage2", false);
			ad936x_tuners[2].quadrature = iio_device_find_channel(rx_device, "voltage3", false);
			ad936x_tuners[2].config = iio_device_find_channel(phy, "voltage1", false);
			ad936x_tuners[2].buffer.resize(2*buffer_size);

			ad936x_tuners[3].inphase = iio_device_find_channel(tx_device, "voltage2", true);
			ad936x_tuners[3].quadrature = iio_device_find_channel(tx_device, "voltage3", true);
			ad936x_tuners[3].config = iio_device_find_channel(phy, "voltage1", true);
			ad936x_tuners[3].buffer.resize(0); // we don't need this buffer, at least for now

			if(!ad936x_tuners[2].inphase || !ad936x_tuners[2].quadrature || !ad936x_tuners[2].config
					|| !ad936x_tuners[3].inphase || !ad936x_tuners[3].quadrature || !ad936x_tuners[3].config){
				LOG_ERROR(AD936X_LIBIIO_i,"Could not find iio channels!");
				throw CF::PropertySet::InvalidConfiguration();
			}
    	}

    	long long aux;
		if(target_device.type != "ad9363"){
			iio_device_attr_read_longlong(phy,"dcxo_tune_coarse", &aux);
			global_settings.dcxo_tune_coarse = (CORBA::Long)aux;
			iio_device_attr_read_longlong(phy,"dcxo_tune_fine", &aux);
			global_settings.dcxo_tune_fine = (CORBA::Long)aux;
		}
		iio_device_attr_read_longlong(phy,"xo_correction", &aux);
		global_settings.xo_correction = (CORBA::Long)aux;

		iio_channel_attr_write_bool(iio_device_find_channel(phy, "out", false),"voltage_filter_fir_en", false);
		global_settings.filter_fir_en = false;
		global_settings.filter_fir_config = "";

    	loadCurrentConfig();

		// Initialize remaining values for frontend_tuner_status and ad936x_tuners structs
		for (size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id++) {

			if (ad936x_tuners[tuner_id].lock.cond == NULL)
				ad936x_tuners[tuner_id].lock.cond = new boost::condition_variable;
			if (ad936x_tuners[tuner_id].lock.mutex == NULL)
				ad936x_tuners[tuner_id].lock.mutex = new boost::mutex;

			frontend_tuner_status[tuner_id].allocation_id_csv = "";
			frontend_tuner_status[tuner_id].enabled = iio_channel_is_enabled(ad936x_tuners[tuner_id].inphase);;
			frontend_tuner_status[tuner_id].group_id = global_group_id;

			if(iio_channel_is_output(ad936x_tuners[tuner_id].config))
				frontend_tuner_status[tuner_id].tuner_type = "TX";
			else
				frontend_tuner_status[tuner_id].tuner_type = "RX_DIGITIZER";

			frontend_tuner_status[tuner_id].stream_id.clear();
			frontend_tuner_status[tuner_id].sample_rate_tolerance = 0.0;
			frontend_tuner_status[tuner_id].bandwidth_tolerance = 0.0;
		}

		// Initialize buffer locks
		rx_buffer_lock.cond = new boost::condition_variable;
		rx_buffer_lock.mutex = new boost::mutex;
		tx_buffer_lock.cond = new boost::condition_variable;
		tx_buffer_lock.mutex = new boost::mutex;

    }catch (...) {
        LOG_ERROR(AD936X_LIBIIO_i,"AD936X COULD NOT BE INITIALIZED!");

        throw CF::PropertySet::InvalidConfiguration();
    }

}

void AD936X_LIBIIO_i::updateReceiveFrequency(double frequency){

	if(frequency)
		iio_channel_attr_write_longlong(rx_LO,"frequency",(long long)frequency);

	for(size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over RX tuners
		iio_channel_attr_read_double(rx_LO,"frequency", &frontend_tuner_status[tuner_id].center_frequency);
		ad936x_tuners[tuner_id].update_sri = true;
	}

	receive_chain.frequency = frontend_tuner_status[0].center_frequency;
}

void AD936X_LIBIIO_i::updateReceiveBandwidth(double bandwidth){

	if(bandwidth)
		iio_channel_attr_write_longlong(ad936x_tuners[0].config,"rf_bandwidth",(long long)bandwidth);

	for(size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over RX tuners
		iio_channel_attr_read_double(ad936x_tuners[tuner_id].config,"rf_bandwidth", &frontend_tuner_status[tuner_id].bandwidth);
		ad936x_tuners[tuner_id].update_sri = true;
	}

	receive_chain.rf_bandwidth = frontend_tuner_status[0].bandwidth;
}

void AD936X_LIBIIO_i::updateReceiveSampleRate(double sampleRate){

	if(sampleRate)
		iio_channel_attr_write_longlong(ad936x_tuners[0].config,"sampling_frequency",(long long)sampleRate);

	for(size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id++){ //iterate over all tuners
		iio_channel_attr_read_double(ad936x_tuners[tuner_id].config,"sampling_frequency", &frontend_tuner_status[tuner_id].sample_rate);
		ad936x_tuners[tuner_id].update_sri = true;
	}

	receive_chain.sampling_frequency = frontend_tuner_status[0].sample_rate;
	transmit_chain.sampling_frequency = frontend_tuner_status[1].sample_rate;
}

void AD936X_LIBIIO_i::updateReceivePort(const std::string& rf_port){
	char tmp[15];

	if(!rf_port.empty())
		iio_channel_attr_write(ad936x_tuners[0].config, "rf_port_select", rf_port.c_str());

	for (size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over RX tuners
		iio_channel_attr_read(ad936x_tuners[tuner_id].config,"rf_port_select", tmp, sizeof(tmp));
		frontend_tuner_status[tuner_id].rf_port_select = std::string(tmp);

		for(size_t i = 0; i < RX_info_port.size(); i++){
			if(RX_info_port[i].tuner_id == tuner_id && RX_info_port[i].rf_port_select == frontend_tuner_status[tuner_id].rf_port_select){
				frontend_tuner_status[tuner_id].rf_flow_id = RX_info_port[i].rfinfo_pkt.rf_flow_id;
				ad936x_tuners[tuner_id].update_sri = true;
				break;
			}
		}
	}

	receive_chain.rf_port_select = frontend_tuner_status[0].rf_port_select;
}

void AD936X_LIBIIO_i::updateTransmitFrequency(double frequency){

	if(frequency)
		iio_channel_attr_write_longlong(tx_LO,"frequency",(long long)frequency);

	for(size_t tuner_id = 1; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over TX tuners
		iio_channel_attr_read_double(tx_LO,"frequency", &frontend_tuner_status[tuner_id].center_frequency);
		ad936x_tuners[tuner_id].update_sri = true;
	}

	transmit_chain.frequency = frontend_tuner_status[1].center_frequency;
}

void AD936X_LIBIIO_i::updateTransmitBandwidth(double bandwidth){

	if(bandwidth)
		iio_channel_attr_write_longlong(ad936x_tuners[1].config,"rf_bandwidth",(long long)bandwidth);

	for(size_t tuner_id = 1; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over TX tuners
		iio_channel_attr_read_double(ad936x_tuners[tuner_id].config,"rf_bandwidth", &frontend_tuner_status[tuner_id].bandwidth);
		ad936x_tuners[tuner_id].update_sri = true;
	}

	transmit_chain.rf_bandwidth = frontend_tuner_status[1].bandwidth;
}

void AD936X_LIBIIO_i::updateTransmitSampleRate(double sampleRate){

	if(sampleRate)
		iio_channel_attr_write_longlong(ad936x_tuners[1].config,"sampling_frequency",(long long)sampleRate);

	for(size_t tuner_id = 0; tuner_id < ad936x_tuners.size(); tuner_id++){ //iterate over all tuners
		iio_channel_attr_read_double(ad936x_tuners[tuner_id].config,"sampling_frequency", &frontend_tuner_status[tuner_id].sample_rate);
		ad936x_tuners[tuner_id].update_sri = true;
	}

	receive_chain.sampling_frequency = frontend_tuner_status[0].sample_rate;
	transmit_chain.sampling_frequency = frontend_tuner_status[1].sample_rate;
}

void AD936X_LIBIIO_i::updateTransmitPort(const std::string& rf_port){
	char tmp[15];

	if(!rf_port.empty())
		iio_channel_attr_write(ad936x_tuners[1].config, "rf_port_select", rf_port.c_str());

	for (size_t tuner_id = 1; tuner_id < ad936x_tuners.size(); tuner_id+=2){ //iterate over TX tuners
		iio_channel_attr_read(ad936x_tuners[tuner_id].config,"rf_port_select", tmp, sizeof(tmp));
		frontend_tuner_status[tuner_id].rf_port_select = std::string(tmp);

		for(size_t i = 0; i < TX_info_port.size(); i++){
			if(RX_info_port[i].tuner_id == tuner_id && TX_info_port[i].rf_port_select == frontend_tuner_status[tuner_id].rf_port_select){
				frontend_tuner_status[tuner_id].rf_flow_id = TX_info_port[i].rfinfo_pkt.rf_flow_id;
				ad936x_tuners[tuner_id].update_sri = true;
				break;
			}
		}
	}

	transmit_chain.rf_port_select = frontend_tuner_status[1].rf_port_select;
}

/*
 * This is function is called get_context taken from:
 *
 * 	github.com/analogdevicesinc/gr-iio/blob/master/lib/device_source_impl.cc
 *
 * Copyright 2014 Analog Devices Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 *
 * same GNU General Public License applies here
 */
struct iio_context * AD936X_LIBIIO_i::getContext(const std::string &uri){

	struct iio_context *context;

	if (uri.empty()) {
		context = iio_create_default_context();
		if (!context)
			context = iio_create_network_context(NULL);
	}else{
		context = iio_create_context_from_uri(uri.c_str());
		if(!context)
			context = iio_create_network_context(uri.c_str());
	}

	return context;
}



bool AD936X_LIBIIO_i::createTransmitBuffer(){

	tx_buffer = iio_device_create_buffer(tx_device, buffer_size, false);

	return !!tx_buffer;
}

bool AD936X_LIBIIO_i::createReceiveBuffer(){

	rx_buffer = iio_device_create_buffer(rx_device, buffer_size, false);

	return !!rx_buffer;
}

bool AD936X_LIBIIO_i::destroyReceiveBuffer(){
	if (rx_buffer)
		iio_buffer_cancel(rx_buffer);

	if(rx_buffer){
		iio_buffer_destroy(rx_buffer);
		rx_buffer = NULL;
	}

	return true;
}

bool AD936X_LIBIIO_i::destroyTransmitBuffer(){
	if (tx_buffer)
		iio_buffer_cancel(tx_buffer);

	if(tx_buffer){
		iio_buffer_destroy(tx_buffer);
		tx_buffer = NULL;
	}

	return true;
}

void AD936X_LIBIIO_i::tunerReceive(size_t tuner_id, short *output){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);

	uintptr_t p_dat, p_end;
	ptrdiff_t p_inc;

	p_inc = iio_buffer_step(rx_buffer);
	p_end = (uintptr_t) iio_buffer_end(rx_buffer);
	for (p_dat = (uintptr_t)iio_buffer_first(rx_buffer, ad936x_tuners[tuner_id].inphase); p_dat < p_end; p_dat += p_inc) {
		*(output++) = ((short*)p_dat)[0]; // Real (I)
		*(output++) = ((short*)p_dat)[1]; // Imag (Q)
	}
}

void AD936X_LIBIIO_i::tunerTransmit(size_t tuner_id, bulkio::ShortDataBlock block){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	uintptr_t p_dat, p_end;
	ptrdiff_t p_inc;
	size_t count;
	short * input = block.data();

	if(ad936x_tuners[tuner_id].update_sri){

		extendedRFInfoPkt *info_port = NULL;
		for(size_t i = 0; i < TX_info_port.size(); i++){
			if(TX_info_port[i].tuner_id == tuner_id && TX_info_port[i].rf_port_select == frontend_tuner_status[tuner_id].rf_port_select){
				TX_info_port[i].rfinfo_pkt.rf_center_freq = frontend_tuner_status[tuner_id].center_frequency;
				TX_info_port[i].rfinfo_pkt.if_center_freq = frontend_tuner_status[tuner_id].center_frequency;
				TX_info_port[i].rfinfo_pkt.rf_bandwidth = frontend_tuner_status[tuner_id].bandwidth;
				info_port = &TX_info_port[i];
				break;
			}
		}
		if (info_port == NULL) {
			LOG_ERROR(AD936X_LIBIIO_i,"tunerTransmit|tuner_id=" << tuner_id << "No matching RFInfo port found!! Failed transmit.");
			return;
		}

		if(info_port->port == "TX1A_0")
			TX1A_0->rfinfo_pkt(info_port->rfinfo_pkt);
		else if(info_port->port == "TX1B_1")
			TX1B_1->rfinfo_pkt(info_port->rfinfo_pkt);
		else if(!isAD9364 && info_port->port == "TX2A_2")
			TX2A_2->rfinfo_pkt(info_port->rfinfo_pkt);
		else if(!isAD9364 && info_port->port == "TX2B_3")
			TX2B_3->rfinfo_pkt(info_port->rfinfo_pkt);

		ad936x_tuners[tuner_id].update_sri = false;
	}

	p_inc = iio_buffer_step(tx_buffer);
	p_end = (uintptr_t) iio_buffer_end(tx_buffer);
	for(p_dat = (uintptr_t)iio_buffer_first(tx_buffer, ad936x_tuners[tuner_id].inphase), count = 0;
			p_dat < p_end || count < block.cxsize();
			p_dat += p_inc, count++) {
		((short*)p_dat)[0] = *(input++); // Real (I)
		((short*)p_dat)[1] = *(input++); // Imag (Q)
	}

	// In case of incomplete block just pad with zeros
	for(;p_dat < p_end; p_dat += p_inc){
		((short*)p_dat)[0] = 0; // Real (I)
		((short*)p_dat)[1] = 0; // Imag (Q)
	}
}

void AD936X_LIBIIO_i::loadCurrentConfig(){
	LOG_TRACE(AD936X_LIBIIO_i,__PRETTY_FUNCTION__);
	char tmp[15];

	// Initialize Receive Chain
	updateReceivePort("");
	updateReceiveFrequency(0);
	updateReceiveBandwidth(0);
	updateReceiveSampleRate(0);

	if(target_device.type != "ad9363")
		iio_channel_attr_read_bool(rx_LO,"external", &receive_chain.rx_lo_external);
	iio_channel_attr_read_bool(ad936x_tuners[0].config,"quadrature_tracking_en", &receive_chain.quadrature_tracking_en);
	iio_channel_attr_read_bool(ad936x_tuners[0].config,"bb_dc_offset_tracking_en", &receive_chain.bb_dc_offset_tracking_en);
	iio_channel_attr_read_bool(ad936x_tuners[0].config,"rf_dc_offset_tracking_en", &receive_chain.rf_dc_offset_tracking_en);
	iio_channel_attr_read_double(ad936x_tuners[0].config,"hardwaregain", &receive_chain.rx1_hardwaregain);
	iio_channel_attr_read(ad936x_tuners[0].config,"gain_control_mode", tmp, sizeof(tmp));
	receive_chain.rx1_gain_control_mode = std::string(tmp);
	if(!isAD9364){
		iio_channel_attr_read_double(ad936x_tuners[2].config,"hardwaregain", &receive_chain.rx2_hardwaregain);
		iio_channel_attr_read(ad936x_tuners[2].config,"gain_control_mode", tmp, sizeof(tmp));
		receive_chain.rx2_gain_control_mode = std::string(tmp);
	}

	// Initialize Transmit Chain
	updateTransmitPort("");
	updateTransmitFrequency(0);
	updateTransmitBandwidth(0);
	updateTransmitSampleRate(0);

	if(target_device.type != "ad9363")
		iio_channel_attr_read_bool(tx_LO,"external", &transmit_chain.tx_lo_external);
	iio_channel_attr_read_double(ad936x_tuners[1].config,"hardwaregain", &transmit_chain.tx1_hardwaregain);
	if(!isAD9364)
		iio_channel_attr_read_double(ad936x_tuners[3].config,"hardwaregain", &transmit_chain.tx2_hardwaregain);
}


/*
 * This is function is called load_fir_filter taken from:
 *
 * 	github.com/analogdevicesinc/gr-iio/blob/master/lib/device_source_impl.cc
 *
 * Copyright 2014 Analog Devices Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 *
 * same GNU General Public License applies here
 */
bool AD936X_LIBIIO_i::loadFIRFilter(const std::string &filter){

	if (filter.empty() || !iio_device_find_attr(phy, "filter_fir_config"))
			return false;

	std::ifstream ifs(filter.c_str(), std::ifstream::binary);
	if (!ifs)
		return false;

	/* Here, we verify that the filter file contains data for both RX+TX. */
	{
		char buf[256];

		do {
			ifs.getline(buf, sizeof(buf));
		} while (!(buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')));

		std::string line(buf);
		if (line.find(',') == std::string::npos)
			LOG_ERROR(AD936X_LIBIIO_i,"Incompatible filter file");
	}

	ifs.seekg(0, ifs.end);
	int length = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	char *buffer = new char [length];

	ifs.read(buffer, length);
	ifs.close();

	int ret = iio_device_attr_write_raw(phy,"filter_fir_config", buffer, length);

	delete[] buffer;
	return ret > 0;
}




