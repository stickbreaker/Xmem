# Xmem
Memory Panes Shield, Arduino Mega2560 compatible

10JUL2015
	The MemoryPanes example is reporting a Heap Test failure, I am still verify whether it is a hardware fault or bad coding. 
	
	The my_analyzer example is a modified version of Andrew Gillham SUMP compatible probe software.  It is just a dirty hack.  I have messed up the timings to maximize the sample rates.  The Memory panes board is able to run the 1mhz samples with 55.5k, the 2mhz and 4mhz are a mess.
	
10JULY2015
  The MemoryPanes sketch is still reporting inconsistent errors, I am going to spend some time verifying waveforms, I have not found any pattern to the problems.  My gross testing (mfgtest() will run without errors, The HeapTest() produces link list pointer errors.  Sometimes the high byte is zero. That is odd.
	
	