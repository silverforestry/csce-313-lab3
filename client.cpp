
/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Anna Silva Silvestre
	UIN: 333009704
	Date: 9/16/25
*/
#include "common.h"
#include "FIFORequestChannel.h"

#include <sys/wait.h>
#include <vector>


#include <sstream>
#include <iomanip>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t =0.00;
	int e = 1;
	int m = MAX_MESSAGE;
	bool c = false;
	string filename = "";

	bool t_flag = false;
	bool e_flag = false;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p': p = atoi(optarg); break; // 1-15
			case 't': t = atof(optarg); t_flag = true; break; // 0.000, -59.996
			case 'e': e = atoi(optarg); break; // 1 or 2
			case 'f': filename = optarg; break;
			case 'm': m = atoi(optarg); break;
			case 'c': c = true; break;
			default: break;
		}
	}

	pid_t pid = fork();

	if (pid == 0) {
		string m_str = to_string(m);
		char *args[] = {(char*)"./server", (char*) "-m", (char*) m_str.c_str(),  NULL}; 
		// add m arg
		// 
		execvp("./server", args);
		perror("execvp");
	}
	else if (pid < 0) {
		perror("fork");
		return 1;
	}

	FIFORequestChannel m_chan("control", FIFORequestChannel::CLIENT_SIDE);
	vector<FIFORequestChannel*> channels;
	channels.push_back(&m_chan);

	if(c)
	{
		MESSAGE_TYPE msg = NEWCHANNEL_MSG;
		m_chan.cwrite(&msg, sizeof(MESSAGE_TYPE));
		char* name_buf = new char[30];
		m_chan.cread(name_buf, 30);
		// memset(name_buf, 0, m);
		// name_buf[63] = '\0';
		FIFORequestChannel* chan = new FIFORequestChannel(name_buf, FIFORequestChannel::CLIENT_SIDE);
		usleep(100); // SMALL PAUSE
		channels.push_back(chan);
		// cout << "new channel!" << endl;

		delete [] name_buf;
	}

	FIFORequestChannel chan = * channels.back();
	// cout << chan.name() << endl;
	// channels.pop_back();

	char* buf = new char[0];

	if (filename.empty()) { // Requesting Data Points
		delete [] buf;
		buf = new char[m]; // buffer
		system("mkdir -p received");

		if(! (t_flag || e_flag))
		{
			string output_file = "received/x1.csv";
			FILE* fout = fopen(output_file.c_str(), "w");
			for(int i = 0; i < 1000; ++i)
			{
				// make a directory, make a file using fout to receive x1.csv, write into it
				// but it under received/x.1csv
				
				// t = i * 0.004;
				// e = 1;
				// datamsg dmsg(p, t, e);
				// memcpy(buf, &dmsg, sizeof(datamsg));
				// chan.cwrite(buf, sizeof(datamsg));
				// double reply1;
				// chan.cread(&reply1, sizeof(double)); 
				// e = 2;
				// datamsg dmsg2(p, t, e);
				// memcpy(buf, &dmsg2, sizeof(datamsg));
				// chan.cwrite(buf, sizeof(datamsg));
				// double reply2;
				// chan.cread(&reply2, sizeof(double));
				// string reply = to_string(t) + "," + to_string(reply1) + "," + to_string(reply2) + "\n";

				// fwrite(reply.c_str(), reply.size(), 1, fout);
				// fseek (fout, 0, SEEK_END);
				

				
				t = i * 0.004;
				e = 1;
				datamsg dmsg(p, t, e);
				memcpy(buf, &dmsg, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double reply1;
				chan.cread(&reply1, sizeof(double)); 


				e = 2;
				datamsg dmsg2(p, t, e);
				memcpy(buf, &dmsg2, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double reply2;
				chan.cread(&reply2, sizeof(double));

				string time = to_string(t);
				ostringstream oss;
				oss << setprecision(3) << fixed << t;
				time = oss.str();
				time.erase(time.find_last_not_of('0') + 1, string::npos);
				if (time.back() == '.') time.pop_back();

				string reply1_str = to_string(reply1);
				oss.str("");
				oss << setprecision(3) << fixed << reply1;
				reply1_str = oss.str();
				reply1_str.erase(reply1_str.find_last_not_of('0') + 1, string::npos);
				if (reply1_str.back() == '.') reply1_str.pop_back();

				string reply2_str = to_string(reply2);
				oss.str("");
				oss << setprecision(3) << fixed << reply2;
				reply2_str = oss.str();
				reply2_str.erase(reply2_str.find_last_not_of('0') + 1, string::npos);
				if (reply2_str.back() == '.') reply2_str.pop_back();

				// cout << time;

				string reply = time + "," + reply1_str + "," + reply2_str;

				fwrite(reply.c_str(), sizeof(char), reply.length(), fout);
				fwrite("\n", sizeof(char), 1, fout);
				fseek (fout, 0, SEEK_END);
			}
			fclose(fout);
		}
		else
		{
			datamsg dmsg(p, t, e);
			memcpy(buf, &dmsg, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			// cout << chan.name();
			double reply;
			chan.cread(&reply, sizeof(double)); 
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}
	}
	else { // Requesting a File
		system("mkdir -p received");
		// don't change m
		filemsg fmsg(0, 0); // initial request for file length
		int startlen = sizeof(fmsg) + filename.size() + 1;
		char* startbuf = new char[startlen];
		memcpy(startbuf, &fmsg, sizeof(fmsg)); // copy fmsg
		memcpy(startbuf + sizeof(fmsg), filename.c_str(), filename.size() + 1); // copy fmsg + NULL
		chan.cwrite(startbuf, startlen);
		delete[] startbuf;

		string output_file = "received/" + filename; // create new file in received
		FILE* fp = fopen(output_file.c_str(), "wb");

		__int64_t filelen;
		chan.cread(&filelen, sizeof(__int64_t)); // get the file length

		// cout << "File length is " << filelen << endl;

		__int64_t num_chunks = filelen/m;
		if (filelen % m) num_chunks++;
		__int64_t offset = 0;
		
		// buf = new char[filelen]; // buffer

		for(int i = 0; i < num_chunks; i++) { // iterate through chunks
			int new_m = (i == num_chunks-1)? ((filelen % m != 0)? filelen % m : m) : m; // last chunk might be smaller
			filemsg fmsg(offset, new_m);
			int len = sizeof(fmsg) + filename.size() + 1; // get the length of the request
			char* req_buf = new char[len]; // create buffer for request

			memcpy(req_buf, &fmsg, sizeof(fmsg));
			memcpy(req_buf + sizeof(fmsg), filename.c_str(), filename.size() + 1);
			chan.cwrite(req_buf, len); // send the request
			delete[] req_buf;

			buf = new char[new_m]; // create buffer for data

			chan.cread(buf, new_m); // read the data
			// cout << "Received " << m << " bytes" << endl;

			// for(int j = 0; j < m; j++)
			// {
			// 	cout << out_buf[j];
			// }

			fwrite(buf, 1, new_m, fp);
			fseek(fp, 0, SEEK_END);

			delete[] buf;

			offset += new_m; // update offset
		}

		buf = new char[0];

		fclose(fp);
		// cout << "File transfer complete" << endl;

		// char * file_buf = new char[];
	}
	
	// Wrapping Up
	
	MESSAGE_TYPE qmsg = QUIT_MSG;

	while(! channels.empty())
	{
		channels.back()->cwrite(&qmsg, sizeof(MESSAGE_TYPE));
		channels.pop_back();
	}

	delete[] buf;
	waitpid(pid, NULL, 0);
	return 0;
}
	// // example data point request

	// char buf[MAX_MESSAGE]; // 256
	// datamsg x(1, 0.0, 1);
	// memcpy(buf, &x, sizeof(datamsg));
	// chan.cwrite(buf, sizeof(datamsg)); // question
	// double reply;

	// chan.cread(&reply, sizeof(double)); //answer

	// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
    
	// // sending a non-sense message, you need to change this
	// filemsg fm(0, 0);

	// string fname = "teslkansdlkjflasjdf.dat";
    
	// int len = sizeof(filemsg) + (fname.size() + 1);

	// char* buf2 = new char[len];

	// memcpy(buf2, &fm, sizeof(filemsg));

	// strcpy(buf2 + sizeof(filemsg), fname.c_str());

	// chan.cwrite(buf2, len);  // I want the file length;

	// delete[] buf2;

	// // closing the channel    
	// MESSAGE_TYPE m = QUIT_MSG;

	// chan.cwrite(&m, sizeof(MESSAGE_TYPE));