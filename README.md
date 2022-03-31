# PA1 - Noisy Channel
This reposatory is part of a small project I did in my B.sc in Elecrical engineering at [Tel-Aviv University](https://www.tau.ac.il). The project about computer comunication and sockets operation.

### *Description & Functuinallity*:

The purpose of this exercise is to implement a noisy channel model with sender, receiver and channel.
the channel in the command line arguments the type of noise and a integer n (noise parameter). there are 2 types of noise:
-  Random `-r` - in this case every bit have a flipping probabilty of <n>/<2>.
-  Deterministic `-d` - in this case every *n-th* bit flipped.

The channel receives data from the sender and add noise to it according to the set arguments, and send it to the receiver.

The sender gets in the command line arguments IP number and port number (manually insert from the server output). With those argument the sender connects to the channel via TCP socket.
The sender gets from stdin a file name (or full path to it) with suffix, read all data from the file, encode it with *hamming-code(31,26,3)*  method and send it to the channel via TCP socket in 4 bytes portions.

The receiver gets in the command line arguments IP number and port number (manually insert from the server output). With those argument the sender connects to the channel via TCP socket.
The sender gets from stdin a file name (or full path to it) with suffix to be written to. The receiver get the data from the channel in 4 bytes portions decode it with *hamming-code(31,26,3)*  method and write it to the desired file.

For more information on *Hamming Code* :
- [3Blue1Brown](https://www.youtube.com/watch?v=X8jsijhllIA&t=906s) - 2 Videos which explains it well in simple language.
- [Wikipedia](https://en.wikipedia.org/wiki/Hamming_code) - there are a lot of websites and books that explain hamming code well.

### **Comiplation**:
1. Compile via Visual Studio -
- Open project in Visual Studio.
- Build Solution & Run (F5/ctrl + F5).
2. Compile via Developer Command Prompt for VS -
- Open Developer 'Command Prompt for VS' (application file inside 'Visual Studio Tools' directory).
- Go to directory of the wanted C file.
- Type on the Command Prompt :`cl /EHsc name.c` (name of the file instead of 'name.c').
- Type command for the executable file: 'name argument1 argument2'
* for more information : [https://docs.microsoft.com](https://docs.microsoft.com/en-us/cpp/build/walkthrough-compiling-a-native-cpp-program-on-the-command-line?view=msvc-170).


### **Operation & Conditions**:
1. The first file must be channel.c such that we will have the ports numbers and ip number. the order doesn't matter between sender.c and receiver.c.
2. To continue to another channel interval answer 'yes' (case-unsensitive), every other answer will stop channel's run.
3. To continue to another sender/receiver interval put in an existing file name with suffix to be read, non-valid name will end the run.
4. To end sender/receiver run put in 'quit' (case-unsensitive) when asked for file name.
5. Channel's noise: the noise parameter is in hamming border of success.


