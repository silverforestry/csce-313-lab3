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

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.00;
	int e = 1;
	int m = MAX_MESSAGE;
	string filename = "";

	bool t_flag = false;
	bool e_flag = false;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {
		switch (opt) {
			case 'p': p = atoi(optarg); break; // 1-15
			case 't': t = atof(optarg); t_flag = true; break; // 0.000, -59.996
			case 'e': e = atoi(optarg); e_flag = true; break; // 1 or 2
			case 'f': filename = optarg; break;
			case 'm': m = atoi(optarg); break;
			default: break;
		}
	}

	pid_t pid = fork();

	if (pid == 0) {
		char *args[] = {(char*)"./server", NULL};
		execvp("./server", args);
	}
	else if (pid < 0) {
		perror("fork");
		return 1;
	}

	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	MESSAGE_TYPE msg = NEWCHANNEL_MSG;
	chan.cwrite(&msg, sizeof(MESSAGE_TYPE));
	char chan_name[64];
	chan.cread(chan_name, sizeof(chan_name));
	FIFORequestChannel n_chan(chan_name, FIFORequestChannel::CLIENT_SIDE);

	// start with a null pointer; allocating 0 bytes and later overwriting
	// it caused a 1-byte leak on some runtimes. Use nullptr and allocate
	// when needed.
	char* buf = nullptr;

	if (filename.empty()) { // Requesting Data Points
		delete [] buf;
		buf = new char[m]; // buffer

		if(! (e_flag || t_flag))
		{
			for(int i = 0; i < 1000; ++i)
			{
				t = i * 0.004;
				e = 1;
				datamsg dmsg(p, t, e);
				memcpy(buf, &dmsg, sizeof(datamsg));
				n_chan.cwrite(buf, sizeof(datamsg));
				double reply1;
				n_chan.cread(&reply1, sizeof(double)); 
				// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
				e = 2;
				datamsg dmsg2(p, t, e);
				memcpy(buf, &dmsg2, sizeof(datamsg));
				n_chan.cwrite(buf, sizeof(datamsg));
				double reply2;
				n_chan.cread(&reply2, sizeof(double));
				// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply2 << endl;
				// cout << reply1 << " " << reply2 << endl;
			}
		}
		else
		{
			t = 0.0;
			e = 1;
		}
		datamsg dmsg(p, t, e);
		memcpy(buf, &dmsg, sizeof(datamsg));
		n_chan.cwrite(buf, sizeof(datamsg));
		double reply;
		n_chan.cread(&reply, sizeof(double)); 
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else { // Requesting a File
		filemsg fmsg(0, 0); // initial request for file length
		int startlen = sizeof(fmsg) + filename.size() + 1;
		char* startbuf = new char[startlen];
		memcpy(startbuf, &fmsg, sizeof(fmsg)); // copy fmsg
		memcpy(startbuf + sizeof(fmsg), filename.c_str(), filename.size() + 1); // copy fmsg + NULL
		n_chan.cwrite(startbuf, startlen);
		delete[] startbuf;

		string output_file = "received/" + filename; // create new file in received
		FILE* fp = fopen(output_file.c_str(), "wb");

		__int64_t filelen;
		n_chan.cread(&filelen, sizeof(__int64_t)); // get the file length

		cout << "File length is " << filelen << endl;

		__int64_t num_chunks = filelen/m;
		if (filelen % m) num_chunks++;
		__int64_t offset = 0;
		
		buf = new char[filelen]; // buffer

		for(int i = 0; i < num_chunks; i++) { // iterate through chunks
			m = (i == num_chunks-1)? (filelen % m) : m; // last chunk might be smaller
			filemsg fmsg(offset, m);
			int len = sizeof(fmsg) + filename.size() + 1; // get the length of the request
			char* req_buf = new char[len]; // create buffer for request

			memcpy(req_buf, &fmsg, sizeof(fmsg));
			memcpy(req_buf + sizeof(fmsg), filename.c_str(), filename.size() + 1);
			n_chan.cwrite(req_buf, len); // send the request
			delete[] req_buf;

			char* out_buf = new char[m]; // create buffer for data

			n_chan.cread(out_buf, m); // read the data
			// cout << "Received " << m << " bytes" << endl;

			// for(int j = 0; j < m; j++)
			// {
			// 	cout << out_buf[j];
			// }

			for(int j = offset; j < offset + m; j++)
			{
				buf[j] = out_buf[j - offset]; // add to main buffer
			}

			delete[] out_buf;

			offset += m;
		}

		fwrite(buf, 1, filelen, fp);
		fclose(fp);
		// cout << "File transfer complete" << endl;

		// char * file_buf = new char[];
	}
	
	// Wrapping Up
	
	MESSAGE_TYPE qmsg = QUIT_MSG;
	n_chan.cwrite(&qmsg, sizeof(MESSAGE_TYPE));
	chan.cwrite(&qmsg, sizeof(MESSAGE_TYPE));

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
