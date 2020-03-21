#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/stream.hpp>
#include <uhd/convert.hpp>
#include <Boost/format.hpp>
#include <iostream>
#include <fstream>
#include <complex>
#include <typeinfo>

using namespace std;

static bool stop_signal_called = false;

void sig_int_handler(int){

	stop_signal_called = true;

}

void send_from_file(
	uhd::tx_streamer::sptr tx_streamer,
	const string& file,
	size_t samps_per_buff/*,
	uhd::convert::converter::sptr converter*/)
{
	int count(0);
	size_t num_tx_samps_;
	uhd::tx_metadata_t md;
	md.start_of_burst = false;
	md.end_of_burst = false;
	samps_per_buff = 3000 ; 
	vector<complex<float>> input_buff(samps_per_buff);
	vector<complex<int>> output_buff(samps_per_buff);
	
	ifstream infile(file, ifstream::binary);

	while(not stop_signal_called and not md.end_of_burst){
		
		infile.read((char*)&input_buff.front(), input_buff.size()*sizeof(complex<float>));

		size_t num_tx_samps = size_t(infile.gcount() / sizeof(complex<float>));

		//converter->conv(input_buff, output_buff, num_tx_samps);

		/*for(int i= 0; i < num_tx_samps; i++){
			cout << input_buff[i] << endl;
			count ++;
		}		
		cout << count << endl;
		count = 0;
		*/
		

		md.end_of_burst = infile.eof();

		num_tx_samps_ = tx_streamer->send(&input_buff.front(), num_tx_samps, md);

		//cout << "num_tx_samps : " << num_tx_samps_ << endl;
	
	}	

}

uhd::convert::converter::sptr make_converter(){
	
	uhd::convert::id_type converter_id;
	converter_id.input_format  = "fc32"  ;
	converter_id.output_format = "sc16"  ;  
	converter_id.num_inputs    = 3000    ; //??
	converter_id.num_outputs   = 3000    ; //??

	return get_converter(converter_id, 0)();

}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
	string dev_args       ("name = MyB200")   ;
	string clock_ref      ("internal") 	  ;
	string ant 	      ("TX/RX")		  ;
	string cpu_format     ("fc32")		  ;
	string wire_format    ("sc16")		  ;
	string file 	      ("final_test.bin")  ;
	size_t samps_per_buff			  ;
	size_t chan 	      ( 0 )          	  ;
	size_t mboard 	      ( 0 )	       	  ;
	double gain 	      ( 40.0 )         	  ;
	double rate 	      ( 5.0e6 )      	  ;
	double master_rate    ( 40.0e6 )    	  ;   	 
	double freq 	      ( 2.6e9 )		  ;
	double bw	      ( 10.0e6 )	  ;
	double num_samps      ( 5000 )		  ;
	bool   repeat	      ( 1 )		  ;
	vector<string> filter_names;
	/*
	//set converter	
	uhd::convert::id_type converter_id;
	converter_id.input_format  = cpu_format  ;
	converter_id.output_format = wire_format  ;  
	converter_id.num_inputs    = 3000    ; //??
	converter_id.num_outputs   = 3000    ; //??

	uhd::convert::register_converter(converter_id, make_converter, 0);

	uhd::convert::converter::sptr converter = make_converter();	
	converter->set_scalar(32767.); // fc32 -> sc16
	*/

	uhd::tune_request_t tune_request (freq);
	uhd::stream_args_t stream_args(cpu_format, wire_format);


	//make a new multi usrp 
	uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(dev_args);

	cout << boost::format("Using Devices : %s") % usrp->get_pp_string() << endl;
	

	//set master clock rate --> integer multiple of the sampling rate
	usrp->set_master_clock_rate(master_rate, mboard);	
	cout << boost::format("Master clock rate : %lf Msps") % (usrp->get_master_clock_rate(mboard)/1e6)  << endl;
	
	//set clock source
	usrp->set_clock_source(clock_ref, mboard);

	//set gain to PGA(gain element)
	usrp->set_tx_gain(gain, "PGA", chan); 
	cout << boost::format("TX Gain : %lf dB") % usrp->get_tx_gain(chan) << endl;

	//set tx rate
	usrp->set_tx_rate(rate, chan);
	cout << boost::format("TX sampling rate : %lf Msps") % (usrp->get_tx_rate(chan)/1e6) << endl;
	
	//set tx RF center frequency
	usrp->set_tx_freq(tune_request, chan);
	cout << boost::format("TX RF center frequency %lf GHz") % ( usrp->get_tx_freq(chan) / 1e9 ) << endl;

	//set tx antenna
	usrp->set_tx_antenna(ant, chan);
	cout << boost::format("TX antenna : %s") % usrp->get_tx_antenna(chan) << endl;

	//set tx bandwidth
	usrp->set_tx_bandwidth(bw, chan);
	cout << boost::format("TX bandwidth : %lf MHz") % (usrp->get_tx_bandwidth(chan) / 1e6 ) << endl;
	
	//NOT YET//
	//set filter 
	filter_names = usrp->get_filter_names();
	
	for(int i = 0; i < filter_names.size(); i++){
	
		cout << boost::format("%s") % (filter_names[i]) << endl;
	}


	cout << boost::format("The # of tx channels: %zu") % (usrp->get_tx_num_channels()) << endl;
	cout << boost::format("The # of rx channels: %zu") % (usrp->get_rx_num_channels()) << endl;

	
	//make tx streamer
	uhd::tx_streamer::sptr tx_streamer = usrp->get_tx_stream(stream_args);
	
	while(not stop_signal_called){
	//while(repeat){
		send_from_file(tx_streamer, file, samps_per_buff/*, converter*/);
		//repeat = 0;
	}

	
	return EXIT_SUCCESS;
}
