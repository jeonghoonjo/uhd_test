#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/stream.hpp>
#include <uhd/convert.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <fstream>
#include <complex>
#include <csignal>

using namespace std;

static bool stop_signal_called = false;

void sig_int_handler(int){

	stop_signal_called = true;
}

void send_from_file(
	uhd::tx_streamer::sptr tx_streamer,
	const string&	 file,
	size_t samps_per_buff)
{

	uhd::tx_metadata_t md;
	md.start_of_burst = false;
	md.end_of_burst = false;
	vector<complex<float>> buff(samps_per_buff); 
	size_t num_tx_samps, num_tx_samps_;
	
	ifstream infile(file.c_str(), ifstream::binary);

	while(not md.end_of_burst){
		
		infile.read((char*)&buff.front(), buff.size()*sizeof(complex<float>));
		num_tx_samps = size_t(infile.gcount() / sizeof(complex<float>));
		
		md.end_of_burst = infile.eof();
		
		num_tx_samps_ = tx_streamer->send(&buff.front(), num_tx_samps, md);

		cout << num_tx_samps_ << endl;
				
	}
	
	infile.close();	

}


int UHD_SAFE_MAIN(int argc, char* argv[])
{
	string dev_args       ("name = MyB200")   ;
	string clock_ref      ("internal") 	  ;
	string ant 	      ("TX/RX")		  ;
	string cpu_format     ("fc32")		  ;
	string wire_format    ("sc16")		  ;
	string file 	      ("test.bin")        ;
	size_t samps_per_buff ( 3000 )		  ;
	size_t chan 	      ( 0 )          	  ;
	size_t mboard 	      ( 0 )	       	  ;
	double gain 	      ( 73.2 )         	  ;
	double rate 	      ( 5.0e6 )      	  ;
	double master_rate    ( 40.0e6 )    	  ;   	 
	double freq 	      ( 2.6e9 )		  ;
	double bw	      ( 0.2e6 )	  	  ;
	double num_samps      ( 5000 )		  ;
	bool   repeat	      ( 1 )		  ;
	vector<string> filter_names;

	uhd::tune_request_t tune_request (freq);
	uhd::stream_args_t stream_args(cpu_format, wire_format);

	std::signal(SIGINT, &sig_int_handler);
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
	

	//make tx streamer
	uhd::tx_streamer::sptr tx_streamer = usrp->get_tx_stream(stream_args);
	
	while(not stop_signal_called){
		send_from_file(tx_streamer, file, samps_per_buff);
	}
	return EXIT_SUCCESS;
}
